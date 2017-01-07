/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod (http://www.thedarkmod.com/)
 
 $Revision$ (Revision of last commit) 
 $Date$ (Date of last commit)
 $Author$ (Author of last commit)
 
******************************************************************************/
#include "precompiled_engine.h"
#pragma hdrstop

static bool versioned = RegisterVersionedFile("$Id$");

#include "tr_local.h"


frameData_t		*frameData;
backEndState_t	backEnd;


/*
======================
RB_SetDefaultGLState

This should initialize all GL state that any part of the entire program
may touch, including the editor.
======================
*/
void RB_SetDefaultGLState( void ) {

	RB_LogComment( "--- R_SetDefaultGLState ---\n" );

	qglClearDepth( 1.0f );
	qglColor4f (1.0f, 1.0f, 1.0f, 1.0f);

	// the vertex array is always enabled
	qglEnableClientState( GL_VERTEX_ARRAY );
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
	qglDisableClientState( GL_COLOR_ARRAY );

	// make sure our GL state vector is set correctly
	memset( &backEnd.glState, 0, sizeof( backEnd.glState ) );
	backEnd.glState.forceGlState = true;

	qglColorMask( 1, 1, 1, 1 );

	qglEnable( GL_DEPTH_TEST );
	qglEnable( GL_BLEND );
	qglEnable( GL_SCISSOR_TEST );
	qglEnable( GL_CULL_FACE );
	qglDisable( GL_LIGHTING );
	qglDisable( GL_LINE_STIPPLE );
	qglDisable( GL_STENCIL_TEST );

	qglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	qglDepthMask( GL_TRUE );
	qglDepthFunc( GL_ALWAYS );
 
	qglCullFace( GL_FRONT_AND_BACK );
	qglShadeModel( GL_SMOOTH );

	if ( r_useScissor.GetBool() ) {
		qglScissor( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	}

	for ( int i = glConfig.maxTextureUnits - 1 ; i >= 0 ; i-- ) {
		GL_SelectTexture( i );

		// object linear texgen is our default
		qglTexGenf( GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
		qglTexGenf( GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
		qglTexGenf( GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
		qglTexGenf( GL_Q, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );

		GL_TexEnv( GL_MODULATE );
		qglDisable( GL_TEXTURE_2D );
		if ( glConfig.texture3DAvailable ) {
			qglDisable( GL_TEXTURE_3D );
		}
		if ( glConfig.cubeMapAvailable ) {
			qglDisable( GL_TEXTURE_CUBE_MAP_EXT );
		}
	}
}


/*
====================
RB_LogComment
====================
*/
void RB_LogComment( const char *comment, ... ) {
	if ( !tr.logFile ) {
		return;
	}

	va_list marker;

	fprintf( tr.logFile, "// " );
	va_start( marker, comment );
	vfprintf( tr.logFile, comment, marker );
	va_end( marker );
}


//=============================================================================



/*
====================
GL_SelectTexture
====================
*/
void GL_SelectTexture( const int unit ) {
	if ( backEnd.glState.currenttmu == unit ) {
		return;
	}

	if ( unit < 0 || (unit >= glConfig.maxTextureUnits && unit >= glConfig.maxTextureImageUnits) ) {
		common->Warning( "GL_SelectTexture: unit = %i", unit );
		return;
	}

	qglActiveTextureARB( GL_TEXTURE0_ARB + unit );
	qglClientActiveTextureARB( GL_TEXTURE0_ARB + unit );
	RB_LogComment( "glActiveTextureARB( %i );\nglClientActiveTextureARB( %i );\n", unit, unit );

	backEnd.glState.currenttmu = unit;
}


/*
====================
GL_Cull

This handles the flipping needed when the view being
rendered is a mirored view.
====================
*/
void GL_Cull( const int cullType ) {
	if ( backEnd.glState.faceCulling == cullType ) {
		return;
	}

	if ( cullType == CT_TWO_SIDED ) {
		qglDisable( GL_CULL_FACE );
	} else {
		if ( backEnd.glState.faceCulling == CT_TWO_SIDED ) {
			qglEnable( GL_CULL_FACE );
		}

		if ( cullType == CT_BACK_SIDED ) {
			if ( backEnd.viewDef->isMirror ) {
				qglCullFace( GL_FRONT );
			} else {
				qglCullFace( GL_BACK );
			}
		} else {
			if ( backEnd.viewDef->isMirror ) {
				qglCullFace( GL_BACK );
			} else {
				qglCullFace( GL_FRONT );
			}
		}
	}

	backEnd.glState.faceCulling = cullType;
}

/*
====================
GL_TexEnv
====================
*/
void GL_TexEnv( int env ) {

	tmu_t *tmu = &backEnd.glState.tmu[backEnd.glState.currenttmu];
	if ( env == tmu->texEnv ) {
		return;
	}

	tmu->texEnv = env;

	if ( env & (GL_COMBINE_EXT|GL_MODULATE|GL_REPLACE|GL_DECAL|GL_ADD) ) {
		qglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, env );
	} else {
		common->Error( "GL_TexEnv: invalid env '%d' passed\n", env );
	}

}

/*
=================
GL_ClearStateDelta

Clears the state delta bits, so the next GL_State
will set every item
=================
*/
void GL_ClearStateDelta( void ) {
	backEnd.glState.forceGlState = true;
}

/*
====================
GL_State

This routine is responsible for setting the most commonly changed state
====================
*/
void GL_State( const int stateBits ) {

#if 1
	int diff;
	if ( !r_useStateCaching.GetBool() || backEnd.glState.forceGlState ) {
		// make sure everything is set all the time, so we
		// can see if our delta checking is screwing up
		diff = -1;
		backEnd.glState.forceGlState = false;
	} else {
		diff = stateBits ^ backEnd.glState.glStateBits;
		if ( !diff ) {
			return;
		}
	}
#else
	// angua: this caused light gem problems (lg changed based on view angle)
	// it's important to set diff to -1 if force gl state is true
	const int diff = stateBits ^ backEnd.glState.glStateBits;

	if ( !diff ) {
		return;
	}

	if ( backEnd.glState.forceGlState ) {
		backEnd.glState.forceGlState = false;
	}
#endif

	// check depthFunc bits
	if ( diff & ( GLS_DEPTHFUNC_EQUAL | GLS_DEPTHFUNC_LESS | GLS_DEPTHFUNC_ALWAYS ) ) {
		if ( stateBits & GLS_DEPTHFUNC_EQUAL ) {
			qglDepthFunc( GL_EQUAL );
		} else if ( stateBits & GLS_DEPTHFUNC_ALWAYS ) {
			qglDepthFunc( GL_ALWAYS );
		} else {
			qglDepthFunc( GL_LEQUAL );
		}
	}

	// check blend bits
	if ( diff & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) {
		GLenum srcFactor, dstFactor;

		switch ( stateBits & GLS_SRCBLEND_BITS ) {
		case GLS_SRCBLEND_ONE:
			srcFactor = GL_ONE;
			break;
		case GLS_SRCBLEND_ZERO:
			srcFactor = GL_ZERO;
			break;
		case GLS_SRCBLEND_DST_COLOR:
			srcFactor = GL_DST_COLOR;
			break;
		case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
			srcFactor = GL_ONE_MINUS_DST_COLOR;
			break;
		case GLS_SRCBLEND_SRC_ALPHA:
			srcFactor = GL_SRC_ALPHA;
			break;
		case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:
			srcFactor = GL_ONE_MINUS_SRC_ALPHA;
			break;
		case GLS_SRCBLEND_DST_ALPHA:
			srcFactor = GL_DST_ALPHA;
			break;
		case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
			srcFactor = GL_ONE_MINUS_DST_ALPHA;
			break;
		case GLS_SRCBLEND_ALPHA_SATURATE:
			srcFactor = GL_SRC_ALPHA_SATURATE;
			break;
		default:
			srcFactor = GL_ONE;		// to get warning to shut up
			common->Error( "GL_State: invalid src blend state bits\n" );
			break;
		}

		switch ( stateBits & GLS_DSTBLEND_BITS ) {
		case GLS_DSTBLEND_ZERO:
			dstFactor = GL_ZERO;
			break;
		case GLS_DSTBLEND_ONE:
			dstFactor = GL_ONE;
			break;
		case GLS_DSTBLEND_SRC_COLOR:
			dstFactor = GL_SRC_COLOR;
			break;
		case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:
			dstFactor = GL_ONE_MINUS_SRC_COLOR;
			break;
		case GLS_DSTBLEND_SRC_ALPHA:
			dstFactor = GL_SRC_ALPHA;
			break;
		case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:
			dstFactor = GL_ONE_MINUS_SRC_ALPHA;
			break;
		case GLS_DSTBLEND_DST_ALPHA:
			dstFactor = GL_DST_ALPHA;
			break;
		case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
			dstFactor = GL_ONE_MINUS_DST_ALPHA;
			break;
		default:
			dstFactor = GL_ONE;		// to get warning to shut up
			common->Error( "GL_State: invalid dst blend state bits\n" );
			break;
		}

		qglBlendFunc( srcFactor, dstFactor );
	}

	// check depthmask
	if ( diff & GLS_DEPTHMASK ) {
		if ( stateBits & GLS_DEPTHMASK ) {
			qglDepthMask( GL_FALSE );
		} else {
			qglDepthMask( GL_TRUE );
		}
	}

	// check colormask
	if ( diff & (GLS_REDMASK|GLS_GREENMASK|GLS_BLUEMASK|GLS_ALPHAMASK) ) {
		qglColorMask(
		!( stateBits & GLS_REDMASK ),
		!( stateBits & GLS_GREENMASK ),
		!( stateBits & GLS_BLUEMASK ),
		!( stateBits & GLS_ALPHAMASK )
		);
	}

	// fill/line mode
	if ( diff & GLS_POLYMODE_LINE ) {
		if ( stateBits & GLS_POLYMODE_LINE ) {
			qglPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		} else {
			qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		}
	}

	// alpha test
	if ( diff & GLS_ATEST_BITS ) {
		switch ( stateBits & GLS_ATEST_BITS ) {
		case 0:
			qglDisable( GL_ALPHA_TEST );
			break;
		case GLS_ATEST_EQ_255:
			qglEnable( GL_ALPHA_TEST );
			qglAlphaFunc( GL_EQUAL, 1 );
			break;
		case GLS_ATEST_LT_128:
			qglEnable( GL_ALPHA_TEST );
			qglAlphaFunc( GL_LESS, 0.5 );
			break;
		case GLS_ATEST_GE_128:
			qglEnable( GL_ALPHA_TEST );
			qglAlphaFunc( GL_GEQUAL, 0.5 );
			break;
		default:
			assert( 0 );
			break;
		}
	}

	backEnd.glState.glStateBits = stateBits;
}

//anon begin
/*
========================
GL_DepthBoundsTest
========================
*/
void GL_DepthBoundsTest(const float zmin, const float zmax)
{
	if (!glConfig.depthBoundsTestAvailable || zmin > zmax)
	{
		return;
	}

	if (zmin == 0.0f && zmax == 0.0f)
	{
		qglDisable(GL_DEPTH_BOUNDS_TEST_EXT);
	}
	else
	{
		qglEnable(GL_DEPTH_BOUNDS_TEST_EXT);
		qglDepthBoundsEXT(zmin, zmax);
	}
}
//anon end

/*
============================================================================

RENDER BACK END THREAD FUNCTIONS

============================================================================
*/

/*
=============
RB_SetGL2D

This is not used by the normal game paths, just by some tools
=============
*/
void RB_SetGL2D( void ) {
	// set 2D virtual screen size
	qglViewport( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	if ( r_useScissor.GetBool() ) {
		qglScissor( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	}
	qglMatrixMode( GL_PROJECTION );
    qglLoadIdentity();
	qglOrtho( 0, 640, 480, 0, 0, 1 );		// always assume 640x480 virtual coordinates
	qglMatrixMode( GL_MODELVIEW );
    qglLoadIdentity();

	GL_State( GLS_DEPTHFUNC_ALWAYS |
			  GLS_SRCBLEND_SRC_ALPHA |
			  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

	GL_Cull( CT_TWO_SIDED );

	qglDisable( GL_DEPTH_TEST );
	qglDisable( GL_STENCIL_TEST );
}



/*
=============
RB_SetBuffer

=============
*/
static void	RB_SetBuffer( const void *data ) {
	const setBufferCommand_t	*cmd;

	// see which draw buffer we want to render the frame to

	cmd = (const setBufferCommand_t *)data;

	backEnd.frameCount = cmd->frameCount;

	if (!r_useFbo.GetBool()) // duzenko #4425: not applicable, raises gl errors
		qglDrawBuffer( cmd->buffer );

	// clear screen for debugging
	// automatically enable this with several other debug tools
	// that might leave unrendered portions of the screen
	if ( r_clear.GetFloat() || idStr::Length( r_clear.GetString() ) != 1 || r_lockSurfaces.GetBool() || r_singleArea.GetBool() || r_showOverDraw.GetBool() ) {
		float c[3];
		if ( sscanf( r_clear.GetString(), "%f %f %f", &c[0], &c[1], &c[2] ) == 3 ) {
			qglClearColor( c[0], c[1], c[2], 1 );
		} else if ( r_clear.GetInteger() == 2 ) {
			qglClearColor( 0.0f, 0.0f,  0.0f, 1.0f );
		} else if ( r_showOverDraw.GetBool() ) {
			qglClearColor( 1.0f, 1.0f, 1.0f, 1.0f );
		} else {
			qglClearColor( 0.4f, 0.0f, 0.25f, 1.0f );
		}
		if (!r_useFbo.GetBool()) // duzenko #4425: not needed for default framebuffer, happens elsewhere for fbo
			qglClear( GL_COLOR_BUFFER_BIT );
	}
}

/*
===============
RB_ShowImages

Draw all the images to the screen, on top of whatever
was there.  This is used to test for texture thrashing.
===============
*/
void RB_ShowImages( void ) {
	idImage	*image;
	float	x, y, w, h;
	//int		start, end;

	// Serp - Disabled in gpl - draw with grey background
	//RB_SetGL2D();
	//qglClearColor( 0.2, 0.2, 0.2, 1 );
	//qglClear( GL_COLOR_BUFFER_BIT );
	//qglFinish();

	//start = Sys_Milliseconds();

	for ( int i = 0 ; i < globalImages->images.Num() ; i++ ) {
		image = globalImages->images[i];

		if ( image->texnum == idImage::TEXTURE_NOT_LOADED && image->partialImage == NULL ) {
			continue;
		}

		w = glConfig.vidWidth / 20;
		h = glConfig.vidHeight / 15;
		x = i % 20 * w;
		y = i / 20 * h;

		// show in proportional size in mode 2
		if ( r_showImages.GetInteger() == 2 ) {
			w *= image->uploadWidth / 512.0f;
			h *= image->uploadHeight / 512.0f;
		}

		image->Bind();
		qglBegin (GL_QUADS);
		qglTexCoord2f( 0, 0 );
		qglVertex2f( x, y );
		qglTexCoord2f( 1, 0 );
		qglVertex2f( x + w, y );
		qglTexCoord2f( 1, 1 );
		qglVertex2f( x + w, y + h );
		qglTexCoord2f( 0, 1 );
		qglVertex2f( x, y + h );
		qglEnd();
	}

	qglFinish();

	//end = Sys_Milliseconds();

	//Serp : This was enabled in gpl, it's fairly annoying however.
	// You will need to uncomment the vars above.
	//common->Printf( "%i msec to draw all images\n", end - start );
}


/*
=============
RB_SwapBuffers

=============
*/
const void	RB_SwapBuffers( const void *data ) {
	// texture swapping test
	if ( r_showImages.GetInteger() != 0 ) {
		RB_ShowImages();
	}

	// force a gl sync if requested
	if ( r_finish.GetBool() ) {
		qglFinish();
	}

    RB_LogComment( "***************** RB_SwapBuffers *****************\n" );

	// don't flip if drawing to front buffer
	if ( !r_frontBuffer.GetBool() ) {
	    GLimp_SwapBuffers();
	}
}

bool doFboBloom; // duzenko FIXME - use r_postprocess instead
/*
=============
RB_CopyRender

Copy part of the current framebuffer to an image
=============
*/

const void RB_CopyRender( const void *data ) {
	if (r_skipCopyTexture.GetBool()) {
		return;
	}

	const copyRenderCommand_t *cmd = (copyRenderCommand_t *)data;

    RB_LogComment( "***************** RB_CopyRender *****************\n" );

	if (cmd->image) {
		if (cmd->image->imgName.Icmp( "_bloomImage" ) == 0) // duzenko: hack? better to extend renderCommand_t? not necessary at all because of r_postprocess?
			doFboBloom = true;
		else
			cmd->image->CopyFramebuffer( cmd->x, cmd->y, cmd->imageWidth, cmd->imageHeight, false );
	}
}

void RB_DrawFullScreenQuad() {
	qglBegin( GL_QUADS );
	qglTexCoord2f( 0, 0 );
	qglVertex2f( 0, 0 );
	qglTexCoord2f( 0, 1 );
	qglVertex2f( 0, 1 );
	qglTexCoord2f( 1, 1 );
	qglVertex2f( 1, 1 );
	qglTexCoord2f( 1, 0 );
	qglVertex2f( 1, 0 );
	qglEnd();
}

/*
=============
RB_FboBloom

Originally in front renderer (idPlayerView::dnPostProcessManager), moved here fbo-only
=============
*/

const void RB_FboBloom() {
	doFboBloom = false;
	int w = globalImages->currentRenderImage->uploadWidth, h = globalImages->currentRenderImage->uploadHeight;

	//GL_State( GLS_DEPTHMASK );
	qglEnable( GL_VERTEX_PROGRAM_ARB );
	qglEnable( GL_FRAGMENT_PROGRAM_ARB );
	GL_SelectTexture( 0 );
	extern void RB_DumpFramebuffer( const char *fileName );
	float	parm[4];

	qglViewport( 0, 0, 256, 1 );
	qglClear( GL_COLOR_BUFFER_BIT );
	qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, VPROG_BLOOM_COOK_MATH1 );
	qglBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, FPROG_BLOOM_COOK_MATH1 );
	parm[0] = 0.8f;
	parm[1] = 0.82f;
	parm[2] = 0.9f;
	parm[3] = 1;
	qglProgramLocalParameter4fvARB( GL_VERTEX_PROGRAM_ARB, 0, parm );
	RB_DrawFullScreenQuad();
	globalImages->GetImage( "_cookedMath" )->CopyFramebuffer( 0, 0, 256, 1, false );
	RB_DumpFramebuffer( "be_bl_m1.tga" );

	qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, VPROG_BLOOM_COOK_MATH2 );
	qglBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, FPROG_BLOOM_COOK_MATH2 );
	parm[0] = 0.2f;
	parm[1] = 2;
	parm[2] = 5;
	parm[3] = 0.1f;
	qglProgramLocalParameter4fvARB( GL_VERTEX_PROGRAM_ARB, 0, parm );
	RB_DrawFullScreenQuad();
	globalImages->GetImage( "_cookedMath" )->CopyFramebuffer( 0, 0, 256, 1, false );
	RB_DumpFramebuffer( "be_bl_m2.tga" );

	qglViewport( 0, 0, w/2, h/2 );
	GL_SelectTexture( 0 );
	globalImages->currentRenderImage->Bind();
	GL_SelectTexture( 1 );
	globalImages->GetImage( "_cookedMath" )->Bind();
	qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, VPROG_BLOOM_BRIGHTNESS );
	qglBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, FPROG_BLOOM_BRIGHTNESS );
	RB_DrawFullScreenQuad();
	GL_SelectTexture( 0 );
	globalImages->GetImage( "_bloomImage" )->CopyFramebuffer( 0, 0, w / 2, h / 2, false );
	RB_DumpFramebuffer( "be_bl_br.tga" );

	globalImages->GetImage( "_bloomImage" )->Bind();
	qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, VPROG_BLOOM_GAUSS_BLRX );
	qglBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, FPROG_BLOOM_GAUSS_BLRX );
	parm[0] = 2/w;
	parm[1] = 1;
	parm[2] = 1;
	parm[3] = 1;
	qglProgramLocalParameter4fvARB( GL_VERTEX_PROGRAM_ARB, 0, parm );
	RB_DrawFullScreenQuad();
	globalImages->GetImage( "_bloomImage" )->CopyFramebuffer( 0, 0, w/2, h/2, false );
	RB_DumpFramebuffer( "be_bl_gx.tga" );

	qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, VPROG_BLOOM_GAUSS_BLRY );
	qglBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, FPROG_BLOOM_GAUSS_BLRY );
	parm[0] = 2 / h;
	qglProgramLocalParameter4fvARB( GL_VERTEX_PROGRAM_ARB, 0, parm );
	RB_DrawFullScreenQuad();
	globalImages->GetImage( "_bloomImage" )->CopyFramebuffer( 0, 0, w / 2, h / 2, false );
	RB_DumpFramebuffer( "be_bl_gy.tga" );

	qglViewport( 0, 0, w, h );
	GL_SelectTexture( 0 );
	globalImages->currentRenderImage->Bind();
	GL_SelectTexture( 1 );
	globalImages->GetImage( "_bloomImage" )->Bind();
	GL_SelectTexture( 2 );
	globalImages->GetImage( "_cookedMath" )->Bind();
	qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, VPROG_BLOOM_FINAL_PASS );
	qglBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, FPROG_BLOOM_FINAL_PASS );
	parm[0] = 2;
	parm[1] = 0.05f;
	qglProgramLocalParameter4fvARB( GL_VERTEX_PROGRAM_ARB, 0, parm );
	RB_DrawFullScreenQuad();
	RB_DumpFramebuffer( "be_bl_fi.tga" );
	GL_SelectTexture( 2 );
	globalImages->BindNull(); // or else GUI is screwed

	qglDisable( GL_VERTEX_PROGRAM_ARB );
	qglDisable( GL_FRAGMENT_PROGRAM_ARB );
}

