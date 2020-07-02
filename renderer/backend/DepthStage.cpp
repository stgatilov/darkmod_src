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

#include "../tr_local.h"
#include "DepthStage.h"
#include "RenderBackend.h"
#include "ShaderParamsBuffer.h"
#include "../FrameBuffer.h"
#include "../Profiling.h"
#include "../glsl.h"
#include "../FrameBufferManager.h"
#include "../GLSLProgramManager.h"


namespace {
	struct DepthUniforms : GLSLUniformGroup {
		UNIFORM_GROUP_DEF( DepthUniforms )

		DEFINE_UNIFORM( vec4, clipPlane )
		DEFINE_UNIFORM( mat4, inverseView )
		DEFINE_UNIFORM( sampler, texture )
	};

	void LoadShader( GLSLProgram *shader, int maxSupportedDrawsPerBatch, bool bindless ) {
		idDict defines;
		defines.Set( "MAX_SHADER_PARAMS", idStr::Fmt( "%d", maxSupportedDrawsPerBatch ) );
		if (bindless) {
			defines.Set( "BINDLESS_TEXTURES", "1" );
		}
		shader->InitFromFiles( "stages/depth/depth.vert.glsl", "stages/depth/depth.frag.glsl", defines );
		if (!bindless) {
			DepthUniforms *uniforms = shader->GetUniformGroup<DepthUniforms>();
			uniforms->texture.Set( 0 );
		}
		shader->BindUniformBlockLocation( 0, "ViewParamsBlock" );
		shader->BindUniformBlockLocation( 1, "ShaderParamsBlock" );
	}

	idVec4 CalcScissorParam( const idScreenRect &screenRect ) {
		float xScale = static_cast<float>(frameBuffers->activeFbo->Width()) / glConfig.vidWidth;
		float yScale = static_cast<float>(frameBuffers->activeFbo->Height()) / glConfig.vidHeight;

		idVec4 r;
		r.x = xScale * (backEnd.viewDef->viewport.x1 + screenRect.x1);
		r.y = yScale * (backEnd.viewDef->viewport.y1 + screenRect.y1);
		r.z = xScale * (backEnd.viewDef->viewport.x1 + screenRect.x2);
		r.w = yScale * (backEnd.viewDef->viewport.y1 + screenRect.y2);
		return r;
	}
}

struct DepthStage::ShaderParams {
	idMat4 modelViewMatrix;
	idMat4 textureMatrix;
	idVec4 color;
	idVec4 scissor;
	idVec2 alphaTest;
	uint64_t textureHandle;
};

DepthStage::DepthStage(ShaderParamsBuffer* shaderParamsBuffer, DrawBatchExecutor* drawBatchExecutor)
	: shaderParamsBuffer(shaderParamsBuffer), drawBatchExecutor(drawBatchExecutor)
{
}

void DepthStage::Init() {
	maxSupportedDrawsPerBatch = shaderParamsBuffer->MaxSupportedParamBufferSize<ShaderParams>();
	depthShader = programManager->LoadFromGenerator( "depth_pass", [=](GLSLProgram *program) { LoadShader(program, maxSupportedDrawsPerBatch, false); } );

	if( GLAD_GL_ARB_bindless_texture ) {
		depthShaderBindless = programManager->LoadFromGenerator( "depth_bindless", [=](GLSLProgram *program) { LoadShader(program, maxSupportedDrawsPerBatch, true); } );
	}
}

void DepthStage::Shutdown() {}

