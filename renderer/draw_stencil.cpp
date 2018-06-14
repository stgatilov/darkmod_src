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

/*
==============================================================================

BACK END RENDERING OF STENCIL SHADOWS

==============================================================================
*/

/*
=====================
RB_T_Shadow

the shadow volumes face INSIDE
=====================
*/
static void RB_T_Shadow( const drawSurf_t *surf ) {
	GL_CheckErrors();
	const srfTriangles_t	*tri;

	// set the light position if we are using a vertex program to project the rear surfaces
	if ( surf->space != backEnd.currentSpace ) {
		idVec4 localLight;

		R_GlobalPointToLocal( surf->space->modelMatrix, backEnd.vLight->globalLightOrigin, localLight.ToVec3() );
		localLight.w = 0.0f;
		if ( r_useGLSL.GetBool() )
			qglUniform4fv( stencilShadowShader.lightOrigin, 1, localLight.ToFloatPtr() );
		else
			qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_LIGHT_ORIGIN, localLight.ToFloatPtr() );
	}

	tri = surf->backendGeo;

	if ( !tri->shadowCache.IsValid() ) {
		return;
	}

	qglVertexAttribPointer( 0, 4, GL_FLOAT, false, sizeof( shadowCache_t ), vertexCache.VertexPosition( tri->shadowCache ) );

	// we always draw the sil planes, but we may not need to draw the front or rear caps
	int	numIndexes;
	bool external = false;

	if ( !r_useExternalShadows.GetInteger() ) {
		numIndexes = tri->numIndexes;
	} else if ( r_useExternalShadows.GetInteger() == 2 ) { // force to no caps for testing
		numIndexes = tri->numShadowIndexesNoCaps;
	} else if ( !(surf->dsFlags & DSF_VIEW_INSIDE_SHADOW) ) {
		// if we aren't inside the shadow projection, no caps are ever needed needed
		numIndexes = tri->numShadowIndexesNoCaps;
		external = true;
	} else if ( !backEnd.vLight->viewInsideLight && !(surf->backendGeo->shadowCapPlaneBits & SHADOW_CAP_INFINITE) ) {
		// if we are inside the shadow projection, but outside the light, and drawing
		// a non-infinite shadow, we can skip some caps
		if ( backEnd.vLight->viewSeesShadowPlaneBits & surf->backendGeo->shadowCapPlaneBits ) {
			// we can see through a rear cap, so we need to draw it, but we can skip the
			// caps on the actual surface
			numIndexes = tri->numShadowIndexesNoFrontCaps;
		} else {
			// we don't need to draw any caps
			numIndexes = tri->numShadowIndexesNoCaps;
		}
		external = true;
	} else {
		// must draw everything
		numIndexes = tri->numIndexes;
	}

	// set depth bounds
	if ( glConfig.depthBoundsTestAvailable && r_useDepthBoundsTest.GetBool() ) {
		qglDepthBoundsEXT( surf->scissorRect.zmin, surf->scissorRect.zmax );
	}

	// debug visualization
	if ( r_showShadows.GetInteger() ) {
		if ( r_showShadows.GetInteger() == 3 ) {
			if ( external ) {
				qglColor3f( 0.1 / backEnd.overBright, 1 / backEnd.overBright, 0.1 / backEnd.overBright );
			} else {
				// these are the surfaces that require the reverse
				qglColor3f( 1 / backEnd.overBright, 0.1 / backEnd.overBright, 0.1 / backEnd.overBright );
			}
		} else {
			// draw different color for turboshadows
			if ( surf->backendGeo->shadowCapPlaneBits & SHADOW_CAP_INFINITE ) {
				if ( numIndexes == tri->numIndexes ) {
					qglColor3f( .5 / backEnd.overBright, 0.1 / backEnd.overBright, 0.1 / backEnd.overBright );
				} else {
					qglColor3f( .5 / backEnd.overBright, 0.4 / backEnd.overBright, 0.1 / backEnd.overBright );
				}
			} else {
				if ( numIndexes == tri->numIndexes ) {
					qglColor3f( 0.1 / backEnd.overBright, 1 / backEnd.overBright, 0.1 / backEnd.overBright );
				} else if ( numIndexes == tri->numShadowIndexesNoFrontCaps ) {
					qglColor3f( 0.1 / backEnd.overBright, 1 / backEnd.overBright, 0.6 / backEnd.overBright );
				} else {
					qglColor3f( 0.6 / backEnd.overBright, 1 / backEnd.overBright, 0.1 / backEnd.overBright );
				}
			}
		}

		qglStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
		qglDisable( GL_STENCIL_TEST );
		GL_Cull( CT_TWO_SIDED );
		RB_DrawShadowElementsWithCounters( tri, numIndexes );
		GL_Cull( CT_FRONT_SIDED );
		qglEnable( GL_STENCIL_TEST );
		GL_CheckErrors();
		return;
	}

	// patent-free work around
	if ( !external ) {
		// depth-fail stencil shadows
		if ( r_useTwoSidedStencil.GetBool() && glConfig.twoSidedStencilAvailable ) {
			qglStencilOpSeparate( backEnd.viewDef->isMirror ? GL_FRONT : GL_BACK, GL_KEEP, tr.stencilDecr, GL_KEEP );
			qglStencilOpSeparate( backEnd.viewDef->isMirror ? GL_BACK : GL_FRONT, GL_KEEP, tr.stencilIncr, GL_KEEP );
			GL_Cull( CT_TWO_SIDED );
			RB_DrawShadowElementsWithCounters( tri, numIndexes );
		} else {
			// "preload" the stencil buffer with the number of volumes
			// that get clipped by the near or far clip plane
			qglStencilOp( GL_KEEP, tr.stencilDecr, tr.stencilDecr );
			GL_Cull( CT_FRONT_SIDED );
			RB_DrawShadowElementsWithCounters( tri, numIndexes );

			qglStencilOp( GL_KEEP, tr.stencilIncr, tr.stencilIncr );
			GL_Cull( CT_BACK_SIDED );
			RB_DrawShadowElementsWithCounters( tri, numIndexes );
		}
	} else {
		// traditional depth-pass stencil shadows
		if ( r_useTwoSidedStencil.GetBool() && glConfig.twoSidedStencilAvailable ) {
			qglStencilOpSeparate( backEnd.viewDef->isMirror ? GL_FRONT : GL_BACK, GL_KEEP, GL_KEEP, tr.stencilIncr );
			qglStencilOpSeparate( backEnd.viewDef->isMirror ? GL_BACK : GL_FRONT, GL_KEEP, GL_KEEP, tr.stencilDecr );
			GL_Cull( CT_TWO_SIDED );
			RB_DrawShadowElementsWithCounters( tri, numIndexes );
		} else {
			qglStencilOp( GL_KEEP, GL_KEEP, tr.stencilIncr );
			GL_Cull( CT_FRONT_SIDED );
			RB_DrawShadowElementsWithCounters( tri, numIndexes );

			qglStencilOp( GL_KEEP, GL_KEEP, tr.stencilDecr );
			GL_Cull( CT_BACK_SIDED );
			RB_DrawShadowElementsWithCounters( tri, numIndexes );
		}
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

	RB_LogComment( "---------- RB_StencilShadowPass ----------\n" );

	globalImages->BindNull();

	// for visualizing the shadows
	switch ( r_showShadows.GetInteger() )
	{
	case -1:
		GL_State( /*GLS_DEPTHMASK |/**/ GLS_COLORMASK | GLS_ALPHAMASK | GLS_DEPTHFUNC_LESS );
		break;
	case 1:
		// draw filled in
		GL_State( GLS_DEPTHMASK | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_LESS );
		break;
	case 2:
		GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_POLYMODE_LINE | GLS_DEPTHFUNC_ALWAYS );
		break;
	case 3:
		GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_POLYMODE_LINE | GLS_DEPTHFUNC_LESS );
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

	if ( glConfig.depthBoundsTestAvailable && r_useDepthBoundsTest.GetBool() ) 
		qglEnable( GL_DEPTH_BOUNDS_TEST_EXT );

	RB_RenderDrawSurfChainWithFunction( drawSurfs, RB_T_Shadow );

	GL_Cull( CT_FRONT_SIDED );

	if ( r_shadowPolygonFactor.GetFloat() || r_shadowPolygonOffset.GetFloat() ) 
		qglDisable( GL_POLYGON_OFFSET_FILL );

	if ( glConfig.depthBoundsTestAvailable && r_useDepthBoundsTest.GetBool() ) 
		qglDisable( GL_DEPTH_BOUNDS_TEST_EXT );

	qglStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
	if ( !r_softShadowsQuality.GetBool() || backEnd.viewDef->IsLightGem() )
		qglStencilFunc( GL_GEQUAL, 128, 255 );
}