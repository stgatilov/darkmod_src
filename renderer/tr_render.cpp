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
#include "glsl.h"
#include "FrameBuffer.h"

/*

  back end scene + lights rendering functions

*/


/*
=================
RB_DrawElementsImmediate

Draws with immediate mode commands, which is going to be very slow.
This should never happen if the vertex cache is operating properly.
=================
*/
void RB_DrawElementsImmediate( const srfTriangles_t *tri ) {

	backEnd.pc.c_drawElements++;
	backEnd.pc.c_drawIndexes += tri->numIndexes;
	backEnd.pc.c_drawVertexes += tri->numVerts;

	if ( tri->ambientSurface ) {
		if ( tri->indexes == tri->ambientSurface->indexes ) {
			backEnd.pc.c_drawRefIndexes += tri->numIndexes;
		}
		if ( tri->verts == tri->ambientSurface->verts ) {
			backEnd.pc.c_drawRefVertexes += tri->numVerts;
		}
	}

	qglBegin( GL_TRIANGLES );
	for ( int i = 0 ; i < tri->numIndexes ; i++ ) {
		qglTexCoord2fv( tri->verts[ tri->indexes[i] ].st.ToFloatPtr() );
		qglVertex3fv( tri->verts[ tri->indexes[i] ].xyz.ToFloatPtr() );
	}
	qglEnd();
}


/*
================
RB_DrawElementsWithCounters
================
*/
void RB_DrawElementsWithCounters( const srfTriangles_t *tri ) {
	if( vertexCache.currentVertexBuffer == 0 ) {
		common->Printf( "RB_DrawElementsWithCounters called, but no vertex buffer is bound. Vertex cache resize?\n" );
		return;
	}
	if (r_showPrimitives.GetBool() && !backEnd.viewDef->IsLightGem() ) {
		backEnd.pc.c_drawElements++;
		backEnd.pc.c_drawIndexes += tri->numIndexes;
		backEnd.pc.c_drawVertexes += tri->numVerts;

		if (tri->ambientSurface && (tri->indexes == tri->ambientSurface->indexes || tri->verts == tri->ambientSurface->verts)) {
			backEnd.pc.c_drawRefIndexes += tri->numIndexes;
			backEnd.pc.c_drawRefVertexes += tri->numVerts;
		}
	}

	if ( tri->indexCache.IsValid() ) {
		qglDrawElements( GL_TRIANGLES, 
		                 tri->numIndexes,
		                 GL_INDEX_TYPE,
		                 vertexCache.IndexPosition( tri->indexCache ) );
		if (r_showPrimitives.GetBool() && !backEnd.viewDef->IsLightGem() ) 
			backEnd.pc.c_vboIndexes += tri->numIndexes;
	} else {
		vertexCache.UnbindIndex();
		qglDrawElements( GL_TRIANGLES, tri->numIndexes, GL_INDEX_TYPE, tri->indexes );
	}
}

void RB_DrawElementsWithCounters( const srfTriangles_t *tri, int baseVertex ) {
	if( vertexCache.currentVertexBuffer == 0 ) {
		common->Printf( "RB_DrawElementsWithCounters called, but no vertex buffer is bound. Vertex cache resize?\n" );
		return;
	}
	if (r_showPrimitives.GetBool() && !backEnd.viewDef->IsLightGem() ) {
		backEnd.pc.c_drawElements++;
		backEnd.pc.c_drawIndexes += tri->numIndexes;
		backEnd.pc.c_drawVertexes += tri->numVerts;

		if (tri->ambientSurface && (tri->indexes == tri->ambientSurface->indexes || tri->verts == tri->ambientSurface->verts)) {
			backEnd.pc.c_drawRefIndexes += tri->numIndexes;
			backEnd.pc.c_drawRefVertexes += tri->numVerts;
		}
	}

	static PFNGLDRAWELEMENTSBASEVERTEXPROC qglDrawElementsBaseVertex = (PFNGLDRAWELEMENTSBASEVERTEXPROC)GLimp_ExtensionPointer("glDrawElementsBaseVertex");

	if ( tri->indexCache.IsValid() ) {
		qglDrawElementsBaseVertex( GL_TRIANGLES, 
		                 tri->numIndexes,
		                 GL_INDEX_TYPE,
		                 vertexCache.IndexPosition( tri->indexCache ), baseVertex );
		if (r_showPrimitives.GetBool() && !backEnd.viewDef->IsLightGem() ) 
			backEnd.pc.c_vboIndexes += tri->numIndexes;
	} else {
		vertexCache.UnbindIndex();
		qglDrawElementsBaseVertex( GL_TRIANGLES, tri->numIndexes, GL_INDEX_TYPE, tri->indexes, baseVertex );
	}
}

