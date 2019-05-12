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

// all false at start
bool primaryOn = false, shadowOn = false;
bool depthCopiedThisView = false;
GLuint fboPrimary, fboResolve, fboPostProcess, fboShadowStencil, pbo, fboShadowAtlas;
GLuint postProcessWidth, postProcessHeight;
renderCrop_t ShadowAtlasPages[42];
idCVar r_fboResolution( "r_fboResolution", "1", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE, "internal rendering resolution factor" );

#if defined(_MSC_VER) && _MSC_VER >= 1800 && !defined(DEBUG)
//#pragma optimize("t", off) // duzenko: used in release to enforce breakpoints in inlineable code. Please do not remove
#endif

struct {
	GLenum attachment, internalformat;
	GLuint handle;
	GLsizei width, height;
	int msaa;
	void Attach() {
		if ( internalformat == 0 )
			common->Warning( "Internal error in Renderbuffer.Attach" );
		qglFramebufferRenderbuffer( GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, handle );
	}
	void Size( GLsizei newWidth, GLsizei newHeight, int newMsaa = 0, GLenum newFormat = GL_KEEP ) {
		if ( internalformat == 0 && newFormat == GL_KEEP ) // that's ok
			return;
		if ( width == newWidth && height == newHeight && newMsaa == msaa && (newFormat == GL_KEEP || internalformat == newFormat) )
			return;
		Bind();
		internalformat = newFormat == GL_KEEP ? internalformat : newFormat;
		width = newWidth;
		height = newHeight;
		msaa = newMsaa;
		if ( msaa > 1 )
			qglRenderbufferStorageMultisample( GL_RENDERBUFFER, msaa, internalformat, width, height );
		else
			qglRenderbufferStorage( GL_RENDERBUFFER, internalformat, width, height );
	}
private:
	void GenerateIf0() {
		if ( !handle )
			qglGenRenderbuffers( 1, &handle );
	}
	void Bind() {
		GenerateIf0();
		qglBindRenderbuffer( GL_RENDERBUFFER, handle );
	}
}	renderBufferColor = { GL_COLOR_ATTACHMENT0, GL_RGBA },
	renderBufferDepthStencil = { GL_DEPTH_STENCIL_ATTACHMENT },
	renderBufferPostProcess = { GL_COLOR_ATTACHMENT0, GL_RGBA };

void FB_CreatePrimaryResolve( GLuint width, GLuint height, int msaa ) {
	if ( !fboPrimary ) {
		qglGenFramebuffers( 1, &fboPrimary );
	}
	if ( !r_fboSharedDepth.GetBool() || msaa > 1 ) {
		if ( !fboResolve ) {
			qglGenFramebuffers( 1, &fboResolve );
		}
		renderBufferColor.Size( width, height, msaa );
		// revert to old behaviour, switches are to specific
		int depthFormat = ( r_fboDepthBits.GetInteger() == 32 ) ? GL_DEPTH32F_STENCIL8 : GL_DEPTH24_STENCIL8;
		renderBufferDepthStencil.Size( width, height, msaa, depthFormat );

		qglBindFramebuffer( GL_FRAMEBUFFER, fboPrimary );
		renderBufferColor.Attach();
		renderBufferDepthStencil.Attach();
		common->Printf( "Generated render buffers for COLOR & DEPTH/STENCIL: %dx%dx%d\n", width, height, msaa );
	} else {
		// only need the color render buffer, depth will be bound directly to texture
		renderBufferColor.Size( width, height );
		qglBindFramebuffer( GL_FRAMEBUFFER, fboPrimary );
		renderBufferColor.Attach();
	}
}

