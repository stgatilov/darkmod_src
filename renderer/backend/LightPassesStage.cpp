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

#include "LightPassesStage.h"

#include "RenderBackend.h"
#include "../GLSLProgramManager.h"
#include "../GLSLProgram.h"
#include "../FrameBuffer.h"
#include "../GLSLUniforms.h"

struct BlendLightUniforms : GLSLUniformGroup {
	UNIFORM_GROUP_DEF( BlendLightUniforms )

	DEFINE_UNIFORM( mat4, modelViewMatrix )
	DEFINE_UNIFORM( mat4, projectionMatrix )
	DEFINE_UNIFORM( vec4, blendColor )
	DEFINE_UNIFORM( mat4, lightProjectionFalloff )
	DEFINE_UNIFORM( vec4/*[2]*/, lightTextureMatrix )
	DEFINE_UNIFORM( sampler, lightFalloffTexture )
	DEFINE_UNIFORM( sampler, lightProjectionTexture )
};	

struct FogLightUniforms : GLSLUniformGroup {
	UNIFORM_GROUP_DEF( FogLightUniforms )

	DEFINE_UNIFORM( mat4, modelViewMatrix )
	DEFINE_UNIFORM( mat4, projectionMatrix )
	DEFINE_UNIFORM( vec4, texPlaneFalloffS )
	DEFINE_UNIFORM( vec4, texPlaneFalloffT )
	DEFINE_UNIFORM( vec4, texPlaneEnterS )
	DEFINE_UNIFORM( vec4, texPlaneEnterT )
	DEFINE_UNIFORM( vec3, fogColor )
	DEFINE_UNIFORM( float, fogAlpha )
	DEFINE_UNIFORM( sampler, fogFalloffImage )
	DEFINE_UNIFORM( sampler, fogEnterImage )
};	

static GLSLProgram *LoadShader(const idStr &name) {
	// TODO: delete the "new" suffix after old backend code is deleted
	return programManager->LoadFromFiles( name + "_new", "stages/light_passes/" + name + ".vert.glsl", "stages/light_passes/" + name + ".frag.glsl" );
}

void LightPassesStage::Init() {
	blendLightShader = LoadShader( "blend_light" );
	fogLightShader = LoadShader( "fog_light" );
}

void LightPassesStage::Shutdown() {}

void LightPassesStage::DrawAllFogLights( const viewDef_t *viewDef, const DrawMask &mask ) {
	if ( r_skipFogLights.GetInteger() == 7 ) {	// full bitmask
		return;
	}

	TRACE_GL_SCOPE( "LightPassesStage:Fog" );
	qglDisable( GL_STENCIL_TEST );
	qglDisable( GL_SCISSOR_TEST );

	for ( viewLight_t *vLight = viewDef->viewLights; vLight; vLight = vLight->next ) {
		if ( vLight->lightShader->IsFogLight() ) {
			this->viewDef = viewDef;
			this->vLight = vLight;

			DrawFogLight( mask );
		}
	}

	qglEnable( GL_SCISSOR_TEST );
	qglEnable( GL_STENCIL_TEST );
	GL_SelectTexture( 0 );
}

void LightPassesStage::DrawAllBlendLights( const viewDef_t *viewDef ) {
	if ( r_skipBlendLights.GetBool() ) {
		return;
	}

	TRACE_GL_SCOPE( "LightPassesStage:Blend" );
	qglDisable( GL_STENCIL_TEST );

	for ( viewLight_t *vLight = viewDef->viewLights; vLight; vLight = vLight->next ) {
		if ( vLight->lightShader->IsBlendLight() ) {
			this->viewDef = viewDef;
			this->vLight = vLight;

			DrawBlendLight();
		}
	}

	qglEnable( GL_STENCIL_TEST );
	GL_SelectTexture( 0 );
}

