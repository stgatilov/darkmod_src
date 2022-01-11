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

#include "ManyLightInteractionStage.h"

#include "RenderBackend.h"
#include "../glsl.h"
#include "../GLSLProgramManager.h"
#include "../FrameBuffer.h"
#include "../AmbientOcclusionStage.h"
#include "DrawBatchExecutor.h"
#include "../FrameBufferManager.h"

// NOTE: must match struct in shader, beware of std140 layout requirements and alignment!
struct ManyLightInteractionStage::ShaderParams {
	idMat4 modelMatrix;
	idMat4 modelViewMatrix;
	idVec4 bumpMatrix[2];
	idVec4 diffuseMatrix[2];
	idVec4 specularMatrix[2];
	idVec4 colorModulate;
	idVec4 colorAdd;
	idVec4 diffuseColor;
	idVec4 specularColor;
	idVec4 hasTextureDNS;
	idVec4 ambientRimColor;
	uint32_t lightMask;
	uint32_t padding;
	// bindless texture handles, if supported
	uint64_t normalTexture;
	uint64_t diffuseTexture;
	uint64_t specularTexture;
};

struct ManyLightInteractionStage::LightParams {
	idVec4 scissor;
	idVec4 globalLightOrigin;
	idVec4 shadowRect;
	idVec4 color;
	idMat4 projection;
	int shadows;
	int cubic;
	int ambient;
	int padding;
	// bindless texture handles, if supported
	uint64_t fallOffTexture;
	uint64_t projectionTexture;
};

struct ManyLightInteractionStage::DrawInteraction {
	const drawSurf_t *surf;

	idImage *bumpImage;
	idImage	*diffuseImage;
	idImage *specularImage;

	idVec4 diffuseColor;	// may have a light color baked into it, will be < tr.backEndRendererMaxLight
	idVec4 specularColor;	// may have a light color baked into it, will be < tr.backEndRendererMaxLight
	idVec4 ambientRimColor;
	stageVertexColor_t vertexColor;	// applies to both diffuse and specular

	idVec4 bumpMatrix[2];
	idVec4 diffuseMatrix[2];
	idVec4 specularMatrix[2];
	uint32_t lightMask;
};


namespace {
	struct InteractionUniforms: GLSLUniformGroup {
		UNIFORM_GROUP_DEF( InteractionUniforms )

		DEFINE_UNIFORM( sampler, normalTexture )
		DEFINE_UNIFORM( sampler, diffuseTexture )
		DEFINE_UNIFORM( sampler, specularTexture )
		DEFINE_UNIFORM( sampler, ssaoTexture )
		DEFINE_UNIFORM( sampler, shadowMap )
		DEFINE_UNIFORM( sampler, lightProjectionTexture )
		DEFINE_UNIFORM( sampler, lightProjectionCubemap )
		DEFINE_UNIFORM( sampler, lightFalloffTexture )
		DEFINE_UNIFORM( sampler, lightFalloffCubemap )

		DEFINE_UNIFORM( vec3, globalViewOrigin )
		DEFINE_UNIFORM( int, useBumpmapLightTogglingFix )
		DEFINE_UNIFORM( float, gamma )
		DEFINE_UNIFORM( float, minLevel )
		DEFINE_UNIFORM( int, ssaoEnabled )

		DEFINE_UNIFORM( int, softShadowsQuality )
		DEFINE_UNIFORM( float, softShadowsRadius )
		DEFINE_UNIFORM( int, shadowMapCullFront )

		DEFINE_UNIFORM( int, numLights )
	};

	enum TextureUnits {
		TU_NORMAL = 0,
		TU_DIFFUSE = 1,
		TU_SPECULAR = 2,
		TU_SSAO = 3,
		TU_SHADOW_MAP = 4,
		TU_DISABLED = 5,
		TU_FIRST_LIGHT = 6,
	};
}

