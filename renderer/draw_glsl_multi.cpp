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
#include "FrameBuffer.h"
#include "GLSLProgramManager.h"
#include "FrameBufferManager.h"

static const uint MAX_LIGHTS = 16;

const bool useGeometryShader = true;

GLSLProgram* shadowShader() {
	return useGeometryShader ? programManager->shadowMapMultiGShader: programManager->shadowMapMultiShader;
}

struct MultiShadowUniforms : GLSLUniformGroup {
	UNIFORM_GROUP_DEF( MultiShadowUniforms )

	DEFINE_UNIFORM( mat4, modelMatrix )
	DEFINE_UNIFORM( int, lightCount )
	DEFINE_UNIFORM( vec3, lightOrigin )
	DEFINE_UNIFORM( vec4, shadowRect )
	DEFINE_UNIFORM( float, lightRadius )
	//DEFINE_UNIFORM( vec4, lightFrustum )
	DEFINE_UNIFORM( float, shadowTexelStep )
	DEFINE_UNIFORM( mat4, lightProjectionFalloff )
};

struct MultiLightShaderData { // used by both interaction and shadow map shaders
	//idList<viewLight_t*> vLights;
	idList<idVec3> lightOrigins;
	idList<idVec4> shadowRects;
	idList<float> softShadowRads;
	idList<idVec3> lightColors;
	idList<idMat4> projectionFalloff;
	idList<idMat3> bounds;
	MultiLightShaderData( const drawSurf_t* surf, bool shadowPass, const drawInteraction_t* din = nullptr ) :
		surf( surf ), shadowPass( shadowPass ), din( din ) {
		if ( !surf->onLights )
			return;
		for ( auto pLight = surf->onLights; *pLight; pLight++ ) {
			auto vLight = *pLight;
			backEnd.vLight = vLight; // GetEffectiveLightRadius needs this
			if ( shadowPass ) {
				if ( !vLight->shadowMapIndex || vLight->shadowMapIndex > 42 )
					continue;
			} else {
				if ( vLight->singleLightOnly )
					continue;
			}
			AddLightData();
		}
		backEnd.vLight = nullptr; // just in case
	}
private:
	const drawSurf_t* surf;
	bool shadowPass;
	const drawInteraction_t* din = nullptr;
	void AddLightData() {
		auto vLight = backEnd.vLight;
#if 0
		idVec3 localLightOrigin;
		R_GlobalPointToLocal( surf->space->modelMatrix, vLight->globalLightOrigin, localLightOrigin );
		//vLights.push_back(vLight);
		if ( shadowPass )
			lightOrigins.Append( vLight->globalLightOrigin );
		else
			lightOrigins.Append( localLightOrigin );
#else
		lightOrigins.Append( vLight->globalLightOrigin );
#endif

		if ( vLight->lightShader->IsAmbientLight() )
			shadowRects.Append( idVec4( 0, 0, -2, 0 ) );
		else if ( vLight->shadowMapIndex <= 0 )
			shadowRects.Append( idVec4( 0, 0, -1, 0 ) ); 
		else {
			auto & page = ShadowAtlasPages[vLight->shadowMapIndex - 1];
			idVec4 v( page.x, page.y, 0, page.width - 1 );
			v.ToVec2() = (v.ToVec2() * 2 + idVec2( 1, 1 )) / (2 * 6 * r_shadowMapSize.GetInteger());
			v.w /= 6 * r_shadowMapSize.GetFloat();
			v.z = vLight->shadowMapIndex - 1;
			shadowRects.Append( v );
		}
		softShadowRads.Append( GetEffectiveLightRadius() );
		if ( din ) {
			const float* lightRegs = vLight->shaderRegisters;
			const idMaterial* lightShader = vLight->lightShader;
			const shaderStage_t* lightStage = lightShader->GetStage( 0 );
			idVec4 lightColor(
				backEnd.lightScale * lightRegs[lightStage->color.registers[0]] * din->diffuseColor[0],
				backEnd.lightScale * lightRegs[lightStage->color.registers[1]] * din->diffuseColor[1],
				backEnd.lightScale * lightRegs[lightStage->color.registers[2]] * din->diffuseColor[2],
				lightRegs[lightStage->color.registers[3]]
			);
			lightColors.Append( lightColor.ToVec3() );
#if 0
			idPlane lightProject[4];
			R_GlobalPlaneToLocal( surf->space->modelMatrix, vLight->lightProject[0], lightProject[0] );
			R_GlobalPlaneToLocal( surf->space->modelMatrix, vLight->lightProject[1], lightProject[1] );
			R_GlobalPlaneToLocal( surf->space->modelMatrix, vLight->lightProject[2], lightProject[2] );
			R_GlobalPlaneToLocal( surf->space->modelMatrix, vLight->lightProject[3], lightProject[3] );
			idMat4* p = (idMat4*)&lightProject;
#else
			idMat4* p = (idMat4*)&vLight->lightProject;
#endif
			projectionFalloff.Append( *p );
		} else {
			idMat4 bounds;
			bounds[0].ToVec3() = vLight->frustumTris->bounds[0];
			bounds[1].ToVec3() = vLight->frustumTris->bounds[1];
			projectionFalloff.Append( bounds );
		}
	}
};

