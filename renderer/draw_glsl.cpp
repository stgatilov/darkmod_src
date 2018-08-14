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

struct shadowMapProgram_t : lightProgram_t {
	virtual void Use();
};

struct interactionProgram_t : lightProgram_t {
	GLint localViewOrigin;
	GLint rgtc;

	GLint lightProjectionS;
	GLint lightProjectionT;
	GLint lightProjectionQ;
	GLint lightFalloff;

	GLint cubic;
	GLint lightProjectionCubemap, lightProjectionTexture, lightFalloffCubemap, lightFalloffTexture;

	GLint colorModulate;
	GLint colorAdd;

	GLint bumpMatrixS;
	GLint bumpMatrixT;

	GLint diffuseMatrixS;
	GLint diffuseMatrixT;
	GLint diffuseColor;

	GLint specularMatrixS;
	GLint specularMatrixT;
	GLint specularColor;

	virtual	void AfterLoad();
	virtual void UpdateUniforms( bool translucent ) {}
	virtual void UpdateUniforms( const drawInteraction_t *din );
	virtual void Use();
	static void ChooseInteractionProgram();
};

struct pointInteractionProgram_t : interactionProgram_t {
	GLint advanced, shadows, lightOrigin2;
	GLint softShadowsQuality, softShadowsRadius, softShadowSamples, shadowMipMap, renderResolution;
	GLint shadowMap, stencilTexture, depthTexture;
	//TODO: is this global variable harming multithreading?
	idList<idVec2> g_softShadowsSamples;
	virtual	void AfterLoad();
	virtual void UpdateUniforms( bool translucent );
	virtual void UpdateUniforms( const drawInteraction_t *din );
};

struct ambientInteractionProgram_t : interactionProgram_t {
	GLint gamma;
	virtual	void AfterLoad();
	virtual void UpdateUniforms( const drawInteraction_t *din );
};

