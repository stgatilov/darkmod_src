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
	GL_CheckErrors();

	qglClearDepth( 1.0f );
	qglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

	// the vertex array is always enabled
	qglEnableVertexAttribArray( 0 );

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

	qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	qglDepthMask( GL_TRUE );
	qglDepthFunc( GL_ALWAYS );

	qglCullFace( GL_FRONT_AND_BACK );
	qglShadeModel( GL_SMOOTH );

	if ( r_useScissor.GetBool() ) {
		GL_Scissor( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	}

	for ( int i = MAX_MULTITEXTURE_UNITS - 1 ; i >= 0 ; i-- ) {
		GL_SelectTexture( i );

		qglDisable( GL_TEXTURE_2D );
		qglDisable( GL_TEXTURE_CUBE_MAP );
	}
}

/*
====================
RB_LogComment
====================
*/
/*void RB_LogComment( const char *comment, ... ) {
	if ( !tr.logFile ) {
		return;
	}
	va_list marker;

	fprintf( tr.logFile, "// " );
	va_start( marker, comment );
	vfprintf( tr.logFile, comment, marker );
	va_end( marker );
}*/


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

	if ( unit < 0 || unit >= MAX_MULTITEXTURE_UNITS ) {
		common->Warning( "GL_SelectTexture: unit = %i", unit );
		return;
	}
	qglActiveTexture( GL_TEXTURE0 + unit );

	RB_LogComment( "glActiveTextureARB( %i );\n", unit );

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
GL_Scissor

Utility function,
if you absolutly must
check for anything out of the ordinary,
then do it here.
====================
*/
void GL_Scissor( int x /* left*/, int y /* bottom */, int w, int h ) {
	// x and y can be negative, but neither width nor height must be.
	if ( w <= 0 || h <= 0 ) {
		return;
	}
	qglScissor( x, y, w, h );
}