static void RB_DrawMultiLightInteraction( const drawInteraction_t *din ) {
	auto surf = din->surf;
	MultiLightShaderData data( surf, false, din );

	Uniforms::Interaction *interactionUniforms = programManager->multiLightInteractionShader->GetUniformGroup<Uniforms::Interaction>();
	interactionUniforms->SetForInteractionBasic( din );
	interactionUniforms->minLevel.Set( backEnd.viewDef->IsLightGem() ? 0 : r_ambientMinLevel.GetFloat() );
	interactionUniforms->gamma.Set( backEnd.viewDef->IsLightGem() ? 1 : r_ambientGamma.GetFloat() );

	for ( int i = 0; i < (int)data.lightOrigins.Num(); i += MAX_LIGHTS ) {
		int thisCount = idMath::Imin( (uint)data.lightOrigins.Num() - i, MAX_LIGHTS );

		interactionUniforms->lightCount.Set( thisCount );
		interactionUniforms->lightOrigin.SetArray( thisCount, data.lightOrigins[i].ToFloatPtr() );
		interactionUniforms->lightColor.SetArray( thisCount, data.lightColors[i].ToFloatPtr() );
		interactionUniforms->lightProjectionFalloff.SetArray( thisCount, data.projectionFalloff[i].ToFloatPtr() );
		interactionUniforms->shadowRect.SetArray( thisCount, data.shadowRects[i].ToFloatPtr() );
		interactionUniforms->softShadowsRadius.SetArray( thisCount, &data.softShadowRads[i] );
		GL_CheckErrors();

		RB_DrawElementsWithCounters( surf );

		if ( r_showMultiLight.GetInteger() == 1 ) {
			backEnd.pc.c_interactions++;
			backEnd.pc.c_interactionLights += (uint)data.lightOrigins.Num();
			backEnd.pc.c_interactionMaxLights = idMath::Imax( backEnd.pc.c_interactionMaxLights, (uint)data.lightOrigins.Num() );
			auto shMaps = std::count_if( data.shadowRects.begin(), data.shadowRects.end(), []( idVec4 v ) {
				return v.z >= 0;
			} );
			if ( backEnd.pc.c_interactionMaxShadowMaps < (uint)shMaps )
				backEnd.pc.c_interactionMaxShadowMaps = (uint)shMaps;
		}
	}
	GL_CheckErrors();
}

static idCVarInt r_warnMultiLight( "r_warnMultiLight", "0", CVAR_RENDERER, "warns about heavy draw calls" );