// duzenko #4425: use framebuffer object for rendering in virtual resolution, separate stencil, lower color depth and depth precision, etc
GLuint fboId;
bool fboUsed, fboRenderCopied;

void RB_FboEnter() {
	if (fboUsed && fboId)
		return;
	GL_CheckErrors(); // debug
	fboRenderCopied = false;
	bool fboSeparateStencil = strcmp(glConfig.vendor_string, "Intel") == 0; // may change after a vid_restart
	// virtual resolution as a modern alternative for actual desktop resolution affecting all other windows
	GLuint curWidth = r_fboResolution.GetFloat() * glConfig.vidWidth, curHeight = r_fboResolution.GetFloat() * glConfig.vidHeight;
	
	// reset textures 
	if (curWidth != globalImages->currentRenderImage->uploadWidth || curHeight != globalImages->currentRenderImage->uploadHeight 
		|| curWidth != globalImages->currentDepthImage->uploadWidth || curHeight != globalImages->currentDepthImage->uploadHeight
		|| r_fboColorBits.IsModified()
	) { // FIXME don't allocate memory if sharing color/depth
		r_fboColorBits.ClearModified();

		globalImages->currentRenderImage->Bind();
		globalImages->currentRenderImage->uploadWidth = curWidth; // used as a shader param
		globalImages->currentRenderImage->uploadHeight = curHeight;
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		qglTexImage2D(GL_TEXTURE_2D, 0, r_fboColorBits.GetInteger() == 15 ? GL_RGB5_A1 : GL_RGBA, curWidth, curHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL); //NULL means reserve texture memory, but texels are undefined

		globalImages->currentRenderFbo->Bind();
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		qglTexImage2D(GL_TEXTURE_2D, 0, r_fboColorBits.GetInteger() == 15 ? GL_RGB5_A1 : GL_RGBA, curWidth, curHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL); //NULL means reserve texture memory, but texels are undefined

		if (fboSeparateStencil) {
			globalImages->currentStencilFbo->Bind();
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			qglTexImage2D(GL_TEXTURE_2D, 0, GL_STENCIL_INDEX8, curWidth, curHeight, 0, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, 0);
		}

		globalImages->currentDepthFbo->Bind();
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		if (fboSeparateStencil) {
			qglTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, curWidth, curHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
		} else {
			qglTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_STENCIL, curWidth, curHeight, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
		}

		globalImages->currentDepthImage->Bind();
		globalImages->currentDepthImage->uploadWidth = curWidth; // used as a shader param
		globalImages->currentDepthImage->uploadHeight = curHeight;
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		if (fboSeparateStencil) {
			qglTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, curWidth, curHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
		} else {
			qglTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_STENCIL, curWidth, curHeight, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
		}
	}

	// (re-)attach textures to FBO
	if (!fboId || r_fboSharedColor.IsModified() || r_fboSharedDepth.IsModified()) {
		// create a framebuffer object, you need to delete them when program exits.
		if (!fboId)
			glGenFramebuffers(1, &fboId);
		r_fboSharedColor.ClearModified();
		r_fboSharedDepth.ClearModified();
		glBindFramebuffer(GL_FRAMEBUFFER_EXT, fboId);
		// attach a texture to FBO color attachement point
		if (r_fboSharedColor.GetBool())
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, globalImages->currentRenderImage->texnum, 0);
		else
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, globalImages->currentRenderFbo->texnum, 0);
		// attach a renderbuffer to depth attachment point
		GLuint depthTex = r_fboSharedDepth.GetBool() ? globalImages->currentDepthImage->texnum : globalImages->currentDepthFbo->texnum;
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex, 0);
		if (fboSeparateStencil)
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, globalImages->currentStencilFbo->texnum, 0);
		else
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthTex, 0);
		int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (GL_FRAMEBUFFER_COMPLETE != status) {
			common->Printf("glCheckFramebufferStatus %d\n", status); 
			r_useFbo.SetBool(false);
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0); // not obvious, but let it be 
	}
	glBindFramebuffer(GL_FRAMEBUFFER, fboId);
	qglClear(GL_COLOR_BUFFER_BIT); // otherwise transparent skybox blends with previous frame
	fboUsed = true;
	GL_CheckErrors();
}