shaderProgram_t cubeMapShader;
oldStageProgram_t oldStageShader;
depthProgram_t depthShader;
lightProgram_t stencilShadowShader;
shadowMapProgram_t shadowMapShader;
fogProgram_t fogShader;
blendProgram_t blendShader;
pointInteractionProgram_t pointInteractionShader;
ambientInteractionProgram_t ambientInteractionShader;
lightProgram_t *currrentInteractionShader;

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

	if ( ( r_softShadowsQuality.GetBool() ) && !backEnd.viewDef->IsLightGem() || r_shadows.GetInteger() == 2 ) {
		FB_BindShadowTexture();
	}

	// draw it
	GL_CheckErrors();
	RB_DrawElementsWithCounters( din->surf->backendGeo );
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

		// set the vertex pointers
		idDrawVert	*ac = ( idDrawVert * )vertexCache.VertexPosition( surf->backendGeo->ambientCache );
		qglVertexAttribPointer( 3, 4, GL_UNSIGNED_BYTE, true, sizeof( idDrawVert ), &ac->color );
		qglVertexAttribPointer( 11, 3, GL_FLOAT, false, sizeof( idDrawVert ), ac->normal.ToFloatPtr() );
		qglVertexAttribPointer( 10, 3, GL_FLOAT, false, sizeof( idDrawVert ), ac->tangents[1].ToFloatPtr() );
		qglVertexAttribPointer( 9, 3, GL_FLOAT, false, sizeof( idDrawVert ), ac->tangents[0].ToFloatPtr() );
		qglVertexAttribPointer( 8, 2, GL_FLOAT, false, sizeof( idDrawVert ), ac->st.ToFloatPtr() );
		qglVertexAttribPointer( 0, 3, GL_FLOAT, false, sizeof( idDrawVert ), &ac->xyz );

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

	bool useShadowFbo = r_softShadowsQuality.GetBool() && !backEnd.viewDef->IsLightGem();
	pointInteractionShader.Use();
	qglUniform1f( pointInteractionShader.shadows, 1 );

	// set depth bounds for the whole light
	if ( backEnd.useLightDepthBounds ) {
		GL_DepthBoundsTest( backEnd.vLight->scissorRect.zmin, backEnd.vLight->scissorRect.zmax );
	}

	// clear the stencil buffer if needed
	if ( backEnd.vLight->globalShadows || backEnd.vLight->localShadows ) {
		backEnd.currentScissor = backEnd.vLight->scissorRect;

		if ( r_useScissor.GetBool() ) {
			qglScissor( backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
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

	const bool NoSelfShadows = true; // otherwise low-poly "round" models cast ugly shadows on themselves

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

/*
=============
RB_GLSL_CreateDrawInteractions
=============
*/
void RB_GLSL_DrawInteractions_ShadowMap( const drawSurf_t *surf, bool clear = false ) {
	if ( !surf ) {
		return;
	}
	GL_PROFILE( "GLSL_DrawInteractions_ShadowMap" );

	FB_ToggleShadow( true, clear );

	shadowMapShader.Use();

	qglUniform4fv( shadowMapShader.lightOrigin, 1, backEnd.vLight->globalLightOrigin.ToFloatPtr() );

	for ( ; surf; surf = surf->nextOnLight ) {
		if ( !surf->material->SurfaceCastsShadow() ) {
			continue;    //most of dynamic models don't have this flag but use an _invisible_ shadow material
		}

		if ( surf->dsFlags & DSF_SHADOW_MAP_IGNORE ) {
			continue;    // this flag is set by entities with parms.noShadow in R_LinkLightSurf (candles, torches, etc)
		}
		qglUniformMatrix4fv( shadowMapShader.modelMatrix, 1, false, surf->space->modelMatrix );

		// set the vertex pointers
		idDrawVert	*ac = ( idDrawVert * )vertexCache.VertexPosition( surf->backendGeo->ambientCache );
		qglVertexAttribPointer( 0, 3, GL_FLOAT, false, sizeof( idDrawVert ), &ac->xyz );
		RB_DrawElementsWithCounters( surf->backendGeo );
	}
	qglUseProgram( 0 );

	FB_ToggleShadow( false );

	GL_CheckErrors();
}

/*
==================
RB_GLSL_DrawLight_ShadowMap
==================
*/
void RB_GLSL_DrawLight_ShadowMap() {
	GL_PROFILE( "GLSL_DrawLight_ShadowMap" );

	GL_CheckErrors();

	if ( !backEnd.vLight->lightShader->IsAmbientLight() ) {
		bool doShadows = !backEnd.vLight->lightDef->parms.noShadows &&
		                 backEnd.vLight->lightShader->LightCastsShadows();

		if ( doShadows ) {
			RB_GLSL_DrawInteractions_ShadowMap( backEnd.vLight->globalInteractions, true );

			pointInteractionShader.Use();
			qglUniform1f( pointInteractionShader.shadows, 2 );
			qglUniform1i( pointInteractionShader.shadowMipMap, ShadowMipMap );
			RB_GLSL_CreateDrawInteractions( backEnd.vLight->localInteractions );

			RB_GLSL_DrawInteractions_ShadowMap( backEnd.vLight->localInteractions );

			pointInteractionShader.Use();
		} else {
			pointInteractionShader.Use();
			qglUniform1f( pointInteractionShader.shadows, 0 );
			RB_GLSL_CreateDrawInteractions( backEnd.vLight->localInteractions );
		}
	}
	RB_GLSL_CreateDrawInteractions( backEnd.vLight->globalInteractions );

	qglUseProgram( 0 );

	GL_CheckErrors();
}

/*
==================
RB_GLSL_DrawInteractions
==================
*/
void RB_GLSL_DrawInteractions() {
	GL_PROFILE( "GLSL_DrawInteractions" );

	GL_SelectTexture( 0 );

	// for each light, perform adding and shadowing
	for ( backEnd.vLight = backEnd.viewDef->viewLights; backEnd.vLight; backEnd.vLight = backEnd.vLight->next ) {
		// do fogging later
		if ( backEnd.vLight->lightShader->IsFogLight() ) {
			continue;
		}

		if ( backEnd.vLight->lightShader->IsBlendLight() ) {
			continue;
		}

		// if there are no interactions, get out!
		if ( !backEnd.vLight->localInteractions && !backEnd.vLight->globalInteractions && !backEnd.vLight->translucentInteractions ) {
			continue;
		}

		if ( r_shadows.GetInteger() == 2 ) {
			RB_GLSL_DrawLight_ShadowMap();
		} else {
			RB_GLSL_DrawLight_Stencil();
		}

		// translucent surfaces never get stencil shadowed
		if ( r_skipTranslucent.GetBool() ) {
			continue;
		}
		qglStencilFunc( GL_ALWAYS, 128, 255 );
		backEnd.depthFunc = GLS_DEPTHFUNC_LESS;
		RB_GLSL_CreateDrawInteractions( backEnd.vLight->translucentInteractions );
		backEnd.depthFunc = GLS_DEPTHFUNC_EQUAL;
	}

	// disable stencil shadow test
	qglStencilFunc( GL_ALWAYS, 128, 255 );
	GL_SelectTexture( 0 );
}

/*
==================
R_ReloadGLSLPrograms
==================
*/
bool R_ReloadGLSLPrograms() {
	bool ok = true;
	ok &= pointInteractionShader.Load( "interaction" );				// filenames hardcoded here since they're not used elsewhere
	ok &= ambientInteractionShader.Load( "ambientInteraction" );
	ok &= stencilShadowShader.Load( "stencilShadow" );
	ok &= shadowMapShader.Load( "shadowMap" );
	ok &= oldStageShader.Load( "oldStage" );
	ok &= depthShader.Load( "depthAlpha" );
	ok &= fogShader.Load( "fog" );
	ok &= blendShader.Load( "blend" );
	ok &= cubeMapShader.Load( "cubeMap" );
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
			common->Warning( "shaderCompileFromFile: \'%s\' not found", fileName );
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

	if ( result == GL_FALSE ) {
		char *log;

		/* get the shader info log */
		qglGetShaderiv( shader, GL_INFO_LOG_LENGTH, &length );
		log = new char[length];
		qglGetShaderInfoLog( shader, length, &result, log );

		/* print an error message and the info log */
		common->Warning( "shaderCompileFromFile(%s) validation\n%s\n", fileName, log );
		delete log;

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

void depthProgram_t::AfterLoad() {
	clipPlane = qglGetUniformLocation( program, "clipPlane" );
	matViewRev = qglGetUniformLocation( program, "matViewRev" );
	color = qglGetUniformLocation( program, "color" );
	alphaTest = qglGetUniformLocation( program, "alphaTest" );
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

void shadowMapProgram_t::Use() {
	lightProgram_t::Use();
	currrentInteractionShader = this;
}

void interactionProgram_t::ChooseInteractionProgram() {
	if ( backEnd.vLight->lightShader->IsAmbientLight() ) {
		ambientInteractionShader.Use();
	} else {
		pointInteractionShader.Use();
	}
	GL_CheckErrors();
}

void interactionProgram_t::Use() {
	lightProgram_t::Use();
	currrentInteractionShader = this;
	qglUniform1f( rgtc, globalImages->image_useNormalCompression.GetInteger() == 2 && glConfig.textureCompressionRgtcAvailable ? 1 : 0 );
}

void interactionProgram_t::AfterLoad() {
	lightProgram_t::AfterLoad();

	rgtc = qglGetUniformLocation( program, "u_RGTC" );

	localViewOrigin = qglGetUniformLocation( program, "u_viewOrigin" );
	lightProjectionS = qglGetUniformLocation( program, "u_lightProjectionS" );
	lightProjectionT = qglGetUniformLocation( program, "u_lightProjectionT" );
	lightProjectionQ = qglGetUniformLocation( program, "u_lightProjectionQ" );
	lightFalloff = qglGetUniformLocation( program, "u_lightFalloff" );

	bumpMatrixS = qglGetUniformLocation( program, "u_bumpMatrixS" );
	bumpMatrixT = qglGetUniformLocation( program, "u_bumpMatrixT" );

	diffuseMatrixS = qglGetUniformLocation( program, "u_diffuseMatrixS" );
	diffuseMatrixT = qglGetUniformLocation( program, "u_diffuseMatrixT" );
	diffuseColor = qglGetUniformLocation( program, "u_diffuseColor" );

	colorModulate = qglGetUniformLocation( program, "u_colorModulate" );
	colorAdd = qglGetUniformLocation( program, "u_colorAdd" );

	specularMatrixS = qglGetUniformLocation( program, "u_specularMatrixS" );
	specularMatrixT = qglGetUniformLocation( program, "u_specularMatrixT" );
	specularColor = qglGetUniformLocation( program, "u_specularColor" );

	cubic = qglGetUniformLocation( program, "u_cubic" );

	GLint normalTexture = qglGetUniformLocation( program, "u_normalTexture" );
	lightProjectionTexture = qglGetUniformLocation( program, "u_lightProjectionTexture" );
	lightProjectionCubemap = qglGetUniformLocation( program, "u_lightProjectionCubemap" );
	lightFalloffTexture = qglGetUniformLocation( program, "u_lightFalloffTexture" );
	lightFalloffCubemap = qglGetUniformLocation( program, "u_lightFalloffCubemap" );
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
	qglUniform1i( lightFalloffCubemap, 5 );
	qglUseProgram( 0 );
}

void interactionProgram_t::UpdateUniforms( const drawInteraction_t *din ) {
	static const float	zero[4]		= { 0, 0, 0, 0 },
	                    one[4]		= { 1, 1, 1, 1 },
	                    negOne[4]	= { -1, -1, -1, -1 };

	qglUniform4fv( lightProjectionS, 1, din->lightProjection[0].ToFloatPtr() );
	qglUniform4fv( lightProjectionT, 1, din->lightProjection[1].ToFloatPtr() );
	qglUniform4fv( lightProjectionQ, 1, din->lightProjection[2].ToFloatPtr() );
	qglUniform4fv( lightFalloff, 1, din->lightProjection[3].ToFloatPtr() );
	qglUniform4fv( bumpMatrixS, 1, din->bumpMatrix[0].ToFloatPtr() );
	qglUniform4fv( bumpMatrixT, 1, din->bumpMatrix[1].ToFloatPtr() );
	qglUniform4fv( diffuseMatrixS, 1, din->diffuseMatrix[0].ToFloatPtr() );
	qglUniform4fv( diffuseMatrixT, 1, din->diffuseMatrix[1].ToFloatPtr() );
	// set the constant color
	qglUniform4fv( diffuseColor, 1, din->diffuseColor.ToFloatPtr() );
	qglUniform4fv( diffuseColor, 1, din->diffuseColor.ToFloatPtr() );
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
	if ( backEnd.vLight->lightShader->IsCubicLight() ) {
		qglUniform1f( cubic, 1.0 );
		qglUniform1i( lightProjectionTexture, MAX_MULTITEXTURE_UNITS );
		qglUniform1i( lightProjectionCubemap, 2 );
		qglUniform1i( lightFalloffTexture, MAX_MULTITEXTURE_UNITS );
		qglUniform1i( lightFalloffCubemap, 1 );
	} else {
		qglUniform1f( cubic, 0.0 );
		qglUniform1i( lightProjectionTexture, 2 );
		qglUniform1i( lightProjectionCubemap, MAX_MULTITEXTURE_UNITS + 1 );
		qglUniform1i( lightFalloffTexture, 1 );
		qglUniform1i( lightFalloffCubemap, MAX_MULTITEXTURE_UNITS + 1 );
	}
	qglUniform4fv( localViewOrigin, 1, din->localViewOrigin.ToFloatPtr() );
	qglUniform4fv( specularMatrixS, 1, din->specularMatrix[0].ToFloatPtr() );
	qglUniform4fv( specularMatrixT, 1, din->specularMatrix[1].ToFloatPtr() );
	qglUniform4fv( specularColor, 1, din->specularColor.ToFloatPtr() );
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
	shadowMipMap = qglGetUniformLocation( program, "u_shadowMipMap" );

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

	if ( !translucent && ( backEnd.vLight->globalShadows || backEnd.vLight->localShadows || r_shadows.GetInteger() == 2 ) && !backEnd.viewDef->IsLightGem() ) {
		qglUniform1i( softShadowsQuality, r_softShadowsQuality.GetInteger() );
		qglUniform1f( softShadowsRadius, r_softShadowsRadius.GetFloat() );

		int sampleK = r_softShadowsQuality.GetInteger();
		if ( sampleK > 0 ) { // texcoords for screen-space softener filter
			if ( g_softShadowsSamples.Num() != sampleK || g_softShadowsSamples.Num() == 0 ) {
				GeneratePoissonDiskSampling( g_softShadowsSamples, sampleK );
				qglUniform2fv( softShadowSamples, sampleK, ( float * )g_softShadowsSamples.Ptr() );
			}
		}
		if ( sampleK < 0 ) { // WIP low res stencil shadows
			qglUniform2f( renderResolution, glConfig.vidWidth, glConfig.vidHeight );
		}
	} else {
		qglUniform1i( softShadowsQuality, 0 );
		qglUniform1f( softShadowsRadius, 0.0f );
	}
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
	qglUniformMatrix4fv( modelMatrix, 1, false, din->surf->space->modelMatrix );
	qglUniform4fv( lightOrigin, 1, din->localLightOrigin.ToFloatPtr() );
	qglUniform3fv( lightOrigin2, 1, backEnd.vLight->globalLightOrigin.ToFloatPtr() );
	GL_CheckErrors();
}

void ambientInteractionProgram_t::AfterLoad() {
	interactionProgram_t::AfterLoad();
	gamma = qglGetUniformLocation( program, "u_gamma" );
}

void ambientInteractionProgram_t::UpdateUniforms( const drawInteraction_t *din ) {
	interactionProgram_t::UpdateUniforms( din );
	qglUniform1f( gamma, backEnd.viewDef->IsLightGem() ? 0 : r_gamma.GetFloat() - 1 );
	qglUniform4fv( lightOrigin, 1, din->worldUpLocal.ToFloatPtr() );
	qglUniformMatrix4fv( modelMatrix, 1, false, din->surf->space->modelMatrix );
	GL_CheckErrors();
}