void DepthStage::DrawDepth( const viewDef_t *viewDef, drawSurf_t **drawSurfs, int numDrawSurfs ) {
	GL_PROFILE( "DepthStage" );

	GLSLProgram *shader = renderBackend->ShouldUseBindlessTextures() ? depthShaderBindless : depthShader;
	shader->Activate();
	DepthUniforms *depthUniforms = shader->GetUniformGroup<DepthUniforms>();

	idMat4 inverseView;
	memcpy( inverseView.ToFloatPtr(), viewDef->worldSpace.modelViewMatrix, sizeof( inverseView ) );
	inverseView.InverseSelf();
	depthUniforms->inverseView.Set( inverseView );

	// pass mirror clip plane details to vertex shader if needed
	if ( viewDef->clipPlane) {
		depthUniforms->clipPlane.Set( *viewDef->clipPlane );
	} else {
		depthUniforms->clipPlane.Set( colorBlack );
	}

	// the first texture will be used for alpha tested surfaces
	GL_SelectTexture( 0 );
	depthUniforms->texture.Set( 0 );

	// decal surfaces may enable polygon offset
	qglPolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() );

	GL_State( GLS_DEPTHFUNC_LESS );

	// Enable stencil test if we are going to be using it for shadows.
	// If we didn't do this, it would be legal behavior to get z fighting
	// from the ambient pass and the light passes.
	qglEnable( GL_STENCIL_TEST );
	qglStencilFunc( GL_ALWAYS, 1, 255 );

	vertexCache.BindVertex();

	ResetShaderParams();

	bool subViewEnabled = false;
	for ( int i = 0; i < numDrawSurfs; ++i ) {
		const drawSurf_t *drawSurf = drawSurfs[i];
		if ( !ShouldDrawSurf( drawSurf ) ) {
			continue;
		}

		bool isSubView = drawSurf->material->GetSort() == SS_SUBVIEW;
		if( isSubView != subViewEnabled ) {
			ExecuteDrawCalls();
			if( isSubView ) {
				GL_State( GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO | GLS_DEPTHFUNC_LESS );
			} else {
				GL_State( GLS_DEPTHFUNC_LESS );
			}
			subViewEnabled = isSubView;
		}

		DrawSurf( drawSurf );
	}

	ExecuteDrawCalls();

	// Make the early depth pass available to shaders. #3877
	if ( !viewDef->IsLightGem() && !r_skipDepthCapture.GetBool() ) {
		frameBuffers->UpdateCurrentDepthCopy();
	}
}

bool DepthStage::ShouldDrawSurf(const drawSurf_t *surf) const {
    const idMaterial *shader = surf->material;

    if ( !shader->IsDrawn() ) {
        return false;
    }

    // some deforms may disable themselves by setting numIndexes = 0
    if ( !surf->numIndexes ) {
        return false;
    }

    // translucent surfaces don't put anything in the depth buffer and don't
    // test against it, which makes them fail the mirror clip plane operation
    if ( shader->Coverage() == MC_TRANSLUCENT ) {
        return false;
    }

    if ( !surf->ambientCache.IsValid() || !surf->indexCache.IsValid() ) {
        common->Printf( "DepthStage: missing vertex or index cache\n" );
        return false;
    }

    if ( surf->material->GetSort() == SS_PORTAL_SKY && g_enablePortalSky.GetInteger() == 2 ) {
        return false;
    }

    // get the expressions for conditionals / color / texcoords
    const float *regs = surf->shaderRegisters;

    // if all stages of a material have been conditioned off, don't do anything
    int stage;
    for ( stage = 0; stage < shader->GetNumStages() ; stage++ ) {
        const shaderStage_t *pStage = shader->GetStage( stage );
        // check the stage enable condition
        if ( regs[ pStage->conditionRegister ] != 0 ) {
            break;
        }
    }
    return stage != shader->GetNumStages();
}