void ManyLightInteractionStage::LoadInteractionShader( GLSLProgram *shader, bool bindless ) {
	idHashMapDict defines;
	defines.Set( "MAX_SHADER_PARAMS", idStr::Fmt( "%d", maxShaderParamsArraySize ) );
	defines.Set( "MAX_LIGHTS", idStr::Fmt( "%d", bindless ? MAX_BINDLESS_LIGHTS : MAX_LIGHTS ) );
	if (bindless) {
		defines.Set( "BINDLESS_TEXTURES", "1" );
	}
	shader->LoadFromFiles( "stages/interaction/manylight.vert.glsl", "stages/interaction/manylight.frag.glsl", defines );
	InteractionUniforms *uniforms = shader->GetUniformGroup<InteractionUniforms>();
	uniforms->ssaoTexture.Set( TU_SSAO );
	uniforms->shadowMap.Set( TU_SHADOW_MAP );
	if (!bindless) {
		uniforms->normalTexture.Set( TU_NORMAL );
		uniforms->diffuseTexture.Set( TU_DIFFUSE );
		uniforms->specularTexture.Set( TU_SPECULAR );
	}
	shader->BindUniformBlockLocation( 0, "ViewParamsBlock" );
	shader->BindUniformBlockLocation( 1, "PerDrawCallParamsBlock" );
	shader->BindUniformBlockLocation( 2, "ShadowSamplesBlock" );
	shader->BindUniformBlockLocation( 3, "PerLightParamsBlock" );
}


ManyLightInteractionStage::ManyLightInteractionStage( DrawBatchExecutor *drawBatchExecutor )
	: drawBatchExecutor( drawBatchExecutor ), interactionShader( nullptr )
{}

void ManyLightInteractionStage::Init() {
	lightParams = new LightParams[MAX_BINDLESS_LIGHTS];
	maxShaderParamsArraySize = drawBatchExecutor->MaxShaderParamsArraySize<ShaderParams>();
	
	shadowMapInteractionShader = programManager->LoadFromGenerator( "manylight", 
		[this](GLSLProgram *shader) { LoadInteractionShader( shader, false ); } );
	if (GLAD_GL_ARB_bindless_texture) {
		bindlessShadowMapInteractionShader = programManager->LoadFromGenerator( "manylight_bindless", 
			[this](GLSLProgram *shader) { LoadInteractionShader( shader, true ); } );
	}

	qglGenBuffers( 1, &poissonSamplesUbo );
	qglBindBuffer( GL_UNIFORM_BUFFER, poissonSamplesUbo );
	qglBufferData( GL_UNIFORM_BUFFER, 0, nullptr, GL_STATIC_DRAW );
}

void ManyLightInteractionStage::Shutdown() {
	qglDeleteBuffers( 1, &poissonSamplesUbo );
	poissonSamplesUbo = 0;
	poissonSamples.ClearFree();
	delete[] lightParams;
	lightParams = nullptr;
}

