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
#include "ShadowMapStage.h"
#include "RenderBackend.h"
#include "../FrameBuffer.h"
#include "../FrameBufferManager.h"
#include "../glsl.h"
#include "../GLSLProgramManager.h"

namespace {
	struct ShadowMapUniforms : GLSLUniformGroup {
		UNIFORM_GROUP_DEF( ShadowMapUniforms )
		DEFINE_UNIFORM( vec4, lightOrigin )
	};
}

struct ShadowMapStage::ShaderParams {
	idMat4 modelMatrix;
};

ShadowMapStage::ShadowMapStage( DrawBatchExecutor *drawBatchExecutor ) : drawBatchExecutor( drawBatchExecutor ) {
}

void ShadowMapStage::Init() {
	const uint maxShaderParamsArraySize = drawBatchExecutor->MaxShaderParamsArraySize<ShaderParams>();
	idHashMapDict defines;
	defines.Set( "MAX_SHADER_PARAMS", idStr::Fmt( "%d", maxShaderParamsArraySize ) );
	shadowMapShader = programManager->LoadFromFiles( "shadow_map", "stages/shadow_map/shadow_map.vert.glsl", defines );
}

void ShadowMapStage::Shutdown() {}

void ShadowMapStage::DrawShadowMap( const viewDef_t *viewDef ) {
	TRACE_GL_SCOPE( "RenderShadowMap" );

	if ( glConfig.vendor == glvIntel ) {
		// for some reason, Intel has massive performance problems with this path, cause as of yet is unknown
		// we'll just use the existing shadow mapping code, but applied consecutively
		FallbackPathForIntel( viewDef );
		return;
	}

	GL_CheckErrors();
	frameBuffers->EnterShadowMap();

	shadowMapShader->Activate();
	GL_SelectTexture( 0 );
	GL_Cull( r_shadowMapCullFront ? CT_BACK_SIDED : CT_TWO_SIDED );
	qglPolygonOffset( 0, 0 );
	qglEnable( GL_POLYGON_OFFSET_FILL );
	for ( int i = 0; i < 4; i++ ) {
		qglEnable( GL_CLIP_PLANE0 + i );
	}

	ShadowMapUniforms *shadowMapUniforms = shadowMapShader->GetUniformGroup<ShadowMapUniforms>();

	for ( viewLight_t *vLight = viewDef->viewLights; vLight; vLight = vLight->next ) {
		if ( vLight->noShadows || vLight->shadows != LS_MAPS || vLight->shadowMapIndex > 42 ) {
			continue;
		}
		idVec4 lightOrigin;
		lightOrigin.x = vLight->globalLightOrigin.x;
		lightOrigin.y = vLight->globalLightOrigin.y;
		lightOrigin.z = vLight->globalLightOrigin.z;
		lightOrigin.w = 0;
		shadowMapUniforms->lightOrigin.Set( lightOrigin );

		auto &page = ShadowAtlasPages[vLight->shadowMapIndex-1];
		qglViewport( page.x, page.y, 6*page.width, page.width );
		if ( r_useScissor.GetBool() ) {
			qglScissor( page.x, page.y, 6*page.width, page.width );
		}
		qglClear( GL_DEPTH_BUFFER_BIT );
		BeginDrawBatch();
		DrawLightInteractions( vLight->globalInteractions );
		DrawLightInteractions( vLight->localInteractions );
	}

	for ( int i = 0; i < 4; i++ ) {
		qglDisable( GL_CLIP_PLANE0 + i );
	}

	qglDisable( GL_POLYGON_OFFSET_FILL );
	GL_Cull( CT_FRONT_SIDED );

	frameBuffers->LeaveShadowMap();
}

float ShadowMapStage::GetEffectiveLightRadius( viewLight_t *vLight ) {
	return ::GetEffectiveLightRadius();
}

bool ShadowMapStage::ShouldDrawSurf( const drawSurf_t *surf ) const {
    const idMaterial *shader = surf->material;

    if ( /*!shader->IsDrawn() || !shader->SurfaceCastsShadow() ||*/ (surf->dsFlags & DSF_SHADOW_MAP_IGNORE) ) {
        return false;
    }

    // some deforms may disable themselves by setting numIndexes = 0
    if ( !surf->numIndexes ) {
        return false;
    }

	// translucent surfaces are not supported for shadows
    if ( shader->Coverage() == MC_TRANSLUCENT ) {
        //return false;
    }

    if ( !surf->ambientCache.IsValid() || !surf->indexCache.IsValid() ) {
        #ifdef _DEBUG
        common->Printf( "ShadowMapStage: missing vertex or index cache\n" );
        #endif
        return false;
    }

    // get the expressions for conditionals / color / texcoords
    const float *regs = surf->shaderRegisters;

    // if all stages of a material have been conditioned off, don't do anything
    /*int stage;
    for ( stage = 0; stage < shader->GetNumStages() ; stage++ ) {
        const shaderStage_t *pStage = shader->GetStage( stage );
        // check the stage enable condition
        if ( regs[ pStage->conditionRegister ] != 0 ) {
            break;
        }
    }
    return stage != shader->GetNumStages();*/
	return true;
}