void FB_CreatePostProcess( GLuint width, GLuint height ) {
	if ( !fboPostProcess ) {
		qglGenFramebuffers( 1, &fboPostProcess );
	}
	renderBufferPostProcess.Size( width, height );
	qglBindFramebuffer( GL_FRAMEBUFFER, fboPostProcess );
	renderBufferPostProcess.Attach();
	int status = qglCheckFramebufferStatus( GL_FRAMEBUFFER );
	if ( GL_FRAMEBUFFER_COMPLETE != status ) {
		common->Printf( "glCheckFramebufferStatus postProcess: %d\n", status );
		qglDeleteFramebuffers( 1, &fboPostProcess );
		fboPostProcess = 0; // try from scratch next time
	}
	postProcessWidth = width;
	postProcessHeight = height;
}

/*
When using AA, we can't just qglCopyTexImage2D from MSFB to a regular texture.
This function blits to fboResolve, then we have a copy of MSFB in the currentRender texture
*/
void FB_ResolveMultisampling( GLbitfield mask, GLenum filter ) {
	qglDisable( GL_SCISSOR_TEST );
	qglBindFramebuffer( GL_DRAW_FRAMEBUFFER, fboResolve );
	qglBlitFramebuffer( 0, 0, globalImages->currentRenderImage->uploadWidth,
	                    globalImages->currentRenderImage->uploadHeight,
	                    0, 0, globalImages->currentRenderImage->uploadWidth,
	                    globalImages->currentRenderImage->uploadHeight,
	                    mask, filter );
	qglBindFramebuffer( GL_DRAW_FRAMEBUFFER, fboPrimary );
	qglEnable( GL_SCISSOR_TEST );
}

/*
This function blits to fboShadow as a resolver to have a workable copy of the stencil texture
*/
void FB_ResolveShadowAA() {
	if (!fboShadowStencil)
		return;	//happens once when game starts
	qglDisable( GL_SCISSOR_TEST );
	qglBindFramebuffer( GL_DRAW_FRAMEBUFFER, fboShadowStencil );
	qglBlitFramebuffer( 0, 0, globalImages->currentRenderImage->uploadWidth,
	                    globalImages->currentRenderImage->uploadHeight,
	                    0, 0, globalImages->currentRenderImage->uploadWidth,
	                    globalImages->currentRenderImage->uploadHeight,
	                    GL_STENCIL_BUFFER_BIT, GL_NEAREST );
	qglBindFramebuffer( GL_DRAW_FRAMEBUFFER, fboPrimary );
	qglEnable( GL_SCISSOR_TEST );
}

/*
called when post-processing is about to start, needs pixels
we need to copy render separately for water/smoke and then again for bloom
*/
void FB_CopyColorBuffer() {
	bool msaa = (r_multiSamples.GetInteger() > 1);
	if ( primaryOn && msaa ) {
		FB_ResolveMultisampling( GL_COLOR_BUFFER_BIT );
	} else {
		GL_SelectTexture( 0 );
		FB_CopyRender(globalImages->currentRenderImage,
		    backEnd.viewDef->viewport.x1,
		    backEnd.viewDef->viewport.y1,
		    backEnd.viewDef->viewport.x2 -
		    backEnd.viewDef->viewport.x1 + 1,
		    backEnd.viewDef->viewport.y2 -
		    backEnd.viewDef->viewport.y1 + 1, true );
	}
}

