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

bool isInFbo;
bool depthCopiedThisView;
GLuint fboPrimary, fboShadow;

/*
called when post-proceesing is about to start, needs pixels
we need to copy render separately for water/smoke and then again for bloom
*/
void FB_CopyColorBuffer() {
	GL_SelectTexture( 0 );
	if ( !isInFbo || !r_fboSharedColor.GetBool() ) {
		globalImages->currentRenderImage->Bind();
		qglCopyTexImage2D( GL_TEXTURE_2D, 0, isInFbo && r_fboColorBits.GetInteger() == 15 ? GL_RGB5_A1 : GL_RGBA,
			0, 0, globalImages->currentRenderImage->uploadWidth, globalImages->currentRenderImage->uploadHeight, 0 );
	}
}

void CheckCreatePrimary() {
	GL_CheckErrors(); // debug
	// virtual resolution as a modern alternative for actual desktop resolution affecting all other windows
	GLuint curWidth = r_fboResolution.GetFloat() * glConfig.vidWidth, curHeight = r_fboResolution.GetFloat() * glConfig.vidHeight;

	// reset textures 
	if ( curWidth != globalImages->currentRenderImage->uploadWidth || curHeight != globalImages->currentRenderImage->uploadHeight
		|| curWidth != globalImages->currentDepthImage->uploadWidth || curHeight != globalImages->currentDepthImage->uploadHeight
		|| r_fboColorBits.IsModified()
		) { // FIXME don't allocate memory if sharing color/depth
		r_fboColorBits.ClearModified();

		globalImages->currentRenderImage->Bind();
		globalImages->currentRenderImage->uploadWidth = curWidth; // used as a shader param
		globalImages->currentRenderImage->uploadHeight = curHeight;
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		qglTexImage2D( GL_TEXTURE_2D, 0, r_fboColorBits.GetInteger() == 15 ? GL_RGB5_A1 : GL_RGBA, curWidth, curHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL ); //NULL means reserve texture memory, but texels are undefined

		globalImages->currentRenderFbo->Bind();
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		qglTexImage2D( GL_TEXTURE_2D, 0, r_fboColorBits.GetInteger() == 15 ? GL_RGB5_A1 : GL_RGBA, curWidth, curHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL ); //NULL means reserve texture memory, but texels are undefined

		if ( glConfig.vendor == glvIntel ) {
			globalImages->currentStencilFbo->Bind();
			globalImages->currentStencilFbo->uploadWidth = curWidth;
			globalImages->currentStencilFbo->uploadHeight = curHeight;
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
			qglTexImage2D( GL_TEXTURE_2D, 0, GL_STENCIL_INDEX8, curWidth, curHeight, 0, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, 0 );
		}

		/*globalImages->stencilCopy->Bind();
		globalImages->stencilCopy->uploadWidth = curWidth;
		globalImages->stencilCopy->uploadHeight = curHeight;
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		qglTexImage2D( GL_TEXTURE_2D, 0, GL_STENCIL_INDEX8, curWidth, curHeight, 0, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, 0 );*/

		globalImages->currentDepthImage->Bind();
		globalImages->currentDepthImage->uploadWidth = curWidth; // used as a shader param
		globalImages->currentDepthImage->uploadHeight = curHeight;
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		if ( glConfig.vendor == glvIntel ) { // FIXME allow 24-bit depth for low-res monitors
			qglTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, curWidth, curHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0 );
		} else {
			qglTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, curWidth, curHeight, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0 );
		}
	}

	// (re-)attach textures to FBO
	if ( !fboPrimary || r_fboSharedColor.IsModified() /*|| r_fboSharedDepth.IsModified() */) {
		if ( !fboPrimary )
			qglGenFramebuffers( 1, &fboPrimary );
		r_fboSharedColor.ClearModified();
		//r_fboSharedDepth.ClearModified();
		qglBindFramebuffer( GL_FRAMEBUFFER_EXT, fboPrimary );
		// attach a texture to FBO color attachement point
		if ( r_fboSharedColor.GetBool() )
			qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, globalImages->currentRenderImage->texnum, 0 );
		else
			qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, globalImages->currentRenderFbo->texnum, 0 );
		// attach a renderbuffer to depth attachment point
		//GLuint depthTex = r_fboSharedDepth.GetBool() ? globalImages->currentDepthImage->texnum : globalImages->currentDepthFbo->texnum;
		GLuint depthTex = globalImages->currentDepthImage->texnum;
		if ( glConfig.vendor == glvIntel ) { // separate stencil, thank God
			qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex, 0 );
			qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, globalImages->currentStencilFbo->texnum, 0 );
		} else {
			qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthTex, 0 );
		}
		int status = qglCheckFramebufferStatus( GL_FRAMEBUFFER );
		if ( GL_FRAMEBUFFER_COMPLETE != status ) { // something went wrong, fall back to default
			common->Printf( "glCheckFramebufferStatus %d\n", status );
			qglDeleteFramebuffers( 1, &fboPrimary );
			fboPrimary = 0; // try from scratch next time
			r_useFbo.SetBool( false );
			r_softShadowsQuality.SetInteger( 0 );
		}
		qglBindFramebuffer( GL_FRAMEBUFFER, 0 );
	}
}

