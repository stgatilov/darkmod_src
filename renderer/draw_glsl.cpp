/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "precompiled.h"
#pragma hdrstop

#include "tr_local.h"
#include "glsl.h"
#include "FrameBuffer.h"
#include "Profiling.h"

#if defined(_MSC_VER) && _MSC_VER >= 1800 && !defined(DEBUG)
//#pragma optimize("t", off) // duzenko: used in release to enforce breakpoints in inlineable code. Please do not remove
#endif

struct basicInteractionProgram_t : lightProgram_t {
	GLint lightProjectionFalloff, bumpMatrix, diffuseMatrix, specularMatrix;
	GLint colorModulate, colorAdd;

	virtual	void AfterLoad();
	virtual void UpdateUniforms( bool translucent ) {}
	virtual void UpdateUniforms( const drawInteraction_t *din );
};

struct interactionProgram_t : basicInteractionProgram_t {
	GLint localViewOrigin;
	GLint rgtc;

	GLint cubic;
	GLint lightProjectionCubemap, lightProjectionTexture, lightFalloffTexture;

	GLint diffuseColor, specularColor;

	virtual	void AfterLoad();
	virtual void UpdateUniforms( bool translucent ) {}
	virtual void UpdateUniforms( const drawInteraction_t *din );
	static void ChooseInteractionProgram();
};

struct pointInteractionProgram_t : interactionProgram_t {
	GLint advanced, shadows, lightOrigin2;
	GLint softShadowsQuality, softShadowsRadius, softShadowSamples, shadowRect, renderResolution;
	GLint shadowMap, stencilTexture, depthTexture;
	//TODO: is this global variable harming multithreading?
	idList<idVec2> g_softShadowsSamples;
	virtual	void AfterLoad();
	virtual void UpdateUniforms( bool translucent );
	virtual void UpdateUniforms( const drawInteraction_t *din );
};

struct ambientInteractionProgram_t : interactionProgram_t {
	GLint minLevel, gamma, lightFalloffCubemap;
	virtual	void AfterLoad();
	virtual void UpdateUniforms( const drawInteraction_t *din );
};

struct shadowMapProgram_t : basicDepthProgram_t {
	GLint lightOrigin, lightRadius, modelMatrix;
	GLint lightCount, shadowRect, shadowTexelStep; // multi-light stuff
	virtual	void AfterLoad();
	void RenderAllLights();
	void RenderAllLights(drawSurf_t *surf);
};

struct MultiLightShaderData { // used by both interaction and shadow map shaders
	std::vector<viewLight_t *> vLights;
	std::vector<idVec3> lightOrigins;
	std::vector<idVec4> shadowRects;
	std::vector<float> softShadowRads;
	MultiLightShaderData( const drawSurf_t *surf, bool shadowPass );
};

struct multiLightInteractionProgram_t : basicInteractionProgram_t {
	static const uint MAX_LIGHTS = 16;
	GLint lightCount, lightOrigin, lightColor, shadowRect, softShadowsRadius;
	GLint minLevel, gamma;
	virtual	void AfterLoad();
	virtual void Draw( const drawInteraction_t *din );
};

shaderProgram_t cubeMapShader;
oldStageProgram_t oldStageShader;
depthProgram_t depthShader;
lightProgram_t stencilShadowShader;
shadowMapProgram_t shadowMapShader, shadowMapMultiShader;
fogProgram_t fogShader;
blendProgram_t blendShader;
pointInteractionProgram_t pointInteractionShader;
ambientInteractionProgram_t ambientInteractionShader;
multiLightInteractionProgram_t multiLightShader;

interactionProgram_t *currrentInteractionShader; // dynamic, either pointInteractionShader or ambientInteractionShader

std::map<idStr, shaderProgram_t*> dynamicShaders; // shaders referenced from materials, stored by their file names

idCVar r_shadowMapSinglePass( "r_shadowMapSinglePass", "1", CVAR_ARCHIVE | CVAR_RENDERER, "render shadow maps for all lights in a single pass" );

/*
==================
RB_GLSL_DrawInteraction
==================
*/
void RB_GLSL_DrawInteraction( const drawInteraction_t *din ) {
	// load all the shader parameters
	GL_CheckErrors();
	currrentInteractionShader->UpdateUniforms( din );

	// set the textures
	// texture 0 will be the per-surface bump map
	GL_SelectTexture( 0 );
	din->bumpImage->Bind();

	// texture 1 will be the light falloff texture
	GL_SelectTexture( 1 );
	din->lightFalloffImage->Bind();

	// texture 2 will be the light projection texture
	GL_SelectTexture( 2 );
	din->lightImage->Bind();

	// texture 3 is the per-surface diffuse map
	GL_SelectTexture( 3 );
	din->diffuseImage->Bind();

	// texture 4 is the per-surface specular map
	GL_SelectTexture( 4 );
	din->specularImage->Bind();

	if( !(r_shadows.GetInteger() == 2 && backEnd.vLight->tooBigForShadowMaps) ) // special case - no softening
	if ( r_softShadowsQuality.GetBool() && !backEnd.viewDef->IsLightGem() || r_shadows.GetInteger() == 2 ) {
		FB_BindShadowTexture();
	}

	// draw it
	GL_CheckErrors();
	RB_DrawElementsWithCounters( din->surf );
	GL_CheckErrors();
}

/*
=============
RB_GLSL_CreateDrawInteractions
=============
*/
void RB_GLSL_CreateDrawInteractions( const drawSurf_t *surf ) {
	if ( !surf ) {
		return;
	}
	GL_PROFILE( "GLSL_CreateDrawInteractions" );

	// perform setup here that will be constant for all interactions
	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | backEnd.depthFunc );

	// bind the vertex and fragment program
	interactionProgram_t::ChooseInteractionProgram();
	currrentInteractionShader->UpdateUniforms( surf == backEnd.vLight->translucentInteractions );

	// enable the vertex arrays
	qglEnableVertexAttribArray( 8 );
	qglEnableVertexAttribArray( 9 );
	qglEnableVertexAttribArray( 10 );
	qglEnableVertexAttribArray( 11 );
	qglEnableVertexAttribArray( 3 );

	for ( /**/; surf; surf = surf->nextOnLight ) {
		if ( surf->dsFlags & DSF_SHADOW_MAP_ONLY ) {
			continue;
		}
		if ( backEnd.currentSpace != surf->space ) // FIXME needs a better integration with RB_CreateSingleDrawInteractions
			qglUniformMatrix4fv( currrentInteractionShader->modelMatrix, 1, false, surf->space->modelMatrix );

		// set the vertex pointers
		idDrawVert	*ac = ( idDrawVert * )vertexCache.VertexPosition( surf->ambientCache );
		qglVertexAttribPointer( 3, 4, GL_UNSIGNED_BYTE, true, sizeof( idDrawVert ), &ac->color );
		qglVertexAttribPointer( 11, 3, GL_FLOAT, false, sizeof( idDrawVert ), ac->normal.ToFloatPtr() );
		qglVertexAttribPointer( 10, 3, GL_FLOAT, false, sizeof( idDrawVert ), ac->tangents[1].ToFloatPtr() );
		qglVertexAttribPointer( 9, 3, GL_FLOAT, false, sizeof( idDrawVert ), ac->tangents[0].ToFloatPtr() );
		qglVertexAttribPointer( 8, 2, GL_FLOAT, false, sizeof( idDrawVert ), ac->st.ToFloatPtr() );
		qglVertexAttribPointer( 0, 3, GL_FLOAT, false, sizeof( idDrawVert ), ac->xyz.ToFloatPtr() );

		// this may cause RB_GLSL_DrawInteraction to be executed multiple
		// times with different colors and images if the surface or light have multiple layers
		RB_CreateSingleDrawInteractions( surf );
	}
	qglDisableVertexAttribArray( 8 );
	qglDisableVertexAttribArray( 9 );
	qglDisableVertexAttribArray( 10 );
	qglDisableVertexAttribArray( 11 );
	qglDisableVertexAttribArray( 3 );

	// disable features
	if ( r_softShadowsQuality.GetBool() && !backEnd.viewDef->IsLightGem() || r_shadows.GetInteger() == 2 ) {
		GL_SelectTexture( 6 );
		globalImages->BindNull();
		GL_SelectTexture( 7 );
		globalImages->BindNull();
	}
	GL_SelectTexture( 4 );
	globalImages->BindNull();

	GL_SelectTexture( 3 );
	globalImages->BindNull();

	GL_SelectTexture( 2 );
	globalImages->BindNull();

	GL_SelectTexture( 1 );
	globalImages->BindNull();

	GL_SelectTexture( 0 );

	qglUseProgram( 0 );
	GL_CheckErrors();
}