/*
====================
CopyDepthbuffer

This should just be part of copyFramebuffer once we have a proper image type field
Fixed #3877. Allow shaders to access scene depth -- revelator + SteveL
Moved from image_load.cpp so that can use internal FBO resolution ratio and state
====================
*/
void CopyDepthBuffer( idImage *image, int x, int y, int imageWidth, int imageHeight, bool useOversizedBuffer ) {
	image->Bind();
	// Ensure we are reading from the back buffer:
	if ( !r_useFbo.GetBool() ) // duzenko #4425: not applicable, raises gl errors
		qglReadBuffer( GL_BACK );
	if ( primaryOn ) {
		x *= r_fboResolution.GetFloat();
		y *= r_fboResolution.GetFloat();
		imageWidth *= r_fboResolution.GetFloat();
		imageHeight *= r_fboResolution.GetFloat();
	}
	// only resize if the current dimensions can't hold it at all,
	// otherwise subview renderings could thrash this
	if ( (useOversizedBuffer && (image->uploadWidth < imageWidth || image->uploadHeight < imageHeight)) ||
		(!useOversizedBuffer && (image->uploadWidth != imageWidth || image->uploadHeight != imageHeight)) ) {
		image->uploadWidth = imageWidth;
		image->uploadHeight = imageHeight;

		// This bit runs once only at map start, because it tests whether the image is too small to hold the screen.
		// It resizes the texture to a power of two that can hold the screen,
		// and then subsequent captures to the texture put the depth component into the RGB channels
		// this part sets depthbits to the max value the gfx card supports, it could also be used for FBO.
		switch ( r_fboDepthBits.GetInteger() ) {
		case 16:
			qglTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16_ARB, imageWidth, imageHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr );
			break;
		case 32:
			qglTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32_ARB, imageWidth, imageHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr );
			break;
		default:
			qglTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24_ARB, imageWidth, imageHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr );
			break;
		}
	}   //REVELATOR: dont need an else condition here.

	// otherwise, just subimage upload it so that drivers can tell we are going to be changing
	// it and don't try and do texture compression or some other silliness.
	qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, x, y, imageWidth, imageHeight );

	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); // GL_NEAREST for Soft Shadow ~SS
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR ); // GL_NEAREST

	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	backEnd.c_copyDepthBuffer++;

	// Debug this as well
	GL_CheckErrors();
}

void FB_CopyDepthBuffer() {
	bool msaa = (r_multiSamples.GetInteger() > 1);
	if (r_fboSharedDepth.GetBool() && !msaa)
		return;	//do not copy depth buffer, use the FBO-attached texture in postprocessing shaders

	if ( primaryOn && msaa ) {
		FB_ResolveMultisampling( GL_DEPTH_BUFFER_BIT );
	} else {
		GL_SelectTexture( 0 );
		CopyDepthBuffer( globalImages->currentDepthImage,
			backEnd.viewDef->viewport.x1,
			backEnd.viewDef->viewport.y1,
			backEnd.viewDef->viewport.x2 -
			backEnd.viewDef->viewport.x1 + 1,
			backEnd.viewDef->viewport.y2 -
			backEnd.viewDef->viewport.y1 + 1, true);
	}
}

