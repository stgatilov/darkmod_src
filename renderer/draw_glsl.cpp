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
#include "GLSLProgram.h"
#include "GLSLProgramManager.h"

#if defined(_MSC_VER) && _MSC_VER >= 1800 && !defined(DEBUG)
//#pragma optimize("t", off) // duzenko: used in release to enforce breakpoints in inlineable code. Please do not remove
#endif

//TODO: is this global variable harming multithreading?
idList<idVec2> g_softShadowsSamples;

struct ShadowMapUniforms : GLSLUniformGroup {
	UNIFORM_GROUP_DEF( ShadowMapUniforms )

	DEFINE_UNIFORM( vec4, lightOrigin )
	DEFINE_UNIFORM( float, lightRadius )
	DEFINE_UNIFORM( float, alphaTest )
	DEFINE_UNIFORM( mat4, modelMatrix )
};

shadowMapProgram_t shadowmapMultiShader;

GLSLProgram *currrentInteractionShader; // dynamic, either pointInteractionShader or ambientInteractionShader

std::map<std::string, shaderProgram_t*> dynamicShaders; // shaders referenced from materials, stored by their file names

idCVar r_shadowMapSinglePass( "r_shadowMapSinglePass", "0", CVAR_ARCHIVE | CVAR_RENDERER, "render shadow maps for all lights in a single pass" );

static void ChooseInteractionProgram() {
	if ( backEnd.vLight->lightShader->IsAmbientLight() ) {
		currrentInteractionShader = programManager->ambientInteractionShader;
	} else {
		if ( backEnd.vLight->shadowMapIndex )
			currrentInteractionShader = programManager->shadowMapInteractionShader;
		else
			currrentInteractionShader = programManager->stencilInteractionShader;
	}
	currrentInteractionShader->Activate();
	currrentInteractionShader->GetUniformGroup<Uniforms::Interaction>()->RGTC.Set( globalImages->image_useNormalCompression.GetInteger() == 2 && glConfig.textureCompressionRgtcAvailable ? 1 : 0 );
	GL_CheckErrors();
}

