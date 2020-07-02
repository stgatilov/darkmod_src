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

#include "InteractionStage.h"

#include "RenderBackend.h"
#include "../Profiling.h"
#include "../glsl.h"
#include "../GLSLProgramManager.h"
#include "../FrameBuffer.h"
#include "ShaderParamsBuffer.h"
#include "../AmbientOcclusionStage.h"
#include "DrawBatchExecutor.h"

// NOTE: must match struct in shader, beware of std140 layout requirements and alignment!
struct InteractionStage::ShaderParams {
	idMat4 modelMatrix;
	idMat4 modelViewMatrix;
	idVec4 bumpMatrix[2];
	idVec4 diffuseMatrix[2];
	idVec4 specularMatrix[2];
	idMat4 lightProjectionFalloff;
	idVec4 colorModulate;
	idVec4 colorAdd;
	idVec4 lightOrigin;
	idVec4 viewOrigin;
	idVec4 diffuseColor;
	idVec4 specularColor;
	idVec4 hasTextureDNS;
	idVec4 ambientRimColor;
	// bindless texture handles, if supported
	uint64_t normalTexture;
	uint64_t diffuseTexture;
	uint64_t specularTexture;
	uint64_t padding;
};

namespace {
	struct InteractionUniforms: GLSLUniformGroup {
		UNIFORM_GROUP_DEF( InteractionUniforms )

		DEFINE_UNIFORM( sampler, normalTexture )
		DEFINE_UNIFORM( sampler, diffuseTexture )
		DEFINE_UNIFORM( sampler, specularTexture )
		DEFINE_UNIFORM( sampler, lightProjectionTexture )
		DEFINE_UNIFORM( sampler, lightProjectionCubemap )
		DEFINE_UNIFORM( sampler, lightFalloffTexture )
		DEFINE_UNIFORM( sampler, lightFalloffCubemap )
		DEFINE_UNIFORM( sampler, ssaoTexture )

		DEFINE_UNIFORM( int, advanced )
		DEFINE_UNIFORM( int, testSpecularFix )
		DEFINE_UNIFORM( int, testBumpmapLightTogglingFix )
		DEFINE_UNIFORM( int, cubic )
		DEFINE_UNIFORM( float, gamma )
		DEFINE_UNIFORM( float, minLevel )
		DEFINE_UNIFORM( int, ssaoEnabled )
	};

	enum TextureUnits {
		TU_LIGHT_PROJECT = 2,
		TU_LIGHT_FALLOFF = 1,
		TU_NORMAL = 0,
		TU_DIFFUSE = 3,
		TU_SPECULAR = 4,
		TU_SSAO = 6,
		TU_SHADOW_MAP = 6,
		TU_SHADOW_DEPTH = 6,
		TU_SHADOW_STENCIL = 7,
	};
}

void InteractionStage::LoadInteractionShader( GLSLProgram *shader, const idStr &baseName, bool bindless ) {
	shader->Init();
	idDict defines;
	defines.Set( "MAX_SHADER_PARAMS", idStr::Fmt( "%d", maxSupportedDrawsPerBatch ) );
	if (bindless) {
		defines.Set( "BINDLESS_TEXTURES", "1" );
	}
	shader->AttachVertexShader("stages/interaction/" + baseName + ".vs.glsl", defines);
	shader->AttachFragmentShader("stages/interaction/" + baseName + ".fs.glsl", defines);
	Attributes::Default::Bind(shader);
	shader->Link();
	shader->Activate();
	InteractionUniforms *uniforms = shader->GetUniformGroup<InteractionUniforms>();
	uniforms->lightProjectionCubemap.Set( TU_LIGHT_PROJECT );
	uniforms->lightProjectionTexture.Set( TU_LIGHT_PROJECT );
	uniforms->lightFalloffCubemap.Set( TU_LIGHT_FALLOFF );
	uniforms->lightFalloffTexture.Set( TU_LIGHT_FALLOFF );
	uniforms->ssaoTexture.Set( TU_SSAO );
	if (!bindless) {
		uniforms->normalTexture.Set( TU_NORMAL );
		uniforms->diffuseTexture.Set( TU_DIFFUSE );
		uniforms->specularTexture.Set( TU_SPECULAR );
	}
	shader->Deactivate();
}