// called when post-proceesing is about to start, needs depth and pixels as both input and output for water and smoke
void RB_FboAccessColorDepth() {
	if (!fboUsed) // we need to copy render separately for water/smoke and then again for bloom
		return;
	if (!r_fboSharedColor.GetBool()) {
		globalImages->currentRenderImage->Bind();
		qglCopyTexImage2D(GL_TEXTURE_2D, 0, r_fboColorBits.GetInteger() == 15 ? GL_RGB5_A1 : GL_RGBA,
			0, 0, globalImages->currentRenderImage->uploadWidth, globalImages->currentRenderImage->uploadHeight, 0);
	}
	if (!r_fboSharedDepth.GetBool() && !fboRenderCopied) {
		globalImages->currentDepthImage->Bind();
		qglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
			0, 0, globalImages->currentDepthImage->uploadWidth, globalImages->currentDepthImage->uploadHeight, 0);
	}
	fboRenderCopied = true;
}

// switch from fbo to default framebuffer, copy content
void RB_FboLeave( viewDef_t* viewDef ) {
	if (!fboUsed)
		return;
	GL_CheckErrors();
	if (doFboBloom) // need currentRenderImage for bloom
		RB_FboAccessColorDepth();
	// hasn't worked very well at the first approach, maybe retry later
	/*glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, globalImages->currentRenderImage->uploadWidth, globalImages->currentRenderImage->uploadHeight, 0, 0,
	glConfig.vidWidth, glConfig.vidHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST); */
	glBindFramebuffer( GL_FRAMEBUFFER_EXT, 0 );
	qglLoadIdentity();
	qglMatrixMode( GL_PROJECTION );
	qglPushMatrix();
	qglLoadIdentity();
	qglOrtho( 0, 1, 0, 1, -1, 1 );
	qglViewport( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	qglScissor( 0, 0, glConfig.vidWidth, glConfig.vidHeight );

	qglEnable( GL_TEXTURE_2D );
	qglDisable( GL_DEPTH_TEST );
	qglDisable( GL_STENCIL_TEST );
	if (doFboBloom) {
		RB_FboBloom();
	} else {
		switch (r_fboDebug.GetInteger())
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
			if (r_fboSharedColor.GetBool())
				globalImages->currentRenderImage->Bind();
			else
				globalImages->currentRenderFbo->Bind();
		}
		RB_DrawFullScreenQuad();
	}
	qglPopMatrix();
	qglEnable(GL_DEPTH_TEST);
	qglMatrixMode(GL_MODELVIEW);
	if (viewDef) { // normal resolition 2d
		tr.renderCrops[0].width = glConfig.vidWidth;
		tr.renderCrops[0].height = glConfig.vidHeight;
		viewDef->viewport.x2 = glConfig.vidWidth - 1;
		viewDef->viewport.y2 = glConfig.vidHeight - 1;
		viewDef->scissor.x2 = glConfig.vidWidth - 1;
		viewDef->scissor.y2 = glConfig.vidHeight - 1;
	}
	fboUsed = 0;
	GL_CheckErrors();
}

