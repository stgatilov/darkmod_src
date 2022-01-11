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
#include "GLSLProgramManager.h"
#include "backend/RenderBackend.h"

#if defined(_MSC_VER) && _MSC_VER >= 1800 && !defined(DEBUG)
//#pragma optimize("t", off) // duzenko: used in release to enforce breakpoints in inlineable code. Please do not remove
#endif

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
	if ( tri->ambientSurface ) {
		if ( tri->indexes == tri->ambientSurface->indexes ) {
			backEnd.pc.c_drawRefIndexes += tri->numIndexes;
		}
		if ( tri->verts == tri->ambientSurface->verts ) {
			backEnd.pc.c_drawRefVertexes += tri->numVerts;
		}
	}

	if (r_glCoreProfile.GetInteger() > 0) {
#ifdef _DEBUG
		common->Warning("Drawing without index buffer not supported in Core profile!");
#endif
		return;
	}
	vertexCache.UnbindIndex();
	static vertCacheHandle_t nil;
	vertexCache.VertexPosition( nil );
	auto ac = tri->verts;
	qglDrawElements( GL_TRIANGLES, tri->numIndexes, GL_INDEX_TYPE, tri->indexes );
}

ID_INLINE void RB_PerfCounters( const drawSurf_t* surf, int instances = 1, bool shadows = false ) {
	if ( r_showPrimitives.GetBool() && backEnd.viewDef->viewEntitys ) {
		if ( shadows ) {
			if ( r_showPrimitives.GetBool() && !backEnd.viewDef->IsLightGem() ) {
				backEnd.pc.c_shadowElements++;
				backEnd.pc.c_shadowIndexes += surf->numIndexes;
				backEnd.pc.c_shadowVertexes += surf->frontendGeo->numVerts;
			}
		} else {
			backEnd.pc.c_drawElements++;
			backEnd.pc.c_drawIndexes += surf->numIndexes * instances;
			if ( surf->frontendGeo )
				backEnd.pc.c_drawVertexes += surf->frontendGeo->numVerts;
		}
	}
	if ( r_showEntityDraws && surf->space )
		if ( r_showEntityDraws & 4 ) {
			( (viewEntity_t*) surf->space )->drawCalls += surf->frontendGeo->numIndexes / 3;
		} else
			( (viewEntity_t*) surf->space )->drawCalls++;
}

/*
================
RB_DrawElementsWithCounters
================
*/
void RB_DrawElementsWithCounters( const drawSurf_t *surf ) {
	RB_PerfCounters( surf );
	void* indexPtr;
	if ( surf->indexCache.IsValid() ) {
		indexPtr = vertexCache.IndexPosition( surf->indexCache );
		if ( r_showPrimitives.GetBool() && !backEnd.viewDef->IsLightGem() ) {
			backEnd.pc.c_vboIndexes += surf->numIndexes;
		}
	} else {
		//note: this happens briefly when index cache is being resized
		if (r_glCoreProfile.GetInteger() > 0) {
#ifdef _DEBUG
			common->Warning("Drawing without index buffer not supported in Core profile!");
#endif
			return;
		}
		//TODO: remove this code?
		vertexCache.UnbindIndex();
		if ( !surf->frontendGeo ) return;
		indexPtr = surf->frontendGeo->indexes; // FIXME
	}
	int basePointer = vertexCache.GetBaseVertex();
	if ( basePointer < 0 )
		qglDrawElements( GL_TRIANGLES, surf->numIndexes, GL_INDEX_TYPE, indexPtr );
	else
		qglDrawElementsBaseVertex( GL_TRIANGLES, surf->numIndexes, GL_INDEX_TYPE, indexPtr, basePointer );
}

void RB_DrawTriangles( const srfTriangles_t &tri) {
	void* indexPtr;
	if ( tri.indexCache.IsValid() ) {
		indexPtr = vertexCache.IndexPosition( tri.indexCache );
		if ( r_showPrimitives.GetBool() && !backEnd.viewDef->IsLightGem() ) {
			backEnd.pc.c_vboIndexes += tri.numIndexes;
		}
	} else {
		if (r_glCoreProfile.GetInteger() > 0) {
#ifdef _DEBUG
			common->Warning("Drawing without index buffer not supported in Core profile!");
#endif
			return;
		}
		//TODO: remove this code?
		vertexCache.UnbindIndex();
		indexPtr = tri.indexes;
	}
	int basePointer = vertexCache.GetBaseVertex();
	if ( basePointer < 0 )
		qglDrawElements( GL_TRIANGLES, tri.numIndexes, GL_INDEX_TYPE, indexPtr );
	else
		qglDrawElementsBaseVertex( GL_TRIANGLES, tri.numIndexes, GL_INDEX_TYPE, indexPtr, basePointer );
}