/*
==================
RB_GLSL_DrawLight_Stencil
==================
*/
void RB_GLSL_DrawLight_Stencil() {
	GL_PROFILE( "GLSL_DrawLight_Stencil" );

	bool useShadowFbo = r_softShadowsQuality.GetBool() && !backEnd.viewDef->IsLightGem() && (r_shadows.GetInteger() != 2);

	// set depth bounds for the whole light
	if ( backEnd.useLightDepthBounds ) {
		GL_DepthBoundsTest( backEnd.vLight->scissorRect.zmin, backEnd.vLight->scissorRect.zmax );
	}

	// clear the stencil buffer if needed
	if ( backEnd.vLight->globalShadows || backEnd.vLight->localShadows ) {
		backEnd.currentScissor = backEnd.vLight->scissorRect;

		if ( r_useScissor.GetBool() ) {
			GL_Scissor( backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
			            backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
			            backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
			            backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1 );
		}

		if ( useShadowFbo ) {
			FB_ToggleShadow( true );
		}
		qglClear( GL_STENCIL_BUFFER_BIT );
	} else {
		// no shadows, so no need to read or write the stencil buffer
		qglStencilFunc( GL_ALWAYS, 128, 255 );
	}
	stencilShadowShader.Use();

	RB_StencilShadowPass( backEnd.vLight->globalShadows );
	if ( useShadowFbo && r_multiSamples.GetInteger() > 1 && r_softShadowsQuality.GetInteger() >= 0 ) {
		FB_ResolveShadowAA();
	}

	const bool NoSelfShadows = true; // don't delete - debug check for low-poly "round" models casting ugly shadows on themselves

	if ( NoSelfShadows ) {
		if ( useShadowFbo ) {
			FB_ToggleShadow( false );
		}
		RB_GLSL_CreateDrawInteractions( backEnd.vLight->localInteractions );

		if ( useShadowFbo ) {
			FB_ToggleShadow( true );
		}
	}
	stencilShadowShader.Use();

	RB_StencilShadowPass( backEnd.vLight->localShadows );
	if ( useShadowFbo && r_multiSamples.GetInteger() > 1 && r_softShadowsQuality.GetInteger() >= 0 ) {
		FB_ResolveShadowAA();
	}


	if ( useShadowFbo ) {
		FB_ToggleShadow( false );
	}

	if ( !NoSelfShadows ) {
		RB_GLSL_CreateDrawInteractions( backEnd.vLight->localInteractions );
	}
	RB_GLSL_CreateDrawInteractions( backEnd.vLight->globalInteractions );

	// reset depth bounds
	if ( backEnd.useLightDepthBounds ) {
		GL_DepthBoundsTest( 0.0f, 0.0f );
	}
	qglUseProgram( 0 );	// if there weren't any globalInteractions, it would have stayed on
}


static float GetEffectiveLightRadius() {
	float lightRadius = backEnd.vLight->lightDef->parms.radius;
	if (r_softShadowsRadius.GetFloat() < 0.0)
		lightRadius = -r_softShadowsRadius.GetFloat();	//override
	else if (lightRadius < 0.0)
		lightRadius = r_softShadowsRadius.GetFloat();	//default value
	return lightRadius;
}

/*
=============
RB_GLSL_CreateDrawInteractions
=============
*/
void RB_GLSL_DrawInteractions_ShadowMap( const drawSurf_t *surf, bool clear = false ) {
	if ( r_shadowMapSinglePass.GetBool() ) 
		return;
	GL_PROFILE( "GLSL_DrawInteractions_ShadowMap" );

	FB_ToggleShadow( true );

	shadowMapShader.Use();
	GL_SelectTexture( 0 );

	qglUniform4fv( shadowMapShader.lightOrigin, 1, backEnd.vLight->globalLightOrigin.ToFloatPtr() );
	qglUniform1f( shadowMapShader.lightRadius, GetEffectiveLightRadius() );
	backEnd.currentSpace = NULL;

	GL_Cull( CT_TWO_SIDED );
	qglPolygonOffset( 0, 0 );
	qglEnable( GL_POLYGON_OFFSET_FILL );

	auto &page = ShadowAtlasPages[backEnd.vLight->shadowMapIndex-1];
	qglViewport( page.x, page.y, 6*page.width, page.width );
	if ( r_useScissor.GetBool() )
		GL_Scissor( page.x, page.y, 6*page.width, page.width );
	if ( clear )
		qglClear( GL_DEPTH_BUFFER_BIT );
	for ( int i = 0; i < 4; i++ )
		qglEnable( GL_CLIP_PLANE0 + i );
	for ( ; surf; surf = surf->nextOnLight ) {
		if ( !surf->material->SurfaceCastsShadow() ) 
			continue;    // some dynamic models use a no-shadow material and for shadows have a separate geometry with an invisible (in main render) material

		if ( surf->dsFlags & DSF_SHADOW_MAP_IGNORE ) 
			continue;    // this flag is set by entities with parms.noShadow in R_LinkLightSurf (candles, torches, etc)

		float customOffset = surf->space->entityDef->parms.shadowMapOffset + surf->material->GetShadowMapOffset();
		if ( customOffset != 0 )
			qglPolygonOffset( customOffset, 0 );

		if ( backEnd.currentSpace != surf->space ) {
			qglUniformMatrix4fv( shadowMapShader.modelMatrix, 1, false, surf->space->modelMatrix );
			backEnd.currentSpace = surf->space;
			backEnd.pc.c_matrixLoads++;
		}

		shadowMapShader.FillDepthBuffer( surf );

		if ( customOffset != 0 )
			qglPolygonOffset( 0, 0 );
	}
	for ( int i = 0; i < 4; i++ )
		qglDisable( GL_CLIP_PLANE0 + i );

	qglDisable( GL_POLYGON_OFFSET_FILL );
	GL_Cull( CT_FRONT_SIDED );

	backEnd.currentSpace = NULL; // or else conflicts with qglLoadMatrixf
	qglUseProgram( 0 );

	FB_ToggleShadow( false );

	GL_CheckErrors();
}

