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
#include "FrameBuffer.h"
#include "glsl.h"
#include "GLSLProgramManager.h"
#include "AmbientOcclusionStage.h"
#include "BloomStage.h"
#include "FrameBufferManager.h"

FrameBuffer::FrameBuffer( const idStr &name, const Generator &generator ) {
	this->name = name;
	this->generator = generator;
}

FrameBuffer::~FrameBuffer() {
	Destroy();
}

void FrameBuffer::Init( int width, int height, int msaa ) {
	if (initialized) {
		common->Warning("Trying to initialize already created framebuffer %s", name.c_str());
		return;
	}

	qglGenFramebuffers(1, &fbo);
	this->width = width;
	this->height = height;
	this->msaa = std::min(msaa, glConfig.maxSamples);
	initialized = true;
	Bind();
	GL_SetDebugLabel(GL_FRAMEBUFFER, fbo, name);
}

void FrameBuffer::Destroy() {
	if (!initialized) {
		return;
	}

	qglDeleteFramebuffers(1, &fbo);
	fbo = 0;
	for( int i = 0; i < MAX_COLOR_ATTACHMENTS; ++i ) {
		if( colorRenderBuffers[i] != 0 ) {
			qglDeleteRenderbuffers(1, &colorRenderBuffers[i]);
			colorRenderBuffers[i] = 0;
		}
	}
	if (depthRenderBuffer != 0) {
		qglDeleteRenderbuffers(1, &depthRenderBuffer);
		depthRenderBuffer = 0;
	}
	initialized = false;
}

void FrameBuffer::AddColorRenderBuffer( int attachment, GLenum format ) {
	if (attachment < 0 || attachment >= MAX_COLOR_ATTACHMENTS) {
		common->Error( "Trying to add color attachment to a framebuffer outside valid range" );
	}	

	AddRenderBuffer( colorRenderBuffers[attachment], GL_COLOR_ATTACHMENT0 + attachment, format, name + "_color" + attachment );
}

void FrameBuffer::AddColorRenderTexture( int attachment, idImage *texture, int mipLevel ) {
	if (attachment < 0 || attachment >= MAX_COLOR_ATTACHMENTS) {
		common->Error( "Trying to add color attachment to a framebuffer outside valid range" );
	}	

	AddRenderTexture( texture, GL_COLOR_ATTACHMENT0 + attachment, mipLevel );
}

void FrameBuffer::AddDepthStencilRenderBuffer( GLenum format ) {
	AddRenderBuffer( depthRenderBuffer, GL_DEPTH_STENCIL_ATTACHMENT, format, name + "_depth" );
}

void FrameBuffer::AddDepthStencilRenderTexture( idImage *texture ) {
	AddRenderTexture( texture, GL_DEPTH_STENCIL_ATTACHMENT, 0 );
}

void FrameBuffer::AddDepthRenderTexture( idImage *texture ) {
	AddRenderTexture( texture, GL_DEPTH_ATTACHMENT, 0 );
}

void FrameBuffer::AddStencilRenderTexture( idImage *texture ) {
	AddRenderTexture( texture, GL_STENCIL_ATTACHMENT, 0 );
}

void FrameBuffer::Validate() {
	Bind();
	GLenum result = qglCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (result == GL_FRAMEBUFFER_COMPLETE) {
		return;
	}

	//Destroy();

	switch (result) {
	case GL_FRAMEBUFFER_UNDEFINED:
		common->Warning( "Default framebuffer does not exist" );
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		common->Warning( "Incomplete attachment (%s)", name.c_str() );
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		common->Warning( "Missing attachment (%s)", name.c_str() );
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
		common->Warning( "Incomplete draw buffer (%s)", name.c_str() );
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
		common->Warning( "Incomplete read buffer (%s)", name.c_str() );
		break;
	case GL_FRAMEBUFFER_UNSUPPORTED:
		common->Warning( "Unsupported framebuffer configuration (%s)", name.c_str() );
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
		common->Warning( "Framebuffer multisample mismatch (%s)", name.c_str() );
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS_ARB:
		common->Warning( "Framebuffer layer mismatch (%s)", name.c_str() );
		break;
	default:
		common->Warning( "Unknown framebuffer error (%s)", name.c_str() );
		break;
	}
}

void FrameBuffer::Bind() {
	if (!initialized) {
		Generate();
		// definitely bind in this case since we recreated the fbo object
		qglBindFramebuffer(GL_FRAMEBUFFER, fbo);
	}

	if (frameBuffers->activeFbo != this || frameBuffers->activeDrawFbo != this) {
		qglBindFramebuffer(GL_FRAMEBUFFER, fbo);
		frameBuffers->activeFbo = this;
		frameBuffers->activeDrawFbo = this;
	}
}

void FrameBuffer::BlitTo( FrameBuffer *target, GLbitfield mask, GLenum filter ) {
	FrameBuffer *previous = frameBuffers->activeFbo;
	Bind();
	qglDisable(GL_SCISSOR_TEST);
	target->BindDraw();
	qglBlitFramebuffer(0, 0, width, height, 0, 0, target->width, target->height, mask, filter);
	previous->Bind();
	qglEnable(GL_SCISSOR_TEST);
}

void FrameBuffer::CreateDefaultFrameBuffer(FrameBuffer *fbo) {
	// this doesn't actually create a new framebuffer object, but creates a class that represents the "default" (0) framebuffer
	fbo->fbo = 0;
	fbo->width = glConfig.vidWidth;
	fbo->height = glConfig.vidHeight;
	fbo->msaa = 0;
	fbo->initialized = true;
}

void FrameBuffer::Generate() {
	generator( this );
	Validate();
}