void DepthStage::DrawSurf( const drawSurf_t *surf ) {
	/* TODO
	if ( drawSurf->space->weaponDepthHack ) {
		RB_EnterWeaponDepthHack();
		GL_CheckErrors();
	}

	if ( r_useScissor.GetBool() && !backEnd.currentScissor.Equals( drawSurf->scissorRect ) ) {
		backEnd.currentScissor = drawSurf->scissorRect;
		FB_ApplyScissor();
		GL_CheckErrors();
	}*/

	// render it

	const idMaterial *shader = surf->material;

	/*// set polygon offset if necessary
	if ( shader->TestMaterialFlag( MF_POLYGONOFFSET ) ) {
		qglEnable( GL_POLYGON_OFFSET_FILL );
		qglPolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * shader->GetPolygonOffset() );
	}*/

	CreateDrawCommands( surf );

	// reset polygon offset
	/*if ( shader->TestMaterialFlag( MF_POLYGONOFFSET ) ) {
		qglDisable( GL_POLYGON_OFFSET_FILL );
	}*/

	/*if ( drawSurf->space->weaponDepthHack || drawSurf->space->modelDepthHack != 0.0f ) {
		RB_LeaveDepthHack();
		GL_CheckErrors();
	}*/
}

void DepthStage::CreateDrawCommands( const drawSurf_t *surf ) {
	const idMaterial		*shader = surf->material;
	const float				*regs = surf->shaderRegisters;

	bool drawSolid = false;

	if ( shader->Coverage() == MC_OPAQUE ) {
		drawSolid = true;
	}

	// we may have multiple alpha tested stages
	if ( shader->Coverage() == MC_PERFORATED ) {
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
	}

	if ( drawSolid ) {  // draw the entire surface solid
		IssueDrawCommand( surf, nullptr );
	}
}

void DepthStage::IssueDrawCommand( const drawSurf_t *surf, const shaderStage_t *stage ) {
	if( stage && !renderBackend->ShouldUseBindlessTextures() && stage->texture.image->texnum != backEnd.glState.tmu[0].current2DMap ) {
		ExecuteDrawCalls();
		stage->texture.image->Bind();
	}

	ShaderParams &params = shaderParams[currentIndex++];

	memcpy( params.modelViewMatrix.ToFloatPtr(), surf->space->modelViewMatrix, sizeof(idMat4) );
	params.scissor = CalcScissorParam( surf->scissorRect );
	params.alphaTest.x = -1.f;

	if ( surf->material->GetSort() == SS_SUBVIEW ) {
		// subviews will just down-modulate the color buffer by overbright
		params.color[0] = params.color[1] = params.color[2] = 1.0f / backEnd.overBright;
		params.color[3] = 1;
	} else {
		// others just draw black
		params.color = idVec4(0, 0, 0, 1);
	}

	if( stage ) {
		// set the alpha modulate
		params.color[3] = surf->shaderRegisters[stage->color.registers[3]];
		params.alphaTest.x = surf->shaderRegisters[stage->alphaTestRegister];

		if( renderBackend->ShouldUseBindlessTextures() ) {
			stage->texture.image->MakeResident();
			params.textureHandle = stage->texture.image->BindlessHandle();
		}

		if( stage->texture.hasMatrix ) {
			RB_GetShaderTextureMatrix( surf->shaderRegisters, &stage->texture, params.textureMatrix.ToFloatPtr() );
		} else {
			params.textureMatrix.Identity();
		}
	}

	drawBatchExecutor->AddDrawVertSurf( surf );
	if ( currentIndex == maxSupportedDrawsPerBatch ) {
		ExecuteDrawCalls();
	}
}

void DepthStage::ResetShaderParams() {
	currentIndex = 0;
	shaderParams = shaderParamsBuffer->Request<ShaderParams>( maxSupportedDrawsPerBatch );
	drawBatchExecutor->BeginBatch( maxSupportedDrawsPerBatch );
}

void DepthStage::ExecuteDrawCalls() {
	int totalDrawCalls = currentIndex;
	if (totalDrawCalls == 0) {
		return;
	}

	shaderParamsBuffer->Commit( shaderParams, totalDrawCalls );
	shaderParamsBuffer->BindRange( 1, shaderParams, totalDrawCalls );
	drawBatchExecutor->DrawBatch();

	ResetShaderParams();
}