InteractionStage::InteractionStage( ShaderParamsBuffer *shaderParamsBuffer, DrawBatchExecutor *drawBatchExecutor )
	: shaderParamsBuffer( shaderParamsBuffer ), drawBatches( drawBatchExecutor ), interactionShader( nullptr )
{}

void InteractionStage::Init() {
	maxSupportedDrawsPerBatch = shaderParamsBuffer->MaxSupportedParamBufferSize<ShaderParams>();
	
	ambientInteractionShader = programManager->LoadFromGenerator( "interaction_ambient", 
		[this](GLSLProgram *shader) { LoadInteractionShader( shader, "interaction.ambient", false ); } );
	stencilInteractionShader = programManager->LoadFromGenerator( "interaction_stencil", 
		[this](GLSLProgram *shader) { LoadInteractionShader( shader, "interaction.stencil", false ); } );
	if (GLAD_GL_ARB_bindless_texture) {
		bindlessAmbientInteractionShader = programManager->LoadFromGenerator( "interaction_ambient_bindless", 
			[this](GLSLProgram *shader) { LoadInteractionShader( shader, "interaction.ambient", true ); } );
		bindlessStencilInteractionShader = programManager->LoadFromGenerator( "interaction_stencil_bindless", 
			[this](GLSLProgram *shader) { LoadInteractionShader( shader, "interaction.stencil", true ); } );
	}
}

void InteractionStage::Shutdown() {}

void InteractionStage::DrawInteractions( viewLight_t *vLight, const drawSurf_t *interactionSurfs ) {
	if ( !interactionSurfs ) {
		return;
	}
	if ( vLight->lightShader->IsAmbientLight() ) {
		if ( r_skipAmbient.GetInteger() & 2 )
			return;
	} else if ( r_skipInteractions.GetBool() ) {
		return;
	}

	GL_PROFILE( "DrawInteractions" );

	// if using float buffers, alpha values are not clamped and can stack up quite high, since most interactions add 1 to its value
	// this in turn causes issues with some shader stage materials that use DST_ALPHA blending.
	// masking the alpha channel for interactions seems to fix those issues, but only do it for float buffers in case it has
	// unwanted side effects
	int alphaMask = r_fboColorBits.GetInteger() == 64 ? GLS_ALPHAMASK : 0;

	// perform setup here that will be constant for all interactions
	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | alphaMask | backEnd.depthFunc );
	backEnd.currentSpace = NULL; // ambient/interaction shaders conflict

	if ( r_useScissor.GetBool() && !backEnd.currentScissor.Equals( vLight->scissorRect ) ) {
		backEnd.currentScissor = vLight->scissorRect;
		GL_ScissorVidSize( backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
		            backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
		            backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
		            backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1 );
		GL_CheckErrors();
	}

	// bind the vertex and fragment program
	ChooseInteractionProgram( vLight );
	InteractionUniforms *uniforms = interactionShader->GetUniformGroup<InteractionUniforms>();
	uniforms->cubic.Set( vLight->lightShader->IsCubicLight() ? 1 : 0 );
	//interactionUniforms->SetForShadows( interactionSurfs == vLight->translucentInteractions );

	if( backEnd.vLight->lightShader->IsAmbientLight() && ambientOcclusion->ShouldEnableForCurrentView() ) {
		ambientOcclusion->BindSSAOTexture( 6 );
	}

	std::vector<const drawSurf_t*> drawSurfs;
	for ( const drawSurf_t *surf = interactionSurfs; surf; surf = surf->nextOnLight) {
		drawSurfs.push_back( surf );
	}
	std::sort( drawSurfs.begin(), drawSurfs.end(), [](const drawSurf_t *a, const drawSurf_t *b) {
		return a->material < b->material;
	} );

	GL_SelectTexture( TU_LIGHT_FALLOFF );
	vLight->falloffImage->Bind();

	if ( r_softShadowsQuality.GetBool() && !backEnd.viewDef->IsLightGem() || vLight->shadows == LS_MAPS )
		BindShadowTexture();

	const idMaterial	*lightShader = vLight->lightShader;
	const float			*lightRegs = vLight->shaderRegisters;
	for ( int lightStageNum = 0; lightStageNum < lightShader->GetNumStages(); lightStageNum++ ) {
		const shaderStage_t	*lightStage = lightShader->GetStage( lightStageNum );

		// ignore stages that fail the condition
		if ( !lightRegs[lightStage->conditionRegister] ) {
			continue;
		}

		GL_SelectTexture( TU_LIGHT_PROJECT );
		lightStage->texture.image->Bind();
		// careful - making bindless textures resident could bind an arbitrary texture to the currently active
		// slot, so reset this to something that is safe to override in bindless mode!
		GL_SelectTexture(TU_NORMAL);

		ResetShaderParams();
		for ( const drawSurf_t *surf : drawSurfs ) {
			if ( surf->dsFlags & DSF_SHADOW_MAP_ONLY ) {
				continue;
			}
			if ( !surf->ambientCache.IsValid() ) {
				common->Warning( "Found invalid ambientCache!" );
				continue;
			}

			// set the vertex pointers
			vertexCache.VertexPosition( surf->ambientCache );

			ProcessSingleSurface( vLight, lightStage, surf );
		}
		ExecuteDrawCalls();
	}

	GL_SelectTexture( 0 );

	GLSLProgram::Deactivate();
}

