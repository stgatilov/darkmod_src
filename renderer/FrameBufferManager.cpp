/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/
#include "precompiled.h"
#pragma hdrstop
#include "tr_local.h"
#include "FrameBufferManager.h"
#include "FrameBuffer.h"

FrameBufferManager frameBuffersImpl;
FrameBufferManager *frameBuffers = &frameBuffersImpl;

namespace {
	GLenum ColorBufferFormat() {
		if( r_fboColorBits.GetInteger() == 64 ) {
			return GL_RGBA16F;
		} 
		return ( glConfig.srgb ? GL_SRGB_ALPHA : GL_RGBA );
	}

	GLenum DepthBufferFormat() {
		return ( r_fboDepthBits.GetInteger() == 32 ) ? GL_DEPTH32F_STENCIL8 : GL_DEPTH24_STENCIL8;
	}
}

FrameBufferManager::~FrameBufferManager() {
	Shutdown();
}

void FrameBufferManager::Init() {
	UpdateResolutionAndFormats();
	defaultFbo = CreateFromGenerator( "default", FrameBuffer::CreateDefaultFrameBuffer );
	primaryFbo = CreateFromGenerator( "primary", [this](FrameBuffer *fbo) { CreatePrimary( fbo ); } );
	resolveFbo = CreateFromGenerator( "resolve", [this](FrameBuffer *fbo) { CreateResolve( fbo ); } );
	guiFbo = CreateFromGenerator( "gui", [this](FrameBuffer *fbo) { CreateGui( fbo ); } );
	shadowStencilFbo = CreateFromGenerator( "shadowStencil", [this](FrameBuffer *fbo) { CreateStencilShadow( fbo ); } );
	shadowMapFbo = CreateFromGenerator( "shadowMap", [this](FrameBuffer *fbo) { CreateMapsShadow( fbo ); } );

	activeFbo = defaultFbo;
	activeDrawFbo = defaultFbo;
}

void FrameBufferManager::Shutdown() {
	for (auto fbo : fbos) {
		delete fbo;
	}
	fbos.ClearFree();

	if (pbo != 0) {
		qglDeleteBuffers(1, &pbo);
		pbo = 0;
	}
}

FrameBuffer * FrameBufferManager::CreateFromGenerator( const idStr &name, Generator generator ) {
	for (auto fbo : fbos) {
		if (name == fbo->Name()) {
			return fbo;
		}
	}
	FrameBuffer *fbo = new FrameBuffer(name, generator);
	fbos.Append( fbo );
	return fbo;
}

void FrameBufferManager::PurgeAll() {
	for (auto fbo : fbos) {
		fbo->Destroy();
	}

	if (pbo != 0) {
		qglDeleteBuffers(1, &pbo);
		pbo = 0;
	}
}

void FrameBufferManager::BeginFrame() {
	if (
		r_multiSamples.IsModified() || 
		r_fboResolution.IsModified() ||
		r_fboColorBits.IsModified() ||
		r_fboDepthBits.IsModified() ||
		r_shadows.IsModified() ||
		r_shadowMapSize.IsModified()
	) {
		r_multiSamples.ClearModified();
		r_fboResolution.ClearModified();
		r_fboColorBits.ClearModified();
		r_fboDepthBits.ClearModified();
		r_shadows.ClearModified();
		r_shadowMapSize.ClearModified();

		// something FBO-related has changed, let's recreate everything from scratch
		UpdateResolutionAndFormats();
		PurgeAll();
	}

	currentRenderFbo = defaultFbo;
	defaultFbo->Bind();
}