void RB_ShadowMap_RenderAllLights( drawSurf_t *surf ) {
	if ( !surf->material->SurfaceCastsShadow() )
		return;    // some dynamic models use a no-shadow material and for shadows have a separate geometry with an invisible (in main render) material

	if ( surf->dsFlags & DSF_SHADOW_MAP_IGNORE )
		return;    // this flag is set by entities with parms.noShadow (candles, torches, models with separate shadow geometry, etc)

	if ( surf->sort >= SS_AFTER_FOG )
		return;

	Uniforms::Depth *depthUniforms = shadowShader()->GetUniformGroup<Uniforms::Depth>();
	MultiShadowUniforms* shadowUniforms = shadowShader()->GetUniformGroup<MultiShadowUniforms>();

	/*float customOffset = 0;
	if ( auto entityDef = surf->space->entityDef )
		customOffset = entityDef->parms.shadowMapOffset + surf->material->GetShadowMapOffset();
	if ( customOffset != 0 )
		qglPolygonOffset( customOffset, 0 );*/

	if ( backEnd.currentSpace != surf->space ) {
		shadowUniforms->modelMatrix.Set( surf->space->modelMatrix );
		backEnd.currentSpace = surf->space;
		backEnd.pc.c_matrixLoads++;
	}

	MultiLightShaderData data( surf, true );

	for ( int i = 0; i < (int)data.lightOrigins.Num(); i += MAX_LIGHTS ) {
		int thisCount = idMath::Imin( (int)data.lightOrigins.Num() - i, MAX_LIGHTS );

		shadowUniforms->lightCount.Set( thisCount );
		shadowUniforms->lightOrigin.SetArray( thisCount, data.lightOrigins[i].ToFloatPtr() );
		shadowUniforms->shadowRect.SetArray( thisCount, data.shadowRects[i].ToFloatPtr() );
		shadowUniforms->lightRadius.SetArray( thisCount, &data.softShadowRads[i] );
		shadowUniforms->lightProjectionFalloff.SetArray( thisCount, data.projectionFalloff[i].ToFloatPtr() );
		GL_CheckErrors();

		//depthUniforms->instances = thisCount * 6;
		depthUniforms->instances = useGeometryShader ? 0 : thisCount * 6;
		RB_SingleSurfaceToDepthBuffer( shadowShader(), surf );

		if ( r_showMultiLight.GetInteger() == 2 ) {
			backEnd.pc.c_interactions++;
			backEnd.pc.c_interactionLights += (uint)data.lightOrigins.Num();
			backEnd.pc.c_interactionMaxLights = idMath::Imax( backEnd.pc.c_interactionMaxLights, (uint)data.lightOrigins.Num() );
			auto shMaps = std::count_if( data.shadowRects.begin(), data.shadowRects.end(), []( idVec4 v ) {
				return v.z >= 0;
			} );
			if ( backEnd.pc.c_interactionMaxShadowMaps < (uint)shMaps )
				backEnd.pc.c_interactionMaxShadowMaps = (uint)shMaps;
		}
	}
#if 0
	if ( r_ignore.GetBool() && data.lightOrigins.Num() * (int)surf->ambientCache.size > r_ignore.GetInteger() )
		common->Warning( "%d %d %d\n", surf->space->entityDef->index, data.lightOrigins.Num(), surf->ambientCache.size );
#endif
	if ( r_warnMultiLight > 0 ) {
		if ( data.lightOrigins.Num() > r_warnMultiLight )
			common->Warning("%i %i %i %s", surf->space->entityDef->index, data.lightOrigins.Num(), surf->numIndexes, surf->space->entityDef->parms.hModel->Name() );
	}

	/*if ( customOffset != 0 )
		qglPolygonOffset( 0, 0 );*/
}

void RB_ShadowMap_RenderAllLights() {
	TRACE_GL_SCOPE( "ShadowMap_RenderAllLights" );

	frameBuffers->EnterShadowMap();

	GL_CheckErrors();
	shadowShader()->Activate();
	Uniforms::Depth *depthUniforms = shadowShader()->GetUniformGroup<Uniforms::Depth>();
	MultiShadowUniforms *shadowUniforms = shadowShader()->GetUniformGroup<MultiShadowUniforms>();
	GL_SelectTexture( 0 );

	backEnd.currentSpace = NULL;

	GL_Cull( r_shadowMapCullFront ? CT_BACK_SIDED : CT_TWO_SIDED );
	qglPolygonOffset( 0, 0 );
	qglEnable( GL_POLYGON_OFFSET_FILL );

	float texSize = globalImages->shadowAtlas->uploadHeight;
	shadowUniforms->shadowTexelStep.Set( 1 / texSize );
	depthUniforms->alphaTest.Set( -1 );	// no alpha test by default

	qglViewport( 0, 0, texSize, texSize );
	if ( r_useScissor.GetBool() )
		GL_ScissorVidSize( 0, 0, texSize, texSize );
	qglClear( GL_DEPTH_BUFFER_BIT );
	for ( int i = 0; i < 4; i++ ) // clip the geometry shader output to each of the atlas pages
		qglEnable( GL_CLIP_PLANE0 + i );
	auto viewDef = backEnd.viewDef;
	GL_CheckErrors();
	for ( int i = 0; i < viewDef->numDrawSurfs + viewDef->numOffscreenSurfs; i++ )
		RB_ShadowMap_RenderAllLights( viewDef->drawSurfs[i] );
	for ( int i = 0; i < 4; i++ )
		qglDisable( GL_CLIP_PLANE0 + i );
	qglDisable( GL_POLYGON_OFFSET_FILL );
	GL_Cull( CT_FRONT_SIDED );

	backEnd.currentSpace = NULL; // or else conflicts with qglLoadMatrixf
	GLSLProgram::Deactivate();

	frameBuffers->LeaveShadowMap();

	GL_CheckErrors();
}