void FrameBuffer::BindDraw() {
	if (!initialized) {
		Generate();
		// definitely bind in this case since we recreated the fbo object
		qglBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
	}

	if (frameBuffers->activeDrawFbo != this) {
		qglBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
		frameBuffers->activeDrawFbo = this;
	}
}

void FrameBuffer::AddRenderBuffer( GLuint &buffer, GLenum attachment, GLenum format, const idStr &name ) {
	if (buffer != 0) {
		qglDeleteRenderbuffers(1, &buffer);
	}
	qglGenRenderbuffers(1, &buffer);
	qglBindRenderbuffer(GL_RENDERBUFFER, buffer);
	GL_SetDebugLabel( GL_RENDERBUFFER, buffer, name );

	if (msaa > 1) {
		qglRenderbufferStorageMultisample(GL_RENDERBUFFER, msaa, format, width, height);
	} else {
		qglRenderbufferStorage(GL_RENDERBUFFER, format, width, height);
	}

	Bind();
	qglFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, buffer);
}

void FrameBuffer::AddRenderTexture( idImage *texture, GLenum attachment, int mipLevel ) {
	if ( (texture->uploadWidth >> mipLevel) != width || (texture->uploadHeight >> mipLevel) != height ) {
		common->Warning("Adding texture %s to framebuffer %s: size mismatch", texture->imgName.c_str(), name.c_str());
	}
	Bind();
	qglFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture->texnum, mipLevel);
}


renderCrop_t ShadowAtlasPages[42];
idCVar r_fboResolution( "r_fboResolution", "1", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE, "internal rendering resolution factor" );

void FB_RenderTexture(idImage *texture) {
	texture->type = TT_2D;	
}

void FB_ApplyScissor() {
	if ( r_useScissor.GetBool() ) {
		GL_ScissorVidSize(
			backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
		    backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
		    backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
		    backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1 );
	}
}

void FB_DebugShowContents() {
	if (!r_showFBO.GetBool()) {
		return;
	}

	if ( r_multiSamples.GetInteger() > 1 ) {
		frameBuffers->ResolvePrimary( GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
	}
	GL_ViewportRelative( 0, 0, 1, 1 );
	GL_ScissorRelative( 0, 0, 1, 1 );

	GL_SetProjection( mat4_identity.ToFloatPtr() );

	GL_State( GLS_DEFAULT );
	qglDisable( GL_DEPTH_TEST );

	programManager->oldStageShader->Activate();
	Uniforms::Global* transformUniforms = programManager->oldStageShader->GetUniformGroup<Uniforms::Global>();
	idMat4 ninety = mat4_identity * .9f;
	ninety[3][3] = 1;
	transformUniforms->modelViewMatrix.Set( ninety );

	GL_SelectTexture( 0 );
	switch ( r_showFBO.GetInteger() ) {
	case 1:
		globalImages->shadowAtlas->Bind();
		break;
	case 2:
		globalImages->currentDepthImage->Bind();
		break;
	case 3:
		globalImages->shadowDepthFbo->Bind();
		qglTexParameteri( GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_DEPTH_COMPONENT );
		break;
	case 4:
		ambientOcclusion->ShowSSAO();
		break;
	case 5:
		bloom->BindBloomTexture();
		break;
	default:
		globalImages->currentRenderImage->Bind();
	}
	RB_DrawFullScreenQuad();
	transformUniforms->modelViewMatrix.Set( mat4_identity );
	GLSLProgram::Deactivate();

	qglEnable( GL_DEPTH_TEST );
}

/*
====================
GL_Scissor

Utility function,
if you absolutely must check for anything out of the ordinary, then do it here.
====================
*/
void GL_ScissorAbsolute( int x, int y, int w, int h ) {
	if (w <= 0 || h <= 0) {
		// apparently, this happens quite a bit for some reason
		//common->Warning("Invalid scissor dimensions: (%d, %d, %d, %d)", x, y, w, h);
		return;
	}
	qglScissor(x, y, w, h);
}

void GL_ScissorVidSize( int x /* left*/, int y /* bottom */, int w, int h ) {
	float xScale = static_cast<float>(frameBuffers->activeFbo->Width()) / glConfig.vidWidth;
	float yScale = static_cast<float>(frameBuffers->activeFbo->Height()) / glConfig.vidHeight;
	GL_ScissorAbsolute( x * xScale, y * yScale, w * xScale, h * yScale );
}

void GL_ScissorRelative( float x, float y, float w, float h ) {
	int width = frameBuffers->activeFbo->Width();
	int height = frameBuffers->activeFbo->Height();
	GL_ScissorAbsolute( x * width, y * height, w * width, h * height );
}

/*
====================
GL_Viewport

Utility function,
if you absolutly must check for anything out of the ordinary, then do it here.
====================
*/
void GL_ViewportAbsolute( int x, int y, int w, int h ) {
	if (w <= 0 || h <= 0) {
		return;
	}
	qglViewport(x, y, w, h);
}

void GL_ViewportVidSize( int x /* left */, int y /* bottom */, int w, int h ) {
	float xScale = static_cast<float>(frameBuffers->activeFbo->Width()) / glConfig.vidWidth;
	float yScale = static_cast<float>(frameBuffers->activeFbo->Height()) / glConfig.vidHeight;
	GL_ViewportAbsolute( x * xScale, y * yScale, w * xScale, h * yScale );
}

void GL_ViewportRelative( float x, float y, float w, float h ) {
	int width = frameBuffers->activeFbo->Width();
	int height = frameBuffers->activeFbo->Height();
	GL_ViewportAbsolute( x * width, y * height, w * width, h * height );
}