void FrameBufferManager::EnterPrimary() {
	depthCopiedThisView = false;
	if (currentRenderFbo == primaryFbo) return;

	currentRenderFbo = primaryFbo;
	primaryFbo->Bind();

	GL_ViewportVidSize( tr.viewportOffset[0] + backEnd.viewDef->viewport.x1,
	             tr.viewportOffset[1] + backEnd.viewDef->viewport.y1,
	             backEnd.viewDef->viewport.x2 + 1 - backEnd.viewDef->viewport.x1,
	             backEnd.viewDef->viewport.y2 + 1 - backEnd.viewDef->viewport.y1 );

	GL_ScissorVidSize( tr.viewportOffset[0] + backEnd.viewDef->viewport.x1 + backEnd.viewDef->scissor.x1,
	            tr.viewportOffset[1] + backEnd.viewDef->viewport.y1 + backEnd.viewDef->scissor.y1,
	            backEnd.viewDef->scissor.x2 + 1 - backEnd.viewDef->scissor.x1,
	            backEnd.viewDef->scissor.y2 + 1 - backEnd.viewDef->scissor.y1 );

	qglClear( GL_COLOR_BUFFER_BIT ); // otherwise transparent skybox blends with previous frame
}

void FrameBufferManager::LeavePrimary(bool copyToDefault) {
	// if we want to do tonemapping later, we need to continue to render to a texture,
	// otherwise we can render the remaining UI views straight to the back buffer
	FrameBuffer *targetFbo = r_tonemap ? guiFbo : defaultFbo;
	if (currentRenderFbo == targetFbo) return;

	currentRenderFbo = targetFbo;

	if (copyToDefault) {
		if ( r_multiSamples.GetInteger() > 1 ) {
			ResolvePrimary();
			resolveFbo->BlitTo( targetFbo, GL_COLOR_BUFFER_BIT, GL_LINEAR );
		} else {
			primaryFbo->BlitTo( targetFbo, GL_COLOR_BUFFER_BIT, GL_LINEAR );
		}

		if ( r_frontBuffer.GetBool() && !r_tonemap ) {
			qglFinish();
		}
	}

	targetFbo->Bind();
	GL_ViewportRelative( 0, 0, 1, 1 );
	GL_ScissorRelative( 0, 0, 1, 1 );
}

void FrameBufferManager::EnterShadowStencil() {
	if( r_multiSamples.GetInteger() > 1 ) {
		// with MSAA on, we need to render against the multisampled primary buffer, otherwise stencil is drawn
		// against a lower-quality depth map which may cause render errors with shadows
		primaryFbo->Bind();
	} else {
		// most vendors can't do separate stencil so we need to copy depth from the main/default FBO
		if ( !depthCopiedThisView ) {
			currentRenderFbo->BlitTo( shadowStencilFbo, GL_DEPTH_BUFFER_BIT, GL_NEAREST );
			depthCopiedThisView = true;
		}

		shadowStencilFbo->Bind();
	}
}

void FrameBufferManager::LeaveShadowStencil() {
	currentRenderFbo->Bind();
}

void FrameBufferManager::ResolveShadowStencilAA() {
	primaryFbo->BlitTo( shadowStencilFbo, GL_STENCIL_BUFFER_BIT, GL_NEAREST );
}

void FrameBufferManager::EnterShadowMap() {
	shadowMapFbo->Bind();
	qglDepthMask( true );
	GL_State( GLS_DEPTHFUNC_LESS ); // reset in RB_GLSL_CreateDrawInteractions
	// the caller is now responsible for proper setup of viewport/scissor
}

void FrameBufferManager::LeaveShadowMap() {
	currentRenderFbo->Bind();

	const idScreenRect &r = backEnd.viewDef->viewport;
	GL_ViewportVidSize( r.x1, r.y1, r.x2 - r.x1 + 1, r.y2 - r.y1 + 1 );
	GL_ScissorVidSize( backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
		backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
		backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
		backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1 );
}

void FrameBufferManager::ResolvePrimary( GLbitfield mask, GLenum filter ) {
	primaryFbo->BlitTo( resolveFbo, mask, filter );
}

void FrameBufferManager::UpdateCurrentRenderCopy() {
	currentRenderFbo->BlitTo( resolveFbo, GL_COLOR_BUFFER_BIT, GL_NEAREST );
}

void FrameBufferManager::UpdateCurrentDepthCopy() {
	currentRenderFbo->BlitTo( resolveFbo, GL_DEPTH_BUFFER_BIT, GL_NEAREST );
}