/*
====================
CopyFramebuffer
x, y, imageWidth, imageHeigh for subviews are full screen size, scissored by backend.viewdef
Moved from image_load.cpp so that can use internal FBO resolution ratio and state
====================
*/
void FB_CopyRender( idImage *image, int x, int y, int imageWidth, int imageHeight, bool useOversizedBuffer ) {
	image->Bind();
	if ( !r_useFbo.GetBool() ) // duzenko #4425: not applicable, raises gl errors
		qglReadBuffer( GL_BACK );
	if ( primaryOn ) {
		x *= r_fboResolution.GetFloat();
		y *= r_fboResolution.GetFloat();
		imageWidth *= r_fboResolution.GetFloat();
		imageHeight *= r_fboResolution.GetFloat();
	}
	// only resize if the current dimensions can't hold it at all, otherwise subview renderings could thrash this
	if ( (useOversizedBuffer && (image->uploadWidth < imageWidth || image->uploadHeight < imageHeight)) ||
		(!useOversizedBuffer && (image->uploadWidth != imageWidth || image->uploadHeight != imageHeight)) ) {
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

	// Debug
	GL_CheckErrors();
}

void FB_CopyRender( const copyRenderCommand_t &cmd ) {
	//stgatilov #4754: this happens during lightgem calculating in minimized windowed TDM
	if ( cmd.imageWidth * cmd.imageHeight == 0 ) {
		return;	//no pixels to be read
	}
	int backEndStartTime = Sys_Milliseconds();

	if ( !primaryOn ) { // #4425: not applicable, raises gl errors
		qglReadBuffer( GL_BACK );
	}

	if ( primaryOn && r_multiSamples.GetInteger() > 1 ) {
		FB_ResolveMultisampling( GL_COLOR_BUFFER_BIT );
		qglBindFramebuffer( GL_FRAMEBUFFER, fboResolve );
	}

	// #4395 lightem pixel pack buffer optimization
	if ( cmd.buffer ) {
		if ( cmd.usePBO ) {
			static int pboSize = -1;

			if ( !pbo ) {
				pboSize = cmd.imageWidth * cmd.imageHeight * 3;
				qglGenBuffers( 1, &pbo );
				qglBindBuffer( GL_PIXEL_PACK_BUFFER, pbo );
				qglBufferData( GL_PIXEL_PACK_BUFFER, pboSize, NULL, GL_STREAM_READ );
				qglBindBuffer( GL_PIXEL_PACK_BUFFER, 0 );
			}

			if ( cmd.imageWidth * cmd.imageHeight * 3 != pboSize ) {
				common->Error( "CaptureRenderToBuffer: wrong PBO size %dx%d/%d", cmd.imageWidth, cmd.imageHeight, pboSize );
			}
			qglBindBuffer( GL_PIXEL_PACK_BUFFER, pbo );

			byte *ptr = reinterpret_cast< byte * >( qglMapBuffer( GL_PIXEL_PACK_BUFFER, GL_READ_ONLY ) );

			// #4395 moved to initializer
			if ( ptr ) {
				memcpy( cmd.buffer, ptr, pboSize );
				qglUnmapBuffer( GL_PIXEL_PACK_BUFFER );
			}

			// revelator: added c++11 nullptr
			qglReadPixels( cmd.x, cmd.y, cmd.imageWidth, cmd.imageHeight, GL_RGB, GL_UNSIGNED_BYTE, nullptr );
			qglBindBuffer( GL_PIXEL_PACK_BUFFER, 0 );
		} else {
			qglReadPixels( cmd.x, cmd.y, cmd.imageWidth, cmd.imageHeight, GL_RGB, GL_UNSIGNED_BYTE, cmd.buffer );
		}
	}

	if ( cmd.image )
		FB_CopyRender( cmd.image, cmd.x, cmd.y, cmd.imageWidth, cmd.imageHeight, false );

	if ( primaryOn && r_multiSamples.GetInteger() > 1 ) {
		qglBindFramebuffer( GL_FRAMEBUFFER, fboPrimary );
	}
	int backEndFinishTime = Sys_Milliseconds();
	backEnd.pc.msec += backEndFinishTime - backEndStartTime;
	GL_CheckErrors();
}

// force recreate on a cvar change
void DeleteFramebuffers() {
	qglDeleteFramebuffers( 1, &fboPrimary );
	qglDeleteFramebuffers( 1, &fboResolve );
	qglDeleteFramebuffers( 1, &fboShadowStencil );
	qglDeleteFramebuffers( 1, &fboShadowAtlas );
	fboPrimary = 0;
	fboResolve = 0;
	fboShadowStencil = 0;
	fboShadowAtlas = 0;
}

void CheckCreatePrimary() {
	// debug
	GL_CheckErrors();

	// virtual resolution as a modern alternative for actual desktop resolution affecting all other windows
	GLuint curWidth = r_fboResolution.GetFloat() * glConfig.vidWidth, curHeight = r_fboResolution.GetFloat() * glConfig.vidHeight;

	if (
		r_multiSamples.IsModified() || 
		r_fboSeparateStencil.IsModified() ||
		r_fboSharedDepth.IsModified() ||
		r_fboResolution.IsModified()
	) {
		// something FBO-related has changed, let's recreate everything from scratch
		r_multiSamples.ClearModified();
		r_fboSeparateStencil.ClearModified();
		r_fboSharedDepth.ClearModified();
		r_fboResolution.ClearModified();
		DeleteFramebuffers();
	}
		
	// create global depth texture (usually holds screen copy, used by postprocessing)
	if ( r_fboSeparateStencil.GetBool() ) {
		// intel optimization
		globalImages->currentDepthImage->GenerateAttachment( curWidth, curHeight, GL_DEPTH );
		if ( !r_softShadowsQuality.GetBool() ) // currentStencilFbo will be initialized in CheckCreateShadow with possibly different resolution
			globalImages->currentStencilFbo->GenerateAttachment( curWidth, curHeight, GL_STENCIL );
	} else {
		// AMD/nVidia fast enough already, separate depth/stencil not supported
		globalImages->currentDepthImage->GenerateAttachment( curWidth, curHeight, GL_DEPTH_STENCIL );
	}
	// create global color texture (used by post processing)
	globalImages->currentRenderImage->GenerateAttachment( curWidth, curHeight, GL_COLOR );

	// recreate FBOs and attach textures to them
	int msaa = r_multiSamples.GetInteger();
	if ( !fboPrimary ) {
		FB_CreatePrimaryResolve( curWidth, curHeight, msaa );
		bool useResolveFbo = ( !r_fboSharedDepth.GetBool() || msaa > 1 );

		qglBindFramebuffer( GL_FRAMEBUFFER, useResolveFbo ? fboResolve : fboPrimary );

		if ( useResolveFbo ) {
			// attach a texture to FBO color attachment point
			qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, globalImages->currentRenderImage->texnum, 0 );
		}

		// attach a texture to depth attachment point
		GLuint depthTex = globalImages->currentDepthImage->texnum;
		if ( r_fboSeparateStencil.GetBool() ) {
			// intel optimization
			qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex, 0 );
			qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, globalImages->currentStencilFbo->texnum, 0 );
		} else {
			// shorthand for "both depth and stencil"
			qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthTex, 0 );
		}
		int statusResolve = qglCheckFramebufferStatus( GL_FRAMEBUFFER );


		qglBindFramebuffer( GL_FRAMEBUFFER, fboPrimary );
		int statusPrimary = qglCheckFramebufferStatus( GL_FRAMEBUFFER );

		// something went wrong, fall back to default
		if ( GL_FRAMEBUFFER_COMPLETE != statusResolve || GL_FRAMEBUFFER_COMPLETE != statusPrimary ) {
			common->Printf( "glCheckFramebufferStatus - primary: %d - resolve: %d\n", statusPrimary, statusResolve );
			DeleteFramebuffers(); // try from scratch next time
			r_useFbo.SetBool( false );
			r_softShadowsQuality.SetInteger( 0 );
		}
		qglBindFramebuffer( GL_FRAMEBUFFER, 0 );
	}
	renderBufferColor.Size( curWidth, curHeight, msaa );
	renderBufferDepthStencil.Size( curWidth, curHeight, msaa );
}