/*
================
RB_DrawShadowElementsWithCounters

May not use all the indexes in the surface if caps are skipped
================
*/
void RB_DrawShadowElementsWithCounters( const srfTriangles_t *tri, int numIndexes ) {
	if ( r_showPrimitives.GetBool() && !backEnd.viewDef->IsLightGem() ) {
		backEnd.pc.c_shadowElements++;
		backEnd.pc.c_shadowIndexes += numIndexes;
		backEnd.pc.c_shadowVertexes += tri->numVerts;
	}

	if ( tri->indexCache.IsValid() ) {
		qglDrawElements( GL_TRIANGLES, 
		                 numIndexes,
		                 GL_INDEX_TYPE,
		                 vertexCache.IndexPosition( tri->indexCache ) );
		if (r_showPrimitives.GetBool() && !backEnd.viewDef->IsLightGem() )
			backEnd.pc.c_vboIndexes += numIndexes;
	} else {
		vertexCache.UnbindIndex();
		qglDrawElements( GL_TRIANGLES, numIndexes, GL_INDEX_TYPE, tri->indexes );
	}
}


/*
===============
RB_RenderTriangleSurface

Sets texcoord and vertex pointers
===============
*/
void RB_RenderTriangleSurface( const srfTriangles_t *tri ) {

	if ( !tri->ambientCache.IsValid() ) {
		RB_DrawElementsImmediate( tri );
		return;
	}

	const idDrawVert *ac = (idDrawVert *)vertexCache.VertexPosition( tri->ambientCache );
	//qglVertexPointer( 3, GL_FLOAT, sizeof( idDrawVert ), ac->xyz.ToFloatPtr() );
	qglVertexAttribPointer( 0, 3, GL_FLOAT, false, sizeof( idDrawVert ), &ac->xyz );
	//qglTexCoordPointer( 2, GL_FLOAT, sizeof( idDrawVert ), ac->st.ToFloatPtr() );

	RB_DrawElementsWithCounters( tri );
}

/*
===============
RB_T_RenderTriangleSurface

===============
*/
void RB_T_RenderTriangleSurface( const drawSurf_t *surf ) {
	RB_RenderTriangleSurface( surf->backendGeo );
}

/*
===============
RB_EnterWeaponDepthHack
===============
*/
void RB_EnterWeaponDepthHack() {
	qglDepthRange( 0.0f, 0.5f );

	float	matrix[16];

	memcpy( matrix, backEnd.viewDef->projectionMatrix, sizeof( matrix ) );

	matrix[14] *= 0.25f;

	qglMatrixMode(GL_PROJECTION);
	qglLoadMatrixf( matrix );
	qglMatrixMode(GL_MODELVIEW);
}

/*
===============
RB_EnterModelDepthHack
===============
*/
void RB_EnterModelDepthHack( float depth ) {
	qglDepthRange( 0.0f, 1.0f );

	float	matrix[16];

	memcpy( matrix, backEnd.viewDef->projectionMatrix, sizeof( matrix ) );

	matrix[14] -= depth;

	qglMatrixMode(GL_PROJECTION);
	qglLoadMatrixf( matrix );
	qglMatrixMode(GL_MODELVIEW);
}

/*
===============
RB_LeaveDepthHack
===============
*/
void RB_LeaveDepthHack() {
	qglDepthRange( 0.0f, 1.0f );

	qglMatrixMode(GL_PROJECTION);
	qglLoadMatrixf( backEnd.viewDef->projectionMatrix );
	qglMatrixMode(GL_MODELVIEW);
}