void ManyLightInteractionStage::DrawInteractions( const viewDef_t *viewDef ) {
	TRACE_GL_SCOPE( "DrawInteractionsMultiLight" );

	PreparePoissonSamples();
	SetGlState(GLS_DEPTHFUNC_EQUAL);
	backEnd.currentSpace = NULL;

	BindShadowTexture();
	ambientOcclusion->BindSSAOTexture( TU_SSAO );
	GL_SelectTexture( TU_DISABLED );
	globalImages->whiteImage->Bind();

	PrepareInteractionProgram();

	idList<const drawSurf_t *> drawSurfs;
	for ( int i = 0; i < viewDef->numDrawSurfs; ++i ) {
		const drawSurf_t *surf = viewDef->drawSurfs[i];
		drawSurfs.AddGrow( surf );
	}
	std::sort( drawSurfs.begin(), drawSurfs.end(), [](const drawSurf_t *a, const drawSurf_t *b) {
		if ( a->ambientCache.isStatic != b->ambientCache.isStatic )
			return a->ambientCache.isStatic;
		if ( a->indexCache.isStatic != b->indexCache.isStatic )
			return a->indexCache.isStatic;

		return a->material < b->material;
	} );

	ResetLightParams( viewDef );
	for ( viewLight_t *vLight = viewDef->viewLights; vLight; vLight = vLight->next ) {
		if ( vLight->lightShader->IsFogLight() || vLight->lightShader->IsBlendLight() ) {
			continue;
		}
		if ( vLight->lightShader->IsAmbientLight() ) {
			if ( r_skipAmbient.GetInteger() & 2 ) {
				continue;
			}
		} else if ( r_skipInteractions.GetBool() ) {
			continue;
		}
		if ( !vLight->noShadows && vLight->shadows == LS_STENCIL ) {
			continue;
		}

		const idMaterial *lightShader = vLight->lightShader;
		const float	*lightRegs = vLight->shaderRegisters;
		int maxSupportedLights = renderBackend->ShouldUseBindlessTextures() ? MAX_BINDLESS_LIGHTS : MAX_LIGHTS;
		for ( int lightStageNum = 0; lightStageNum < lightShader->GetNumStages(); lightStageNum++ ) {
			const shaderStage_t	*lightStage = lightShader->GetStage( lightStageNum );

			// ignore stages that fail the condition
			if ( !lightRegs[lightStage->conditionRegister] ) {
				continue;
			}

			LightParams &params = lightParams[curLight];

			vLight->lightMask |= (1u << curLight );
			if ( renderBackend->ShouldUseBindlessTextures() ) {
				vLight->falloffImage->MakeResident();
				params.fallOffTexture = vLight->falloffImage->BindlessHandle();
				lightStage->texture.image->MakeResident();
				params.projectionTexture = lightStage->texture.image->BindlessHandle();
			} else {
				GL_SelectTexture( TU_FIRST_LIGHT + 2 * curLight );
				vLight->falloffImage->Bind();
				if ( vLight->falloffImage->type == TT_CUBIC ) {
					falloffCubeTextureUnits[curLight] = TU_FIRST_LIGHT + 2 * curLight;
				} else {
					falloffTextureUnits[curLight] = TU_FIRST_LIGHT + 2 * curLight;
				}
				GL_SelectTexture( TU_FIRST_LIGHT + 2 * curLight + 1 );
				lightStage->texture.image->Bind();
				if ( lightStage->texture.image->type == TT_CUBIC) {
					projectionCubeTextureUnits[curLight] = TU_FIRST_LIGHT + 2 * curLight + 1;
				} else {
					projectionTextureUnits[curLight] = TU_FIRST_LIGHT + 2 * curLight + 1;
				}
			}

			params.ambient = lightShader->IsAmbientLight();
			params.cubic = lightShader->IsCubicLight();
			// FIXME shadowmap only valid when globalInteractions not empty, otherwise garbage
			bool doShadows = !vLight->noShadows && lightShader->LightCastsShadows() && vLight->globalInteractions != nullptr;
			params.shadows = doShadows;
			params.globalLightOrigin = idVec4(vLight->globalLightOrigin.x, vLight->globalLightOrigin.y, vLight->globalLightOrigin.z, 1);
			params.color.x = backEnd.lightScale * lightRegs[lightStage->color.registers[0]];
			params.color.y = backEnd.lightScale * lightRegs[lightStage->color.registers[1]];
			params.color.z = backEnd.lightScale * lightRegs[lightStage->color.registers[2]];
			params.color.w = lightRegs[lightStage->color.registers[3]];

			if (r_useScissor.GetBool()) {
				float xScale = static_cast<float>(frameBuffers->activeFbo->Width()) / glConfig.vidWidth;
				float yScale = static_cast<float>(frameBuffers->activeFbo->Height()) / glConfig.vidHeight;
				params.scissor = idVec4( vLight->scissorRect.x1 * xScale, vLight->scissorRect.y1 * yScale, vLight->scissorRect.x2 * xScale, vLight->scissorRect.y2 * yScale );
			} else {
				params.scissor = idVec4(0, 0, frameBuffers->activeFbo->Width(), frameBuffers->activeFbo->Height());
			}

			memcpy( params.projection.ToFloatPtr(), vLight->lightProject, sizeof( idMat4 ) );

			float lightTexMatrix[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
			if ( lightStage->texture.hasMatrix )
				RB_GetShaderTextureMatrix( lightRegs, &lightStage->texture, lightTexMatrix );
			// stgatilov: we no longer merge two transforms together, since we need light-volume coords in fragment shader
			//RB_BakeTextureMatrixIntoTexgen( reinterpret_cast<class idPlane *>(params.projection.ToFloatPtr()), lightTexMatrix );
			//TODO
			/*inter.lightTextureMatrix[0].Set( lightTexMatrix[0], lightTexMatrix[4], 0, lightTexMatrix[12] );
			inter.lightTextureMatrix[1].Set( lightTexMatrix[1], lightTexMatrix[5], 0, lightTexMatrix[13] );*/

			if ( vLight->shadowMapIndex > 0 && vLight->shadowMapIndex <= 42) {
				auto &page = ShadowAtlasPages[vLight->shadowMapIndex-1];
				// https://stackoverflow.com/questions/5879403/opengl-texture-coordinates-in-pixel-space
				idVec4 v( page.x, page.y, 0, page.width-1 );
				v.ToVec2() = (v.ToVec2() * 2 + idVec2( 1, 1 )) / (2 * 6 * r_shadowMapSize.GetInteger());
				v.w /= 6 * r_shadowMapSize.GetFloat();
				params.shadowRect = v;
			} else {
				params.shadowRect = idVec4(0, 0, 0, 0);
				params.shadows = 0;
			}

			++curLight;
			if ( curLight == maxSupportedLights ) {
				DrawAllSurfaces( drawSurfs );
				ResetLightParams( viewDef );
				curLight = 0;
			}
		}
	}

	if ( curLight > 0 ) {
		DrawAllSurfaces( drawSurfs );
	}

	GL_SelectTexture( 0 );
	GLSLProgram::Deactivate();
}

void ManyLightInteractionStage::SetGlState(int depthFunc) {
	// if using float buffers, alpha values are not clamped and can stack up quite high, since most interactions add 1 to its value
	// this in turn causes issues with some shader stage materials that use DST_ALPHA blending.
	// masking the alpha channel for interactions seems to fix those issues, but only do it for float buffers in case it has
	// unwanted side effects
	int alphaMask = r_fboColorBits.GetInteger() == 64 ? GLS_ALPHAMASK : 0;
	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | alphaMask | depthFunc );
	currentDepthFunc = depthFunc;
}