/*
================
RB_DrawElementsInstanced
================
*/
void RB_DrawElementsInstanced( const drawSurf_t *surf, int instances ) {
	RB_PerfCounters( surf, instances );

	void* indexPtr;
	if ( surf->indexCache.IsValid() ) {
		indexPtr = vertexCache.IndexPosition( surf->indexCache );
		if ( r_showPrimitives.GetBool() && !backEnd.viewDef->IsLightGem() ) {
			backEnd.pc.c_vboIndexes += surf->numIndexes;
		}
	} else {
		if (r_glCoreProfile.GetInteger() > 0) {
#ifdef _DEBUG
			common->Warning("Drawing without index buffer not supported in Core profile!");
#endif
			return;
		}
		//TODO: remove this code?
		indexPtr = surf->frontendGeo->indexes; // FIXME?
		vertexCache.UnbindIndex();
	}
	int basePointer = vertexCache.GetBaseVertex();
	if ( basePointer < 0 )
		qglDrawElementsInstanced( GL_TRIANGLES, surf->numIndexes, GL_INDEX_TYPE, indexPtr, instances );
	else
		qglDrawElementsInstancedBaseVertex( GL_TRIANGLES, surf->numIndexes, GL_INDEX_TYPE, indexPtr, instances, basePointer );
}

/*
================
RB_DrawElementsMulti

Useful for batched calls with no state changes (depth pass, shadow maps)
================
*/
idCVarInt r_skipMultiDraw( "r_skipMultiDraw", "0", CVAR_RENDERER, "1 - skip single only, 2 - skip multi only" );

idList<const drawSurf_t*> allSurfaces( 1 << 11 );

void RB_Multi_AddSurf( const drawSurf_t* surf ) {
	auto ent = surf->space;
	if ( !ent->weaponDepthHack && ent->modelDepthHack == 0 )
		allSurfaces.Append( surf );
	else
		RB_DrawElementsWithCounters( surf );
}

void RB_Multi_DrawElements( int instances ) {
	idList<const drawSurf_t*> spaceSurfaces( 1 << 7 );
	idList<GLsizei> multiDrawCount( 1 << 7 );
	idList<void*> multiDrawIndices( 1 << 7 );
	idList<GLint> multiDrawBaseVertices( 1 << 7 );

	auto DrawSpaceSurfaces = [&]() {
		if ( !spaceSurfaces.Num() )
			return;
		if ( GLSLProgram::GetCurrentProgram() != nullptr ) {
			Uniforms::Global* transformUniforms = GLSLProgram::GetCurrentProgram()->GetUniformGroup<Uniforms::Global>();
			transformUniforms->Set( spaceSurfaces[0]->space );
		}
		if ( spaceSurfaces.Num() == 1 ) {
			if ( !( r_skipMultiDraw & 1 ) ) {
				vertexCache.VertexPosition( spaceSurfaces[0]->ambientCache );
				RB_DrawElementsWithCounters( spaceSurfaces[0] );
			}
			spaceSurfaces.SetNum( 0, false );
			return;
		}
		if ( r_skipMultiDraw & 2 ) {
			spaceSurfaces.SetNum( 0, false );
			return;
		}
		if ( r_showPrimitives.GetBool() && !backEnd.viewDef->IsLightGem() && backEnd.viewDef->viewEntitys ) {
			backEnd.pc.c_drawElements++;
		}
		for ( auto surf : spaceSurfaces ) {
			multiDrawCount.Append( surf->numIndexes );
			void* offset = (void*)(size_t)surf->indexCache.offset;
			multiDrawIndices.Append( offset );
			vertexCache.VertexPosition( surf->ambientCache );
			int baseVertex = vertexCache.GetBaseVertex();
			if ( baseVertex < 0 )
				common->Error( "Invalid base vertex in RB_Multi_AddSurf" );
			multiDrawBaseVertices.Append( baseVertex );
			RB_PerfCounters( surf );
		}
		vertCacheHandle_t hBufferStart{ 1,0,0 };
		vertexCache.IndexPosition( hBufferStart );
		vertexCache.VertexPosition( hBufferStart );
		auto indices = (const void* const*)multiDrawIndices.Ptr();
#if 1
		qglMultiDrawElementsBaseVertex( GL_TRIANGLES, multiDrawCount.Ptr(), GL_INDEX_TYPE, indices, multiDrawCount.Num(), multiDrawBaseVertices.Ptr() );
#else
		for ( int i = 0; i < multiDrawCount.Num(); i++ ) {
			qglDrawElementsBaseVertex( GL_TRIANGLES, multiDrawCount[i], GL_INDEX_TYPE, indices[i], multiDrawBaseVertices[i] );
			GL_CheckErrors();
		}
#endif
		multiDrawCount.SetNum( 0, false );
		multiDrawIndices.SetNum( 0, false );
		multiDrawBaseVertices.SetNum( 0, false );
		spaceSurfaces.SetNum( 0, false );
	};

	backEnd.currentSpace = NULL;
	for ( auto surf : allSurfaces ) {
		if ( surf->space != backEnd.currentSpace ) {
			DrawSpaceSurfaces();
			backEnd.currentSpace = surf->space;
		}
		spaceSurfaces.Append( surf );
	}
	DrawSpaceSurfaces();
	allSurfaces.SetNum( 0, false );
}