void LightPassesStage::DrawFogLight( const DrawMask &mask ) {
	// create a surface for the light frustom triangles, which are oriented drawn side out
 	const srfTriangles_t *frustumTris = vLight->frustumTris;
	if ( !frustumTris->ambientCache.IsValid() )
		return;

	// find the current color and density of the fog
	const idMaterial *lightShader = vLight->lightShader;

	// assume fog shaders have only a single stage
	const shaderStage_t *stage = lightShader->GetStage( 0 );
	idVec4 color = vLight->GetStageColor( stage );

	// calculate the falloff planes
	float a;
	if ( color.w <= 1.0 ) {
		// if they left the default value on, set a fog distance of 500
		a = -0.5f / DEFAULT_FOG_DISTANCE;
	} else {
		// otherwise, distance = alpha color
		a = -0.5f / color.w;
	}

	idMat4 viewMatrix = idMat4::FromGL( viewDef->worldSpace.modelViewMatrix );
	idVec4 fogPlanes[4];	// TODO: what is this trash?
	fogPlanes[0] = a * viewMatrix[2];
	fogPlanes[1] = a * viewMatrix[0];
	// T will get a texgen for the fade plane, which is always the "top" plane on unrotated lights
	fogPlanes[2] = 0.001f * vLight->fogPlane.ToVec4();
	// S is based on the view origin
	float s = viewDef->renderView.vieworg * fogPlanes[2].ToVec3() + fogPlanes[2][3];
	fogPlanes[3].Set(0, 0, 0, FOG_ENTER + s);

	fogLightShader->Activate();
	FogLightUniforms *uniforms = fogLightShader->GetUniformGroup<FogLightUniforms>();
	uniforms->projectionMatrix.Set( viewDef->projectionMatrix );

	uniforms->fogColor.Set( color.ToVec3() );

	// texture 0 is the falloff image
	GL_SelectTexture( 0 );
	globalImages->fogImage->Bind();
	uniforms->fogFalloffImage.Set( 0 );
	// texture 1 is the entering plane fade correction
	GL_SelectTexture( 1 );
	globalImages->fogEnterImage->Bind();
	uniforms->fogEnterImage.Set( 1 );

	auto perSurface = [&](const drawSurf_t *surf) {
		uniforms->modelViewMatrix.Set( surf->space->modelViewMatrix );

		if ( surf->material && surf->material->Coverage() == MC_TRANSLUCENT && surf->material->ReceivesLighting() )
			uniforms->fogAlpha.Set( surf->material->FogAlpha() );
		else
			uniforms->fogAlpha.Set( 1 );

		idPlane	local;
		R_GlobalPlaneToLocal( surf->space->modelMatrix, ((idPlane*)fogPlanes)[0], local );
		local[3] += 0.5;
		uniforms->texPlaneFalloffS.Set( local );
		uniforms->texPlaneFalloffT.Set( 0, 0, 0, 0.5 );

		R_GlobalPlaneToLocal( surf->space->modelMatrix, ((idPlane*)fogPlanes)[3], local );
		uniforms->texPlaneEnterS.Set( local );
		// GL_S is constant per viewer
		R_GlobalPlaneToLocal( surf->space->modelMatrix, ((idPlane*)fogPlanes)[2], local );
		local[3] += FOG_ENTER;
		uniforms->texPlaneEnterT.Set( local );
	};

	if ( mask.opaque ) {
		if ( !( r_skipFogLights.GetInteger() & 1 ) ) {
			GL_State( GLS_DEPTHMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL );
			GL_Cull( CT_FRONT_SIDED );
			for ( drawSurf_t *surf = vLight->globalInteractions; surf; surf = surf->nextOnLight )
				DrawSurf( surf, perSurface );
			for ( drawSurf_t *surf = vLight->localInteractions; surf; surf = surf->nextOnLight )
				DrawSurf( surf, perSurface );
		}

		if ( !vLight->noFogBoundary && !( r_skipFogLights.GetInteger() & 4 ) ) { // Let mappers suppress fogging the bounding box -- SteveL #3664
			drawSurf_t ds;
			memset( &ds, 0, sizeof( ds ) );
			ds.space = &viewDef->worldSpace;
			//ds.backendGeo = frustumTris;
			ds.frontendGeo = frustumTris; // FIXME JIC
			ds.numIndexes = frustumTris->numIndexes;
			ds.indexCache = frustumTris->indexCache;
			ds.ambientCache = frustumTris->ambientCache;
			ds.scissorRect = viewDef->scissor;
			// the light frustum bounding planes aren't in the depth buffer, so use depthfunc_less instead
			GL_State( GLS_DEPTHMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_LESS );
			GL_Cull( CT_BACK_SIDED );
			DrawSurf( &ds, perSurface );
		}
	}

	if ( mask.translucent ) {
		if ( !( r_skipFogLights.GetInteger() & 2 ) ) {
			GL_State( GLS_DEPTHMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_LESS );
			GL_Cull( CT_TWO_SIDED );
			for ( drawSurf_t *surf = vLight->translucentInteractions; surf; surf = surf->nextOnLight )
				DrawSurf( surf, perSurface );
		}
	}

	GL_Cull( CT_FRONT_SIDED );
	GL_SelectTexture( 0 );
	GLSLProgram::Deactivate();
}