void ManyLightInteractionStage::DrawAllSurfaces( idList<const drawSurf_t *> &drawSurfs ) {
	static_assert( sizeof(LightParams) % 16 == 0, "Light params size must be a multiple of 16" );

	if ( drawSurfs.Num() == 0 ) {
		return;
	}

	InteractionUniforms *uniforms = interactionShader->GetUniformGroup<InteractionUniforms>();
	if ( !renderBackend->ShouldUseBindlessTextures() ) {
		uniforms->lightFalloffTexture.SetArray( MAX_LIGHTS, falloffTextureUnits );
		uniforms->lightFalloffCubemap.SetArray( MAX_LIGHTS, falloffCubeTextureUnits );
		uniforms->lightProjectionTexture.SetArray( MAX_LIGHTS, projectionTextureUnits );
		uniforms->lightProjectionCubemap.SetArray( MAX_LIGHTS, projectionCubeTextureUnits );
	}
	uniforms->numLights.Set( curLight );

	drawBatchExecutor->UploadExtraUboData( lightParams, sizeof(LightParams) * curLight, 3 );

	BeginDrawBatch();
	const drawSurf_t *curBatchCaches = drawSurfs[0];
	for ( const drawSurf_t *surf : drawSurfs ) {
		if ( surf->dsFlags & DSF_SHADOW_MAP_ONLY ) {
			continue;
		}
		if ( !surf->ambientCache.IsValid() ) {
			common->Warning( "Found invalid ambientCache!" );
			continue;
		}

		if ( curBatchCaches->ambientCache.isStatic != surf->ambientCache.isStatic || curBatchCaches->indexCache.isStatic != surf->indexCache.isStatic ) {
			ExecuteDrawCalls();
		}

		if ( surf->space->weaponDepthHack ) {
			// GL state change, need to execute previous draw calls
			ExecuteDrawCalls();
			RB_EnterWeaponDepthHack();
		}

		int requiredDepthFunc = surf->material->Coverage() == MC_TRANSLUCENT ? GLS_DEPTHFUNC_LESS : GLS_DEPTHFUNC_EQUAL;
		if ( requiredDepthFunc != currentDepthFunc ) {
			ExecuteDrawCalls();
			// need to ensure the right depth func is set, as translucent surfaces require LEQUAL.
			SetGlState( requiredDepthFunc );
			uniforms->softShadowsQuality.Set( surf->material->Coverage() == MC_TRANSLUCENT ? 0 : r_softShadowsQuality.GetInteger() );
		}

		curBatchCaches = surf;
		ProcessSingleSurface( surf );

		if ( surf->space->weaponDepthHack ) {
			ExecuteDrawCalls();
			RB_LeaveDepthHack();
		}
	}
	ExecuteDrawCalls();
}