void RB_GLSL_GenerateShadowMaps() {
	if ( r_shadows.GetBool() == 0 )
		return;
	for ( backEnd.vLight = backEnd.viewDef->viewLights; backEnd.vLight; backEnd.vLight = backEnd.vLight->next ) {
		if ( !backEnd.vLight->shadowMapIndex || backEnd.vLight->singleLightOnly )
			continue;
		RB_GLSL_DrawInteractions_ShadowMap( backEnd.vLight->globalInteractions, true );
		RB_GLSL_DrawInteractions_ShadowMap( backEnd.vLight->localInteractions, false );
	}
}

/*
==================
RB_GLSL_DrawLight_ShadowMap
==================
*/
void RB_GLSL_DrawLight_ShadowMap() {
	GL_PROFILE( "GLSL_DrawLight_ShadowMap" );

	GL_CheckErrors();

	if ( backEnd.vLight->lightShader->LightCastsShadows() ) {
		RB_GLSL_DrawInteractions_ShadowMap( backEnd.vLight->globalInteractions, true );
		RB_GLSL_CreateDrawInteractions( backEnd.vLight->localInteractions );
		RB_GLSL_DrawInteractions_ShadowMap( backEnd.vLight->localInteractions );
	} else {
		RB_GLSL_CreateDrawInteractions( backEnd.vLight->localInteractions );
	}
	RB_GLSL_CreateDrawInteractions( backEnd.vLight->globalInteractions );

	qglUseProgram( 0 );

	GL_CheckErrors();
}

void RB_GLSL_DrawInteraction_MultiLight( const drawInteraction_t *din ) {
	// load all the shader parameters
	GL_CheckErrors();

	// set the textures
	// texture 0 will be the per-surface bump map
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

	multiLightShader.Draw( din );
}

void RB_GLSL_DrawInteractions_SingleLight() {
	// do fogging later
	if ( backEnd.vLight->lightShader->IsFogLight() ) {
		return;
	}

	if ( backEnd.vLight->lightShader->IsBlendLight() ) {
		return;
	}

	// if there are no interactions, get out!
	if ( r_singleLight.GetInteger() < 0 ) // duzenko 2018: I need a way to override this for debugging 
	if ( !backEnd.vLight->localInteractions && !backEnd.vLight->globalInteractions && !backEnd.vLight->translucentInteractions )
		return;

	if ( r_shadows.GetInteger() == 2 && !backEnd.vLight->tooBigForShadowMaps ) {
		RB_GLSL_DrawLight_ShadowMap();
	} else {
		RB_GLSL_DrawLight_Stencil();
	}

	// translucent surfaces never get stencil shadowed
	if ( r_skipTranslucent.GetBool() ) {
		return;
	}
	qglStencilFunc( GL_ALWAYS, 128, 255 );
	backEnd.depthFunc = GLS_DEPTHFUNC_LESS;
	RB_GLSL_CreateDrawInteractions( backEnd.vLight->translucentInteractions );
	backEnd.depthFunc = GLS_DEPTHFUNC_EQUAL;
}

void RB_GLSL_DrawInteractions_MultiLight() {
	if ( !backEnd.viewDef->viewLights )
		return;

	RB_GLSL_GenerateShadowMaps();

	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | backEnd.depthFunc );

	qglEnableVertexAttribArray( 3 );
	qglEnableVertexAttribArray( 8 );
	qglEnableVertexAttribArray( 9 );
	qglEnableVertexAttribArray( 10 );
	qglEnableVertexAttribArray( 11 );

	GL_SelectTexture( 5 );
	globalImages->shadowAtlas->Bind();

	multiLightShader.Use();

	backEnd.currentSpace = NULL; // shadow map shader uses a uniform instead of qglLoadMatrixf, needs reset

	for ( int i = 0; i < backEnd.viewDef->numDrawSurfs; i++ ) {
		auto surf = backEnd.viewDef->drawSurfs[i];
		auto material = surf->material;
		if ( material->SuppressInSubview() || material->GetSort() < SS_OPAQUE )
			continue;
		if ( surf->material->GetSort() >= SS_AFTER_FOG )
			break;

		if ( surf->space != backEnd.currentSpace ) {
			backEnd.currentSpace = surf->space;
			qglLoadMatrixf( surf->space->modelViewMatrix );
			qglUniformMatrix4fv( multiLightShader.modelMatrix, 1, false, surf->space->modelMatrix );
		}

		idDrawVert *ac = (idDrawVert *)vertexCache.VertexPosition( surf->ambientCache );
		qglVertexAttribPointer( 0, 3, GL_FLOAT, false, sizeof( idDrawVert ), ac->xyz.ToFloatPtr() );
		qglVertexAttribPointer( 3, 4, GL_UNSIGNED_BYTE, true, sizeof( idDrawVert ), &ac->color );
		qglVertexAttribPointer( 8, 2, GL_FLOAT, false, sizeof( idDrawVert ), ac->st.ToFloatPtr() );
		qglVertexAttribPointer( 9, 3, GL_FLOAT, false, sizeof( idDrawVert ), ac->tangents[0].ToFloatPtr() );
		qglVertexAttribPointer( 10, 3, GL_FLOAT, false, sizeof( idDrawVert ), ac->tangents[1].ToFloatPtr() );
		qglVertexAttribPointer( 11, 3, GL_FLOAT, false, sizeof( idDrawVert ), ac->normal.ToFloatPtr() );

		extern void RB_CreateMultiDrawInteractions( const drawSurf_t *surf );
		RB_CreateMultiDrawInteractions( surf );
	}

	qglUseProgram( 0 );

	GL_SelectTexture( 5 );
	globalImages->BindNull();

	GL_SelectTexture( 4 );
	globalImages->BindNull();
	GL_SelectTexture( 3 );
	globalImages->BindNull();
	GL_SelectTexture( 2 );
	globalImages->BindNull();
	GL_SelectTexture( 1 );
	globalImages->BindNull();

	GL_SelectTexture( 0 );

	qglDisableVertexAttribArray( 3 );
	qglDisableVertexAttribArray( 8 );
	qglDisableVertexAttribArray( 9 );
	qglDisableVertexAttribArray( 10 );
	qglDisableVertexAttribArray( 11 );
}

/*
==================
RB_GLSL_DrawInteractions
==================
*/
void RB_GLSL_DrawInteractions() {
	GL_PROFILE( "GLSL_DrawInteractions" );
	GL_SelectTexture( 0 );

	if ( r_shadows.GetInteger() == 2 ) {
		// assign shadow pages and prepare lights for single/multi processing // singleLightOnly flag is now set in frontend
		for ( backEnd.vLight = backEnd.viewDef->viewLights; backEnd.vLight; backEnd.vLight = backEnd.vLight->next ) {
			auto shader = backEnd.vLight->lightShader;
			if ( shader->LightCastsShadows() && !backEnd.vLight->tooBigForShadowMaps )
				backEnd.vLight->shadowMapIndex = ++ShadowAtlasIndex;
		}
		ShadowAtlasIndex = 0; // reset for next run

		if ( r_shadowMapSinglePass.GetBool() )
			shadowMapMultiShader.RenderAllLights();

		if ( r_testARBProgram.GetInteger() == 2 ) {
			RB_GLSL_DrawInteractions_MultiLight();
			for ( backEnd.vLight = backEnd.viewDef->viewLights; backEnd.vLight; backEnd.vLight = backEnd.vLight->next ) {
				if ( backEnd.vLight->singleLightOnly ) {
					RB_GLSL_DrawInteractions_SingleLight();
					backEnd.pc.c_interactionSingleLights++;
				}
			}
			return;
		}
	}

	// for each light, perform adding and shadowing
	for ( backEnd.vLight = backEnd.viewDef->viewLights; backEnd.vLight; backEnd.vLight = backEnd.vLight->next ) 
		RB_GLSL_DrawInteractions_SingleLight();

	// disable stencil shadow test
	qglStencilFunc( GL_ALWAYS, 128, 255 );
	GL_SelectTexture( 0 );
}