/*
====================
RB_RenderDrawSurfListWithFunction

The triangle functions can check backEnd.currentSpace != surf->space
to see if they need to perform any new matrix setup.  The modelview
matrix will already have been loaded, and backEnd.currentSpace will
be updated after the triangle function completes.
====================
*/
void RB_RenderDrawSurfListWithFunction( drawSurf_t **drawSurfs, int numDrawSurfs, 
											  void (*triFunc_)( const drawSurf_t *) ) {
	const drawSurf_t		*drawSurf;
	backEnd.currentSpace = NULL;

	for ( int i = 0  ; i < numDrawSurfs ; i++ ) {
		drawSurf = drawSurfs[i];

		// change the matrix if needed
		// Note (Serp) : this used to be ( drawSurf->space != backEnd.currentSpace) however, since it's always going to be NULL... 
		// Note (SteveL) : FIXME: It *won't* always be NULL, we're in a loop and it gets set at the end. This change might be wiping out 
		// all (marginal) benefits from sorting DrawSurfs by material, as it'll cause a blocking change in GL state on every draw. However, 
		// reverting the change causes all static solid surfaces to become invisible. Don't know why.
		// Note (duzenko): is the "return" causing it?
		const bool cacheMatrix = true;
		if ( cacheMatrix ) {
			if ( drawSurf->space != backEnd.currentSpace ) {
				qglLoadMatrixf( drawSurf->space->modelViewMatrix );
				//backEnd.currentSpace = drawSurf->space;
			}
		} else {
			if ( drawSurf->space ) {
				qglLoadMatrixf( drawSurf->space->modelViewMatrix );
			} else {
				return; // duzenko: what is this supposed to mean???
			}
		}

		if ( drawSurf->space->weaponDepthHack ) {
			RB_EnterWeaponDepthHack();
		}

		if ( drawSurf->space->modelDepthHack != 0.0f ) {
			RB_EnterModelDepthHack( drawSurf->space->modelDepthHack );
		}

		// change the scissor if needed
		if ( r_useScissor.GetBool() && !backEnd.currentScissor.Equals( drawSurf->scissorRect ) ) {
			backEnd.currentScissor = drawSurf->scissorRect;
			qglScissor( backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1, 
				backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
				backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
				backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1 );
		}

		// render it
		triFunc_( drawSurf );

		if ( drawSurf->space->weaponDepthHack || drawSurf->space->modelDepthHack != 0.0f ) {
			RB_LeaveDepthHack();
		}

		//if( !cacheMatrix )
		if ( drawSurf->space != backEnd.currentSpace )
			backEnd.currentSpace = drawSurf->space;
	}
}

/*
======================
RB_RenderDrawSurfChainWithFunction
======================
*/
void RB_RenderDrawSurfChainWithFunction( const drawSurf_t *drawSurfs, 
										void (*triFunc_)( const drawSurf_t *) ) {
	GL_CheckErrors();
	const drawSurf_t		*drawSurf;

	backEnd.currentSpace = NULL;

	for ( drawSurf = drawSurfs ; drawSurf ; drawSurf = drawSurf->nextOnLight ) {
		// change the matrix if needed
		// Note (Serp) : this used to be ( drawSurf->space != backEnd.currentSpace) however, since it's always going to be NULL...
		if ( drawSurf->space ) 
			qglLoadMatrixf( drawSurf->space->modelViewMatrix );
		else
			return;

		// change the scissor if needed
		if ( r_useScissor.GetBool() && !backEnd.currentScissor.Equals( drawSurf->scissorRect ) ) {
			const idScreenRect &r = drawSurf->scissorRect;
			if ( r.x1 <= r.x2 && r.y1 <= r.y2 ) { // duzenko: FIXME find out why they are negative sometimes
				backEnd.currentScissor = drawSurf->scissorRect;
				/*qglScissor( backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
					backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
					backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
					backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1 );*/
				FB_ApplyScissor();
			} else
				continue; // duzenko: why bother
		}

		if ( drawSurf->space->weaponDepthHack ) 
			RB_EnterWeaponDepthHack();

		if ( drawSurf->space->modelDepthHack ) 
			RB_EnterModelDepthHack( drawSurf->space->modelDepthHack );

		// render it
		triFunc_( drawSurf );

		if ( drawSurf->space->weaponDepthHack || drawSurf->space->modelDepthHack ) 
			RB_LeaveDepthHack();

		backEnd.currentSpace = drawSurf->space;
	}
	GL_CheckErrors();
}