void ManyLightInteractionStage::BindShadowTexture() {
	GL_SelectTexture( TU_SHADOW_MAP );
	globalImages->shadowAtlas->Bind();
}

void ManyLightInteractionStage::PrepareInteractionProgram() {
	interactionShader = renderBackend->ShouldUseBindlessTextures() ? bindlessShadowMapInteractionShader : shadowMapInteractionShader;
	interactionShader->Activate();

	InteractionUniforms *uniforms = interactionShader->GetUniformGroup<InteractionUniforms>();
	uniforms->gamma.Set( backEnd.viewDef->IsLightGem() ? 1 : r_ambientGamma.GetFloat() );
	uniforms->minLevel.Set( r_ambientMinLevel.GetFloat() );
	uniforms->useBumpmapLightTogglingFix.Set( r_useBumpmapLightTogglingFix.GetBool() );
	uniforms->ssaoEnabled.Set( ambientOcclusion->ShouldEnableForCurrentView() ? 1 : 0 );
	uniforms->shadowMapCullFront.Set( r_shadowMapCullFront );
	if ( !backEnd.viewDef->IsLightGem() ) {
		uniforms->softShadowsQuality.Set( r_softShadowsQuality.GetInteger() );
	} else {
		uniforms->softShadowsQuality.Set( 0 );
	}
	uniforms->softShadowsRadius.Set( /*GetEffectiveLightRadius()*/ r_softShadowsRadius.GetFloat() ); // FIXME: references backend.vLight for soft stencil and all shadow maps
	uniforms->globalViewOrigin.Set( backEnd.viewDef->renderView.vieworg );
}

void ManyLightInteractionStage::ProcessSingleSurface( const drawSurf_t *surf ) {
	const idMaterial *material = surf->material;
	const float *surfaceRegs = surf->shaderRegisters;
	DrawInteraction inter;

	if ( !surf->ambientCache.IsValid() ) {
		return;
	}

	auto ambientRegs = material->GetAmbientRimColor().registers;
	if ( ambientRegs[0] ) {
		for ( int i = 0; i < 3; i++ )
			inter.ambientRimColor[i] = surfaceRegs[ambientRegs[i]];
		inter.ambientRimColor[3] = 1;
	} else
		inter.ambientRimColor.Zero();

	inter.surf = surf;

	inter.bumpImage = NULL;
	inter.specularImage = NULL;
	inter.diffuseImage = NULL;
	inter.diffuseColor[0] = inter.diffuseColor[1] = inter.diffuseColor[2] = inter.diffuseColor[3] = 0;
	inter.specularColor[0] = inter.specularColor[1] = inter.specularColor[2] = inter.specularColor[3] = 0;

	inter.lightMask = 0;
	for ( viewLight_t **pLight = surf->onLights; pLight && *pLight; ++pLight ) {
		inter.lightMask |= (*pLight)->lightMask;
	}
	if ( inter.lightMask == 0 ) {
		return;
	}

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
			inter.vertexColor = surfaceStage->vertexColor;
			break;
		}
		case SL_SPECULAR: {
			// nbohr1more: #4292 nospecular and nodiffuse fix
			//if ( vLight->noSpecular ) { // FIXME: handle in shader
			//	break;
			//}
			if ( inter.specularImage ) {
				PrepareDrawCommand( &inter );
			}
			R_SetDrawInteraction( surfaceStage, surfaceRegs, &inter.specularImage,
								  inter.specularMatrix, inter.specularColor.ToFloatPtr() );
			inter.vertexColor = surfaceStage->vertexColor;
			break;
		}
		}
	}

	// draw the final interaction
	PrepareDrawCommand( &inter );
}

