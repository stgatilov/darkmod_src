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

#include "StencilShadowStage.h"
#include "DrawBatchExecutor.h"
#include "../FrameBuffer.h"
#include "../FrameBufferManager.h"
#include "../GLSLProgram.h"
#include "../GLSLProgramManager.h"

struct StencilShadowStage::ShaderParams {
	idMat4 modelViewMatrix;
	idVec4 localLightOrigin;
};

StencilShadowStage::StencilShadowStage( DrawBatchExecutor *drawBatchExecutor ) {
	this->drawBatchExecutor = drawBatchExecutor;
}

void StencilShadowStage::Init() {
	idHashMapDict defines;
	defines.Set( "MAX_SHADER_PARAMS", idStr::Fmt("%d", drawBatchExecutor->MaxShaderParamsArraySize<ShaderParams>()) );
	stencilShadowShader = programManager->LoadFromFiles( "stencil_shadow", 
		"stages/stencil/stencil_shadow.vert.glsl",
		"stages/stencil/stencil_shadow.frag.glsl",
		defines );
}

void StencilShadowStage::Shutdown() {
	ShutdownMipmaps();
}

void StencilShadowStage::DrawStencilShadows( viewLight_t *vLight, const drawSurf_t *shadowSurfs ) {
	if ( !shadowSurfs || !r_shadows.GetInteger() ) {
		return;
	}
	TRACE_GL_SCOPE( "StencilShadowPass" );

	if ( r_shadowPolygonFactor.GetFloat() || r_shadowPolygonOffset.GetFloat() ) {
		qglPolygonOffset( r_shadowPolygonFactor.GetFloat(), -r_shadowPolygonOffset.GetFloat() );
		qglEnable( GL_POLYGON_OFFSET_FILL );
	}
	GL_State( GLS_DEPTHMASK | GLS_COLORMASK | GLS_ALPHAMASK | GLS_DEPTHFUNC_LESS );
	qglStencilFunc( GL_ALWAYS, 1, 255 );

	GL_Cull( CT_TWO_SIDED );
	stencilShadowShader->Activate();

	idList<const drawSurf_t*> depthFailSurfs;
	idList<const drawSurf_t*> depthPassSurfs;
	for (const drawSurf_t *surf = shadowSurfs; surf; surf = surf->nextOnLight) {
		if (!surf->shadowCache.IsValid()) {
			continue;
		}

		bool external = r_useExternalShadows.GetInteger() && !(surf->dsFlags & DSF_VIEW_INSIDE_SHADOW);
		if (external) {
			depthPassSurfs.AddGrow( surf );
		} else {
			depthFailSurfs.AddGrow( surf );
		}
	}

	// draw depth-fail stencil shadows
	qglStencilOpSeparate( backEnd.viewDef->isMirror ? GL_FRONT : GL_BACK, GL_KEEP, GL_DECR_WRAP, GL_KEEP );
	qglStencilOpSeparate( backEnd.viewDef->isMirror ? GL_BACK : GL_FRONT, GL_KEEP, GL_INCR_WRAP, GL_KEEP );
	DrawSurfs( depthFailSurfs.Ptr(), depthFailSurfs.Num() );
	// draw traditional depth-pass stencil shadows
	qglStencilOpSeparate( backEnd.viewDef->isMirror ? GL_FRONT : GL_BACK, GL_KEEP, GL_KEEP, GL_INCR_WRAP );
	qglStencilOpSeparate( backEnd.viewDef->isMirror ? GL_BACK : GL_FRONT, GL_KEEP, GL_KEEP, GL_DECR_WRAP );
	DrawSurfs( depthPassSurfs.Ptr(), depthPassSurfs.Num() );

	// reset state
	GL_Cull( CT_FRONT_SIDED );
	if ( r_shadowPolygonFactor.GetFloat() || r_shadowPolygonOffset.GetFloat() ) {
		qglDisable( GL_POLYGON_OFFSET_FILL );
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

	DrawBatch<ShaderParams> drawBatch = drawBatchExecutor->BeginBatch<ShaderParams>();
	uint paramsIdx = 0;

	if ( r_useScissor.GetBool() ) {
		backEnd.currentScissor = backEnd.vLight->scissorRect;
		FB_ApplyScissor();
	}
	DepthBoundsTest depthBoundsTest( backEnd.vLight->scissorRect );

	const drawSurf_t *curBatchCaches = surfs[0];
	for (size_t i = 0; i < count; ++i) {
		const drawSurf_t *surf = surfs[i];

		if (paramsIdx == drawBatch.maxBatchSize
				|| (r_useScissor.GetBool() && !backEnd.currentScissor.Equals(surf->scissorRect))
				|| surf->shadowCache.isStatic != curBatchCaches->shadowCache.isStatic
				|| surf->indexCache.isStatic != curBatchCaches->indexCache.isStatic ) {
			drawBatchExecutor->ExecuteShadowVertBatch( paramsIdx );
			drawBatch = drawBatchExecutor->BeginBatch<ShaderParams>();
			paramsIdx = 0;
		}

		if (r_useScissor.GetBool() && !backEnd.currentScissor.Equals(surf->scissorRect)) {
			backEnd.currentScissor = surf->scissorRect;
			FB_ApplyScissor();
		}
		// stgatilov: this is preferable to take effect on this surface
		// but it's too messy to do it properly with batching
		//depthBoundsTest.Update( surf->scissorRect );

		ShaderParams &params = drawBatch.shaderParams[paramsIdx];
		memcpy( params.modelViewMatrix.ToFloatPtr(), surf->space->modelViewMatrix, sizeof(idMat4) );
		R_GlobalPointToLocal( surf->space->modelMatrix, backEnd.vLight->globalLightOrigin, params.localLightOrigin.ToVec3() );
		params.localLightOrigin.w = 0.0f;
		drawBatch.surfs[paramsIdx] = surf;
		++paramsIdx;
		curBatchCaches = surf;
	}

	drawBatchExecutor->ExecuteShadowVertBatch( paramsIdx );
}

//=======================================================================================

idCVar r_softShadowsMipmaps(
	"r_softShadowsMipmaps", "1", CVAR_BOOL | CVAR_RENDERER | CVAR_ARCHIVE,
	"Use mipmap tiles to avoid sampling far away from penumbra"
);

#define max(x, y) idMath::Fmax(x, y)
#include "../../glprogs/tdm_shadowstencilsoft_shared.glsl"
#undef max

// if any of these properties change, then we need to recreate mipmaps
struct MipmapsInitProps {
	int renderWidth = -1;
	int renderHeight = -1;
	int softShadowQuality = -1;

	bool operator== (const MipmapsInitProps &p) const {
		return memcmp(this, &p, sizeof(p)) == 0;
	}
};
static MipmapsInitProps currentMipmapProps;

void StencilShadowStage::ShutdownMipmaps() {
	stencilShadowMipmap.Shutdown();
	currentMipmapProps = MipmapsInitProps();
}

void StencilShadowStage::FillStencilShadowMipmaps( const idScreenRect &lightScissor ) {
	if ( backEnd.viewDef->IsLightGem() )
		return;

	MipmapsInitProps newProps = {
		frameBuffers->shadowStencilFbo->Width(),
		frameBuffers->shadowStencilFbo->Height(),
		r_softShadowsMipmaps.GetBool() ? r_softShadowsQuality.GetInteger() : 0
	};
	if ( !(newProps == currentMipmapProps) ) {
		// some important property has changed: everything should be recreated
		stencilShadowMipmap.Shutdown();

		float maxBlurAxisLength = computeMaxBlurAxisLength( newProps.renderHeight, r_softShadowsQuality.GetInteger() );
		int lodLevel = int(ceil(log2(maxBlurAxisLength * 2)));
		static const int BASE_LEVEL = 2;
		lodLevel = idMath::Imax(lodLevel, BASE_LEVEL);

		if ( newProps.softShadowQuality > 0 ) {
			stencilShadowMipmap.Init(
				TiledCustomMipmapStage::MM_STENCIL_SHADOW, GL_R8,
				newProps.renderWidth,
				newProps.renderHeight,
				lodLevel, BASE_LEVEL
			);
		}
		currentMipmapProps = newProps;
	}

	if ( newProps.softShadowQuality > 0 ) {
		int x = lightScissor.x1 * newProps.renderWidth  / glConfig.vidWidth ;
		int y = lightScissor.y1 * newProps.renderHeight / glConfig.vidHeight;
		int w = lightScissor.GetWidth()  * newProps.renderWidth  / glConfig.vidWidth ;
		int h = lightScissor.GetHeight() * newProps.renderHeight / glConfig.vidHeight;

		stencilShadowMipmap.FillFrom( globalImages->shadowDepthFbo, x, y, w, h );
	}
}