/*
======================
RB_GetShaderTextureMatrix
======================
*/
void RB_GetShaderTextureMatrix( const float *shaderRegisters,
							   const textureStage_t *texture, float matrix[16] ) {
	matrix[0] = shaderRegisters[ texture->matrix[0][0] ];
	matrix[4] = shaderRegisters[ texture->matrix[0][1] ];
	matrix[8] = 0;
	matrix[12] = shaderRegisters[ texture->matrix[0][2] ];

	// we attempt to keep scrolls from generating incredibly large texture values, but
	// center rotations and center scales can still generate offsets that need to be > 1
	if ( matrix[12] < -40.0f || matrix[12] > 40.0f ) {
		matrix[12] -= (int)matrix[12];
	}

	matrix[1] = shaderRegisters[ texture->matrix[1][0] ];
	matrix[5] = shaderRegisters[ texture->matrix[1][1] ];
	matrix[9] = 0;
	matrix[13] = shaderRegisters[ texture->matrix[1][2] ];
	if ( matrix[13] < -40.0f || matrix[13] > 40.0f ) {
		// same with this
		matrix[13] -= (int)matrix[13];
	}

	matrix[2] = 0;
	matrix[6] = 0;
	matrix[10] = 1;
	matrix[14] = 0;

	matrix[3] = 0;
	matrix[7] = 0;
	matrix[11] = 0;
	matrix[15] = 1;
}

/*
======================
RB_LoadShaderTextureMatrix
======================
*/
void RB_LoadShaderTextureMatrix( const float *shaderRegisters, const textureStage_t *texture ) {
	float	matrix[16];

	RB_GetShaderTextureMatrix( shaderRegisters, texture, matrix );
	qglMatrixMode( GL_TEXTURE );
	qglLoadMatrixf( matrix );
	qglMatrixMode( GL_MODELVIEW );
}

/*
======================
RB_BindVariableStageImage

Handles generating a cinematic frame if needed
======================
*/
void RB_BindVariableStageImage( const textureStage_t *texture, const float *shaderRegisters ) {

	if ( texture->cinematic ) {
		if ( r_skipDynamicTextures.GetBool() ) {
			globalImages->defaultImage->Bind();
			return;
		}

		// offset time by shaderParm[7] (FIXME: make the time offset a parameter of the shader?)
		// We make no attempt to optimize for multiple identical cinematics being in view, or
		// for cinematics going at a lower framerate than the renderer.
		const cinData_t cin = texture->cinematic->ImageForTime( (int)(1000 * ( backEnd.viewDef->floatTime + backEnd.viewDef->renderView.shaderParms[11] ) ) );

		if ( cin.image ) {
			globalImages->cinematicImage->UploadScratch( cin.image, cin.imageWidth, cin.imageHeight );
		} else {
			globalImages->blackImage->Bind();
		}
	}
	
	else if ( texture->image ) {
		texture->image->Bind();
	}
}

/*
======================
RB_FinishStageTexture
======================
*/
void RB_FinishStageTexture( const textureStage_t *texture, const drawSurf_t *surf ) {

	if ( texture->texgen & (/*TG_DIFFUSE_CUBE |*/ TG_SKYBOX_CUBE  | TG_WOBBLESKY_CUBE) ) {
		qglTexCoordPointer( 2, GL_FLOAT, sizeof( idDrawVert ),
			(void *)&(((idDrawVert *)vertexCache.VertexPosition( surf->backendGeo->ambientCache ))->st) );
	}
	else if ( texture->texgen == TG_REFLECT_CUBE ) {
		qglDisableClientState( GL_NORMAL_ARRAY );

		qglMatrixMode( GL_TEXTURE );
		qglLoadIdentity();
		qglMatrixMode( GL_MODELVIEW );
	}

	if ( texture->hasMatrix ) {
		qglMatrixMode( GL_TEXTURE );
		qglLoadIdentity();
		qglMatrixMode( GL_MODELVIEW );
	}
}