/*
================
RB_DrawShadowElementsWithCounters

May not use all the indexes in the surface if caps are skipped
================
*/
void RB_DrawShadowElementsWithCounters( const drawSurf_t *surf ) {
	RB_PerfCounters( surf, 1, true );

	void* indexPtr;
	if ( surf->indexCache.IsValid() ) {
		indexPtr = vertexCache.IndexPosition( surf->indexCache );
	} else {
		if (r_glCoreProfile.GetInteger() > 0) {
#ifdef _DEBUG
			common->Warning("Drawing without index buffer not supported in Core profile!");
#endif
			return;
		}
		//TODO: remove this code?
		vertexCache.UnbindIndex();
		indexPtr = surf->frontendGeo->indexes; // FIXME
	}
	int basePointer = vertexCache.GetBaseVertex();
	if ( basePointer < 0 )
		qglDrawElements( GL_TRIANGLES, surf->numIndexes, GL_INDEX_TYPE, indexPtr );
	else
		qglDrawElementsBaseVertex( GL_TRIANGLES, surf->numIndexes, GL_INDEX_TYPE, indexPtr, basePointer );
}


/*
===============
RB_RenderTriangleSurface

Sets vertex pointers
===============
*/
void RB_RenderTriangleSurface( const drawSurf_t *surf ) {
	if ( !surf->ambientCache.IsValid() ) {
		RB_DrawElementsImmediate( surf->frontendGeo );
		return;
	}
	vertexCache.VertexPosition( surf->ambientCache );
	RB_DrawElementsWithCounters( surf );
}

/*
===============
RB_T_RenderTriangleSurface

===============
*/
void RB_T_RenderTriangleSurface( const drawSurf_t *surf ) {
	RB_RenderTriangleSurface( surf );
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

	// FIXME: this part is broken since storing projection matrix in uniform block
	auto prog = GLSLProgram::GetCurrentProgram();
	if ( prog ) {
		Uniforms::Global* transformUniforms = prog->GetUniformGroup<Uniforms::Global>();
		//transformUniforms->projectionMatrix.Set( matrix );
	}
}

/*
===============
RB_EnterModelDepthHack
===============
*/
void RB_EnterModelDepthHack( float depth ) {
	// FIXME: this is completely broken, is it even still needed?
	
	qglDepthRange( 0.0f, 1.0f );

	float	matrix[16];

	memcpy( matrix, backEnd.viewDef->projectionMatrix, sizeof( matrix ) );

	matrix[14] -= depth;

	auto prog = GLSLProgram::GetCurrentProgram();
	if ( prog ) {
		Uniforms::Global* transformUniforms = prog->GetUniformGroup<Uniforms::Global>();
		//transformUniforms->projectionMatrix.Set( matrix );
	}
}