/*
====================
GL_Viewport

Utility function,
if you absolutly must
check for anything out of the ordinary,
then do it here.
====================
*/
void GL_Viewport( int x /* left */, int y /* bottom */, int w, int h ) {
	// x and y can be negative, but neither width nor height must be.
	if ( w <= 0 || h <= 0 ) {
		return;
	}
	qglViewport( x, y, w, h );
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
	if ( diff & ( GLS_REDMASK | GLS_GREENMASK | GLS_BLUEMASK | GLS_ALPHAMASK ) ) {
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
	backEnd.glState.glStateBits = stateBits;
}

//anon begin
/*
========================
GL_DepthBoundsTest
========================
*/
void GL_DepthBoundsTest( const float zmin, const float zmax ) {
	if ( !glConfig.depthBoundsTestAvailable || zmin > zmax ) {
		return;
	}

	if ( zmin == 0.0f && zmax == 0.0f ) {
		qglDisable( GL_DEPTH_BOUNDS_TEST_EXT );
	} else {
		qglEnable( GL_DEPTH_BOUNDS_TEST_EXT );
		qglDepthBoundsEXT( zmin, zmax );
	}
}
//anon end

/*
============================================================================

RENDER BACK END COLOR WRAPPERS

============================================================================
*/

/*
====================
GL_Color

Vector color 3 component (clamped)
====================
*/
void GL_FloatColor( const idVec3 &color ) {
	GLfloat parm[3];
	parm[0] = idMath::ClampFloat( 0.0f, 1.0f, color[0] );
	parm[1] = idMath::ClampFloat( 0.0f, 1.0f, color[1] );
	parm[2] = idMath::ClampFloat( 0.0f, 1.0f, color[2] );
	qglColor3f( parm[0], parm[1], parm[2] );
}

/*
====================
GL_Color

Vector color 4 component (clamped)
====================
*/
void GL_FloatColor( const idVec4 &color ) {
	GLfloat parm[4];
	parm[0] = idMath::ClampFloat( 0.0f, 1.0f, color[0] );
	parm[1] = idMath::ClampFloat( 0.0f, 1.0f, color[1] );
	parm[2] = idMath::ClampFloat( 0.0f, 1.0f, color[2] );
	parm[3] = idMath::ClampFloat( 0.0f, 1.0f, color[3] );
	qglColor4f( parm[0], parm[1], parm[2], parm[3] );
}

/*
====================
GL_Color

Float to vector color 3 or 4 component (clamped)
====================
*/
void GL_FloatColor( const float *color ) {
	GLfloat parm[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	parm[0] = idMath::ClampFloat( 0.0f, 1.0f, color[0] );
	parm[1] = idMath::ClampFloat( 0.0f, 1.0f, color[1] );
	parm[2] = idMath::ClampFloat( 0.0f, 1.0f, color[2] );
	if ( color[3] ) {
		parm[3] = idMath::ClampFloat( 0.0f, 1.0f, color[3] );
	}
	qglColor4fv( parm );
}

/*
====================
GL_Color

Float color 3 component (clamped)
====================
*/
void GL_FloatColor( float r, float g, float b ) {
	GLfloat parm[3];
	parm[0] = idMath::ClampFloat( 0.0f, 1.0f, r );
	parm[1] = idMath::ClampFloat( 0.0f, 1.0f, g );
	parm[2] = idMath::ClampFloat( 0.0f, 1.0f, b );
	qglColor3f( parm[0], parm[1], parm[2] );
}

/*
====================
GL_Color

Float color 4 component (clamped)
====================
*/
void GL_FloatColor( float r, float g, float b, float a ) {
	GLfloat parm[4];
	parm[0] = idMath::ClampFloat( 0.0f, 1.0f, r );
	parm[1] = idMath::ClampFloat( 0.0f, 1.0f, g );
	parm[2] = idMath::ClampFloat( 0.0f, 1.0f, b );
	parm[3] = idMath::ClampFloat( 0.0f, 1.0f, a );
	qglColor4f( parm[0], parm[1], parm[2], parm[3] );
}

/*
====================
GL_Color

Byte to vector color 3 or 4 component (clamped)
====================
*/
void GL_ByteColor( const byte *color ) {
	GLubyte parm[4] = { 255, 255, 255, 255 };
	parm[0] = idMath::ClampByte( 0, 255, color[0] );
	parm[1] = idMath::ClampByte( 0, 255, color[1] );
	parm[2] = idMath::ClampByte( 0, 255, color[2] );
	if ( color[3] ) {
		parm[3] = idMath::ClampByte( 0, 255, color[3] );
	}
	qglColor3ub( parm[0], parm[1], parm[2] );
}


/*
====================
GL_Color

Byte color 3 component (clamped)
====================
*/
void GL_ByteColor( byte r, byte g, byte b ) {
	GLubyte parm[3];
	parm[0] = idMath::ClampByte( 0, 255, r );
	parm[1] = idMath::ClampByte( 0, 255, g );
	parm[2] = idMath::ClampByte( 0, 255, b );
	qglColor3ub( parm[0], parm[1], parm[2] );
}

/*
====================
GL_Color

Byte color 4 component (clamped)
====================
*/
void GL_ByteColor( byte r, byte g, byte b, byte a ) {
	GLubyte parm[4];
	parm[0] = idMath::ClampByte( 0, 255, r );
	parm[1] = idMath::ClampByte( 0, 255, g );
	parm[2] = idMath::ClampByte( 0, 255, b );
	parm[3] = idMath::ClampByte( 0, 255, a );
	qglColor4ub( parm[0], parm[1], parm[2], parm[3] );
}

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
	GL_Viewport( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	if ( r_useScissor.GetBool() ) {
		GL_Scissor( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
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
	cmd = ( const setBufferCommand_t * )data;

	backEnd.frameCount = cmd->frameCount;

	if ( !r_useFbo.GetBool() ) { // duzenko #4425: not applicable, raises gl errors
		qglDrawBuffer( cmd->buffer );
	}

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
		if ( !r_useFbo.GetBool() || !game->PlayerReady() ) { // duzenko #4425: happens elsewhere for fbo, "Click when ready" skips FBO even with r_useFbo 1
			qglClear( GL_COLOR_BUFFER_BIT );
		}
	}
}

/*
=============
RB_DumpFramebuffer

Bloom related debug tool
=============
*/
void RB_DumpFramebuffer( const char *fileName ) {
	renderCrop_t r;

	qglGetIntegerv( GL_VIEWPORT, &r.x );

	if (!r_useFbo.GetBool()) {
		qglReadBuffer( GL_BACK );
	}

	// calculate pitch of buffer that will be returned by qglReadPixels()
	int alignment;
	qglGetIntegerv( GL_PACK_ALIGNMENT, &alignment );

	int pitch = r.width * 4 + alignment - 1;
	pitch = pitch - pitch % alignment;

	byte *data = (byte *)R_StaticAlloc( pitch * r.height );

	// GL_RGBA/GL_UNSIGNED_BYTE seems to be the safest option
	qglReadPixels( r.x, r.y, r.width, r.height, GL_RGBA, GL_UNSIGNED_BYTE, data );

	byte *data2 = (byte *)R_StaticAlloc( r.width * r.height * 4 );

	for ( int y = 0; y < r.height; y++ ) {
		memcpy( data2 + y * r.width * 4, data + y * pitch, r.width * 4 );
	}
	R_WriteTGA( fileName, data2, r.width, r.height, true );

	R_StaticFree( data );
	R_StaticFree( data2 );
}

/*
=============
RB_CheckTools

Revelator: Check for any rendertool and mark it
because it might potentially break with post processing.
=============
*/
static bool RB_CheckTools( int width, int height ) {
	// this has actually happened
	if ( !width || !height ) {
		return true;
	}
	// nbohr1more add checks for render tools
	// revelator: added some more
	if ( r_showLightCount.GetBool() ||
	     r_showShadows.GetBool() ||
	     r_showVertexColor.GetBool() ||
	     r_showShadowCount.GetBool() ||
	     r_showTris.GetBool() ||
	     r_showPortals.GetBool() ||
	     r_showTexturePolarity.GetBool() ||
	     r_showTangentSpace.GetBool() ||
	     r_showDepth.GetBool() ) {
		return true;
	}
	return false;
}

/*
=============
RB_DrawFullScreenQuad

Moved to backend: Revelator
=============
*/
void RB_DrawFullScreenQuad( void ) {
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
RB_Bloom

Originally in front renderer (idPlayerView::dnPostProcessManager)
Moved to backend: Revelator
=============
*/
void RB_Bloom( void ) {
	int w = globalImages->currentRenderImage->uploadWidth;
	int h = globalImages->currentRenderImage->uploadHeight;

	FB_CopyColorBuffer();

	// revelator: added small function to take care of nono's
	if ( RB_CheckTools( w, h ) ) {
		return;
	}
	float	parm[4];

	FB_SelectPostProcess();

	// full screen blends
	qglLoadIdentity();
	qglMatrixMode( GL_PROJECTION );
	qglPushMatrix();
	qglLoadIdentity();
	qglOrtho( 0, 1, 0, 1, -1, 1 );

	GL_State( GLS_DEPTHMASK );

	qglDisable( GL_DEPTH_TEST );

	qglEnable( GL_VERTEX_PROGRAM_ARB );
	qglEnable( GL_FRAGMENT_PROGRAM_ARB );
	GL_SelectTexture( 0 );

	GL_Viewport( 0, 0, 256, 1 );
	qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, VPROG_BLOOM_COOK_MATH1 );
	qglBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, FPROG_BLOOM_COOK_MATH1 );
	parm[0] = r_postprocess_colorCurveBias.GetFloat();
	parm[1] = r_postprocess_sceneGamma.GetFloat();
	parm[2] = r_postprocess_sceneExposure.GetFloat();
	parm[3] = 1;
	qglProgramLocalParameter4fvARB( GL_VERTEX_PROGRAM_ARB, 0, parm );
	RB_DrawFullScreenQuad();
	globalImages->bloomCookedMath->CopyFramebuffer( 0, 0, 256, 1, false );

	qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, VPROG_BLOOM_COOK_MATH2 );
	qglBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, FPROG_BLOOM_COOK_MATH2 );
	parm[0] = r_postprocess_brightPassThreshold.GetFloat();
	parm[1] = r_postprocess_brightPassOffset.GetFloat();
	parm[2] = r_postprocess_colorCorrection.GetFloat();
	parm[3] = Max( Min( r_postprocess_colorCorrectBias.GetFloat(), 1.0f ), 0.0f );
	qglProgramLocalParameter4fvARB( GL_VERTEX_PROGRAM_ARB, 0, parm );
	RB_DrawFullScreenQuad();
	globalImages->bloomCookedMath->CopyFramebuffer( 0, 0, 256, 1, false );

	GL_Viewport( 0, 0, w / 2, h / 2 );
	GL_SelectTexture( 0 );
	globalImages->currentRenderImage->Bind();
	GL_SelectTexture( 1 );
	globalImages->bloomCookedMath->Bind();
	qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, VPROG_BLOOM_BRIGHTNESS );
	qglBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, FPROG_BLOOM_BRIGHTNESS );
	RB_DrawFullScreenQuad();
	GL_SelectTexture( 0 );
	globalImages->bloomImage->CopyFramebuffer( 0, 0, w / 2, h / 2, false );

	globalImages->bloomImage->Bind();
	qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, VPROG_BLOOM_GAUSS_BLRX );
	qglBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, FPROG_BLOOM_GAUSS_BLRX );
	parm[0] = 2 / w;
	parm[1] = 1;
	parm[2] = 1;
	parm[3] = 1;
	qglProgramLocalParameter4fvARB( GL_VERTEX_PROGRAM_ARB, 0, parm );
	RB_DrawFullScreenQuad();
	globalImages->bloomImage->CopyFramebuffer( 0, 0, w / 2, h / 2, false );

	qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, VPROG_BLOOM_GAUSS_BLRY );
	qglBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, FPROG_BLOOM_GAUSS_BLRY );
	parm[0] = 2 / h;
	qglProgramLocalParameter4fvARB( GL_VERTEX_PROGRAM_ARB, 0, parm );
	RB_DrawFullScreenQuad();
	globalImages->bloomImage->CopyFramebuffer( 0, 0, w / 2, h / 2, false );

	FB_SelectPrimary();
	GL_Viewport( 0, 0, w, h );
	FB_TogglePrimary( false );
	GL_SelectTexture( 0 );
	globalImages->currentRenderImage->Bind();
	GL_SelectTexture( 1 );
	globalImages->bloomImage->Bind();
	GL_SelectTexture( 2 );
	globalImages->bloomCookedMath->Bind();
	qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, VPROG_BLOOM_FINAL_PASS );
	qglBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, FPROG_BLOOM_FINAL_PASS );
	parm[0] = r_postprocess_bloomIntensity.GetFloat();
	parm[1] = Max( Min( r_postprocess_desaturation.GetFloat(), 1.0f ), 0.0f );
	qglProgramLocalParameter4fvARB( GL_VERTEX_PROGRAM_ARB, 0, parm );
	RB_DrawFullScreenQuad();
	GL_SelectTexture( 2 );
	globalImages->BindNull(); // or else GUI is screwed
	GL_SelectTexture( 1 );
	globalImages->BindNull(); // or else GUI is screwed
	GL_SelectTexture( 0 );

	qglDisable( GL_VERTEX_PROGRAM_ARB );
	qglDisable( GL_FRAGMENT_PROGRAM_ARB );

	qglPopMatrix();
	qglEnable( GL_DEPTH_TEST );
	qglMatrixMode( GL_MODELVIEW );
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

	for ( int i = 0 ; i < globalImages->images.Num() ; i++ ) {
		image = globalImages->images[i];

		if ( image->texnum == idImage::TEXTURE_NOT_LOADED ) {
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

		qglBegin( GL_QUADS );
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

/*
=============
RB_CopyRender

Copy part of the current framebuffer to an image
=============
*/
void RB_CopyRender( const void *data ) {
	if ( r_skipCopyTexture.GetBool() ) {
		return;
	}
	const copyRenderCommand_t &cmd = *( copyRenderCommand_t * )data;

	RB_LogComment( "***************** RB_CopyRender *****************\n" );

	FB_CopyRender( cmd );
}

/*
====================
RB_ExecuteBackEndCommands

Always runs on the main thread
====================
*/
void RB_ExecuteBackEndCommands( const emptyCommand_t *cmds ) {
	static int backEndStartTime, backEndFinishTime;

	if ( cmds->commandId == RC_NOP && !cmds->next ) {
		return;
	}

	// r_debugRenderToTexture
	// revelator: added bloom to counters.
	int	c_draw3d = 0, c_draw2d = 0, c_setBuffers = 0, c_swapBuffers = 0, c_drawBloom = 0, c_copyRenders = 0;

	backEndStartTime = Sys_Milliseconds();

	// needed for editor rendering
	RB_SetDefaultGLState();

	bool isv3d = false, fboOff = false; // needs to be declared outside of switch case

	while ( cmds ) {
		switch ( cmds->commandId ) {
		case RC_NOP:
			break;
		case RC_DRAW_VIEW: {
			backEnd.viewDef = ( ( const drawSurfsCommand_t * )cmds )->viewDef;
			isv3d = ( backEnd.viewDef->viewEntitys != nullptr );	// view is 2d or 3d
			if ( !backEnd.viewDef->IsLightGem() ) {					// duzenko #4425: create/switch to framebuffer object
				if ( !fboOff ) {									// don't switch to FBO if bloom or some 2d has happened
					if ( isv3d ) {
						FB_TogglePrimary( true );
					} else {
						FB_TogglePrimary( false );					// duzenko: render 2d in default framebuffer, as well as all 3d until frame end
						fboOff = true;
					}
				}
			}
			RB_DrawView();
			if ( isv3d ) {
				c_draw3d++;
			} else {
				c_draw2d++;
			}
			if ( r_frontBuffer.GetBool() ) {					// debug: put a breakpoint to see a per view render
				qglFinish();
			}
			break;
		}
		case RC_SET_BUFFER:
			RB_SetBuffer( cmds );
			c_setBuffers++;
			break;
		case RC_BLOOM:
			RB_Bloom();
			c_drawBloom++;
			fboOff = true;
			break;
		case RC_COPY_RENDER:
			RB_CopyRender( cmds );
			c_copyRenders++;
			break;
		case RC_SWAP_BUFFERS:
			// duzenko #4425: display the fbo content
			FB_TogglePrimary( false );
			RB_SwapBuffers( cmds );
			c_swapBuffers++;
			break;
		default:
			common->Error( "RB_ExecuteBackEndCommands: bad commandId" );
			break;
		}
		cmds = ( const emptyCommand_t * )cmds->next;
	}

	// go back to the default texture so the editor doesn't mess up a bound image
	qglBindTexture( GL_TEXTURE_2D, 0 );
	GL_CheckErrors();
	backEnd.glState.tmu[0].current2DMap = -1;

	// stop rendering on this thread
	backEndFinishTime = Sys_Milliseconds();
	backEnd.pc.msecLast = backEndFinishTime - backEndStartTime;
	backEnd.pc.msec += backEnd.pc.msecLast;

	// revelator: added depthcopy to counters
	if ( r_debugRenderToTexture.GetInteger() ) {
		common->Printf( "3d: %i, 2d: %i, SetBuf: %i, SwpBuf: %i, drwBloom: %i, CpyRenders: %i, CpyFrameBuf: %i, CpyDepthBuf: %i\n", c_draw3d, c_draw2d, c_setBuffers, c_swapBuffers, c_drawBloom, c_copyRenders, backEnd.c_copyFrameBuffer, backEnd.c_copyDepthBuffer );
		backEnd.c_copyFrameBuffer = 0;
		backEnd.c_copyDepthBuffer = 0;
	}
}