/*
==================
R_ReloadGLSLPrograms

If the 'required' shaders fail to compile the r_useGLSL will toggle to 0 so as to fall back to ARB2 shaders
filenames hardcoded here since they're not used elsewhere
FIXME split the stencil and shadowmap interactions in separate shaders as the latter might not compile on DX10 and older hardware
==================
*/
bool R_ReloadGLSLPrograms() { 
	bool ok = true;
	ok &= pointInteractionShader.Load( "interactionA" ); 
	ok &= ambientInteractionShader.Load( "ambientInteraction" );
	ok &= stencilShadowShader.Load( "stencilShadow" );
	ok &= shadowMapShader.Load( "shadowMapA" );
	ok &= oldStageShader.Load( "oldStage" );
	ok &= depthShader.Load( "depthAlpha" );
	ok &= fogShader.Load( "fog" );
	ok &= blendShader.Load( "blend" );
	ok &= cubeMapShader.Load( "cubeMap" );
	// these are optional and don't "need" to build
	multiLightShader.Load( "interactionN" );
	shadowMapMultiShader.Load( "shadowMapN" );
	for ( auto it = dynamicShaders.begin(); it != dynamicShaders.end(); ++it ) {
		auto& fileName = it->first;
		auto& shader = it->second;
		shader->Load( fileName );
	}
	return ok;
}

/*
==================
R_ReloadGLSLPrograms_f
==================
*/
void R_ReloadGLSLPrograms_f( const idCmdArgs &args ) {
	common->Printf( "---------- R_ReloadGLSLPrograms_f -----------\n" );

	if ( !R_ReloadGLSLPrograms() ) {
		r_useGLSL.SetBool( false );
		common->Printf( "GLSL shaders failed to init.\n" );
		return;
	}
	common->Printf( "---------------------------------\n" );
}

int R_FindGLSLProgram( const char *program ) {
	auto iter = dynamicShaders.find( program );
	if( iter == dynamicShaders.end() ) {
		auto shader = new shaderProgram_t();
		shader->Load( program );
		dynamicShaders[program] = shader;
		return shader->program;
	} else
		return iter->second->program;
}

/*
=================
shaderProgram_t::CompileShader
=================
*/
GLuint shaderProgram_t::CompileShader( GLint ShaderType, const char *fileName ) {
	char *source;
	GLuint shader;
	GLint length, result;

	/* get shader source */
	char    *fileBuffer;

	// load the program even if we don't support it
	fileSystem->ReadFile( fileName, ( void ** )&fileBuffer, NULL );

	if ( !fileBuffer ) {
		if ( ShaderType != GL_GEOMETRY_SHADER ) {
			common->Warning( "CompileShader(%s) file not found", fileName );
		}
		return 0;
	}

	switch ( ShaderType ) {
	case GL_VERTEX_SHADER:
		common->Printf( "V" );
		break;
	case GL_GEOMETRY_SHADER:
		common->Printf( "G" );
		break;
	case GL_FRAGMENT_SHADER:
		common->Printf( "F" );
		break;
	default:
		common->Warning( "Unknown ShaderType in shaderProgram_t::CompileShader" );
		break;
	}
	source = fileBuffer;

	/* create shader object, set the source, and compile */
	shader = qglCreateShader( ShaderType );
	length = ( GLint )strlen( source );
	qglShaderSource( shader, 1, ( const char ** )&source, &length );
	qglCompileShader( shader );
	fileSystem->FreeFile( fileBuffer );

	/* make sure the compilation was successful */
	qglGetShaderiv( shader, GL_COMPILE_STATUS, &result );

	char *log;
	/* get the shader info log */
	qglGetShaderiv( shader, GL_INFO_LOG_LENGTH, &length );
	log = new char[length + 1];
	log[0] = 0;
	qglGetShaderInfoLog( shader, length, NULL, log );
	//TODO: print compile log always (bad idea now due to tons of warnings)
	if (result == GL_FALSE)
		common->Warning( "CompileShader(%s): %s\n%s\n", fileName, (result ? "ok" : "FAILED"), log );
	delete log;

	if ( result == GL_FALSE ) {
		qglDeleteShader( shader );
		return 0;
	}
	return shader;
}

/*
=================
shaderProgram_t::AttachShader
=================
*/
void shaderProgram_t::AttachShader( GLint ShaderType, const char *fileName ) {
	idStr fn( "glprogs/" );
	fn.Append( fileName );
	switch ( ShaderType ) {
	case GL_VERTEX_SHADER:
		fn.Append( ".vs" );
		break;
	case GL_GEOMETRY_SHADER:
		fn.Append( ".gs" );
		break;
	case GL_FRAGMENT_SHADER:
		fn.Append( ".fs" );
		break;
	default:
		common->Warning( "Unknown ShaderType in shaderProgram_t::AttachShader" );
		break;
	}
	GLuint shader = CompileShader( ShaderType, fn.c_str() );

	if ( shader != 0 ) {
		/* attach the shader to the program */
		qglAttachShader( program, shader );

		/* delete the shader - it won't actually be
		* destroyed until the program that it's attached
		* to has been destroyed */
		qglDeleteShader( shader );
	}
}

/*
=================
shaderProgram_t::Load
=================
*/
bool shaderProgram_t::Load( const char *fileName ) {
	common->Printf( "%s ", fileName );

	if ( program && qglIsProgram( program ) ) {
		qglDeleteProgram( program );
	}
	program = qglCreateProgram();
	AttachShader( GL_VERTEX_SHADER, fileName );
	AttachShader( GL_GEOMETRY_SHADER, fileName );
	AttachShader( GL_FRAGMENT_SHADER, fileName );
	common->Printf( "\n" );
	qglBindAttribLocation( program, 3, "attr_Color" );
	qglBindAttribLocation( program, 8, "attr_TexCoord" );
	qglBindAttribLocation( program, 9, "attr_Tangent" );
	qglBindAttribLocation( program, 10, "attr_Bitangent" );
	qglBindAttribLocation( program, 11, "attr_Normal" );

	GLint result;/* link the program and make sure that there were no errors */
	qglLinkProgram( program );
	qglGetProgramiv( program, GL_LINK_STATUS, &result );

	if ( result != GL_TRUE ) {
		/* get the program info log */
		GLint length;
		qglGetProgramiv( program, GL_INFO_LOG_LENGTH, &length );
		char *log = new char[length];
		qglGetProgramInfoLog( program, length, &result, log );
		/* print an error message and the info log */
		common->Warning( "Program linking failed\n%s\n", log );
		delete log;

		/* delete the program */
		qglDeleteProgram( program );
		program = 0;
		return false;
	}
	AfterLoad();

	GLint validProgram;
	qglValidateProgram( program );
	qglGetProgramiv( program, GL_VALIDATE_STATUS, &validProgram );

	if ( !validProgram ) {
		/* get the program info log */
		GLint length;
		qglGetProgramiv( program, GL_INFO_LOG_LENGTH, &length );
		char *log = new char[length];
		qglGetProgramInfoLog( program, length, &result, log );
		/* print an error message and the info log */
		common->Warning( "Program validation failed\n%s\n", log );
		delete log;

		/* delete the program */
		qglDeleteProgram( program );
		program = 0;
		return false;
	}
	GL_CheckErrors();

	return true;
}