void ManyLightInteractionStage::PrepareDrawCommand( DrawInteraction *din ) {
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
		if (!din->bumpImage->IsBound( TU_NORMAL )
			|| !din->diffuseImage->IsBound( TU_DIFFUSE )
			|| !din->specularImage->IsBound( TU_SPECULAR ) ) {

			// change in textures, execute previous draw calls
			ExecuteDrawCalls();
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

	ShaderParams &params = drawBatch.shaderParams[currentIndex];

	memcpy( params.modelMatrix.ToFloatPtr(), din->surf->space->modelMatrix, sizeof(idMat4) );
	memcpy( params.modelViewMatrix.ToFloatPtr(), din->surf->space->modelViewMatrix, sizeof(idMat4) );
	params.bumpMatrix[0] = din->bumpMatrix[0];
	params.bumpMatrix[1] = din->bumpMatrix[1];
	params.diffuseMatrix[0] = din->diffuseMatrix[0];
	params.diffuseMatrix[1] = din->diffuseMatrix[1];
	params.specularMatrix[0] = din->specularMatrix[0];
	params.specularMatrix[1] = din->specularMatrix[1];
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

	params.lightMask = din->lightMask;

	drawBatch.surfs[currentIndex] = din->surf;

	++currentIndex;
	if (currentIndex == drawBatch.maxBatchSize) {
		ExecuteDrawCalls();
	}
}

void ManyLightInteractionStage::BeginDrawBatch() {
	currentIndex = 0;
	drawBatch = drawBatchExecutor->BeginBatch<ShaderParams>();
}

void ManyLightInteractionStage::ExecuteDrawCalls() {
	if (currentIndex == 0) {
		return;
	}
	drawBatchExecutor->ExecuteDrawVertBatch( currentIndex );
	BeginDrawBatch();
}

void ManyLightInteractionStage::PreparePoissonSamples() {
	int sampleK = r_softShadowsQuality.GetInteger();
	if ( sampleK > 0 && poissonSamples.Num() != sampleK ) {
		GeneratePoissonDiskSampling( poissonSamples, sampleK );
		// note: due to std140 buffer requirements, array members must be aligned with vec4 size
		// so we have to copy our samples to a vec4 array for upload :-/
		idList<idVec4> uploadSamples;
		uploadSamples.SetNum( poissonSamples.Num() );
		for ( int i = 0; i < poissonSamples.Num(); ++i ) {
			uploadSamples[i].x = poissonSamples[i].x;
			uploadSamples[i].y = poissonSamples[i].y;
		}
		size_t size = uploadSamples.Num() * sizeof(idVec4);
		qglBindBuffer( GL_UNIFORM_BUFFER, poissonSamplesUbo );
		qglBufferData( GL_UNIFORM_BUFFER, size, uploadSamples.Ptr(), GL_STATIC_DRAW );
	}
	qglBindBufferBase( GL_UNIFORM_BUFFER, 2, poissonSamplesUbo );
}

void ManyLightInteractionStage::ResetLightParams(const viewDef_t *viewDef) {
	for ( int i = 0; i < MAX_LIGHTS; ++i ) {
		falloffTextureUnits[i] = TU_DISABLED;
		falloffCubeTextureUnits[i] = TU_DISABLED;
		projectionTextureUnits[i] = TU_DISABLED;
		projectionCubeTextureUnits[i] = TU_DISABLED;
	}	

	for ( viewLight_t *vLight = viewDef->viewLights; vLight; vLight = vLight->next ) {
		vLight->lightMask = 0;
	}

	curLight = 0;
}