void CheckCreateShadow() {
	// (re-)attach textures to FBO
	GLuint curWidth = r_fboResolution.GetFloat() * glConfig.vidWidth, curHeight = r_fboResolution.GetFloat() * glConfig.vidHeight;
	textureType_t type = r_shadows.GetInteger() == 2 ? TT_CUBIC : TT_2D;
	static textureType_t nowType;

	// reset textures 
	if ( curWidth != globalImages->currentDepthFbo->uploadWidth 
		|| curHeight != globalImages->currentDepthFbo->uploadHeight 
		|| nowType != type
	) {
		if ( type == TT_2D ) {
			globalImages->currentDepthFbo->Bind();
			globalImages->currentDepthFbo->uploadWidth = curWidth;
			globalImages->currentDepthFbo->uploadHeight = curHeight;
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
			if ( glConfig.vendor == glvIntel ) // FIXME allow 24-bit depth for low-res monitors
				qglTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, curWidth, curHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0 );
			else 
				qglTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_STENCIL, curWidth, curHeight, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0 );
		} else {
			globalImages->shadowCubeMap->Bind();
			int size = 256;
			globalImages->shadowCubeMap->uploadWidth = size;
			globalImages->shadowCubeMap->uploadHeight = size;
			for ( int sideId = 0; sideId < 6; sideId++ ) 
				qglTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + sideId, 0, GL_DEPTH_COMPONENT, size, size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL );
			qglTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
			qglTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
			qglTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
			qglTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
			qglTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );
			qglTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE );
		}
		globalImages->BindNull();
	}
	if ( (!fboShadow || nowType != type) && (glConfig.vendor != glvIntel || r_shadows.GetInteger() == 2) ) {
		if ( !fboShadow )
			qglGenFramebuffers( 1, &fboShadow );
		qglBindFramebuffer( GL_FRAMEBUFFER, fboShadow );
		if ( r_shadows.GetInteger() == 2 ) {
			GLuint depthTex = globalImages->shadowCubeMap->texnum;
			//qglFramebufferTexture( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTex, 0 );
			/*for ( int sideId = 0; sideId < 6; sideId++ )
				qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + sideId, depthTex, 0 );*/
			qglFramebufferTexture( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTex, 0 );
			qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0 );
			//qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, globalImages->currentRenderFbo->texnum, 0 );
		} else {
			GLuint depthTex = globalImages->currentDepthFbo->texnum;
			qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthTex, 0 );
		}
		int status = qglCheckFramebufferStatus( GL_FRAMEBUFFER );
		if ( GL_FRAMEBUFFER_COMPLETE != status ) { // something went wrong, fall back to default
			common->Printf( "glCheckFramebufferStatus %d\n", status );
			qglDeleteFramebuffers( 1, &fboShadow );
			fboShadow = 0; // try from scratch next time
		}
		qglBindFramebuffer( GL_FRAMEBUFFER, 0 );
		nowType = type;
	}
	GL_CheckErrors();
}

