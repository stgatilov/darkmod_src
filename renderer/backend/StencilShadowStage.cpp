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

#include "StencilShadowStage.h"
#include "../Profiling.h"
#include "ShaderParamsBuffer.h"
#include "DrawBatchExecutor.h"
#include "../GLSLProgram.h"
#include "../GLSLProgramManager.h"

struct StencilShadowStage::ShaderParams {
	idMat4 modelViewMatrix;
	idVec4 localLightOrigin;
};

StencilShadowStage::StencilShadowStage( ShaderParamsBuffer *shaderParamsBuffer, DrawBatchExecutor *drawBatchExecutor ) {
	this->shaderParamsBuffer = shaderParamsBuffer;
	this->drawBatches = drawBatchExecutor;
}

void StencilShadowStage::Init() {
	maxSupportedDrawsPerBatch = shaderParamsBuffer->MaxSupportedParamBufferSize<ShaderParams>();
	idDict defines;
	defines.Set( "MAX_SHADER_PARAMS", idStr::Fmt("%d", maxSupportedDrawsPerBatch) );
	stencilShadowShader = programManager->LoadFromFiles( "shadows_stencil", 
		"stages/stencil/stencilshadow.vert.glsl",
		"stages/stencil/stencilshadow.frag.glsl",
		defines );
}

void StencilShadowStage::Shutdown() {}

void StencilShadowStage::DrawStencilShadows( viewLight_t *vLight, const drawSurf_t *shadowSurfs ) {
	if ( !shadowSurfs || !r_shadows.GetInteger() ) {
		return;
	}
	GL_PROFILE( "StencilShadowPass" );

	if ( r_shadowPolygonFactor.GetFloat() || r_shadowPolygonOffset.GetFloat() ) {
		qglPolygonOffset( r_shadowPolygonFactor.GetFloat(), -r_shadowPolygonOffset.GetFloat() );
		qglEnable( GL_POLYGON_OFFSET_FILL );
	}
	GL_State( GLS_DEPTHMASK | GLS_COLORMASK | GLS_ALPHAMASK | GLS_DEPTHFUNC_LESS );
	qglStencilFunc( GL_ALWAYS, 1, 255 );

	if ( glConfig.depthBoundsTestAvailable && r_useDepthBoundsTest.GetBool() ) {
		qglEnable( GL_DEPTH_BOUNDS_TEST_EXT );
	}
	GL_Cull( CT_TWO_SIDED );
	stencilShadowShader->Activate();

	std::vector<const drawSurf_t*> depthFailSurfs;
	std::vector<const drawSurf_t*> depthPassSurfs;
	for (const drawSurf_t *surf = shadowSurfs; surf; surf = surf->nextOnLight) {
		if (!surf->shadowCache.IsValid()) {
			continue;
		}

		bool external = r_useExternalShadows.GetInteger() && !(surf->dsFlags & DSF_VIEW_INSIDE_SHADOW);
		if (external) {
			depthPassSurfs.push_back( surf );
		} else {
			depthFailSurfs.push_back( surf );
		}
	}

	// draw depth-fail stencil shadows
	qglStencilOpSeparate( backEnd.viewDef->isMirror ? GL_FRONT : GL_BACK, GL_KEEP, GL_DECR_WRAP, GL_KEEP );
	qglStencilOpSeparate( backEnd.viewDef->isMirror ? GL_BACK : GL_FRONT, GL_KEEP, GL_INCR_WRAP, GL_KEEP );
	DrawSurfs( depthFailSurfs.data(), depthFailSurfs.size() );
	// draw traditional depth-pass stencil shadows
	qglStencilOpSeparate( backEnd.viewDef->isMirror ? GL_FRONT : GL_BACK, GL_KEEP, GL_KEEP, GL_INCR_WRAP );
	qglStencilOpSeparate( backEnd.viewDef->isMirror ? GL_BACK : GL_FRONT, GL_KEEP, GL_KEEP, GL_DECR_WRAP );
	DrawSurfs( depthPassSurfs.data(), depthPassSurfs.size() );

	// reset state
	GL_Cull( CT_FRONT_SIDED );
	if ( r_shadowPolygonFactor.GetFloat() || r_shadowPolygonOffset.GetFloat() ) {
		qglDisable( GL_POLYGON_OFFSET_FILL );
	}
	if ( glConfig.depthBoundsTestAvailable && r_useDepthBoundsTest.GetBool() ) {
		qglDisable( GL_DEPTH_BOUNDS_TEST_EXT );
	}
	qglStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
	// FIXME: move to interaction stage
	if ( !r_softShadowsQuality.GetBool() || backEnd.viewDef->IsLightGem() /*|| r_shadows.GetInteger()==2 && backEnd.vLight->tooBigForShadowMaps*/ )
		qglStencilFunc( GL_GEQUAL, 128, 255 ); // enable stencil test - the shadow volume path
	
}

void StencilShadowStage::DrawSurfs( const drawSurf_t **surfs, size_t count ) {
	if (count == 0) {
		return;
	}

	vertexCache.VertexPosition( surfs[0]->shadowCache, ATTRIB_SHADOW );

	ShaderParams *paramsBuffer = shaderParamsBuffer->Request<ShaderParams>( maxSupportedDrawsPerBatch );
	int paramsIdx = 0;
	drawBatches->BeginBatch( maxSupportedDrawsPerBatch );

	for (size_t i = 0; i < count; ++i) {
		const drawSurf_t *surf = surfs[i];
		ShaderParams &params = paramsBuffer[paramsIdx++];
		memcpy( params.modelViewMatrix.ToFloatPtr(), surf->space->modelViewMatrix, sizeof(idMat4) );
		R_GlobalPointToLocal( surf->space->modelMatrix, backEnd.vLight->globalLightOrigin, params.localLightOrigin.ToVec3() );
		params.localLightOrigin.w = 0.0f;
		drawBatches->AddShadowSurf( surf );

		if (paramsIdx == maxSupportedDrawsPerBatch) {
			shaderParamsBuffer->Commit( paramsBuffer, paramsIdx );
			shaderParamsBuffer->BindRange( 1, paramsBuffer, paramsIdx );
			drawBatches->DrawBatch();
			paramsBuffer = shaderParamsBuffer->Request<ShaderParams>( maxSupportedDrawsPerBatch );
			drawBatches->BeginBatch( maxSupportedDrawsPerBatch );
		}
	}

	shaderParamsBuffer->Commit( paramsBuffer, paramsIdx );
	shaderParamsBuffer->BindRange( 1, paramsBuffer, paramsIdx );
	drawBatches->DrawBatch();
}