/*
=================
RB_BeginDrawingView

Any mirrored or portaled views have already been drawn, so prepare
to actually render the visible surfaces for this view
=================
*/
void RB_BeginDrawingView (void) {
	// set the modelview matrix for the viewer
	qglMatrixMode(GL_PROJECTION);
	qglLoadMatrixf( backEnd.viewDef->projectionMatrix );
	qglMatrixMode(GL_MODELVIEW);

	// set the window clipping
	qglViewport( tr.viewportOffset[0] + backEnd.viewDef->viewport.x1, 
		tr.viewportOffset[1] + backEnd.viewDef->viewport.y1, 
		backEnd.viewDef->viewport.x2 + 1 - backEnd.viewDef->viewport.x1,
		backEnd.viewDef->viewport.y2 + 1 - backEnd.viewDef->viewport.y1 );

	// the scissor may be smaller than the viewport for subviews
	qglScissor( tr.viewportOffset[0] + backEnd.viewDef->viewport.x1 + backEnd.viewDef->scissor.x1, 
		tr.viewportOffset[1] + backEnd.viewDef->viewport.y1 + backEnd.viewDef->scissor.y1, 
		backEnd.viewDef->scissor.x2 + 1 - backEnd.viewDef->scissor.x1,
		backEnd.viewDef->scissor.y2 + 1 - backEnd.viewDef->scissor.y1 );
	backEnd.currentScissor = backEnd.viewDef->scissor;

	// ensures that depth writes are enabled for the depth clear
	GL_State( GLS_DEFAULT );

	// we don't have to clear the depth / stencil buffer for 2D rendering
	if ( backEnd.viewDef->viewEntitys ) {
		qglStencilMask( 0xff );
		// some cards may have 7 bit stencil buffers, so don't assume this
		// should be 128
		qglClearStencil( 1<<(glConfig.stencilBits-1) );
		qglClear( GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
		qglEnable( GL_DEPTH_TEST );
	} else {
		qglDisable( GL_DEPTH_TEST );
		qglDisable( GL_STENCIL_TEST );
	}

	backEnd.glState.faceCulling = -1;		// force face culling to set next time
	GL_Cull( CT_FRONT_SIDED );
}

/*
==================
R_SetDrawInteractions
==================
*/
void R_SetDrawInteraction( const shaderStage_t *surfaceStage, const float *surfaceRegs,
						  idImage **image, idVec4 matrix[2], float color[4] ) {
	*image = surfaceStage->texture.image;

	if ( surfaceStage->texture.hasMatrix ) {
		matrix[0][0] = surfaceRegs[surfaceStage->texture.matrix[0][0]];
		matrix[0][1] = surfaceRegs[surfaceStage->texture.matrix[0][1]];
		matrix[0][2] = 0;
		matrix[0][3] = surfaceRegs[surfaceStage->texture.matrix[0][2]];

		matrix[1][0] = surfaceRegs[surfaceStage->texture.matrix[1][0]];
		matrix[1][1] = surfaceRegs[surfaceStage->texture.matrix[1][1]];
		matrix[1][2] = 0;
		matrix[1][3] = surfaceRegs[surfaceStage->texture.matrix[1][2]];

		// we attempt to keep scrolls from generating incredibly large texture values, but
		// center rotations and center scales can still generate offsets that need to be > 1
		if ( matrix[0][3] < -40.0f || matrix[0][3] > 40.0f ) {
			matrix[0][3] -= (int)matrix[0][3];
		}
		if ( matrix[1][3] < -40.0f || matrix[1][3] > 40.0f ) {
			matrix[1][3] -= (int)matrix[1][3];
		}
	} else {
		matrix[0][0] = 1;
		matrix[0][1] = 0;
		matrix[0][2] = 0;
		matrix[0][3] = 0;

		matrix[1][0] = 0;
		matrix[1][1] = 1;
		matrix[1][2] = 0;
		matrix[1][3] = 0;
	}

	
	if ( color ) {
		color[0] = surfaceRegs[surfaceStage->color.registers[0]];
		color[1] = surfaceRegs[surfaceStage->color.registers[1]];
		color[2] = surfaceRegs[surfaceStage->color.registers[2]];
		color[3] = surfaceRegs[surfaceStage->color.registers[3]];

		// Serp - I dont think this is very useful with current cards
#if 0
		for ( int i = 0 ; i < 4 ; i++ ) {
			color[i] = surfaceRegs[surfaceStage->color.registers[i]];
			// clamp here, so card with greater range don't look different.
			// we could perform overbrighting like we do for lights, but
			// it doesn't currently look worth it.
			if ( color[i] < 0 ) {
				color[i] = 0.0f;
			} else if ( color[i] > 1.0f ) {
				color[i] = 1.0f;
			}
		}
#endif
	}
}

/*
=================
RB_SubmittInteraction
=================
*/
static void RB_SubmittInteraction( drawInteraction_t *din
	//, void (*DrawInteraction)(const drawInteraction_t *) 
) {

	if ( !din->bumpImage ) {
		return;
	}

	if ( r_skipBump.GetBool() ) {
		din->bumpImage = globalImages->flatNormalMap;
	}

	if ( !din->diffuseImage || r_skipDiffuse.GetBool() ) {
		din->diffuseImage = globalImages->blackImage;
	}
	// rebb: even ambient light has some specularity
	if ( !din->specularImage || r_skipSpecular.GetBool() /* || din->ambientLight */ ) {
		din->specularImage = globalImages->blackImage;
	}

	extern void RB_ARB2_DrawInteraction( const drawInteraction_t * din ); // duzenko FIXME ugly extern
	if ( r_useGLSL.GetBool() )
		RB_GLSL_DrawInteraction( din );
	else
		RB_ARB2_DrawInteraction( din );
	//DrawInteraction( din );
}

/*
=====================
RB_BakeTextureMatrixIntoTexgen
=====================
*/
void RB_BakeTextureMatrixIntoTexgen( idPlane lightProject[3], const float *textureMatrix ) {
	float	genMatrix[16];
	float	final[16];

	genMatrix[0] = lightProject[0][0];
	genMatrix[4] = lightProject[0][1];
	genMatrix[8] = lightProject[0][2];
	genMatrix[12] = lightProject[0][3];

	genMatrix[1] = lightProject[1][0];
	genMatrix[5] = lightProject[1][1];
	genMatrix[9] = lightProject[1][2];
	genMatrix[13] = lightProject[1][3];

	genMatrix[2] = 0;
	genMatrix[6] = 0;
	genMatrix[10] = 0;
	genMatrix[14] = 0;

	genMatrix[3] = lightProject[2][0];
	genMatrix[7] = lightProject[2][1];
	genMatrix[11] = lightProject[2][2];
	genMatrix[15] = lightProject[2][3];

	myGlMultMatrix( genMatrix, backEnd.lightTextureMatrix, final );

	lightProject[0][0] = final[0];
	lightProject[0][1] = final[4];
	lightProject[0][2] = final[8];
	lightProject[0][3] = final[12];

	lightProject[1][0] = final[1];
	lightProject[1][1] = final[5];
	lightProject[1][2] = final[9];
	lightProject[1][3] = final[13];
}

/*
=============
RB_CreateSingleDrawInteractions

This can be used by different draw_* backends to decompose a complex light / surface
interaction into primitive interactions
=============
*/
void RB_CreateSingleDrawInteractions( const drawSurf_t *surf
	//, void (*DrawInteraction)(const drawInteraction_t *) 
) {
	const idMaterial	*surfaceShader = surf->material;
	const float			*surfaceRegs = surf->shaderRegisters;
	const viewLight_t	*vLight = backEnd.vLight;
	const idMaterial	*lightShader = vLight->lightShader;
	const float			*lightRegs = vLight->shaderRegisters;
	drawInteraction_t	inter;

	//anon begin
	const bool useLightDepthBounds = r_useDepthBoundsTest.GetBool();
	//anon end

	if (!surf->backendGeo || !surf->backendGeo->ambientCache.IsValid()) 
		return;
	if ( vLight->lightShader->IsAmbientLight() ) {
		if ( r_skipAmbient.GetInteger() == 2 )
			return;
	} else
		if ( r_skipInteractions.GetBool() )
			return;

	if ( tr.logFile ) {
		RB_LogComment( "---------- RB_CreateSingleDrawInteractions %s on %s ----------\n", lightShader->GetName(), surfaceShader->GetName() );
	}
	//anon begin
	bool lightDepthBoundsDisabled = false;
	//anon end

	// change the matrix and light projection vectors if needed
	if ( surf->space != backEnd.currentSpace ) {
		backEnd.currentSpace = surf->space;
		qglLoadMatrixf( surf->space->modelViewMatrix );
		if (r_useAnonreclaimer.GetBool()) {
			//anon bengin
			// turn off the light depth bounds test if this model is rendered with a depth hack
			if (useLightDepthBounds)
			{
				if (!surf->space->weaponDepthHack && surf->space->modelDepthHack == 0.0f)
				{
					if (lightDepthBoundsDisabled)
					{
						GL_DepthBoundsTest(vLight->scissorRect.zmin, vLight->scissorRect.zmax);
						lightDepthBoundsDisabled = false;
					}
				}
				else
				{
					if (!lightDepthBoundsDisabled)
					{
						GL_DepthBoundsTest(0.0f, 0.0f);
						lightDepthBoundsDisabled = true;
					}
				}
			}
			//anon end
		}
	}

	// change the scissor if needed
	if ( r_useScissor.GetBool() && !backEnd.currentScissor.Equals( surf->scissorRect ) ) {
		backEnd.currentScissor = surf->scissorRect;
		qglScissor( backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1, 
			backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
			backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
			backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1 );
	}

	// hack depth range if needed
	if ( surf->space->weaponDepthHack ) {
		RB_EnterWeaponDepthHack();
	}

	if ( surf->space->modelDepthHack ) {
		RB_EnterModelDepthHack( surf->space->modelDepthHack );
	}

	inter.surf = surf;
	inter.lightFalloffImage = vLight->falloffImage;

	R_GlobalPointToLocal( surf->space->modelMatrix, vLight->globalLightOrigin, inter.localLightOrigin.ToVec3() );
	R_GlobalPointToLocal( surf->space->modelMatrix, backEnd.viewDef->renderView.vieworg, inter.localViewOrigin.ToVec3() );
	inter.localLightOrigin[3] = 0;
	inter.localViewOrigin[3] = 1;
	inter.cubicLight = lightShader->IsCubicLight(); // nbohr1more #3881: cubemap lights
	//inter.ambientCubicLight = lightShader->IsAmbientCubicLight(); // nbohr1more #3881: cubemap lights further changes
	inter.ambientLight = lightShader->IsAmbientLight();

	// rebb: world-up vector in local coordinates, required for certain effects, currently only for ambient lights. alternatively pass whole modelMatrix and calculate in shader
	// nbohr1more #3881: cubemap lights further changes
	if( lightShader->IsAmbientLight() /*|| lightShader->IsAmbientCubicLight()*/ ) {
		// remove commented code as needed, just shows what was simplified here
		//idVec3 upVec( 0.0f, 0.0f, 1.0f );
		//R_GlobalVectorToLocal( surf->space->modelMatrix, upVec, inter.worldUpLocal.ToVec3() );
		inter.worldUpLocal.x = surf->space->modelMatrix[2];
		inter.worldUpLocal.y = surf->space->modelMatrix[6];
		inter.worldUpLocal.z = surf->space->modelMatrix[10];
	}

	// the base projections may be modified by texture matrix on light stages
	idPlane lightProject[4];
	R_GlobalPlaneToLocal( surf->space->modelMatrix, backEnd.vLight->lightProject[0], lightProject[0] );
	R_GlobalPlaneToLocal( surf->space->modelMatrix, backEnd.vLight->lightProject[1], lightProject[1] );
	R_GlobalPlaneToLocal( surf->space->modelMatrix, backEnd.vLight->lightProject[2], lightProject[2] );
	R_GlobalPlaneToLocal( surf->space->modelMatrix, backEnd.vLight->lightProject[3], lightProject[3] );

	for ( int lightStageNum = 0 ; lightStageNum < lightShader->GetNumStages() ; lightStageNum++ ) {
		const shaderStage_t	*lightStage = lightShader->GetStage( lightStageNum );

		// ignore stages that fail the condition
		if ( !lightRegs[ lightStage->conditionRegister ] ) {
			continue;
		}

		inter.lightImage = lightStage->texture.image;

		memcpy( inter.lightProjection, lightProject, sizeof( inter.lightProjection ) );
		// now multiply the texgen by the light texture matrix
		if ( lightStage->texture.hasMatrix ) {
			RB_GetShaderTextureMatrix( lightRegs, &lightStage->texture, backEnd.lightTextureMatrix );
			RB_BakeTextureMatrixIntoTexgen( reinterpret_cast<class idPlane *>(inter.lightProjection), backEnd.lightTextureMatrix );
		}

		inter.bumpImage = NULL;
		inter.specularImage = NULL;
		inter.diffuseImage = NULL;
		inter.diffuseColor[0] = inter.diffuseColor[1] = inter.diffuseColor[2] = inter.diffuseColor[3] = 0;
		inter.specularColor[0] = inter.specularColor[1] = inter.specularColor[2] = inter.specularColor[3] = 0;

		// backEnd.lightScale is calculated so that lightColor[] will never exceed
		// tr.backEndRendererMaxLight
		float lightColor[4] = {
			lightColor[0] = backEnd.lightScale * lightRegs[ lightStage->color.registers[0] ],
			lightColor[1] = backEnd.lightScale * lightRegs[ lightStage->color.registers[1] ],
			lightColor[2] = backEnd.lightScale * lightRegs[ lightStage->color.registers[2] ],
			lightColor[3] = lightRegs[ lightStage->color.registers[3] ]};

		// go through the individual stages
		for ( int surfaceStageNum = 0 ; surfaceStageNum < surfaceShader->GetNumStages() ; surfaceStageNum++ ) {
			const shaderStage_t	*surfaceStage = surfaceShader->GetStage( surfaceStageNum );

			switch( surfaceStage->lighting ) {
				case SL_AMBIENT: {
					// ignore ambient stages while drawing interactions
					break;
				}
				case SL_BUMP: {
					// ignore stage that fails the condition
					if ( !surfaceRegs[ surfaceStage->conditionRegister ] ) {
						break;
					}
					// draw any previous interaction
					RB_SubmittInteraction( &inter/*, DrawInteraction*/ );
					inter.diffuseImage = NULL;
					inter.specularImage = NULL;
					R_SetDrawInteraction( surfaceStage, surfaceRegs, &inter.bumpImage, inter.bumpMatrix, NULL );
					break;
				}
				case SL_DIFFUSE: {
					// ignore stage that fails the condition
					if ( !surfaceRegs[ surfaceStage->conditionRegister ] ) {
						break;
					}
					else if ( inter.diffuseImage ) {
						RB_SubmittInteraction( &inter/*, DrawInteraction*/ );
					}
					R_SetDrawInteraction( surfaceStage, surfaceRegs, &inter.diffuseImage,
											inter.diffuseMatrix, inter.diffuseColor.ToFloatPtr() );
					inter.diffuseColor[0] *= lightColor[0];
					inter.diffuseColor[1] *= lightColor[1];
					inter.diffuseColor[2] *= lightColor[2];
					inter.diffuseColor[3] *= lightColor[3];
					inter.vertexColor = surfaceStage->vertexColor;
					break;
				}
				case SL_SPECULAR: {
					// ignore stage that fails the condition
					if ( !surfaceRegs[ surfaceStage->conditionRegister ] ) {
						break;
					}
					// nbohr1more: #4292 nospecular and nodiffuse fix
					else if ( backEnd.vLight->lightDef->parms.noSpecular ) {
					    break;
					}	
					else if ( inter.specularImage ) {
						RB_SubmittInteraction( &inter/*, DrawInteraction*/ );
					}
					R_SetDrawInteraction( surfaceStage, surfaceRegs, &inter.specularImage,
											inter.specularMatrix, inter.specularColor.ToFloatPtr() );
					inter.specularColor[0] *= lightColor[0];
					inter.specularColor[1] *= lightColor[1];
					inter.specularColor[2] *= lightColor[2];
					inter.specularColor[3] *= lightColor[3];
					inter.vertexColor = surfaceStage->vertexColor;
					break;
				}
			}
		}

		// draw the final interaction
		RB_SubmittInteraction( &inter/*, DrawInteraction*/ );
	}

	// unhack depth range if needed
	if ( surf->space->weaponDepthHack || surf->space->modelDepthHack != 0.0f ) {
		RB_LeaveDepthHack();
	}

	//anon begin
	if (r_useAnonreclaimer.GetBool())
	if (useLightDepthBounds && lightDepthBoundsDisabled)
	{
		GL_DepthBoundsTest(vLight->scissorRect.zmin, vLight->scissorRect.zmax);
	}
	//anon end
}

/*
=============
RB_DrawView
=============
*/
void RB_DrawView() {
	// we will need to do a new copyTexSubImage of the screen when a SS_POST_PROCESS material is used
	backEnd.currentRenderCopied = false;

	// if there aren't any drawsurfs, do nothing
	if ( !backEnd.viewDef->numDrawSurfs ) {
		return;
	}

	// skip render bypasses everything that has models, assuming
	// them to be 3D views, but leaves 2D rendering visible
	else if ( backEnd.viewDef->viewEntitys && r_skipRender.GetBool() ) {
		return;
	}

	// skip render context sets the wgl context to NULL,
	// which should factor out the API cost, under the assumption
	// that all gl calls just return if the context isn't valid
	else if ( backEnd.viewDef->viewEntitys && r_skipRenderContext.GetBool() ) {
		GLimp_DeactivateContext();
	}

	backEnd.pc.c_surfaces += backEnd.viewDef->numDrawSurfs;

	RB_ShowOverdraw();

	// render the scene, jumping to the hardware specific interaction renderers
	RB_STD_DrawView();

	// restore the context for 2D drawing if we were stubbing it out
	if ( r_skipRenderContext.GetBool() && backEnd.viewDef->viewEntitys ) {
		GLimp_ActivateContext();
		RB_SetDefaultGLState();
	}
}