void FrameBufferManager::CopyRender( const copyRenderCommand_t &cmd ) {
	//stgatilov #4754: this happens during lightgem calculating in minimized windowed TDM
	if ( cmd.imageWidth * cmd.imageHeight == 0 ) {
		return;	//no pixels to be read
	}
	int backEndStartTime = Sys_Milliseconds();

	if ( activeFbo == defaultFbo ) { // #4425: not applicable, raises gl errors
		qglReadBuffer( GL_BACK );
	}

	if ( activeFbo == primaryFbo && r_multiSamples.GetInteger() > 1 ) {
		ResolvePrimary( GL_COLOR_BUFFER_BIT );
		resolveFbo->Bind();
	}

	if ( cmd.buffer ) {
		CopyRender( cmd.buffer, cmd.x, cmd.y, cmd.imageWidth, cmd.imageHeight, cmd.usePBO );
	}

	if ( cmd.image )
		CopyRender( cmd.image, cmd.x, cmd.y, cmd.imageWidth, cmd.imageHeight );

	currentRenderFbo->Bind();

	int backEndFinishTime = Sys_Milliseconds();
	backEnd.pc.msec += backEndFinishTime - backEndStartTime;
}

void FrameBufferManager::UpdateResolutionAndFormats() {
	float scale = r_fboResolution.GetFloat();
	renderWidth = static_cast< int >( glConfig.vidWidth * scale );
	renderHeight = static_cast< int >( glConfig.vidHeight * scale );
	shadowAtlasSize = 6 * r_shadowMapSize;
	colorFormat = ColorBufferFormat();
	depthStencilFormat = DepthBufferFormat();
}

void FrameBufferManager::CreatePrimary( FrameBuffer *primary ) {
	int msaa = r_multiSamples.GetInteger();
	primary->Init( renderWidth, renderHeight, msaa );	
	primary->AddColorRenderBuffer( 0, colorFormat );
	primary->AddDepthStencilRenderBuffer( depthStencilFormat );
}

void FrameBufferManager::CreateResolve( FrameBuffer *resolve ) {
	resolve->Init( renderWidth, renderHeight );
	globalImages->currentRenderImage->GenerateAttachment( renderWidth, renderHeight, colorFormat, GL_LINEAR );
	resolve->AddColorRenderTexture( 0, globalImages->currentRenderImage );
	globalImages->currentDepthImage->GenerateAttachment( renderWidth, renderHeight, depthStencilFormat, GL_NEAREST );
	globalImages->currentDepthImage->Bind();
	GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_RED };
	qglTexParameteriv( GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask );
	resolve->AddDepthRenderTexture( globalImages->currentDepthImage );
}

void FrameBufferManager::CreateGui( FrameBuffer *gui ) {
	gui->Init( glConfig.vidWidth, glConfig.vidHeight );
	globalImages->guiRenderImage->GenerateAttachment( glConfig.vidWidth, glConfig.vidHeight, colorFormat, GL_NEAREST );
	gui->AddColorRenderTexture( 0, globalImages->guiRenderImage );
}

void FrameBufferManager::CopyRender( idImage* image, int x, int y, int imageWidth, int imageHeight ) {
	if ( image->texnum == idImage::TEXTURE_NOT_LOADED ) { // 5257
		image->generatorFunction = R_RGBA8Image; // otherwise texstorage (when enabled) makes the texture immutable
		R_RGBA8Image( image ); // image->MakeDefault() can produce a compressed image, unsuitable for copying into
	}
	image->Bind();
	if ( activeFbo == primaryFbo || activeFbo == resolveFbo ) {
		x *= r_fboResolution.GetFloat();
		y *= r_fboResolution.GetFloat();
		imageWidth *= r_fboResolution.GetFloat();
		imageHeight *= r_fboResolution.GetFloat();
	}

	if ( image->uploadWidth != imageWidth || image->uploadHeight != imageHeight ) {
		image->uploadWidth = imageWidth;
		image->uploadHeight = imageHeight;
		qglCopyTexImage2D( GL_TEXTURE_2D, 0, GL_RGB8, x, y, imageWidth, imageHeight, 0 );
	} else {
		// otherwise, just subimage upload it so that drivers can tell we are going to be changing
		// it and don't try and do a texture compression or some other silliness
		qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, x, y, imageWidth, imageHeight );
	}
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	backEnd.c_copyFrameBuffer++;
}