void CheckCreateShadow() {
	// (re-)attach textures to FBO
	GLuint curWidth = glConfig.vidWidth;
	GLuint curHeight = glConfig.vidHeight;
	if ( primaryOn ) {
		float shadowRes = 1.0f;
		if ( r_softShadowsQuality.GetInteger() < 0 ) {
			shadowRes = r_softShadowsQuality.GetFloat() / -100.0f;
		}

		curWidth *= r_fboResolution.GetFloat() * shadowRes;
		curHeight *= r_fboResolution.GetFloat() * shadowRes;
	}

	bool depthBitsModified = r_fboDepthBits.IsModified();
	// reset textures
	auto stencilPath = r_shadows.GetInteger() == 1 || backEnd.vLight && backEnd.vLight->shadows == LS_STENCIL;
	if ( stencilPath )
		if ( r_fboSeparateStencil.GetBool() ) {
			// currentDepthImage is initialized there
			CheckCreatePrimary();
			globalImages->currentStencilFbo->GenerateAttachment( curWidth, curHeight, GL_STENCIL );
		} else {
			globalImages->shadowDepthFbo->GenerateAttachment( curWidth, curHeight, GL_DEPTH_STENCIL );
		}
	else
		globalImages->shadowAtlas->GenerateAttachment( 6 * r_shadowMapSize.GetInteger(), 6 * r_shadowMapSize.GetInteger(), GL_DEPTH );

	auto check = []( GLuint &fbo ) {
		int status = qglCheckFramebufferStatus( GL_FRAMEBUFFER );

		// something went wrong, fall back to default
		if ( GL_FRAMEBUFFER_COMPLETE != status ) {
			common->Printf( "glCheckFramebufferStatus shadow: %d\n", status );
			qglDeleteFramebuffers( 1, &fbo );
			fbo = 0; // try from scratch next time
		}
		qglBindFramebuffer( GL_FRAMEBUFFER, 0 );
	};

	if ( !stencilPath ) {
		if( !fboShadowAtlas ) {
			qglGenFramebuffers( 1, &fboShadowAtlas );
			qglBindFramebuffer( GL_FRAMEBUFFER, fboShadowAtlas );
			GLuint depthTex = globalImages->shadowAtlas->texnum;
			qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex, 0 );
			qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0 );
			check( fboShadowAtlas );
		}
		if ( ShadowAtlasPages[0].width != r_shadowMapSize.GetInteger() ) {
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
	} else {
		if ( !fboShadowStencil ) {
			qglGenFramebuffers( 1, &fboShadowStencil );
			qglBindFramebuffer( GL_FRAMEBUFFER, fboShadowStencil );
			if ( r_fboSeparateStencil.GetBool() ) {
				qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, globalImages->currentDepthImage->texnum, 0 );
				qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, globalImages->currentStencilFbo->texnum, 0 );
			} else {
				GLuint depthTex = globalImages->shadowDepthFbo->texnum;
				qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthTex, 0 );
			}
		}
	}
	GL_CheckErrors();
}