void RB_GLSL_DrawInteraction_MultiLight( const drawInteraction_t *din ) {
	// load all the shader parameters
	GL_CheckErrors();

	// set the textures
	// texture 0 will be the per-surface bump map
	if ( !din->bumpImage ) return;// FIXME support textureDNS
	GL_SelectTexture( 0 );
	din->bumpImage->Bind();

	// texture 1 will be the light falloff texture
	GL_SelectTexture( 1 );
	//din->lightFalloffImage->Bind();

	// texture 2 will be the light projection texture
	GL_SelectTexture( 2 );
	//din->lightImage->Bind();

	// texture 3 is the per-surface diffuse map
	GL_SelectTexture( 3 );
	din->diffuseImage->Bind();

	// texture 4 is the per-surface specular map
	GL_SelectTexture( 4 );
	din->specularImage->Bind();

	RB_DrawMultiLightInteraction( din );
	GL_CheckErrors();
}

idVec3 softLightSamples[8];

void RB_GLSL_DrawInteractions_MultiLight() {
	if ( !backEnd.viewDef->viewLights )
		return;
	GL_CheckErrors();
	TRACE_GL_SCOPE( "GLSL_MultiLightInteractions" );

	extern void RB_GLSL_GenerateShadowMaps();
	RB_GLSL_GenerateShadowMaps();

	GL_CheckErrors();
	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | backEnd.depthFunc );

	GL_SelectTexture( 5 );
	globalImages->shadowAtlas->Bind();

	//GL_SelectTexture( 6 );
	//globalImages->shadowAtlasHistory->Bind();

	programManager->multiLightInteractionShader->Activate();
	Uniforms::Interaction *interactionUniforms = programManager->multiLightInteractionShader->GetUniformGroup<Uniforms::Interaction>();
	interactionUniforms->shadowMap.Set( 5 );
	interactionUniforms->shadowMapHistory.Set( 6 );
	interactionUniforms->frameCount.Set( backEnd.frameCount );
	//softLightSamples[7] = backEnd.viewDef->lightSample;
	interactionUniforms->lightSamples.SetArray( 8, softLightSamples[0].ToFloatPtr() );

	backEnd.currentSpace = NULL; // shadow map shader uses a uniform instead of qglLoadMatrixf, needs reset

	for ( int i = 0; i < backEnd.viewDef->numDrawSurfs; i++ ) {
		auto surf = backEnd.viewDef->drawSurfs[i];
		auto material = surf->material;
		if ( material->SuppressInSubview() || material->GetSort() < SS_OPAQUE )
			continue;
		if ( surf->material->GetSort() >= SS_AFTER_FOG )
			break;

		if ( surf->space != backEnd.currentSpace ) {
			if ( GLSLProgram::GetCurrentProgram() != nullptr ) {
				Uniforms::Global* transformUniforms = GLSLProgram::GetCurrentProgram()->GetUniformGroup<Uniforms::Global>();
				transformUniforms->Set( surf->space );
			}
		}

		vertexCache.VertexPosition( surf->ambientCache );

		extern void RB_CreateMultiDrawInteractions( const drawSurf_t *surf );
		RB_CreateMultiDrawInteractions( surf );
	}

	GLSLProgram::Deactivate();

	GL_SelectTexture( 0 );

	for ( backEnd.vLight = backEnd.viewDef->viewLights; backEnd.vLight; backEnd.vLight = backEnd.vLight->next ) {
		if ( backEnd.vLight->singleLightOnly ) {
			extern void RB_GLSL_DrawInteractions_SingleLight();
			RB_GLSL_DrawInteractions_SingleLight();
			backEnd.pc.c_interactionSingleLights++;
		}
	}

	// FIXME temporary experimental code
	/*if ( backEnd.viewDef->viewEntitys && !backEnd.viewDef->isSubview ) {
		static int lightHistoryIndex = 0;
		softLightSamples[lightHistoryIndex] = backEnd.viewDef->lightSample;
		FB_ToggleShadow( true );
		globalImages->shadowAtlasHistory->Bind();
		qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, lightHistoryIndex * r_shadowMapSize, 0, 0, r_shadowMapSize * 6, r_shadowMapSize );
		FB_ToggleShadow( false );
		lightHistoryIndex = (lightHistoryIndex + 1) % 8;
	}*/
}