/*
==================
RB_GLSL_DrawInteraction
==================
*/
void RB_GLSL_DrawInteraction( const drawInteraction_t *din ) {
	// load all the shader parameters
	GL_CheckErrors();
	Uniforms::Interaction *interactionUniforms = currrentInteractionShader->GetUniformGroup<Uniforms::Interaction>();
	interactionUniforms->SetForInteraction( din );

	// set the textures
	// texture 0 will be the per-surface bump map
	if ( !din->bumpImage && interactionUniforms->hasTextureDNS.IsPresent() ) {
		interactionUniforms->hasTextureDNS.Set( 1, 0, 1 );
	} else {
		if( !din->bumpImage ) // FIXME Uh-oh! This should not happen
			return;
		GL_SelectTexture( 0 );
		din->bumpImage->Bind();
		if ( interactionUniforms->hasTextureDNS.IsPresent() ) {
			interactionUniforms->hasTextureDNS.Set( 1, 1, 1 );
		}
	}

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

	if ( r_softShadowsQuality.GetBool() && !backEnd.viewDef->IsLightGem() || backEnd.vLight->shadows == LS_MAPS )
		FB_BindShadowTexture();

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
	backEnd.currentSpace = NULL; // ambient/interaction shaders conflict

	// bind the vertex and fragment program
	ChooseInteractionProgram();
	Uniforms::Interaction *interactionUniforms = currrentInteractionShader->GetUniformGroup<Uniforms::Interaction>();
	interactionUniforms->SetForShadows( surf == backEnd.vLight->translucentInteractions );

	// enable the vertex arrays
	qglEnableVertexAttribArray( 8 );
	qglEnableVertexAttribArray( 9 );
	qglEnableVertexAttribArray( 10 );
	qglEnableVertexAttribArray( 2 );
	qglEnableVertexAttribArray( 3 );

	for ( /**/; surf; surf = surf->nextOnLight ) {
		if ( surf->dsFlags & DSF_SHADOW_MAP_ONLY ) {
			continue;
		}
		if ( backEnd.currentSpace != surf->space ) {
			// FIXME needs a better integration with RB_CreateSingleDrawInteractions
			interactionUniforms->modelMatrix.Set( surf->space->modelMatrix );
		}

		// set the vertex pointers
		idDrawVert	*ac = ( idDrawVert * )vertexCache.VertexPosition( surf->ambientCache );
		qglVertexAttribPointer( 3, 4, GL_UNSIGNED_BYTE, true, sizeof( idDrawVert ), &ac->color );
		qglVertexAttribPointer( 2, 3, GL_FLOAT, false, sizeof( idDrawVert ), ac->normal.ToFloatPtr() );
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
	qglDisableVertexAttribArray( 2 );
	qglDisableVertexAttribArray( 3 );

	// disable features
	if ( r_softShadowsQuality.GetBool() && !backEnd.viewDef->IsLightGem() || backEnd.vLight->shadows == LS_STENCIL ) {
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

	bool useShadowFbo = r_softShadowsQuality.GetBool() && !backEnd.viewDef->IsLightGem();// && (r_shadows.GetInteger() != 2);

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
	programManager->stencilShadowShader->Activate();

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
	programManager->stencilShadowShader->Activate();

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

float GetEffectiveLightRadius() {
	float lightRadius = backEnd.vLight->radius;
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
	if ( r_shadowMapSinglePass.GetBool() || r_skipInteractions.GetBool() ) 
		return;
	GL_PROFILE( "GLSL_DrawInteractions_ShadowMap" );

	FB_ToggleShadow( true );

	programManager->shadowMapShader->Activate();
	GL_SelectTexture( 0 );

	ShadowMapUniforms *shadowMapUniforms = programManager->shadowMapShader->GetUniformGroup<ShadowMapUniforms>();
	idVec4 lightOrigin;
	lightOrigin.x = backEnd.vLight->globalLightOrigin.x;
	lightOrigin.y = backEnd.vLight->globalLightOrigin.y;
	lightOrigin.z = backEnd.vLight->globalLightOrigin.z;
	lightOrigin.w = 0;
	shadowMapUniforms->lightOrigin.Set( lightOrigin );
	shadowMapUniforms->lightRadius.Set( GetEffectiveLightRadius() );
	shadowMapUniforms->alphaTest.Set( -1 );
	backEnd.currentSpace = NULL;

	GL_Cull( CT_TWO_SIDED );
	qglPolygonOffset( 0, 0 );
	qglEnable( GL_POLYGON_OFFSET_FILL );

	auto &page = ShadowAtlasPages[backEnd.vLight->shadowMapIndex-1];
	qglViewport( page.x, page.y, 6*page.width, page.width );
	if ( r_useScissor.GetBool() )
		qglScissor( page.x, page.y, 6*page.width, page.width );
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
			shadowMapUniforms->modelMatrix.Set( surf->space->modelMatrix );
			backEnd.currentSpace = surf->space;
			backEnd.pc.c_matrixLoads++;
		}

		RB_SingleSurfaceToDepthBuffer( programManager->shadowMapShader, surf );
		backEnd.pc.c_shadowIndexes += surf->numIndexes;
		backEnd.pc.c_drawIndexes -= surf->numIndexes;

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

	if ( backEnd.vLight->shadows == LS_MAPS ) {
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
		for ( auto vLight = backEnd.viewDef->viewLights; vLight; vLight = vLight->next )
			if ( vLight->shadows == LS_MAPS )
				vLight->shadowMapIndex = ++ShadowAtlasIndex;
		ShadowAtlasIndex = 0; // reset for next run
		if ( r_shadowMapSinglePass.GetBool() )
			shadowmapMultiShader.RenderAllLights();

		if ( r_testARBProgram.GetInteger() == 2 ) {
			extern void RB_GLSL_DrawInteractions_MultiLight();
			RB_GLSL_DrawInteractions_MultiLight();
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
ID_NOINLINE bool R_ReloadGLSLPrograms() { 
	bool ok = true;
	// these are optional and don't "need" to compile
	shadowmapMultiShader.Load( "shadowMapN" );
	for ( auto it = dynamicShaders.begin(); it != dynamicShaders.end(); ++it ) {
		auto& fileName = it->first;
		auto& shader = it->second;
		shader->Load( fileName.c_str() );
	}

	// incorporate new shader interface:
	programManager->ReloadAllPrograms();

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
		r_useGLSL = false;
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

// pass supported extensions for shader IFDEFS
void PrimitivePreprocess(idStr &source) {
	idStr injections;
	if ( glConfig.gpuShader4Available )
		injections = "#define EXT_gpu_shader4";
	source.Replace( "// TDM INJECTIONS", injections );
}

/*
=================
shaderProgram_t::CompileShader
=================
*/
GLuint shaderProgram_t::CompileShader( GLint ShaderType, const char *fileName ) {
	/* get shader source */
	char *fileBuffer = NULL;
	// load the program even if we don't support it
	int length = fileSystem->ReadFile( fileName, ( void ** )&fileBuffer, NULL );
	if ( !fileBuffer || length < 0 ) {
		if ( ShaderType != GL_GEOMETRY_SHADER ) {
			common->Warning( "CompileShader(%s) file not found", fileName );
		}
		return 0;
	}
	//note: ReadFile guarantees null-termination
	assert(fileBuffer && fileBuffer[length] == 0);

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
	idStr source( fileBuffer );
	PrimitivePreprocess( source );
	const char *pSource = source.c_str();

	/* create shader object, set the source, and compile */
	GLuint shader = qglCreateShader( ShaderType );
	qglShaderSource( shader, 1, &pSource, NULL );
	qglCompileShader( shader );
	fileSystem->FreeFile( fileBuffer );

	/* make sure the compilation was successful */
	GLint result;
	qglGetShaderiv( shader, GL_COMPILE_STATUS, &result );

	/* get the shader info log */
	qglGetShaderiv( shader, GL_INFO_LOG_LENGTH, &length );
	char *log = (char*)Mem_ClearedAlloc(length + 1);
	qglGetShaderInfoLog( shader, length, NULL, log );
	//TODO: print compile log always (bad idea now due to tons of warnings)
	if (result == GL_FALSE)
		common->Warning( "CompileShader(%s): %s\n%s\n", fileName, (result ? "ok" : "FAILED"), log );
	Mem_Free(log);

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
	qglBindAttribLocation( program, 0, "attr_Position" );
	qglBindAttribLocation( program, 2, "attr_Normal" );
	qglBindAttribLocation( program, 3, "attr_Color" );
	qglBindAttribLocation( program, 8, "attr_TexCoord" );
	qglBindAttribLocation( program, 9, "attr_Tangent" );
	qglBindAttribLocation( program, 10, "attr_Bitangent" );

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

void basicDepthProgram_t::AfterLoad() {
	color = qglGetUniformLocation( program, "color" );
	alphaTest = qglGetUniformLocation( program, "alphaTest" );
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
	if ( din->bumpImage )
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
			if ( pStage->texture.hasMatrix ) 
				RB_LoadShaderTextureMatrix( surf->shaderRegisters, &pStage->texture );

			// draw it
			if ( instances )
				RB_DrawElementsInstanced( surf, instances );
			else
				RB_DrawElementsWithCounters( surf );

			if ( pStage->texture.hasMatrix ) {
				qglMatrixMode( GL_TEXTURE );
				qglLoadIdentity();
				qglMatrixMode( GL_MODELVIEW );
			}

			qglUniform1f( alphaTest, -1 ); // hint the glsl to skip texturing
		}
		qglUniform4fv( this->color, 1, colorBlack.ToFloatPtr() );
		qglDisableVertexAttribArray( 8 );

		if ( !didDraw ) {
			drawSolid = true;
		}
	}

	if ( drawSolid )  // draw the entire surface solid
		if ( instances ) 
			RB_DrawElementsInstanced( surf, instances );
		else
			RB_DrawElementsWithCounters( surf );

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
	lightFrustum = qglGetUniformLocation( program, "u_lightFrustum" );
	acceptsTranslucent = true;
	instances = 6;
}

void RB_SingleSurfaceToDepthBuffer( GLSLProgram *program, const drawSurf_t *surf ) {
	idVec4 color;
	const idMaterial		*shader = surf->material;
	int						stage;
	const shaderStage_t		*pStage;
	const float				*regs = surf->shaderRegisters;

	Uniforms::Depth *depthUniforms = program->GetUniformGroup<Uniforms::Depth>();

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
	if ( shader->Coverage() == MC_TRANSLUCENT && depthUniforms->acceptsTranslucent ) {
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
			depthUniforms->color.Set( color );
			depthUniforms->alphaTest.Set( regs[pStage->alphaTestRegister] );

			// bind the texture
			pStage->texture.image->Bind();
			if ( pStage->texture.hasMatrix ) 
				RB_LoadShaderTextureMatrix( surf->shaderRegisters, &pStage->texture );

			// draw it
			if ( depthUniforms->instances )
				RB_DrawElementsInstanced( surf, depthUniforms->instances );
			else
				RB_DrawElementsWithCounters( surf );

			if ( pStage->texture.hasMatrix ) {
				qglMatrixMode( GL_TEXTURE );
				qglLoadIdentity();
				qglMatrixMode( GL_MODELVIEW );
			}

			depthUniforms->alphaTest.Set( -1 ); // hint the glsl to skip texturing
		}
		depthUniforms->color.Set( colorBlack );
		qglDisableVertexAttribArray( 8 );

		if ( !didDraw ) {
			drawSolid = true;
		}
	}

	if ( drawSolid )  // draw the entire surface solid
		if ( depthUniforms->instances ) 
			RB_DrawElementsInstanced( surf, depthUniforms->instances );
		else
			RB_DrawElementsWithCounters( surf );

	// reset blending
	if ( shader->GetSort() == SS_SUBVIEW ) {
		depthUniforms->color.Set( colorBlack );
		GL_State( GLS_DEPTHFUNC_LESS );
	}
}

//=============================================================================
// Below goes the suggested new way of handling GLSL parameters.
// TODO: move it to glsl.cpp


void Attributes::Default::Bind(GLSLProgram *program) {
	using namespace Attributes::Default;
	program->BindAttribLocation(Position, "attr_Position");
	program->BindAttribLocation(Normal, "attr_Normal");
	program->BindAttribLocation(Color, "attr_Color");
	program->BindAttribLocation(TexCoord, "attr_TexCoord");
	program->BindAttribLocation(Tangent, "attr_Tangent");
	program->BindAttribLocation(Bitangent, "attr_Bitangent");
}

//I expect this function should be enough for setting up vertex attrib arrays in most cases..
//But I am not sure in it =)
void Attributes::Default::SetDrawVert(size_t startOffset, int arrayMask) {
	using namespace Attributes::Default;
	if (arrayMask & (1 << Position)) {
		qglEnableVertexAttribArray(Position);
		qglVertexAttribPointer(Position, 3, GL_FLOAT, GL_FALSE, sizeof(idDrawVert), (void*)(startOffset + offsetof(idDrawVert, xyz)));
	}
	else {
		qglDisableVertexAttribArray(Position);
	}

	if (arrayMask & (1 << Normal)) {
		qglEnableVertexAttribArray(Normal);
		qglVertexAttribPointer(Normal, 3, GL_FLOAT, GL_FALSE, sizeof(idDrawVert), (void*)(startOffset + offsetof(idDrawVert, normal)));
	}
	else {
		qglDisableVertexAttribArray(Normal);
	}

	if (arrayMask & (1 << Color)) {
		qglEnableVertexAttribArray(Color);
		qglVertexAttribPointer(Color, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(idDrawVert), (void*)(startOffset + offsetof(idDrawVert, color)));
	}
	else {
		qglDisableVertexAttribArray(Color);
	}

	if (arrayMask & (1 << TexCoord)) {
		qglEnableVertexAttribArray(TexCoord);
		qglVertexAttribPointer(TexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(idDrawVert), (void*)(startOffset + offsetof(idDrawVert, st)));
	}
	else {
		qglDisableVertexAttribArray(TexCoord);
	}

	if (arrayMask & (1 << Tangent)) {
		qglEnableVertexAttribArray(Tangent);
		qglVertexAttribPointer(Tangent, 3, GL_FLOAT, GL_FALSE, sizeof(idDrawVert), (void*)(startOffset + offsetof(idDrawVert, tangents)));
	}
	else {
		qglDisableVertexAttribArray(Tangent);
	}

	if (arrayMask & (1 << Bitangent)) {
		qglEnableVertexAttribArray(Bitangent);
		qglVertexAttribPointer(Bitangent, 3, GL_FLOAT, GL_FALSE, sizeof(idDrawVert), (void*)(startOffset + offsetof(idDrawVert, tangents) + sizeof(idDrawVert::tangents[0])));
	}
	else {
		qglDisableVertexAttribArray(Bitangent);
	}
}

void Uniforms::Global::Set(const viewEntity_t *space) {
	modelMatrix.Set( space->modelMatrix );
	projectionMatrix.Set( backEnd.viewDef->projectionMatrix );
	modelViewMatrix.Set( space->modelViewMatrix );
}

void Uniforms::MaterialStage::Set(const shaderStage_t *pStage, const drawSurf_t *surf) {
	//============================================================================
	//note: copied from RB_SetProgramEnvironment and RB_SetProgramEnvironmentSpace
	//============================================================================

	idVec4 parm;
	// screen power of two correction factor, assuming the copy to _currentRender
	// also copied an extra row and column for the bilerp
	int	 w = backEnd.viewDef->viewport.x2 - backEnd.viewDef->viewport.x1 + 1;
	int pot = globalImages->currentRenderImage->uploadWidth;
	parm[0] = ( float )w / pot;
	int	 h = backEnd.viewDef->viewport.y2 - backEnd.viewDef->viewport.y1 + 1;
	pot = globalImages->currentRenderImage->uploadHeight;
	parm[1] = ( float )h / pot;
	parm[2] = 0;
	parm[3] = 1;
 	scalePotToWindow.Set( parm );

	// window coord to 0.0 to 1.0 conversion
	parm[0] = 1.0 / w;
	parm[1] = 1.0 / h;
	parm[2] = 0;
	parm[3] = 1;
 	scaleWindowToUnit.Set( parm );

	// #3877: Allow shaders to access depth buffer.
	// Two useful ratios are packed into this parm: [0] and [1] hold the x,y multipliers you need to map a screen
	// coordinate (fragment position) to the depth image: those are simply the reciprocal of the depth
	// image size, which has been rounded up to a power of two. Slots [3] and [4] hold the ratio of the depth image
	// size to the current render image size. These sizes can differ if the game crops the render viewport temporarily
	// during post-processing effects. The depth render is smaller during the effect too, but the depth image doesn't
	// need to be downsized, whereas the current render image does get downsized when it's captured by the game after
	// the skybox render pass. The ratio is needed to map between the two render images.
	parm[0] = 1.0f / globalImages->currentDepthImage->uploadWidth;
	parm[1] = 1.0f / globalImages->currentDepthImage->uploadHeight;
	parm[2] = static_cast<float>( globalImages->currentRenderImage->uploadWidth ) / globalImages->currentDepthImage->uploadWidth;
	parm[3] = static_cast<float>( globalImages->currentRenderImage->uploadHeight ) / globalImages->currentDepthImage->uploadHeight;
	scaleDepthCoords.Set( parm );

	//
	// set eye position in global space
	//
	parm[0] = backEnd.viewDef->renderView.vieworg[0];
	parm[1] = backEnd.viewDef->renderView.vieworg[1];
	parm[2] = backEnd.viewDef->renderView.vieworg[2];
	parm[3] = 1.0;
	viewOriginGlobal.Set( parm );

	const struct viewEntity_s *space = backEnd.currentSpace;
	// set eye position in local space
	R_GlobalPointToLocal( space->modelMatrix, backEnd.viewDef->renderView.vieworg, parm.ToVec3() );
	parm[3] = 1.0;
	viewOriginLocal.Set( parm );

	// we need the model matrix without it being combined with the view matrix
	// so we can transform local vectors to global coordinates
	parm[0] = space->modelMatrix[0];
	parm[1] = space->modelMatrix[4];
	parm[2] = space->modelMatrix[8];
	parm[3] = space->modelMatrix[12];
 	modelMatrixRow0.Set( parm );
	parm[0] = space->modelMatrix[1];
	parm[1] = space->modelMatrix[5];
	parm[2] = space->modelMatrix[9];
	parm[3] = space->modelMatrix[13];
 	modelMatrixRow1.Set( parm );
	parm[0] = space->modelMatrix[2];
	parm[1] = space->modelMatrix[6];
	parm[2] = space->modelMatrix[10];
	parm[3] = space->modelMatrix[14];
 	modelMatrixRow2.Set( parm );

	//============================================================================

	const newShaderStage_t *newStage = pStage->newStage;

	//setting local parameters (specified in material definition)
	const float	*regs = surf->shaderRegisters;
	for ( int i = 0; i < newStage->numVertexParms; i++ ) {
		parm[0] = regs[newStage->vertexParms[i][0]];
		parm[1] = regs[newStage->vertexParms[i][1]];
		parm[2] = regs[newStage->vertexParms[i][2]];
		parm[3] = regs[newStage->vertexParms[i][3]];
 		localParams[ i ]->Set( parm );
	}

	//setting textures
	//note: the textures are also bound to TUs at this moment
	for ( int i = 0; i < newStage->numFragmentProgramImages; i++ ) {
		if ( newStage->fragmentProgramImages[i] ) {
			GL_SelectTexture( i );
			newStage->fragmentProgramImages[i]->Bind();
 			textures[ i ]->Set( i );
		}
	}

	GL_CheckErrors();
}

void Uniforms::Interaction::SetForInteractionBasic( const drawInteraction_t *din ) {
	if ( din->surf->space != backEnd.currentSpace )
		modelMatrix.Set( din->surf->space->modelMatrix );
	diffuseMatrix.SetArray( 2, din->diffuseMatrix[0].ToFloatPtr() );
	if ( din->bumpImage )
		bumpMatrix.SetArray( 2, din->bumpMatrix[0].ToFloatPtr() );
	specularMatrix.SetArray( 2, din->specularMatrix[0].ToFloatPtr() );

	static const idVec4	zero   { 0, 0, 0, 0 },
	                    one	   { 1, 1, 1, 1 },
	                    negOne { -1, -1, -1, -1 };
	switch ( din->vertexColor ) {
	case SVC_IGNORE:
		colorModulate.Set( zero );
		colorAdd.Set( one );
		break;
	case SVC_MODULATE:
		colorModulate.Set( one );
		colorAdd.Set( zero );
		break;
	case SVC_INVERSE_MODULATE:
		colorModulate.Set( negOne );
		colorAdd.Set( one );
		break;
	}
}

void Uniforms::Interaction::SetForInteraction( const drawInteraction_t *din ) {
	SetForInteractionBasic( din );

	lightProjectionFalloff.Set( din->lightProjection[0].ToFloatPtr() );
	// set the constant color
	diffuseColor.Set( din->diffuseColor );
	specularColor.Set( din->specularColor );
	if ( backEnd.vLight->lightShader->IsCubicLight() ) {
		cubic.Set( 1.f );
		lightProjectionTexture.Set( MAX_MULTITEXTURE_UNITS );
		lightProjectionCubemap.Set( 2 );
		lightFalloffTexture.Set( MAX_MULTITEXTURE_UNITS );
	} else {
		cubic.Set( 0.f );
		lightProjectionTexture.Set( 2 );
		lightProjectionCubemap.Set( MAX_MULTITEXTURE_UNITS + 1 );
		lightFalloffTexture.Set( 1 );
	}
	viewOrigin.Set( din->localViewOrigin );

	if( ambient ) {
		minLevel.Set(backEnd.viewDef->IsLightGem() ? 0 : r_ambientMinLevel.GetFloat() );
		gamma.Set( backEnd.viewDef->IsLightGem() ? 1 : r_ambientGamma.GetFloat() );
		lightOrigin.Set( din->worldUpLocal.ToVec3() );
		idVec4 color;
		din->surf->material->GetAmbientRimColor( color );
		rimColor.Set( color );
		if ( backEnd.vLight->lightShader->IsCubicLight() ) {
			lightFalloffCubemap.Set( 1 );
		} else {
			lightFalloffCubemap.Set( MAX_MULTITEXTURE_UNITS + 1 );
		}
	} else {
		lightOrigin.Set( din->localLightOrigin.ToVec3() );
		lightOrigin2.Set( backEnd.vLight->globalLightOrigin );
	}

	GL_CheckErrors();
}

void Uniforms::Interaction::SetForShadows( bool translucent ) {
	if( ambient ) {
		return;
	}

	advanced.Set( r_testARBProgram.GetFloat() );

	auto vLight = backEnd.vLight;
	bool doShadows = !vLight->noShadows && vLight->lightShader->LightCastsShadows(); 
	if ( doShadows && r_shadows.GetInteger() == 2 ) {
		// FIXME shadowmap only valid when globalInteractions not empty, otherwise garbage
		doShadows = vLight->globalInteractions != NULL;
	}
	if ( doShadows ) {
		shadows.Set( vLight->shadows );
		auto &page = ShadowAtlasPages[vLight->shadowMapIndex-1];
		if ( 0 ) { // select the pixels to TexCoords method for interactionA.fs
			idVec4 v( page.x, page.y, 0, page.width );
			v /= 6 * r_shadowMapSize.GetFloat();
			shadowRect.Set( v );
		} else { // https://stackoverflow.com/questions/5879403/opengl-texture-coordinates-in-pixel-space
			idVec4 v( page.x, page.y, 0, page.width-1 );
			v.ToVec2() = (v.ToVec2() * 2 + idVec2( 1, 1 )) / (2 * 6 * r_shadowMapSize.GetInteger());
			v.w /= 6 * r_shadowMapSize.GetFloat();
			shadowRect.Set( v );
		}
	} else {
		shadows.Set(0);
	}

	if ( !translucent && ( backEnd.vLight->globalShadows || backEnd.vLight->localShadows || r_shadows.GetInteger() == 2 ) && !backEnd.viewDef->IsLightGem() ) {
		softShadowsQuality.Set( r_softShadowsQuality.GetInteger() );

		int sampleK = r_softShadowsQuality.GetInteger();
		if ( sampleK > 0 ) { // texcoords for screen-space softener filter
			if ( g_softShadowsSamples.Num() != sampleK || g_softShadowsSamples.Num() == 0 ) {
				GeneratePoissonDiskSampling( g_softShadowsSamples, sampleK );
				softShadowsSamples.SetArray( sampleK, ( float * )g_softShadowsSamples.Ptr() );
			}
		}
	} else {
		softShadowsQuality.Set( 0 );
	}
	softShadowsRadius.Set( GetEffectiveLightRadius() ); // for soft stencil and all shadow maps
	if ( vLight->shadowMapIndex ) {
		shadowMap.Set( 6 );
		depthTexture.Set( MAX_MULTITEXTURE_UNITS );
		stencilTexture.Set( MAX_MULTITEXTURE_UNITS + 2 );
	} else {
		shadowMap.Set( MAX_MULTITEXTURE_UNITS + 2 );
		depthTexture.Set( 6 );
		stencilTexture.Set( 7 );
		renderResolution.Set( glConfig.vidWidth, glConfig.vidHeight );
	}

	GL_CheckErrors();
}


GLSLProgram* GLSL_LoadMaterialStageProgram(const char *name) {
	GLSLProgram *program = programManager->Find( name );
	if( program == nullptr ) {
		program = programManager->Load( name );
	}
	return program;
}