void InteractionStage::BindShadowTexture() {
	if ( backEnd.vLight->shadowMapIndex ) {
		GL_SelectTexture( TU_SHADOW_MAP );
		globalImages->shadowAtlas->Bind();
	} else {
		GL_SelectTexture( TU_SHADOW_DEPTH );
		globalImages->currentDepthImage->Bind();

		GL_SelectTexture( TU_SHADOW_STENCIL );
		globalImages->shadowDepthFbo->Bind();
		qglTexParameteri( GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_STENCIL_INDEX );
	}
}

void InteractionStage::ChooseInteractionProgram( viewLight_t *vLight ) {
	if ( vLight->lightShader->IsAmbientLight() ) {
		interactionShader = renderBackend->ShouldUseBindlessTextures() ? bindlessAmbientInteractionShader : ambientInteractionShader;
		Uniforms::Interaction *uniforms = interactionShader->GetUniformGroup<Uniforms::Interaction>();
		uniforms->ambient = true;
	} else if ( vLight->shadowMapIndex ) {
		// FIXME: port shadowmap shader
		interactionShader = renderBackend->ShouldUseBindlessTextures() ? bindlessStencilInteractionShader : stencilInteractionShader;
	} else {
		interactionShader = renderBackend->ShouldUseBindlessTextures() ? bindlessStencilInteractionShader : stencilInteractionShader;
	}
	interactionShader->Activate();

	InteractionUniforms *uniforms = interactionShader->GetUniformGroup<InteractionUniforms>();
	uniforms->advanced.Set( r_interactionProgram.GetInteger() );
	uniforms->gamma.Set( r_ambientGamma.GetFloat() );
	uniforms->minLevel.Set( r_ambientMinLevel.GetFloat() );
	uniforms->testSpecularFix.Set( 1 );
	uniforms->testBumpmapLightTogglingFix.Set( 0 );
	uniforms->ssaoEnabled.Set( ambientOcclusion->ShouldEnableForCurrentView() ? 1 : 0 );
}