void ShadowMapStage::DrawLightInteractions( const drawSurf_t *surfs ) {
	const drawSurf_t *curBatchCaches = surfs;
	for ( const drawSurf_t *surf = surfs; surf; surf = surf->nextOnLight ) {
		if ( !ShouldDrawSurf( surf ) ) {
			continue;
		}
		if ( surf->ambientCache.isStatic != curBatchCaches->ambientCache.isStatic || surf->indexCache.isStatic != curBatchCaches->indexCache.isStatic ) {
			ExecuteDrawCalls();
		}
		curBatchCaches = surf;
		DrawSurf( surf );
	}
	ExecuteDrawCalls();
}

void ShadowMapStage::DrawSurf( const drawSurf_t *surf ) {
	/*if ( surf->space->weaponDepthHack ) {
		// this is a state change, need to finish any previous calls
		ExecuteDrawCalls();
		RB_EnterWeaponDepthHack();
	}*/

	const idMaterial *shader = surf->material;

	/*if ( shader->TestMaterialFlag( MF_POLYGONOFFSET ) ) {
		// this is a state change, need to finish any previous calls
		ExecuteDrawCalls();
		qglEnable( GL_POLYGON_OFFSET_FILL );
		qglPolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * shader->GetPolygonOffset() );
	}*/

	CreateDrawCommands( surf );

	// reset polygon offset
	/*if ( shader->TestMaterialFlag( MF_POLYGONOFFSET ) ) {
		ExecuteDrawCalls();
		qglDisable( GL_POLYGON_OFFSET_FILL );
	}*/

/*	if ( surf->space->weaponDepthHack ) {
		ExecuteDrawCalls();
		RB_LeaveDepthHack();
	}*/
}

void ShadowMapStage::CreateDrawCommands( const drawSurf_t *surf ) {
	const idMaterial		*shader = surf->material;
	const float				*regs = surf->shaderRegisters;

	bool drawSolid = false;

	if ( shader->Coverage() == MC_OPAQUE ) {
		drawSolid = true;
	}

	drawSolid = true;
	// we may have multiple alpha tested stages
	/*if ( shader->Coverage() == MC_PERFORATED ) {
		// if the only alpha tested stages are condition register omitted,
		// draw a normal opaque surface
		bool	didDraw = false;

		GL_CheckErrors();

		// perforated surfaces may have multiple alpha tested stages
		for ( int stage = 0; stage < shader->GetNumStages(); stage++ ) {
			const shaderStage_t *pStage = shader->GetStage( stage );

			if ( !pStage->hasAlphaTest ) {
				continue;
			}

			// check the stage enable condition
			if ( regs[pStage->conditionRegister] == 0 ) {
				continue;
			}

			// if we at least tried to draw an alpha tested stage,
			// we won't draw the opaque surface
			didDraw = true;

			// skip the entire stage if alpha would be black
			if ( regs[pStage->color.registers[3]] <= 0 ) {
				continue;
			}
			IssueDrawCommand( surf, pStage );
		}

		if ( !didDraw ) {
			drawSolid = true;
		}
	}*/

	if ( drawSolid ) {  // draw the entire surface solid
		IssueDrawCommand( surf, nullptr );
	}
}

void ShadowMapStage::IssueDrawCommand( const drawSurf_t *surf, const shaderStage_t *stage ) {
	if( stage && !renderBackend->ShouldUseBindlessTextures() && !stage->texture.image->IsBound( 0 ) ) {
		ExecuteDrawCalls();
		stage->texture.image->Bind();
	}

	ShaderParams &params = drawBatch.shaderParams[currentIndex];

	memcpy( params.modelMatrix.ToFloatPtr(), surf->space->modelMatrix, sizeof(idMat4) );

	if( stage ) {
		// set the alpha modulate
		//params.alphaTest = surf->shaderRegisters[stage->alphaTestRegister];

		//if( renderBackend->ShouldUseBindlessTextures() ) {
		//	stage->texture.image->MakeResident();
		//	params.textureHandle = stage->texture.image->BindlessHandle();
		//}

		//if( stage->texture.hasMatrix ) {
		//	RB_GetShaderTextureMatrix( surf->shaderRegisters, &stage->texture, params.textureMatrix.ToFloatPtr() );
		//} else {
		//	params.textureMatrix.Identity();
		//}
	}

	drawBatch.surfs[currentIndex] = surf;
	++currentIndex;
	if ( currentIndex == drawBatch.maxBatchSize ) {
		ExecuteDrawCalls();
	}
}

void ShadowMapStage::BeginDrawBatch() {
	currentIndex = 0;
	drawBatch = drawBatchExecutor->BeginBatch<ShaderParams>();
}

void ShadowMapStage::ExecuteDrawCalls() {
	if (currentIndex == 0) {
		return;
	}

	drawBatchExecutor->ExecuteDrawVertBatch(currentIndex, 6);
	BeginDrawBatch();
}

void ShadowMapStage::FallbackPathForIntel( const viewDef_t *viewDef ) {
	extern void RB_GLSL_DrawInteractions_ShadowMap( const drawSurf_t *surf, bool clear );

	for ( viewLight_t *vLight = viewDef->viewLights; vLight; vLight = vLight->next ) {
		backEnd.vLight = vLight;
		RB_GLSL_DrawInteractions_ShadowMap( vLight->globalInteractions, true );
		RB_GLSL_DrawInteractions_ShadowMap( vLight->localInteractions, false );
	}
}