void shaderProgram_t::AfterLoad() {
	// or else abstract class error in primitive shaders (cubeMap)
}

void shaderProgram_t::Use() {
	qglUseProgram( program );
}

void oldStageProgram_t::AfterLoad() {
	screenTex = qglGetUniformLocation( program, "screenTex" );
	colorMul = qglGetUniformLocation( program, "colorMul" );
	colorAdd = qglGetUniformLocation( program, "colorAdd" );
}

void basicDepthProgram_t::AfterLoad() {
	color = qglGetUniformLocation( program, "color" );
	alphaTest = qglGetUniformLocation( program, "alphaTest" );
}

void depthProgram_t::AfterLoad() {
	basicDepthProgram_t::AfterLoad();
	clipPlane = qglGetUniformLocation( program, "clipPlane" );
	matViewRev = qglGetUniformLocation( program, "matViewRev" );
}

void blendProgram_t::AfterLoad() {
	tex0PlaneS = qglGetUniformLocation( program, "tex0PlaneS" );
	tex0PlaneT = qglGetUniformLocation( program, "tex0PlaneT" );
	tex0PlaneQ = qglGetUniformLocation( program, "tex0PlaneQ" );
	tex1PlaneS = qglGetUniformLocation( program, "tex1PlaneS" );
	texture1 = qglGetUniformLocation( program, "texture1" );
	blendColor = qglGetUniformLocation( program, "blendColor" );
}

void fogProgram_t::AfterLoad() {
	tex0PlaneS = qglGetUniformLocation( program, "tex0PlaneS" );
	tex1PlaneT = qglGetUniformLocation( program, "tex1PlaneT" );
	texture1 = qglGetUniformLocation( program, "texture1" );
	fogColor = qglGetUniformLocation( program, "fogColor" );
	fogEnter = qglGetUniformLocation( program, "fogEnter" );
}

void lightProgram_t::AfterLoad() {
	lightOrigin = qglGetUniformLocation( program, "u_lightOrigin" );
	modelMatrix = qglGetUniformLocation( program, "u_modelMatrix" );
}

void basicInteractionProgram_t::AfterLoad() {
	lightProgram_t::AfterLoad();
	bumpMatrix = qglGetUniformLocation( program, "u_bumpMatrix" );
	diffuseMatrix = qglGetUniformLocation( program, "u_diffuseMatrix" );
	specularMatrix = qglGetUniformLocation( program, "u_specularMatrix" );
	lightProjectionFalloff = qglGetUniformLocation( program, "u_lightProjectionFalloff" );
	colorModulate = qglGetUniformLocation( program, "u_colorModulate" );
	colorAdd = qglGetUniformLocation( program, "u_colorAdd" );
}

void basicInteractionProgram_t::UpdateUniforms( const drawInteraction_t *din ) {
	if ( din->surf->space != backEnd.currentSpace )
		qglUniformMatrix4fv( modelMatrix, 1, false, din->surf->space->modelMatrix );
	qglUniform4fv( diffuseMatrix, 2, din->diffuseMatrix[0].ToFloatPtr() );
	qglUniform4fv( bumpMatrix, 2, din->bumpMatrix[0].ToFloatPtr() );
	qglUniform4fv( specularMatrix, 2, din->specularMatrix[0].ToFloatPtr() );

	static const float	zero[4]		= { 0, 0, 0, 0 },
	                    one[4]		= { 1, 1, 1, 1 },
	                    negOne[4]	= { -1, -1, -1, -1 };
	switch ( din->vertexColor ) {
	case SVC_IGNORE:
		qglUniform4f( colorModulate, zero[0], zero[1], zero[2], zero[3] );
		qglUniform4f( colorAdd, one[0], one[1], one[2], one[3] );
		break;
	case SVC_MODULATE:
		qglUniform4f( colorModulate, one[0], one[1], one[2], one[3] );
		qglUniform4f( colorAdd, zero[0], zero[1], zero[2], zero[3] );
		break;
	case SVC_INVERSE_MODULATE:
		qglUniform4f( colorModulate, negOne[0], negOne[1], negOne[2], negOne[3] );
		qglUniform4f( colorAdd, one[0], one[1], one[2], one[3] );
		break;
	}
}

void interactionProgram_t::ChooseInteractionProgram() {
	if ( backEnd.vLight->lightShader->IsAmbientLight() ) {
		currrentInteractionShader = &ambientInteractionShader;
	} else {
		currrentInteractionShader = &pointInteractionShader;
	}
	currrentInteractionShader->Use();
	qglUniform1f( currrentInteractionShader->rgtc, globalImages->image_useNormalCompression.GetInteger() == 2 && glConfig.textureCompressionRgtcAvailable ? 1 : 0 );
	GL_CheckErrors();
}

void interactionProgram_t::AfterLoad() {
	basicInteractionProgram_t::AfterLoad();

	rgtc = qglGetUniformLocation( program, "u_RGTC" );

	localViewOrigin = qglGetUniformLocation( program, "u_viewOrigin" );

	diffuseColor = qglGetUniformLocation( program, "u_diffuseColor" );
	specularColor = qglGetUniformLocation( program, "u_specularColor" );

	cubic = qglGetUniformLocation( program, "u_cubic" );

	GLint normalTexture = qglGetUniformLocation( program, "u_normalTexture" );
	lightProjectionTexture = qglGetUniformLocation( program, "u_lightProjectionTexture" );
	lightProjectionCubemap = qglGetUniformLocation( program, "u_lightProjectionCubemap" );
	lightFalloffTexture = qglGetUniformLocation( program, "u_lightFalloffTexture" );
	GLint diffuseTexture = qglGetUniformLocation( program, "u_diffuseTexture" );
	GLint specularTexture = qglGetUniformLocation( program, "u_specularTexture" );

	// set texture locations
	qglUseProgram( program );

	// static bindings
	qglUniform1i( normalTexture, 0 );
	qglUniform1i( lightFalloffTexture, 1 );
	qglUniform1i( lightProjectionTexture, 2 );
	qglUniform1i( diffuseTexture, 3 );
	qglUniform1i( specularTexture, 4 );

	// can't have sampler2D, usampler2D, samplerCube have the same TMU index
	qglUniform1i( lightProjectionCubemap, 5 );
	qglUseProgram( 0 );
}

void interactionProgram_t::UpdateUniforms( const drawInteraction_t *din ) {
	basicInteractionProgram_t::UpdateUniforms( din );
	qglUniformMatrix4fv( lightProjectionFalloff, 1, false, din->lightProjection[0].ToFloatPtr() );
	// set the constant color
	qglUniform4fv( diffuseColor, 1, din->diffuseColor.ToFloatPtr() );
	qglUniform4fv( specularColor, 1, din->specularColor.ToFloatPtr() );
	if ( backEnd.vLight->lightShader->IsCubicLight() ) {
		qglUniform1f( cubic, 1.0 );
		qglUniform1i( lightProjectionTexture, MAX_MULTITEXTURE_UNITS );
		qglUniform1i( lightProjectionCubemap, 2 );
		qglUniform1i( lightFalloffTexture, MAX_MULTITEXTURE_UNITS );
	} else {
		qglUniform1f( cubic, 0.0 );
		qglUniform1i( lightProjectionTexture, 2 );
		qglUniform1i( lightProjectionCubemap, MAX_MULTITEXTURE_UNITS + 1 );
		qglUniform1i( lightFalloffTexture, 1 );
	}
	qglUniform4fv( localViewOrigin, 1, din->localViewOrigin.ToFloatPtr() );
}