void InteractionStage::ProcessSingleSurface( viewLight_t *vLight, const shaderStage_t *lightStage, const drawSurf_t *surf ) {
	const idMaterial	*material = surf->material;
	const float			*surfaceRegs = surf->shaderRegisters;
	const idMaterial	*lightShader = vLight->lightShader;
	const float			*lightRegs = vLight->shaderRegisters;
	drawInteraction_t	inter;

	if ( !surf->ambientCache.IsValid() ) {
		return;
	}

	if ( lightShader->IsAmbientLight() ) {
		inter.worldUpLocal.x = surf->space->modelMatrix[2];
		inter.worldUpLocal.y = surf->space->modelMatrix[6];
		inter.worldUpLocal.z = surf->space->modelMatrix[10];
		auto ambientRegs = material->GetAmbientRimColor().registers;
		if ( ambientRegs[0] ) {
			for ( int i = 0; i < 3; i++ )
				inter.ambientRimColor[i] = surfaceRegs[ambientRegs[i]];
			inter.ambientRimColor[3] = 1;
		} else
			inter.ambientRimColor.Zero();
	}

	inter.surf = surf;

	R_GlobalPointToLocal( surf->space->modelMatrix, vLight->globalLightOrigin, inter.localLightOrigin.ToVec3() );
	R_GlobalPointToLocal( surf->space->modelMatrix, backEnd.viewDef->renderView.vieworg, inter.localViewOrigin.ToVec3() );
	inter.localLightOrigin[3] = 0;
	inter.localViewOrigin[3] = 1;
	inter.cubicLight = lightShader->IsCubicLight(); // nbohr1more #3881: cubemap lights
	inter.ambientLight = lightShader->IsAmbientLight();

	// the base projections may be modified by texture matrix on light stages
	idPlane lightProject[4];
	R_GlobalPlaneToLocal( surf->space->modelMatrix, vLight->lightProject[0], lightProject[0] );
	R_GlobalPlaneToLocal( surf->space->modelMatrix, vLight->lightProject[1], lightProject[1] );
	R_GlobalPlaneToLocal( surf->space->modelMatrix, vLight->lightProject[2], lightProject[2] );
	R_GlobalPlaneToLocal( surf->space->modelMatrix, vLight->lightProject[3], lightProject[3] );

	memcpy( inter.lightProjection, lightProject, sizeof( inter.lightProjection ) );

	// now multiply the texgen by the light texture matrix
	if ( lightStage->texture.hasMatrix ) {
		RB_GetShaderTextureMatrix( lightRegs, &lightStage->texture, backEnd.lightTextureMatrix );
		void RB_BakeTextureMatrixIntoTexgen( idPlane lightProject[3], const float *textureMatrix );
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
		lightColor[0] = backEnd.lightScale * lightRegs[lightStage->color.registers[0]],
		lightColor[1] = backEnd.lightScale * lightRegs[lightStage->color.registers[1]],
		lightColor[2] = backEnd.lightScale * lightRegs[lightStage->color.registers[2]],
		lightColor[3] = lightRegs[lightStage->color.registers[3]]
	};

	// go through the individual stages
	for ( int surfaceStageNum = 0; surfaceStageNum < material->GetNumStages(); surfaceStageNum++ ) {
		const shaderStage_t	*surfaceStage = material->GetStage( surfaceStageNum );

		if ( !surfaceRegs[ surfaceStage->conditionRegister ] ) // ignore stage that fails the condition
			continue;


		void R_SetDrawInteraction( const shaderStage_t *surfaceStage, const float *surfaceRegs, idImage **image, idVec4 matrix[2], float color[4] );

		switch ( surfaceStage->lighting ) {
		case SL_AMBIENT: {
			// ignore ambient stages while drawing interactions
			break;
		}
		case SL_BUMP: {				
			if ( !r_skipBump.GetBool() ) {
				PrepareDrawCommand( &inter ); // draw any previous interaction
				inter.diffuseImage = NULL;
				inter.specularImage = NULL;
				R_SetDrawInteraction( surfaceStage, surfaceRegs, &inter.bumpImage, inter.bumpMatrix, NULL );
			}
			break;
		}
		case SL_DIFFUSE: {
			if ( inter.diffuseImage ) {
				PrepareDrawCommand( &inter );
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
			// nbohr1more: #4292 nospecular and nodiffuse fix
			if ( vLight->noSpecular ) {
				break;
			}
			if ( inter.specularImage ) {
				PrepareDrawCommand( &inter );
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
	PrepareDrawCommand( &inter );
}

void InteractionStage::PrepareDrawCommand( drawInteraction_t *din ) {
	if ( !din->bumpImage ) {
		if ( !r_skipBump.GetBool() )
			return;
		din->bumpImage = globalImages->flatNormalMap;		
	}

	if ( !din->diffuseImage || r_skipDiffuse.GetBool() ) {
		din->diffuseImage = globalImages->blackImage;
	}

	if ( !din->specularImage || r_skipSpecular.GetBool() ) {
		din->specularImage = globalImages->blackImage;
	}

	if (currentIndex > 0 && !renderBackend->ShouldUseBindlessTextures()) {
		if (backEnd.glState.tmu[TU_NORMAL].current2DMap != din->bumpImage->texnum
			|| backEnd.glState.tmu[TU_DIFFUSE].current2DMap != din->diffuseImage->texnum
			|| backEnd.glState.tmu[TU_SPECULAR].current2DMap != din->specularImage->texnum) {

			// change in textures, execute previous draw calls
			ExecuteDrawCalls();
			ResetShaderParams();
		}
	}

	if (currentIndex == 0 && !renderBackend->ShouldUseBindlessTextures()) {
		// bind new material textures
		GL_SelectTexture( TU_NORMAL );
		din->bumpImage->Bind();
		GL_SelectTexture( TU_DIFFUSE );
		din->diffuseImage->Bind();
		GL_SelectTexture( TU_SPECULAR );
		din->specularImage->Bind();
	}

	ShaderParams &params = shaderParams[currentIndex];

	memcpy( params.modelMatrix.ToFloatPtr(), din->surf->space->modelMatrix, sizeof(idMat4) );
	memcpy( params.modelViewMatrix.ToFloatPtr(), din->surf->space->modelViewMatrix, sizeof(idMat4) );
	params.bumpMatrix[0] = din->bumpMatrix[0];
	params.bumpMatrix[1] = din->bumpMatrix[1];
	params.diffuseMatrix[0] = din->diffuseMatrix[0];
	params.diffuseMatrix[1] = din->diffuseMatrix[1];
	params.specularMatrix[0] = din->specularMatrix[0];
	params.specularMatrix[1] = din->specularMatrix[1];
	memcpy( params.lightProjectionFalloff.ToFloatPtr(), din->lightProjection, sizeof(idMat4) );
	switch ( din->vertexColor ) {
	case SVC_IGNORE:
		params.colorModulate = idVec4(0, 0, 0, 0);
		params.colorAdd = idVec4(1, 1, 1, 1);
		break;
	case SVC_MODULATE:
		params.colorModulate = idVec4(1, 1, 1, 1);
		params.colorAdd = idVec4(0, 0, 0, 0);
		break;
	case SVC_INVERSE_MODULATE:
		params.colorModulate = idVec4(-1, -1, -1, -1);
		params.colorAdd = idVec4(1, 1, 1, 1);
		break;
	}
	params.lightOrigin = din->ambientLight ? din->worldUpLocal : din->localLightOrigin;
	params.viewOrigin = din->localViewOrigin;
	params.diffuseColor = din->diffuseColor;
	params.specularColor = din->specularColor;
	if ( !din->bumpImage ) {
		params.hasTextureDNS = idVec4(1, 0, 1, 0);
	} else {
		params.hasTextureDNS = idVec4(1, 1, 1, 0);
	}
	params.ambientRimColor = din->ambientRimColor;

	if (renderBackend->ShouldUseBindlessTextures()) {
		din->bumpImage->MakeResident();
		params.normalTexture = din->bumpImage->BindlessHandle();
		din->diffuseImage->MakeResident();
		params.diffuseTexture = din->diffuseImage->BindlessHandle();
		din->specularImage->MakeResident();
		params.specularTexture = din->specularImage->BindlessHandle();
	}

	drawBatches->AddDrawVertSurf( din->surf );

	++currentIndex;
	if (currentIndex == maxSupportedDrawsPerBatch) {
		ExecuteDrawCalls();
		ResetShaderParams();
	}
}

void InteractionStage::ResetShaderParams() {
	currentIndex = 0;
	shaderParams = shaderParamsBuffer->Request<ShaderParams>(maxSupportedDrawsPerBatch);
	drawBatches->BeginBatch( maxSupportedDrawsPerBatch );
}

void InteractionStage::ExecuteDrawCalls() {
	int totalDrawCalls = currentIndex;
	if (totalDrawCalls == 0) {
		return;
	}

	shaderParamsBuffer->Commit( shaderParams, totalDrawCalls );
	shaderParamsBuffer->BindRange( 1, shaderParams, totalDrawCalls );

	drawBatches->DrawBatch();
}