void FB_SelectPrimary() {
	if ( primaryOn ) {
		qglBindFramebuffer( GL_FRAMEBUFFER, fboPrimary );
	}
}

void FB_SelectPostProcess() {
	if ( !primaryOn ) {
		return;
	}
	GLuint curWidth = glConfig.vidWidth * r_fboResolution.GetFloat();
	GLuint curHeight = glConfig.vidHeight * r_fboResolution.GetFloat();

	if ( !fboPostProcess || curWidth != postProcessWidth || curHeight != postProcessHeight ) {
		FB_CreatePostProcess( curWidth, curHeight );
	}
	qglBindFramebuffer( GL_FRAMEBUFFER, fboPostProcess );
}

/*
Soft shadows vendor specific implementation
Intel: separate stencil buffer, direct access, awesome
Others: combined stencil & depth, copy to a separate FBO, meh
*/
void FB_BindShadowTexture() {
	GL_CheckErrors();
	if ( backEnd.vLight->shadowMapIndex ) {
		GL_SelectTexture( 6 );
		globalImages->shadowAtlas->Bind();
	} else {
		GL_SelectTexture( 6 );
		globalImages->currentDepthImage->Bind();
		GL_SelectTexture( 7 );

		if ( !r_fboSeparateStencil.GetBool() ) {
			globalImages->shadowDepthFbo->Bind();
			qglTexParameteri( GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_STENCIL_INDEX );
		} else {
			globalImages->currentStencilFbo->Bind();
		}
	}
	GL_CheckErrors();
}

// accidentally deleted
void FB_ApplyScissor() {
	if ( r_useScissor.GetBool() ) {
		float resFactor = 1.0f;
		GL_Scissor( backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1 * resFactor,
		            backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1 * resFactor,
		            backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1 * resFactor,
		            backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1 * resFactor );
	}
}