void pointInteractionProgram_t::AfterLoad() {
	interactionProgram_t::AfterLoad();
	advanced = qglGetUniformLocation( program, "u_advanced" );
	shadows = qglGetUniformLocation( program, "u_shadows" );
	softShadowsQuality = qglGetUniformLocation( program, "u_softShadowsQuality" );
	softShadowsRadius = qglGetUniformLocation( program, "u_softShadowsRadius" );
	softShadowSamples = qglGetUniformLocation( program, "u_softShadowsSamples" );
	stencilTexture = qglGetUniformLocation( program, "u_stencilTexture" );
	depthTexture = qglGetUniformLocation( program, "u_depthTexture" );
	shadowMap = qglGetUniformLocation( program, "u_shadowMap" );
	renderResolution = qglGetUniformLocation( program, "u_renderResolution" );
	lightOrigin2 = qglGetUniformLocation( program, "u_lightOrigin2" );
	shadowRect = qglGetUniformLocation( program, "u_shadowRect" );

	// set texture locations
	qglUseProgram( program );

	// can't have sampler2D, usampler2D, samplerCube, samplerCubeShadow on the same TMU
	qglUniform1i( shadowMap, 6 );
	qglUniform1i( stencilTexture, 7 );
	qglUseProgram( 0 );
	g_softShadowsSamples.Clear();
}

void pointInteractionProgram_t::UpdateUniforms( bool translucent ) {
	qglUniform1f( advanced, r_testARBProgram.GetFloat() );

	auto vLight = backEnd.vLight;
	bool doShadows = !vLight->lightDef->parms.noShadows && vLight->lightShader->LightCastsShadows(); 
	if ( doShadows && r_shadows.GetInteger() == 2 ) // FIXME shadowmap only valid when globalInteractions not empty, otherwise garbage
		doShadows = vLight->globalInteractions != NULL;
	if ( doShadows ) {
		if(r_shadows.GetInteger() == 2 && vLight->tooBigForShadowMaps )
			qglUniform1f( shadows, 1 );
		else
			qglUniform1f( shadows, r_shadows.GetInteger() );
		auto &page = ShadowAtlasPages[vLight->shadowMapIndex-1];
		if ( 0 ) { // select the pixels to TexCoords method for interactionA.fs
			idVec4 v( page.x, page.y, 0, page.width );
			v /= 6 * r_shadowMapSize.GetFloat();
			qglUniform4fv( shadowRect, 1, v.ToFloatPtr() );
		} else { // https://stackoverflow.com/questions/5879403/opengl-texture-coordinates-in-pixel-space
			idVec4 v( page.x, page.y, 0, page.width-1 );
			v.ToVec2() = (v.ToVec2() * 2 + idVec2( 1, 1 )) / (2 * 6 * r_shadowMapSize.GetInteger());
			v.w /= 6 * r_shadowMapSize.GetFloat();
			qglUniform4fv( shadowRect, 1, v.ToFloatPtr() );
		}
	} else
		qglUniform1f( shadows, 0 );

	if ( !translucent && ( backEnd.vLight->globalShadows || backEnd.vLight->localShadows || r_shadows.GetInteger() == 2 ) && !backEnd.viewDef->IsLightGem() ) {
		qglUniform1i( softShadowsQuality, r_softShadowsQuality.GetInteger() );

		int sampleK = r_softShadowsQuality.GetInteger();
		if ( sampleK > 0 ) { // texcoords for screen-space softener filter
			if ( g_softShadowsSamples.Num() != sampleK || g_softShadowsSamples.Num() == 0 ) {
				GeneratePoissonDiskSampling( g_softShadowsSamples, sampleK );
				qglUniform2fv( softShadowSamples, sampleK, ( float * )g_softShadowsSamples.Ptr() );
			}
		}
		if ( sampleK < 0 ) { // WIP low res stencil shadows
			//qglUniform2f( renderResolution, glConfig.vidWidth, glConfig.vidHeight );
		}
	} else {
		qglUniform1i( softShadowsQuality, 0 );
	}
	qglUniform1f( softShadowsRadius, GetEffectiveLightRadius() ); // for soft stencil and all shadow maps
	if ( r_shadows.GetInteger() == 2 ) {
		qglUniform1i( shadowMap, 6 );
		qglUniform1i( depthTexture, MAX_MULTITEXTURE_UNITS );
		qglUniform1i( stencilTexture, MAX_MULTITEXTURE_UNITS + 2 );
	} else {
		qglUniform1i( shadowMap, MAX_MULTITEXTURE_UNITS + 2 );
		qglUniform1i( depthTexture, 6 );
		qglUniform1i( stencilTexture, 7 );
	}
	GL_CheckErrors();
}

void pointInteractionProgram_t::UpdateUniforms( const drawInteraction_t *din ) {
	interactionProgram_t::UpdateUniforms( din );
	qglUniform4fv( lightOrigin, 1, din->localLightOrigin.ToFloatPtr() );
	qglUniform3fv( lightOrigin2, 1, backEnd.vLight->globalLightOrigin.ToFloatPtr() );
	GL_CheckErrors();
}

void ambientInteractionProgram_t::AfterLoad() {
	interactionProgram_t::AfterLoad();
	minLevel = qglGetUniformLocation( program, "u_minLevel" );
	gamma = qglGetUniformLocation( program, "u_gamma" );
	lightFalloffCubemap = qglGetUniformLocation( program, "u_lightFalloffCubemap" );
	qglUseProgram( program );
	qglUniform1i( lightFalloffCubemap, 5 );
	qglUseProgram( 0 );
}

void ambientInteractionProgram_t::UpdateUniforms( const drawInteraction_t *din ) {
	interactionProgram_t::UpdateUniforms( din );
	qglUniform1f( minLevel, backEnd.viewDef->IsLightGem() ? 0 : r_ambientMinLevel.GetFloat() );
	qglUniform1f( gamma, backEnd.viewDef->IsLightGem() ? 1 : r_ambientGamma.GetFloat() );
	qglUniform4fv( lightOrigin, 1, din->worldUpLocal.ToFloatPtr() );
	if ( backEnd.vLight->lightShader->IsCubicLight() ) {
		qglUniform1i( lightFalloffCubemap, 1 );
	} else {
		qglUniform1i( lightFalloffCubemap, MAX_MULTITEXTURE_UNITS + 1 );
	}
	GL_CheckErrors();
}

