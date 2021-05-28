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
#include "tr_local.h"
#include "glsl.h"
#include "GLSLProgramManager.h"

/*
==============================================================================

BACK END RENDERING OF STENCIL SHADOWS

==============================================================================
*/

struct StencilShadowUniforms : GLSLUniformGroup {
	UNIFORM_GROUP_DEF( StencilShadowUniforms );
	DEFINE_UNIFORM( vec4, lightOrigin );
};

/*
=====================
RB_T_Shadow

the shadow volumes face INSIDE
=====================
*/
static void RB_T_Shadow( const drawSurf_t *surf ) {
	GL_CheckErrors();
	int softCheck = r_softShadowsQuality.GetInteger();

	// set the light position if we are using a vertex program to project the rear surfaces
	if ( surf->space != backEnd.currentSpace ) {
		idVec4 localLight;
		R_GlobalPointToLocal( surf->space->modelMatrix, backEnd.vLight->globalLightOrigin, localLight.ToVec3() );
		localLight.w = 0.0f;
		programManager->stencilShadowShader->GetUniformGroup<StencilShadowUniforms>()->lightOrigin.Set( localLight );
	}

	if ( !surf->shadowCache.IsValid() ) {
		return;
	}
	vertexCache.VertexPosition( surf->shadowCache, ATTRIB_SHADOW );

	// we always draw the sil planes, but we may not need to draw the front or rear caps
	const int numIndexes = surf->numIndexes;
	bool external = false;

	// #7627 simplify a bit revelator.
	// 1 = skip drawing caps when outside the light volume
	if ( r_useExternalShadows.GetInteger() && !( surf->dsFlags & DSF_VIEW_INSIDE_SHADOW ) ) {
		external = true;
	}

	// set depth bounds
	const DepthBoundsTest depthBoundsTest( backEnd.vLight->scissorRect );

	// debug visualization
	if ( r_showShadows.GetInteger() ) {

		if ( r_softShadowsQuality.GetBool() ) {
			r_softShadowsQuality.SetBool( 0 );
		}

		if ( r_showShadows.GetInteger() == 3 ) {
			if ( external ) {
				GL_FloatColor( 0.1f / backEnd.overBright, 1.0f / backEnd.overBright, 0.1f / backEnd.overBright );
			} else {
				// these are the surfaces that require the reverse
				GL_FloatColor( 1.0f / backEnd.overBright, 0.1f / backEnd.overBright, 0.1f / backEnd.overBright );
			}
		}
		qglStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
		qglDisable( GL_STENCIL_TEST );
		GL_Cull( CT_TWO_SIDED );
		RB_DrawShadowElementsWithCounters( surf );
		GL_Cull( CT_FRONT_SIDED );
		qglEnable( GL_STENCIL_TEST );
		GL_CheckErrors();
		return;
	}

	// patent-free work around
	if ( !external ) {
		// depth-fail stencil shadows
		{
			qglStencilOpSeparate( backEnd.viewDef->isMirror ? GL_FRONT : GL_BACK, GL_KEEP, GL_DECR_WRAP, GL_KEEP );
			qglStencilOpSeparate( backEnd.viewDef->isMirror ? GL_BACK : GL_FRONT, GL_KEEP, GL_INCR_WRAP, GL_KEEP );
			GL_Cull( CT_TWO_SIDED );
			RB_DrawShadowElementsWithCounters( surf );
		}
	} else {
		// traditional depth-pass stencil shadows
		{
			qglStencilOpSeparate( backEnd.viewDef->isMirror ? GL_FRONT : GL_BACK, GL_KEEP, GL_KEEP, GL_INCR_WRAP );
			qglStencilOpSeparate( backEnd.viewDef->isMirror ? GL_BACK : GL_FRONT, GL_KEEP, GL_KEEP, GL_DECR_WRAP );
			GL_Cull( CT_TWO_SIDED );
			RB_DrawShadowElementsWithCounters( surf );
		}
	}

	if ( !r_showShadows.GetBool() ) {
		r_softShadowsQuality.SetInteger( softCheck );
	}
	GL_CheckErrors();
}

/*
=====================
RB_StencilShadowPass

Stencil test should already be enabled, and the stencil buffer should have
been set to 128 on any surfaces that might receive shadows
=====================
*/
void RB_StencilShadowPass( const drawSurf_t *drawSurfs ) {
	if ( !r_shadows.GetBool() ) {
		return;
	}

	if ( !drawSurfs ) {
		return;
	}
	TRACE_GL_SCOPE( "StencilShadowPass" );

	RB_LogComment( "---------- RB_StencilShadowPass ----------\n" );

	// for visualizing the shadows
	switch ( r_showShadows.GetInteger() ) {
	case -1:
		GL_State( GLS_COLORMASK | GLS_ALPHAMASK | GLS_DEPTHFUNC_LESS );
		break;
	case 1:
		// draw filled in
		GL_State( GLS_DEPTHMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_LESS );
		break;
	case 2:
		GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_POLYMODE_LINE | GLS_DEPTHFUNC_ALWAYS );
		break;
	case 3:
		GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_POLYMODE_LINE | GLS_DEPTHFUNC_LESS );
		break;
	default:
		// don't write to the color buffer, just the stencil buffer
		GL_State( GLS_DEPTHMASK | GLS_COLORMASK | GLS_ALPHAMASK | GLS_DEPTHFUNC_LESS );
		break;
	}

	if ( r_shadowPolygonFactor.GetFloat() || r_shadowPolygonOffset.GetFloat() ) {
		qglPolygonOffset( r_shadowPolygonFactor.GetFloat(), -r_shadowPolygonOffset.GetFloat() );
		qglEnable( GL_POLYGON_OFFSET_FILL );
	}
	qglStencilFunc( GL_ALWAYS, 1, 255 );

	if ( glConfig.depthBoundsTestAvailable && r_useDepthBoundsTest.GetBool() ) {
		qglEnable( GL_DEPTH_BOUNDS_TEST_EXT );
	}
	RB_RenderDrawSurfChainWithFunction( drawSurfs, RB_T_Shadow );

	GL_Cull( CT_FRONT_SIDED );

	if ( r_shadowPolygonFactor.GetFloat() || r_shadowPolygonOffset.GetFloat() ) {
		qglDisable( GL_POLYGON_OFFSET_FILL );
	}

	if ( glConfig.depthBoundsTestAvailable && r_useDepthBoundsTest.GetBool() ) {
		qglDisable( GL_DEPTH_BOUNDS_TEST_EXT );
	}
	qglStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );

	if ( !r_softShadowsQuality.GetBool() || backEnd.viewDef->IsLightGem() /*|| r_shadows.GetInteger()==2 && backEnd.vLight->tooBigForShadowMaps*/ )
		qglStencilFunc( GL_GEQUAL, 128, 255 ); // enable stencil test - the shadow volume path
}