/*
====================
RB_ExecuteBackEndCommands

This function will be called syncronously if running without
smp extensions, or asyncronously by another thread.
====================
*/
void RB_ExecuteBackEndCommands( const emptyCommand_t *cmds ) {
	static int backEndStartTime, backEndFinishTime;

	if ( cmds->commandId == RC_NOP && !cmds->next ) {
		return;
	}

	// r_debugRenderToTexture
	int	c_draw3d = 0, c_draw2d = 0, c_setBuffers = 0, c_swapBuffers = 0, c_copyRenders = 0;

	backEndStartTime = Sys_Milliseconds();

	// needed for editor rendering
	RB_SetDefaultGLState();

	// upload any image loads that have completed
	globalImages->CompleteBackgroundImageLoads();

	while (cmds) {
		switch ( cmds->commandId ) {
		case RC_NOP:
			break;
		case RC_DRAW_VIEW:
			// duzenko #4425: create/switch to framebuffer object
			if (((const drawSurfsCommand_t *)cmds)->viewDef->renderView.viewID >= TR_SCREEN_VIEW_ID) // not lightgem
				if (r_useFbo.GetBool())
					if (((const drawSurfsCommand_t *)cmds)->viewDef->viewEntitys)
						RB_FboEnter();
					else
						RB_FboLeave(((const drawSurfsCommand_t *)cmds)->viewDef); // duzenko: render 2d in default framebuffer, hopefully no 3d drawing after this
			RB_DrawView(cmds);
			if (((const drawSurfsCommand_t *)cmds)->viewDef->viewEntitys) {
				c_draw3d++;
			} else {
				c_draw2d++;
			}
			break;
		case RC_SET_BUFFER:
			RB_SetBuffer( cmds );
			c_setBuffers++;
			break;
		case RC_COPY_RENDER:
			RB_CopyRender( cmds );
			c_copyRenders++;
			break;
		case RC_SWAP_BUFFERS:
			// duzenko #4425: display the fbo content 
			RB_FboLeave(NULL);
			RB_SwapBuffers(cmds);
			c_swapBuffers++;
			break;
		default:
			common->Error( "RB_ExecuteBackEndCommands: bad commandId" );
			break;
		}
		cmds = (const emptyCommand_t *)cmds->next;
	}

	// go back to the default texture so the editor doesn't mess up a bound image
	qglBindTexture( GL_TEXTURE_2D, 0 );
	backEnd.glState.tmu[0].current2DMap = -1;

	// stop rendering on this thread
	backEndFinishTime = Sys_Milliseconds();
	backEnd.pc.msecLast = backEndFinishTime - backEndStartTime;
	backEnd.pc.msec += backEnd.pc.msecLast;

	if ( r_debugRenderToTexture.GetInteger() ) {
		common->Printf( "3d: %i, 2d: %i, SetBuf: %i, SwpBuf: %i, CpyRenders: %i, CpyFrameBuf: %i\n", c_draw3d, c_draw2d, c_setBuffers, c_swapBuffers, c_copyRenders, backEnd.c_copyFrameBuffer );
		backEnd.c_copyFrameBuffer = 0;
	}
}