void FB_ToggleShadow( bool on ) {
	CheckCreateShadow();
	auto stencilPath = r_shadows.GetInteger() == 1 || backEnd.vLight && backEnd.vLight->shadows == LS_STENCIL;
	if ( on && stencilPath ) {
		// most vendors can't do separate stencil so we need to copy depth from the main/default FBO
		if ( !depthCopiedThisView && !r_fboSeparateStencil.GetBool() ) {
			if( primaryOn && r_multiSamples.GetInteger() > 1 ) {
				FB_ResolveMultisampling( GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
				qglBindFramebuffer( GL_READ_FRAMEBUFFER, fboResolve );
			}

			qglDisable( GL_SCISSOR_TEST );
			qglBindFramebuffer( GL_DRAW_FRAMEBUFFER, fboShadowStencil );
			qglBlitFramebuffer( 0, 0, globalImages->currentRenderImage->uploadWidth, globalImages->currentRenderImage->uploadHeight,
				0, 0, globalImages->currentRenderImage->uploadWidth, globalImages->currentRenderImage->uploadHeight,
				GL_DEPTH_BUFFER_BIT, GL_NEAREST );
			qglBindFramebuffer( GL_DRAW_FRAMEBUFFER, primaryOn ? fboPrimary : 0 );
			qglEnable( GL_SCISSOR_TEST );
			depthCopiedThisView = true;
		}
		GL_CheckErrors();
	}
	qglBindFramebuffer( GL_FRAMEBUFFER, on ? (stencilPath ? fboShadowStencil : fboShadowAtlas) : primaryOn ? fboPrimary : 0 );
	if( on && stencilPath && r_multiSamples.GetInteger() > 1 && r_softShadowsQuality.GetInteger() >= 0 ) {
		// with MSAA on, we need to render against the multisampled primary buffer, otherwise stencil is drawn
		// against a lower-quality depth map which may cause render errors with shadows
		qglBindFramebuffer( GL_FRAMEBUFFER, fboPrimary );
	}

	// stencil softshadows
	if ( stencilPath )
		shadowOn = on;
	GL_CheckErrors();

	if ( !stencilPath ) { // additional steps for shadowmaps
		qglDepthMask( on );
		if ( on ) {
			/*int mipmap = ShadowFboIndex / MAX_SHADOW_MAPS;
			int mapSize = r_shadowMapSize.GetInteger() >> mipmap;
			ShadowMipMap[ShadowFboIndex] = 0;
			int lightScreenSize = idMath::Imax( backEnd.vLight->scissorRect.GetWidth(), backEnd.vLight->scissorRect.GetHeight() ),
			         ScreenSize = idMath::Imin( glConfig.vidWidth, glConfig.vidHeight );

			while ( lightScreenSize < screenSize && ShadowMipMap[ShadowFboIndex] < 5 ) {
				ShadowMipMap[ShadowFboIndex]++; // select a smaller map for small/distant lights
				lightScreenSize <<= 1;
				mapSize >>= 1;
			}
			qglFramebufferTexture( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, globalImages->shadowCubeMap[ShadowFboIndex]->texnum, ShadowMipMap[ShadowFboIndex] );
			*/
			GL_State( GLS_DEPTHFUNC_LESS ); // reset in RB_GLSL_CreateDrawInteractions
			// the caller is now responsible for proper setup of viewport/scissor
		} else {
			const idScreenRect &r = backEnd.viewDef->viewport;

			GL_Viewport( r.x1, r.y1, r.x2 - r.x1 + 1, r.y2 - r.y1 + 1 );

			GL_Scissor( backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
				backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
				backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
				backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1 );
		}
	}
}

void FB_Clear() {
	fboPrimary = fboResolve = fboPostProcess = fboShadowStencil = fboShadowAtlas = pbo = 0;
	renderBufferColor.handle = renderBufferDepthStencil.handle = renderBufferPostProcess.handle = 0;
}

void EnterPrimary() {
	if ( r_softShadowsQuality.GetBool() ) {
		r_useGLSL = true;
	}
	depthCopiedThisView = false;

	if ( !r_useFbo.GetBool() ) {
		return;
	}

	if ( primaryOn ) {
		return;
	}
	CheckCreatePrimary();

	qglBindFramebuffer( GL_FRAMEBUFFER, fboPrimary );

	GL_Viewport( tr.viewportOffset[0] + backEnd.viewDef->viewport.x1,	// FIXME: must not use tr in backend
	             tr.viewportOffset[1] + backEnd.viewDef->viewport.y1,
	             backEnd.viewDef->viewport.x2 + 1 - backEnd.viewDef->viewport.x1,
	             backEnd.viewDef->viewport.y2 + 1 - backEnd.viewDef->viewport.y1 );

	GL_Scissor( tr.viewportOffset[0] + backEnd.viewDef->viewport.x1 + backEnd.viewDef->scissor.x1,
	            tr.viewportOffset[1] + backEnd.viewDef->viewport.y1 + backEnd.viewDef->scissor.y1,
	            backEnd.viewDef->scissor.x2 + 1 - backEnd.viewDef->scissor.x1,
	            backEnd.viewDef->scissor.y2 + 1 - backEnd.viewDef->scissor.y1 );

	qglClear( GL_COLOR_BUFFER_BIT ); // otherwise transparent skybox blends with previous frame

	primaryOn = true;

	GL_CheckErrors();
}

// switch from fbo to default framebuffer, copy content
void LeavePrimary() {
	if ( !primaryOn ) 
		return;
	primaryOn = false;
	GL_CheckErrors();

	GL_Viewport( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	GL_Scissor( 0, 0, glConfig.vidWidth, glConfig.vidHeight );

	if ( r_multiSamples.GetInteger() > 1 ) {
		FB_ResolveMultisampling( GL_COLOR_BUFFER_BIT );
		qglBindFramebuffer( GL_READ_FRAMEBUFFER, fboResolve );
	}
	qglBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 );
	qglBlitFramebuffer( 0, 0, globalImages->currentRenderImage->uploadWidth,
	                    globalImages->currentRenderImage->uploadHeight,
	                    0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR );

	if ( r_showFBO.GetInteger() ) {
		if ( r_multiSamples.GetInteger() > 1 ) {
			FB_ResolveMultisampling( GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
		}
		qglBindFramebuffer( GL_FRAMEBUFFER, 0 );

		qglLoadIdentity();
		qglMatrixMode( GL_PROJECTION );
		qglPushMatrix();
		qglLoadIdentity();
		qglOrtho( 0, 1, 0, 1, -1, 1 );

		GL_State( GLS_DEFAULT );
		qglDisable( GL_DEPTH_TEST );
		qglColor3f( 1, 1, 1 );

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
		default:
			globalImages->currentRenderImage->Bind();
		}
		RB_DrawFullScreenQuad();

		qglEnable( GL_DEPTH_TEST );
		qglPopMatrix();
		qglMatrixMode( GL_MODELVIEW );
		GL_SelectTexture( 0 );
	}
	qglBindFramebuffer( GL_FRAMEBUFFER, 0 );

	if ( r_frontBuffer.GetBool() ) {
		qglFinish();
	}
	GL_CheckErrors();
}

void FB_TogglePrimary( bool on ) {
	if ( on ) {
		EnterPrimary();
	} else {
		LeavePrimary();
	}
}

/*
====================
GL_Scissor

Utility function,
if you absolutely must check for anything out of the ordinary, then do it here.
====================
*/
void GL_Scissor( int x /* left*/, int y /* bottom */, int w, int h ) {
	// x and y can be negative, but neither width nor height must be.
	if ( w <= 0 || h <= 0 )
		return;
	if ( primaryOn ) {
		x *= r_fboResolution.GetFloat();
		y *= r_fboResolution.GetFloat();
		w *= r_fboResolution.GetFloat();
		h *= r_fboResolution.GetFloat();
	}
	qglScissor( x, y, w, h );
}

/*
====================
GL_Viewport

Utility function,
if you absolutly must check for anything out of the ordinary, then do it here.
====================
*/
void GL_Viewport( int x /* left */, int y /* bottom */, int w, int h ) {
	// x and y can be negative, but neither width nor height must be.
	if ( w <= 0 || h <= 0 )
		return;
	if ( primaryOn ) {
		x *= r_fboResolution.GetFloat();
		y *= r_fboResolution.GetFloat();
		w *= r_fboResolution.GetFloat();
		h *= r_fboResolution.GetFloat();
	}
	qglViewport( x, y, w, h );
}