void FrameBufferManager::CopyRender( unsigned char *buffer, int x, int y, int imageWidth, int imageHeight, bool usePBO ) {
	// #4395 lightem pixel pack buffer optimization
	if ( usePBO ) {
		static int pboSize = -1;

		if ( !pbo ) {
			pboSize = imageWidth * imageHeight * 3;
			qglGenBuffers( 1, &pbo );
			qglBindBuffer( GL_PIXEL_PACK_BUFFER, pbo );
			qglBufferData( GL_PIXEL_PACK_BUFFER, pboSize, NULL, GL_STREAM_READ );
			qglBindBuffer( GL_PIXEL_PACK_BUFFER, 0 );
		}

		if ( imageWidth * imageHeight * 3 != pboSize ) {
			common->Error( "CaptureRenderToBuffer: wrong PBO size %dx%d/%d", imageWidth, imageHeight, pboSize );
		}
		qglBindBuffer( GL_PIXEL_PACK_BUFFER, pbo );

		byte *ptr = reinterpret_cast< byte * >( qglMapBuffer( GL_PIXEL_PACK_BUFFER, GL_READ_ONLY ) );

		// #4395 moved to initializer
		if ( ptr ) {
			memcpy( buffer, ptr, pboSize );
			qglUnmapBuffer( GL_PIXEL_PACK_BUFFER );
		}

		// revelator: added c++11 nullptr
		qglReadPixels( x, y, imageWidth, imageHeight, GL_RGB, GL_UNSIGNED_BYTE, nullptr );
		qglBindBuffer( GL_PIXEL_PACK_BUFFER, 0 );
	} else {
		qglReadPixels( x, y, imageWidth, imageHeight, GL_RGB, GL_UNSIGNED_BYTE, buffer );
	}
}

void FrameBufferManager::CreateStencilShadow( FrameBuffer *shadow ) {
	shadow->Init( renderWidth, renderHeight );
	globalImages->shadowDepthFbo->GenerateAttachment( renderWidth, renderHeight, depthStencilFormat, GL_NEAREST );
	shadow->AddDepthStencilRenderTexture( globalImages->shadowDepthFbo );
}

void FrameBufferManager::CreateMapsShadow( FrameBuffer *shadow ) {
	shadow->Init( shadowAtlasSize, shadowAtlasSize );
	globalImages->shadowAtlas->GenerateAttachment( shadowAtlasSize, shadowAtlasSize, GL_DEPTH_COMPONENT32F, GL_NEAREST );
	shadow->AddDepthRenderTexture( globalImages->shadowAtlas );
	for ( int i = 0; i < 2; i++ ) { // 2x full size pages
		ShadowAtlasPages[i].x = 0;
		ShadowAtlasPages[i].y = r_shadowMapSize.GetInteger() * i;
		ShadowAtlasPages[i].width = r_shadowMapSize.GetInteger();
	}
	for ( int i = 0; i < 8; i++ ) { // 8x 1/4 pages
		ShadowAtlasPages[2 + i].x = 0;
		ShadowAtlasPages[2 + i].y = r_shadowMapSize.GetInteger() * 2 + (r_shadowMapSize.GetInteger() >> 1) * i;
		ShadowAtlasPages[2 + i].width = r_shadowMapSize.GetInteger() >> 1;
	}
	for ( int i = 0; i < 16; i++ ) { // 32x 1/16-sized pages
		ShadowAtlasPages[10 + i].x = r_shadowMapSize.GetInteger() * 3;
		ShadowAtlasPages[10 + i].y = r_shadowMapSize.GetInteger() * 2 + (r_shadowMapSize.GetInteger() >> 2) * i;
		ShadowAtlasPages[10 + i].width = r_shadowMapSize.GetInteger() >> 2;
		ShadowAtlasPages[26 + i].x = r_shadowMapSize.GetInteger() * 4.5;
		ShadowAtlasPages[26 + i].y = r_shadowMapSize.GetInteger() * 2 + (r_shadowMapSize.GetInteger() >> 2) * i;
		ShadowAtlasPages[26 + i].width = r_shadowMapSize.GetInteger() >> 2;
	}
}