/*
Soft shadows vendor specific implementation
Intel: separate stencil buffer, direct access, awesome
Others: combined stencil & depth, copy to a separate FBO, meh
*/
void FB_BindShadowTexture() {
	GL_CheckErrors();
	if ( r_shadows.GetInteger() == 2 ) {
		GL_SelectTexture( 6 );
		globalImages->shadowCubeMap->Bind();
	} else {
		GL_SelectTexture( 6 );
		globalImages->currentDepthImage->Bind();
		GL_SelectTexture( 7 );
		if ( glConfig.vendor != glvIntel ) {
			globalImages->currentDepthFbo->Bind();
			const GLenum GL_DEPTH_STENCIL_TEXTURE_MODE = 0x90EA;
			qglTexParameteri( GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_STENCIL_INDEX );
		} else
			globalImages->currentStencilFbo->Bind();
	}
	GL_CheckErrors();
}

void FB_ToggleShadow( bool on ) {
	if ( glConfig.vendor == glvIntel && r_shadows.GetInteger() < 2 ) // "Click when ready" screen calls this when not in FBO
		return;
	CheckCreateShadow();
	if ( on && !depthCopiedThisView && r_shadows.GetInteger() == 1 ) {
		globalImages->currentDepthFbo->Bind();
		qglCopyTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_STENCIL, 0, 0, globalImages->currentDepthFbo->uploadWidth, globalImages->currentDepthFbo->uploadHeight, 0 );
		depthCopiedThisView = true;
		GL_CheckErrors();
	}
	qglBindFramebuffer( GL_FRAMEBUFFER, on ? fboShadow : r_useFbo.GetBool() ? fboPrimary : 0 );
	GL_CheckErrors();
}

void FB_Clear() {
	fboPrimary = fboShadow = 0;
}

void FB_Enter() {
	if ( r_softShadowsQuality.GetBool() ) {
		r_useGLSL.SetBool( true );
		//r_useFbo.SetBool( true );
		//r_fboSharedDepth.SetBool( true );
	}
	depthCopiedThisView = false;
	if ( !r_useFbo.GetBool() )
		return;
	if ( isInFbo )
		return;
	CheckCreatePrimary();
	qglBindFramebuffer( GL_FRAMEBUFFER, fboPrimary );
	qglClear( GL_COLOR_BUFFER_BIT ); // otherwise transparent skybox blends with previous frame
	isInFbo = true;
	GL_CheckErrors();
}

// switch from fbo to default framebuffer, copy content
void FB_Leave( viewDef_t* viewDef ) {
	if ( !isInFbo )
		return;
	GL_CheckErrors();
	//FB_CopyColorBuffer();
	// hasn't worked very well at the first approach, maybe retry later
	/*glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, globalImages->currentRenderImage->uploadWidth, globalImages->currentRenderImage->uploadHeight, 0, 0,
	glConfig.vidWidth, glConfig.vidHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST); */
	qglBindFramebuffer( GL_FRAMEBUFFER_EXT, 0 );
	qglLoadIdentity();
	qglMatrixMode( GL_PROJECTION );
	qglPushMatrix();
	qglLoadIdentity();
	qglOrtho( 0, 1, 0, 1, -1, 1 );
	qglViewport( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	qglScissor( 0, 0, glConfig.vidWidth, glConfig.vidHeight );

	GL_State( GLS_DEFAULT );
	qglDisable( GL_DEPTH_TEST );
	qglColor3f( 1, 1, 1 );
	{
		switch ( r_fboDebug.GetInteger() )
		{
		case 1:
			globalImages->currentRenderImage->Bind();
			break;
		case 2:
			globalImages->currentDepthImage->Bind();
			break;
		case 3:
			globalImages->currentDepthFbo->Bind();
			break;
		default:
			if ( r_fboSharedColor.GetBool() )
				globalImages->currentRenderImage->Bind();
			else
				globalImages->currentRenderFbo->Bind();
		}
		RB_DrawFullScreenQuad();
	}
	qglEnable( GL_DEPTH_TEST );
	qglPopMatrix();
	qglMatrixMode( GL_MODELVIEW );
	GL_SelectTexture( 0 );
	if ( viewDef ) { // switch back to normal resolution for correct 2d
		tr.renderCrops[0].width = glConfig.vidWidth;
		tr.renderCrops[0].height = glConfig.vidHeight;
		viewDef->viewport.x2 = glConfig.vidWidth - 1;
		viewDef->viewport.y2 = glConfig.vidHeight - 1;
		viewDef->scissor.x2 = glConfig.vidWidth - 1;
		viewDef->scissor.y2 = glConfig.vidHeight - 1;
	}
	isInFbo = false;
	GL_CheckErrors();
}