MultiLightShaderData::MultiLightShaderData( const drawSurf_t *surf, bool shadowPass ) {
#ifdef MULTI_LIGHT_IN_FRONT
	idList<int> lightIndex;
	if ( surf->onLights )
		for ( int* pIndex = surf->onLights; *pIndex >= 0; pIndex++ )
			lightIndex.Append( *pIndex );
#endif
	for ( auto *vLight = backEnd.viewDef->viewLights; vLight; vLight = vLight->next ) {
		backEnd.vLight = vLight;
		if ( shadowPass ) {
			if ( !backEnd.vLight->shadowMapIndex )
				continue;
		} else {
			if ( vLight->singleLightOnly )
				continue;
		}
		if ( surf->material->Spectrum() != vLight->lightShader->Spectrum() )
			continue;
		if ( vLight->lightShader->IsAmbientLight() ) {
			if ( r_skipAmbient.GetInteger() == 2 )
				continue;
		} else {
			if ( r_skipInteractions.GetBool() )
				continue;
		}
		idVec3 localLightOrigin;
		R_GlobalPointToLocal( surf->space->modelMatrix, vLight->globalLightOrigin, localLightOrigin );
		if ( 1/* !r_ignore.GetBool()*/ ) {
#ifdef MULTI_LIGHT_IN_FRONT
				if ( !lightIndex.Find( vLight->lightDef->index ) )
					continue;
#else
				auto entDef = surf->space->entityDef;				// happens to be null - font materials, etc?
				if ( !entDef || R_CullLocalBox( surf->frontendGeo->bounds, entDef->modelMatrix, 6, vLight->lightDef->frustum ) )
					continue;
#endif
		}
		vLights.push_back( vLight );
		if ( shadowPass )
			lightOrigins.push_back( vLight->globalLightOrigin );
		else
			lightOrigins.push_back( localLightOrigin );

		if ( vLight->lightShader->IsAmbientLight() )
			shadowRects.push_back( idVec4( 0, 0, -2, 0 ) );
		else {
			if ( vLight->shadowMapIndex <= 0 )
				shadowRects.push_back( idVec4( 0, 0, -1, 0 ) );
			auto &page = ShadowAtlasPages[vLight->shadowMapIndex - 1];
			idVec4 v( page.x, page.y, 0, page.width - 1 );
			v.ToVec2() = (v.ToVec2() * 2 + idVec2( 1, 1 )) / (2 * 6 * r_shadowMapSize.GetInteger());
			v.w /= 6 * r_shadowMapSize.GetFloat();
			v.z = vLight->shadowMapIndex - 1;
			shadowRects.push_back( v );
		}
		softShadowRads.push_back( GetEffectiveLightRadius() );
	}
}

void multiLightInteractionProgram_t::AfterLoad() {
	basicInteractionProgram_t::AfterLoad();
	lightCount = qglGetUniformLocation( program, "u_lightCount" );
	lightOrigin = qglGetUniformLocation( program, "u_lightOrigin" );
	lightColor = qglGetUniformLocation( program, "u_diffuseColor" );
	shadowRect = qglGetUniformLocation( program, "u_shadowRect" );
	minLevel = qglGetUniformLocation( program, "u_minLevel" );
	gamma = qglGetUniformLocation( program, "u_gamma" );
	softShadowsRadius = qglGetUniformLocation( program, "u_softShadowsRadius" );
	auto diffuseTexture = qglGetUniformLocation( program, "u_diffuseTexture" );
	auto shadowMap = qglGetUniformLocation( program, "u_shadowMap" );
	qglUseProgram( program );
	qglUniform1i( diffuseTexture, 3 );
	qglUniform1i( shadowMap, 5 );
	qglUseProgram( 0 );
}

void multiLightInteractionProgram_t::Draw( const drawInteraction_t *din ) {
	auto surf = din->surf;
	MultiLightShaderData data( surf, false );
	idList<idVec3> lightColors;
	idList<idMat4> projectionFalloff;
	for each (auto vLight in data.vLights) {
		const float			*lightRegs = vLight->shaderRegisters;
		const idMaterial	*lightShader = vLight->lightShader;
		const shaderStage_t	*lightStage = lightShader->GetStage( 0 );
		idVec4 lightColor (
			backEnd.lightScale * lightRegs[lightStage->color.registers[0]] * din->diffuseColor[0],
			backEnd.lightScale * lightRegs[lightStage->color.registers[1]] * din->diffuseColor[1],
			backEnd.lightScale * lightRegs[lightStage->color.registers[2]] * din->diffuseColor[2],
			lightRegs[lightStage->color.registers[3]]
		);
		lightColors.Append( lightColor.ToVec3() );
	
		idPlane lightProject[4];
		R_GlobalPlaneToLocal( surf->space->modelMatrix, vLight->lightProject[0], lightProject[0] );
		R_GlobalPlaneToLocal( surf->space->modelMatrix, vLight->lightProject[1], lightProject[1] );
		R_GlobalPlaneToLocal( surf->space->modelMatrix, vLight->lightProject[2], lightProject[2] );
		R_GlobalPlaneToLocal( surf->space->modelMatrix, vLight->lightProject[3], lightProject[3] );
		idMat4 *p = (idMat4*)&lightProject;
		projectionFalloff.Append( *p );
	}

	basicInteractionProgram_t::UpdateUniforms( din );
	qglUniform1f( minLevel, backEnd.viewDef->IsLightGem() ? 0 : r_ambientMinLevel.GetFloat() );
	qglUniform1f( gamma, backEnd.viewDef->IsLightGem() ? 1 : r_ambientGamma.GetFloat() );
	
	for ( size_t i = 0; i < data.lightOrigins.size(); i += MAX_LIGHTS ) {
		int thisCount = idMath::Imin( data.lightOrigins.size() - i, MAX_LIGHTS );

		qglUniform1i( lightCount, thisCount );
		qglUniform3fv( lightOrigin, thisCount, data.lightOrigins[i].ToFloatPtr() );
		qglUniform3fv( lightColor, thisCount, lightColors[i].ToFloatPtr() );
		qglUniformMatrix4fv( lightProjectionFalloff, thisCount, false, projectionFalloff[i].ToFloatPtr() );
		qglUniform4fv( shadowRect, thisCount, data.shadowRects[i].ToFloatPtr() );
		qglUniform1fv( softShadowsRadius, thisCount, &data.softShadowRads[i] );
		GL_CheckErrors();

		RB_DrawElementsWithCounters( surf );

		if ( r_showMultiLight.GetInteger() == 1 ) {
			backEnd.pc.c_interactions++;
			backEnd.pc.c_interactionLights += data.lightOrigins.size();
			backEnd.pc.c_interactionMaxLights = idMath::Imax( backEnd.pc.c_interactionMaxLights, data.lightOrigins.size() );
			auto shMaps = std::count_if( data.shadowRects.begin(), data.shadowRects.end(), []( idVec4 v ) {
				return v.z >= 0;
			} );
			if ( backEnd.pc.c_interactionMaxShadowMaps < (uint)shMaps)
				backEnd.pc.c_interactionMaxShadowMaps = (uint)shMaps;
		}
	}
	GL_CheckErrors();
}