/*
===============
RB_LeaveDepthHack
===============
*/
void RB_LeaveDepthHack() {
	qglDepthRange( 0.0f, 1.0f );

	if ( auto prog = GLSLProgram::GetCurrentProgram() ) {
		Uniforms::Global* transformUniforms = prog->GetUniformGroup<Uniforms::Global>();
		//transformUniforms->projectionMatrix.Set( backEnd.viewDef->projectionMatrix );
	}
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
void RB_RenderDrawSurfListWithFunction( drawSurf_t **drawSurfs, int numDrawSurfs, void ( *triFunc_ )( const drawSurf_t * ) ) {
	GL_CheckErrors();

	backEnd.currentSpace = nullptr;

	/* Reverted all the unnessesary gunk here */
	for ( int i = 0  ; i < numDrawSurfs ; i++ ) {
		const drawSurf_t *drawSurf = drawSurfs[i];

		if ( drawSurf->space != backEnd.currentSpace ) {
			if( GLSLProgram::GetCurrentProgram() != nullptr ) {
				Uniforms::Global *transformUniforms = GLSLProgram::GetCurrentProgram()->GetUniformGroup<Uniforms::Global>();
				transformUniforms->Set( drawSurf->space );
			}
			GL_CheckErrors();
		}

		if ( drawSurf->space->weaponDepthHack ) {
			RB_EnterWeaponDepthHack();
			GL_CheckErrors();
		}

		if ( drawSurf->space->modelDepthHack != 0.0f ) {
			RB_EnterModelDepthHack( drawSurf->space->modelDepthHack );
			GL_CheckErrors();
		}

#if 1 // duzenko: this is needed for portal fogging e.g. in Lone Salvation
		if ( r_useScissor.GetBool() && !backEnd.currentScissor.Equals( drawSurf->scissorRect ) ) {
			backEnd.currentScissor = drawSurf->scissorRect;
			// revelator: test. parts of the functions loaded here also runs through the fbo transforms (the code for filling the depthbuffer for instance)
			FB_ApplyScissor();
			GL_CheckErrors();
		}
#endif

		// render it
		triFunc_( drawSurf );
		GL_CheckErrors();

		if ( drawSurf->space->weaponDepthHack || drawSurf->space->modelDepthHack != 0.0f ) {
			RB_LeaveDepthHack();
			GL_CheckErrors();
		}

		// mark currentSpace if we have drawn.
		backEnd.currentSpace = drawSurf->space;
	}
	GL_CheckErrors();
}

/*
======================
RB_RenderDrawSurfChainWithFunction

The triangle functions can check backEnd.currentSpace != surf->space
to see if they need to perform any new matrix setup.  The modelview
matrix will already have been loaded, and backEnd.currentSpace will
be updated after the triangle function completes.

Revelator: i left in console prints so that devs dont get any nasty ideas about how this works.
Try and enable them if in doubt, but disable them again after use because the console spam will kill performance pretty badly.
======================
*/
void RB_RenderDrawSurfChainWithFunction( const drawSurf_t *drawSurfs, void ( *triFunc_ )( const drawSurf_t * ) ) {
	GL_CheckErrors();

	backEnd.currentSpace = nullptr;

	/* Reverted all the unnessesary gunk here */
	for ( const drawSurf_t *drawSurf = drawSurfs; drawSurf; drawSurf = drawSurf->nextOnLight ) {
		if ( drawSurf->space != backEnd.currentSpace ) {
			//common->Printf( "Yay i just loaded the matrix again, because (drawSurf->space does not equal backEnd.currentSpace) because it is NULL\n" );
			if ( GLSLProgram::GetCurrentProgram() != nullptr ) {
				Uniforms::Global* transformUniforms = GLSLProgram::GetCurrentProgram()->GetUniformGroup<Uniforms::Global>();
				transformUniforms->Set( drawSurf->space );
			}
		}

		if ( drawSurf->space->weaponDepthHack ) {
			//common->Printf( "Yay i just ran a depth hack on viewmodels\n" );
			RB_EnterWeaponDepthHack();
		}

		if ( drawSurf->space->modelDepthHack != 0.0f ) {
			//common->Printf( "Yay i just ran a depth hack on other models\n" );
			RB_EnterModelDepthHack( drawSurf->space->modelDepthHack );
		}

		/* change the scissor if needed
		#7627 revelator reverted and cleaned up. */
		if ( r_useScissor.GetBool() && !backEnd.currentScissor.Equals( drawSurf->scissorRect ) ) {
			//common->Printf( "Yay i just ran the scissor, because now the scissor equals the viewport\n" );
			backEnd.currentScissor = drawSurf->scissorRect;
			FB_ApplyScissor();
		}

		// render it
		//common->Printf( "Yay i just ran a function, i hope someone does not do returns or continues above or im busted\n" );
		triFunc_( drawSurf );

		if ( drawSurf->space->weaponDepthHack || drawSurf->space->modelDepthHack != 0.0f ) {
			//common->Printf( "Booh i just disabled the depth hacks\n" );
			RB_LeaveDepthHack();
		}

		// mark currentSpace if we have drawn.
		//common->Printf( "Yay i just determined that i dont need to run again, so ill set (backEnd.currentSpace to the value of drawSurf->space) so it is no longer NULL\n" );
		backEnd.currentSpace = drawSurf->space;
	}
	GL_CheckErrors();
}

/*
======================
RB_GetShaderTextureMatrix
======================
*/
void RB_GetShaderTextureMatrix( const float *shaderRegisters, const textureStage_t *texture, float matrix[16] ) {
	matrix[0] = shaderRegisters[ texture->matrix[0][0] ];
	matrix[4] = shaderRegisters[ texture->matrix[0][1] ];
	matrix[8] = 0;
	matrix[12] = shaderRegisters[ texture->matrix[0][2] ];

	// we attempt to keep scrolls from generating incredibly large texture values, but
	// center rotations and center scales can still generate offsets that need to be > 1
	if ( matrix[12] < -40.0f || matrix[12] > 40.0f ) {
		matrix[12] -= ( int )matrix[12];
	}
	matrix[1] = shaderRegisters[ texture->matrix[1][0] ];
	matrix[5] = shaderRegisters[ texture->matrix[1][1] ];
	matrix[9] = 0;
	matrix[13] = shaderRegisters[ texture->matrix[1][2] ];

	if ( matrix[13] < -40.0f || matrix[13] > 40.0f ) {
		// same with this
		matrix[13] -= ( int )matrix[13];
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
void RB_LoadShaderTextureMatrix( const float *shaderRegisters, const shaderStage_t* pStage ) {
	const auto *texture = &pStage->texture;
	if ( !texture->hasMatrix )
		return;

	auto prog = GLSLProgram::GetCurrentProgram();
	if ( !prog )
		return;

	if ( shaderRegisters ) {
		float	matrix[16];
		RB_GetShaderTextureMatrix( shaderRegisters, texture, matrix );
		prog->GetUniformGroup<Uniforms::Global>()->textureMatrix.Set( matrix );
	} else 
		prog->GetUniformGroup<Uniforms::Global>()->textureMatrix.Set( mat4_identity );

	/*qglMatrixMode( GL_TEXTURE );
	qglLoadMatrixf( matrix );
	qglMatrixMode( GL_MODELVIEW );*/
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
		const cinData_t cin = texture->cinematic->ImageForTime( ( int )( 1000 * ( backEnd.viewDef->floatTime + backEnd.viewDef->renderView.shaderParms[11] ) ) );

		if ( cin.image ) {
			globalImages->cinematicImage->UploadScratch( cin.image, cin.imageWidth, cin.imageHeight );
		} else {
			globalImages->blackImage->Bind();
		}
	} else if ( texture->image ) {
		texture->image->Bind();
	}
}

/*
=================
RB_BeginDrawingView

Any mirrored or portaled views have already been drawn, so prepare
to actually render the visible surfaces for this view
=================
*/
void RB_BeginDrawingView( void ) {
	auto& viewDef = backEnd.viewDef;
	// set the modelview matrix for the viewer
	GL_SetProjection( (float *)backEnd.viewDef->projectionMatrix );

	// set the window clipping
	GL_ViewportVidSize( tr.viewportOffset[0] + backEnd.viewDef->viewport.x1,
	             tr.viewportOffset[1] + backEnd.viewDef->viewport.y1,
	             backEnd.viewDef->viewport.x2 + 1 - backEnd.viewDef->viewport.x1,
	             backEnd.viewDef->viewport.y2 + 1 - backEnd.viewDef->viewport.y1 );

	// the scissor may be smaller than the viewport for subviews
	GL_ScissorVidSize( tr.viewportOffset[0] + backEnd.viewDef->viewport.x1 + backEnd.viewDef->scissor.x1,
	            tr.viewportOffset[1] + backEnd.viewDef->viewport.y1 + backEnd.viewDef->scissor.y1,
	            backEnd.viewDef->scissor.x2 + 1 - backEnd.viewDef->scissor.x1,
	            backEnd.viewDef->scissor.y2 + 1 - backEnd.viewDef->scissor.y1 );
	backEnd.currentScissor = backEnd.viewDef->scissor;

	// ensures that depth writes are enabled for the depth clear
	GL_State( GLS_DEFAULT );

	// we don't have to clear the depth / stencil buffer for 2D rendering
	if ( backEnd.viewDef->viewEntitys ) {
		qglStencilMask( 0xff );
		// we use framebuffers with one of attachments: GL_DEPTH24_STENCIL8, GL_DEPTH32F_STENCIL8 or GL_STENCIL_INDEX8
		// all of them are exactly 8-bit, so middle value 128 serves as "zero" for us
		qglClearStencil( 128 );
		qglClear( GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
		qglEnable( GL_DEPTH_TEST );
	} else {
		qglDisable( GL_DEPTH_TEST );
		qglDisable( GL_STENCIL_TEST );
	}
	if ( viewDef && ( viewDef->xrayEntityMask || viewDef->superView && viewDef->superView->hasXraySubview ) ) {
		qglClearColor( 0, 0, 0, 0 );
		qglClear( GL_COLOR_BUFFER_BIT );
	} // else allow alpha blending with background
	backEnd.glState.faceCulling = -1;		// force face culling to set next time

	GL_Cull( CT_FRONT_SIDED );
	GL_CheckErrors();
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
			matrix[0][3] -= ( int )matrix[0][3];
		}

		if ( matrix[1][3] < -40.0f || matrix[1][3] > 40.0f ) {
			matrix[1][3] -= ( int )matrix[1][3];
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
	}
}

/*
=================
RB_SubmittInteraction
=================
*/
static void RB_SubmitInteraction( drawInteraction_t* din, bool multi = false ) {
	GL_CheckErrors();
	if ( !din->bumpImage && !r_skipBump.GetBool() )
		return;

	if ( !din->diffuseImage || r_skipDiffuse.GetBool() ) {
		din->diffuseImage = globalImages->blackImage;
	}

	// rebb: even ambient light has some specularity
	if ( !din->specularImage || r_skipSpecular.GetBool() ) {
		din->specularImage = globalImages->blackImage;
	}

	if ( multi ) {
		extern void RB_GLSL_DrawInteraction_MultiLight( const drawInteraction_t * din );
		RB_GLSL_DrawInteraction_MultiLight( din );
	} else
		RB_GLSL_DrawInteraction( din );
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

	myGlMultMatrix( genMatrix, textureMatrix, final );

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
void RB_CreateSingleDrawInteractions( const drawSurf_t *surf ) {
	const idMaterial	*material = surf->material;
	const float			*surfaceRegs = surf->shaderRegisters;
	const viewLight_t	*vLight = backEnd.vLight;
	const idMaterial	*lightShader = vLight->lightShader;
	const float			*lightRegs = vLight->shaderRegisters;
	drawInteraction_t	inter;

	if ( !surf->ambientCache.IsValid() ) {
		return;
	}
	GL_CheckErrors();

	if ( vLight->lightShader->IsAmbientLight() ) {
		if ( r_skipAmbient.GetInteger() & 2 )
			return;
		auto ambientRegs = material->GetAmbientRimColor().registers;
		if ( ambientRegs[0] ) {
			for ( int i = 0; i < 3; i++ )
				inter.ambientRimColor[i] = surfaceRegs[ambientRegs[i]];
			inter.ambientRimColor[3] = 1;
		} else
			inter.ambientRimColor.Zero();
	} else if ( r_skipInteractions.GetBool() ) 
		return;

	if ( tr.logFile ) {
		RB_LogComment( "---------- RB_CreateSingleDrawInteractions %s on %s ----------\n", lightShader->GetName(), material->GetName() );
	}
	GL_CheckErrors();

	// change the matrix and light projection vectors if needed
	if ( surf->space != backEnd.currentSpace ) {
		backEnd.currentSpace = surf->space;

		GL_CheckErrors();
		if( GLSLProgram::GetCurrentProgram() != nullptr ) {
			Uniforms::Global *transformUniforms = GLSLProgram::GetCurrentProgram()->GetUniformGroup<Uniforms::Global>();
			transformUniforms->Set( surf->space );
		}
		GL_CheckErrors();

		// turn off the light depth bounds test if this model is rendered with a depth hack
		/*if ( !surf->space->weaponDepthHack && surf->space->modelDepthHack == 0.0f ) {
			if ( backEnd.lightDepthBoundsDisabled ) {
				GL_DepthBoundsTest( vLight->scissorRect.zmin, vLight->scissorRect.zmax );
				backEnd.lightDepthBoundsDisabled = false;
			}
		} else {
			if ( !backEnd.lightDepthBoundsDisabled ) {
				GL_DepthBoundsTest( 0.0f, 0.0f );
				backEnd.lightDepthBoundsDisabled = true;
			}
		}*/
	}

	// change the scissor if needed
	if ( r_useScissor.GetBool() && !backEnd.currentScissor.Equals( surf->scissorRect ) ) {
		backEnd.currentScissor = surf->scissorRect;
		GL_ScissorVidSize( backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
		            backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
		            backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
		            backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1 );
		GL_CheckErrors();
	}

	// hack depth range if needed
	if ( surf->space->weaponDepthHack ) {
		RB_EnterWeaponDepthHack();
		GL_CheckErrors();
	}

	if ( surf->space->modelDepthHack != 0.0f ) {
		RB_EnterModelDepthHack( surf->space->modelDepthHack );
		GL_CheckErrors();
	}
	inter.surf = surf;
	inter.lightFalloffImage = vLight->falloffImage;

	R_GlobalPointToLocal( surf->space->modelMatrix, vLight->globalLightOrigin, inter.localLightOrigin.ToVec3() );
	R_GlobalPointToLocal( surf->space->modelMatrix, backEnd.viewDef->renderView.vieworg, inter.localViewOrigin.ToVec3() );
	inter.localLightOrigin[3] = 0;
	inter.localViewOrigin[3] = 1;
	inter.cubicLight = lightShader->IsCubicLight(); // nbohr1more #3881: cubemap lights
	inter.ambientLight = lightShader->IsAmbientLight();

	// rebb: world-up vector in local coordinates, required for certain effects, currently only for ambient lights. alternatively pass whole modelMatrix and calculate in shader
	// nbohr1more #3881: cubemap lights further changes
	if ( lightShader->IsAmbientLight() ) {
		// remove commented code as needed, just shows what was simplified here
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

	for ( int lightStageNum = 0; lightStageNum < lightShader->GetNumStages(); lightStageNum++ ) {
		const shaderStage_t	*lightStage = lightShader->GetStage( lightStageNum );

		// ignore stages that fail the condition
		if ( !lightRegs[lightStage->conditionRegister] ) {
			continue;
		}
		inter.lightImage = lightStage->texture.image;

		memcpy( inter.lightProjection, lightProject, sizeof( inter.lightProjection ) );

		float lightTexMatrix[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
		if ( lightStage->texture.hasMatrix )
			RB_GetShaderTextureMatrix( lightRegs, &lightStage->texture, lightTexMatrix );
		// stgatilov: we no longer merge two transforms together, since we need light-volume coords in fragment shader
		//RB_BakeTextureMatrixIntoTexgen( reinterpret_cast<class idPlane *>(inter.lightProjection), lightTexMatrix );
		inter.lightTextureMatrix[0].Set( lightTexMatrix[0], lightTexMatrix[4], 0, lightTexMatrix[12] );
		inter.lightTextureMatrix[1].Set( lightTexMatrix[1], lightTexMatrix[5], 0, lightTexMatrix[13] );

		inter.bumpImage = NULL;
		inter.specularImage = NULL;
		inter.diffuseImage = NULL;
		inter.diffuseColor[0] = inter.diffuseColor[1] = inter.diffuseColor[2] = inter.diffuseColor[3] = 0;
		inter.specularColor[0] = inter.specularColor[1] = inter.specularColor[2] = inter.specularColor[3] = 0;

		// backEnd.lightScale is calculated so that lightColor[] will never exceed
		// tr.backEndRendererMaxLight
		float lightColor[4] = {
			lightColor[0] = backEnd.lightScale * lightRegs[lightStage->color.registers[0]],
			lightColor[1] = backEnd.lightScale * lightRegs[lightStage->color.registers[1]],
			lightColor[2] = backEnd.lightScale * lightRegs[lightStage->color.registers[2]],
			lightColor[3] = lightRegs[lightStage->color.registers[3]]
		};

		// go through the individual stages
		for ( int surfaceStageNum = 0; surfaceStageNum < material->GetNumStages(); surfaceStageNum++ ) {
			const shaderStage_t	*surfaceStage = material->GetStage( surfaceStageNum );

			switch ( surfaceStage->lighting ) {
			case SL_AMBIENT: {
				// ignore ambient stages while drawing interactions
				break;
			}
			case SL_BUMP: {				
				if ( !surfaceRegs[ surfaceStage->conditionRegister ] ) // ignore stage that fails the condition
					break;
				if ( !r_skipBump.GetBool() ) {
					RB_SubmitInteraction( &inter ); // draw any previous interaction
					inter.diffuseImage = NULL;
					inter.specularImage = NULL;
					R_SetDrawInteraction( surfaceStage, surfaceRegs, &inter.bumpImage, inter.bumpMatrix, NULL );
				}
				break;
			}
			case SL_DIFFUSE: {
				// ignore stage that fails the condition
				if ( !surfaceRegs[ surfaceStage->conditionRegister ] ) {
					break;
				} else if ( inter.diffuseImage ) {
					RB_SubmitInteraction( &inter );
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
				else if ( backEnd.vLight->noSpecular ) {
					break;
				} else if ( inter.specularImage ) {
					RB_SubmitInteraction( &inter );
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
		RB_SubmitInteraction( &inter );
		GL_CheckErrors();
	}

	// unhack depth range if needed
	if ( surf->space->weaponDepthHack || surf->space->modelDepthHack != 0.0f ) {
		RB_LeaveDepthHack();
		GL_CheckErrors();
	}

	/*	if ( backEnd.useLightDepthBounds && backEnd.lightDepthBoundsDisabled ) {
			GL_DepthBoundsTest( vLight->scissorRect.zmin, vLight->scissorRect.zmax );
		}*/
}

void RB_CreateMultiDrawInteractions( const drawSurf_t *surf ) {
	const idMaterial	*material = surf->material;
	const float			*surfaceRegs = surf->shaderRegisters;
	drawInteraction_t	inter;

	if ( !surf->ambientCache.IsValid() ) {
		return;
	}

	if ( r_skipInteractions.GetBool() ) {
		return;
	}

	if ( tr.logFile ) {
		//RB_LogComment( "---------- RB_CreateSingleDrawInteractions %s on %s ----------\n", lightShader->GetName(), material->GetName() );
	}

	// change the matrix and light projection vectors if needed
	if ( surf->space != backEnd.currentSpace ) {
		backEnd.currentSpace = surf->space;

		qglLoadMatrixf( surf->space->modelViewMatrix );
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

	R_GlobalPointToLocal( surf->space->modelMatrix, backEnd.viewDef->renderView.vieworg, inter.localViewOrigin.ToVec3() );
	inter.localLightOrigin[3] = 0;
	inter.localViewOrigin[3] = 1;

		inter.bumpImage = NULL;
		inter.specularImage = NULL;
		inter.diffuseImage = NULL;
		inter.diffuseColor[0] = inter.diffuseColor[1] = inter.diffuseColor[2] = inter.diffuseColor[3] = 0;
		inter.specularColor[0] = inter.specularColor[1] = inter.specularColor[2] = inter.specularColor[3] = 0;

		// go through the individual stages
		for ( int surfaceStageNum = 0; surfaceStageNum < material->GetNumStages(); surfaceStageNum++ ) {
			const shaderStage_t	*surfaceStage = material->GetStage( surfaceStageNum );

			switch ( surfaceStage->lighting ) {
			case SL_AMBIENT: {
				// ignore ambient stages while drawing interactions
				break;
			}
			case SL_BUMP: {
				// ignore stage that fails the condition
				if ( !surfaceRegs[surfaceStage->conditionRegister] ) {
					break;
				}
				// draw any previous interaction
				RB_SubmitInteraction( &inter, true );
				inter.diffuseImage = NULL;
				inter.specularImage = NULL;
				R_SetDrawInteraction( surfaceStage, surfaceRegs, &inter.bumpImage, inter.bumpMatrix, NULL );
				break;
			}
			case SL_DIFFUSE: {
				// ignore stage that fails the condition
				if ( !surfaceRegs[surfaceStage->conditionRegister] ) {
					break;
				} else if ( inter.diffuseImage ) {
					RB_SubmitInteraction( &inter, true );
				}
				R_SetDrawInteraction( surfaceStage, surfaceRegs, &inter.diffuseImage,
					inter.diffuseMatrix, inter.diffuseColor.ToFloatPtr() );
				inter.vertexColor = surfaceStage->vertexColor;
				break;
			}
			case SL_SPECULAR: {
				// ignore stage that fails the condition
				if ( !surfaceRegs[surfaceStage->conditionRegister] ) {
					break;
				}
				else if ( inter.specularImage ) {
					RB_SubmitInteraction( &inter, true );
				}
				R_SetDrawInteraction( surfaceStage, surfaceRegs, &inter.specularImage,
					inter.specularMatrix, inter.specularColor.ToFloatPtr() );
				inter.vertexColor = surfaceStage->vertexColor;
				break;
			}
			}
		}

		// draw the final interaction
		RB_SubmitInteraction( &inter, true );

	// unhack depth range if needed
	if ( surf->space->weaponDepthHack || surf->space->modelDepthHack != 0.0f ) {
		RB_LeaveDepthHack();
	}
}

/*
=============
RB_DrawView
=============
*/
void RB_DrawView( void ) {
	// we will need to do a new copyTexSubImage of the screen when a SS_POST_PROCESS material is used
	backEnd.currentRenderCopied = false;
	backEnd.afterFogRendered = false;

	// if there aren't any drawsurfs, do nothing
	if ( !backEnd.viewDef->numDrawSurfs ) {
		//return;
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