void LightPassesStage::DrawBlendLight() {
	// blend light is limited to its volume in fragment shader
	// so we can ignore everything outside light volume
	backEnd.currentScissor = vLight->scissorRect;
	FB_ApplyScissor();

	// Dual texture together the falloff and projection texture with a blend
	// mode to the framebuffer, instead of interacting with the surface texture
	const idMaterial *lightShader = vLight->lightShader;

	blendLightShader->Activate();
	BlendLightUniforms *uniforms = blendLightShader->GetUniformGroup<BlendLightUniforms>();
	uniforms->projectionMatrix.Set( viewDef->projectionMatrix );

	// texture 1: falloff texture
	GL_SelectTexture( 1 );
	vLight->falloffImage->Bind();
	uniforms->lightFalloffTexture.Set( 1 );

	for ( int i = 0 ; i < lightShader->GetNumStages(); i++ ) {
		const shaderStage_t *stage = lightShader->GetStage( i );
		if ( !vLight->IsStageEnabled( stage ) )
			continue;

		GL_State( GLS_DEPTHMASK | stage->drawStateBits | GLS_DEPTHFUNC_EQUAL );

		// sanity check: only allow blend modes where src = (0,0,0,0) causes no change in framebuffer
		int dstBlendFactor = stage->drawStateBits & GLS_DSTBLEND_BITS;
		if ( !(dstBlendFactor == GLS_DSTBLEND_ONE || dstBlendFactor == GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA || dstBlendFactor == GLS_DSTBLEND_ONE_MINUS_SRC_COLOR) )
			continue;	// avoid bad behavior, ugly scissor boundaries, etc.

		// texture 0: projected texture
		GL_SelectTexture( 0 );
		stage->texture.image->Bind();
		uniforms->lightProjectionTexture.Set( 0 );

		// get the modulate values from the light, including alpha, unlike normal lights
		uniforms->blendColor.Set( vLight->GetStageColor( stage ) );

		idMat4 lightTexMatrix = vLight->GetTextureMatrix( stage );
		uniforms->lightTextureMatrix.SetArray( 2, lightTexMatrix.ToFloatPtr() );	// rows 0 and 1

		auto perSurface = [uniforms, this](const drawSurf_t *drawSurf) {
			uniforms->modelViewMatrix.Set( drawSurf->space->modelViewMatrix );
			idPlane lightProject[4];
			R_GlobalPlaneToLocal( drawSurf->space->modelMatrix, vLight->lightProject[0], lightProject[0] );
			R_GlobalPlaneToLocal( drawSurf->space->modelMatrix, vLight->lightProject[1], lightProject[1] );
			R_GlobalPlaneToLocal( drawSurf->space->modelMatrix, vLight->lightProject[2], lightProject[2] );
			R_GlobalPlaneToLocal( drawSurf->space->modelMatrix, vLight->lightProject[3], lightProject[3] );
			uniforms->lightProjectionFalloff.Set( (float*)lightProject );
		};
		// render opaque surfaces touching this light
		for ( drawSurf_t *surf = vLight->globalInteractions; surf; surf = surf->nextOnLight )
			DrawSurf( surf, perSurface );
		for ( drawSurf_t *surf = vLight->localInteractions; surf; surf = surf->nextOnLight )
			DrawSurf( surf, perSurface );
	}
}

void LightPassesStage::DrawSurf( const drawSurf_t *drawSurf, std::function<void(const drawSurf_t *)> perSurfaceSetup ) {
	ApplyDepthTweaks depthTweaks( drawSurf );

	if ( !drawSurf->ambientCache.IsValid() )
		return;
	vertexCache.VertexPosition( drawSurf->ambientCache );

	perSurfaceSetup( drawSurf );	// most importantly: modelview
	RB_DrawElementsWithCounters( drawSurf );
}