void basicDepthProgram_t::FillDepthBuffer( const drawSurf_t *surf ) {
	float color[4];
	const idMaterial		*shader = surf->material;
	int						stage;
	const shaderStage_t		*pStage;
	const float				*regs = surf->shaderRegisters;

	// subviews will just down-modulate the color buffer by overbright
	if ( shader->GetSort() == SS_SUBVIEW ) {
		GL_State( GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO | GLS_DEPTHFUNC_LESS );
		color[0] = color[1] = color[2] = (1.0 / backEnd.overBright);
		color[3] = 1;
	} else {
		// others just draw black
		color[0] = 0;
		color[1] = 0;
		color[2] = 0;
		color[3] = 1;
	}
	idDrawVert *ac = (idDrawVert *)vertexCache.VertexPosition( surf->ambientCache );
	qglVertexAttribPointer( 0, 3, GL_FLOAT, false, sizeof( idDrawVert ), ac->xyz.ToFloatPtr() );

	bool drawSolid = false;

	if ( shader->Coverage() == MC_OPAQUE ) {
		drawSolid = true;
	}
	if ( shader->Coverage() == MC_TRANSLUCENT && acceptsTranslucent ) {
		drawSolid = true;
	}

	// we may have multiple alpha tested stages
	if ( shader->Coverage() == MC_PERFORATED ) {
		// if the only alpha tested stages are condition register omitted,
		// draw a normal opaque surface
		bool	didDraw = false;

		qglEnableVertexAttribArray( 8 );
		qglVertexAttribPointer( 8, 2, GL_FLOAT, false, sizeof( idDrawVert ), ac->st.ToFloatPtr() );

		// perforated surfaces may have multiple alpha tested stages
		for ( stage = 0; stage < shader->GetNumStages(); stage++ ) {
			pStage = shader->GetStage( stage );

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

			// set the alpha modulate
			color[3] = regs[pStage->color.registers[3]];

			// skip the entire stage if alpha would be black
			if ( color[3] <= 0 ) {
				continue;
			}
			qglUniform4fv( this->color, 1, color );
			qglUniform1f( alphaTest, regs[pStage->alphaTestRegister] );

			// bind the texture
			pStage->texture.image->Bind();

			// set texture matrix and texGens
			extern void RB_PrepareStageTexturing( const shaderStage_t *pStage, const drawSurf_t *surf, idDrawVert *ac );
			RB_PrepareStageTexturing( pStage, surf, ac );

			// draw it
			RB_DrawElementsWithCounters( surf );

			// take down texture matrix and texGens
			extern void RB_FinishStageTexturing( const shaderStage_t *pStage, const drawSurf_t *surf, idDrawVert *ac );
			RB_FinishStageTexturing( pStage, surf, ac );

			qglUniform1f( alphaTest, -1 ); // hint the glsl to skip texturing
		}
		qglUniform4fv( this->color, 1, colorBlack.ToFloatPtr() );
		qglDisableVertexAttribArray( 8 );

		if ( !didDraw ) {
			drawSolid = true;
		}
	}

	// draw the entire surface solid
	if ( drawSolid ) {
		// draw it
		RB_DrawElementsWithCounters( surf );
	}

	// reset blending
	if ( shader->GetSort() == SS_SUBVIEW ) {
		qglUniform4fv( this->color, 1, colorBlack.ToFloatPtr() );
		GL_State( GLS_DEPTHFUNC_LESS );
	}
}

void shadowMapProgram_t::AfterLoad() {
	basicDepthProgram_t::AfterLoad();
	lightOrigin = qglGetUniformLocation( program, "u_lightOrigin" );
	lightRadius = qglGetUniformLocation( program, "u_lightRadius" );
	modelMatrix = qglGetUniformLocation( program, "u_modelMatrix" );
	lightCount = qglGetUniformLocation( program, "u_lightCount" );
	shadowRect = qglGetUniformLocation( program, "u_shadowRect" );
	shadowTexelStep = qglGetUniformLocation( program, "u_shadowTexelStep" );
	acceptsTranslucent = true;
}

void shadowMapProgram_t::RenderAllLights(drawSurf_t *surf) {
	if ( !surf->material->SurfaceCastsShadow() )
		return;    // some dynamic models use a no-shadow material and for shadows have a separate geometry with an invisible (in main render) material

	if ( surf->dsFlags & DSF_SHADOW_MAP_IGNORE )
		return;    // this flag is set by entities with parms.noShadow (candles, torches, models with separate shadow geometry, etc)

	float customOffset = 0;
	if( auto entityDef = surf->space->entityDef )
		customOffset = entityDef->parms.shadowMapOffset + surf->material->GetShadowMapOffset();
	if ( customOffset != 0 )
		qglPolygonOffset( customOffset, 0 );

	if ( backEnd.currentSpace != surf->space ) {
		qglUniformMatrix4fv( modelMatrix, 1, false, surf->space->modelMatrix );
		backEnd.currentSpace = surf->space;
		backEnd.pc.c_matrixLoads++;
	}

	MultiLightShaderData data( surf, true );

	for ( size_t i = 0; i < data.lightOrigins.size(); i += multiLightInteractionProgram_t::MAX_LIGHTS ) {
		int thisCount = idMath::Imin( data.lightOrigins.size() - i, multiLightInteractionProgram_t::MAX_LIGHTS );

		qglUniform1i( lightCount, thisCount );
		qglUniform3fv( lightOrigin, thisCount, data.lightOrigins[i].ToFloatPtr() );
		qglUniform4fv( shadowRect, thisCount, data.shadowRects[i].ToFloatPtr() );
		qglUniform1fv( lightRadius, thisCount, &data.softShadowRads[i] );
		GL_CheckErrors();

		FillDepthBuffer( surf );

		if ( r_showMultiLight.GetInteger() == 2 ) {
			backEnd.pc.c_interactions++;
			backEnd.pc.c_interactionLights += data.lightOrigins.size();
			backEnd.pc.c_interactionMaxLights = idMath::Imax( backEnd.pc.c_interactionMaxLights, data.lightOrigins.size() );
			auto shMaps = std::count_if( data.shadowRects.begin(), data.shadowRects.end(), []( idVec4 v ) {
				return v.z >= 0;
			} );
			if ( backEnd.pc.c_interactionMaxShadowMaps < (uint)shMaps )
				backEnd.pc.c_interactionMaxShadowMaps = (uint)shMaps;
		}
	}

	if ( customOffset != 0 )
		qglPolygonOffset( 0, 0 );
}

void shadowMapProgram_t::RenderAllLights() {
	GL_PROFILE( "shadowMapProgram_t::RenderAllLights" );

	FB_ToggleShadow( true );

	Use();
	GL_SelectTexture( 0 );

	backEnd.currentSpace = NULL;

//	GL_Cull( CT_TWO_SIDED );
	qglPolygonOffset( 0, 0 );
	qglEnable( GL_POLYGON_OFFSET_FILL );

	auto &page = ShadowAtlasPages[backEnd.vLight->shadowMapIndex - 1];
	float texSize = globalImages->shadowAtlas->uploadHeight;
	qglUniform1f( shadowTexelStep, 1/texSize );

	qglViewport( 0, 0, texSize, texSize );
	if ( r_useScissor.GetBool() )
		GL_Scissor( 0, 0, texSize, texSize );
	qglClear( GL_DEPTH_BUFFER_BIT );
	for ( int i = 0; i < 4; i++ ) // clip the geometry shader output to each of the atlas pages
		qglEnable( GL_CLIP_PLANE0 + i );
	auto viewDef = backEnd.viewDef;
	for ( int i = 0; i < viewDef->numDrawSurfs + viewDef->numOffscreenSurfs; i++ )
		RenderAllLights( viewDef->drawSurfs[i] );
	for ( int i = 0; i < 4; i++ )
		qglDisable( GL_CLIP_PLANE0 + i );

	qglDisable( GL_POLYGON_OFFSET_FILL );
	GL_Cull( CT_FRONT_SIDED );

	backEnd.currentSpace = NULL; // or else conflicts with qglLoadMatrixf
	qglUseProgram( 0 );

	FB_ToggleShadow( false );

	GL_CheckErrors();
}