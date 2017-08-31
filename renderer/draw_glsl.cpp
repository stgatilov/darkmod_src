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

shaderProgram_t cubeMapShader;
oldStageProgram_t oldStageShader;
depthProgram_t depthShader;
lightProgram_t stencilShadowShader;
fogProgram_t fogShader;
blendProgram_t blendShader;
interactionProgram_t interactionShader, ambientInteractionShader;

/*
=========================================================================================

GENERAL INTERACTION RENDERING

=========================================================================================
*/

/*
====================
GL_SelectTextureNoClient
====================
*/
static void GL_SelectTextureNoClient( int unit ) {
	backEnd.glState.currenttmu = unit;
	qglActiveTextureARB( GL_TEXTURE0_ARB + unit );
	RB_LogComment( "glActiveTexture( %i )\n", unit );
}

/*
==================
RB_GLSL_DrawInteraction
==================
*/
void RB_GLSL_DrawInteraction( const drawInteraction_t *din ) {
    static const float zero[4] = { 0, 0, 0, 0 };
    static const float one[4] = { 1, 1, 1, 1 };
    static const float negOne[4] = { -1, -1, -1, -1 };
    
	interactionProgram_t *shader = din->ambientLight ? &ambientInteractionShader : &interactionShader;
    // load all the shader parameters
	qglUniform4fv( shader->lightProjectionS, 1, din->lightProjection[0].ToFloatPtr() );
	qglUniform4fv( shader->lightProjectionT, 1, din->lightProjection[1].ToFloatPtr() );
	qglUniform4fv( shader->lightProjectionQ, 1, din->lightProjection[2].ToFloatPtr() );
	qglUniform4fv( shader->lightFalloff, 1, din->lightProjection[3].ToFloatPtr() );
	qglUniform4fv( shader->bumpMatrixS, 1, din->bumpMatrix[0].ToFloatPtr() );
	qglUniform4fv( shader->bumpMatrixT, 1, din->bumpMatrix[1].ToFloatPtr() );
	qglUniform4fv( shader->diffuseMatrixS, 1, din->diffuseMatrix[0].ToFloatPtr() );
	qglUniform4fv( shader->diffuseMatrixT, 1, din->diffuseMatrix[1].ToFloatPtr() );
	// set the constant color
	qglUniform4fv( shader->diffuseColor, 1, din->diffuseColor.ToFloatPtr() );
	qglUniform4fv( shader->diffuseColor, 1, din->diffuseColor.ToFloatPtr() );
	switch ( din->vertexColor ) {
	case SVC_IGNORE:
		qglUniform4f( shader->colorModulate, zero[0], zero[1], zero[2], zero[3] );
		qglUniform4f( shader->colorAdd, one[0], one[1], one[2], one[3] );
		break;
	case SVC_MODULATE:
		qglUniform4f( shader->colorModulate, one[0], one[1], one[2], one[3] );
		qglUniform4f( shader->colorAdd, zero[0], zero[1], zero[2], zero[3] );
		break;
	case SVC_INVERSE_MODULATE:
		qglUniform4f( shader->colorModulate, negOne[0], negOne[1], negOne[2], negOne[3] );
		qglUniform4f( shader->colorAdd, one[0], one[1], one[2], one[3] );
		break;
	}
	if ( !din->ambientLight ) {
		qglUniform4fv( interactionShader.localViewOrigin, 1, din->localViewOrigin.ToFloatPtr() );
		qglUniform4fv( interactionShader.specularMatrixS, 1, din->specularMatrix[0].ToFloatPtr() );
		qglUniform4fv( interactionShader.specularMatrixT, 1, din->specularMatrix[1].ToFloatPtr() );
		qglUniform4fv( interactionShader.specularColor, 1, din->specularColor.ToFloatPtr() );
		qglUniform4fv( shader->localLightOrigin, 1, din->localLightOrigin.ToFloatPtr() );
	} else {
		qglUniform4fv( shader->localLightOrigin, 1, din->worldUpLocal.ToFloatPtr() );
	}

	// set the textures

	// texture 0 will be the per-surface bump map
	GL_SelectTextureNoClient( 0 );
	din->bumpImage->Bind();

	// texture 1 will be the light falloff texture
	GL_SelectTextureNoClient( 1 );
	din->lightFalloffImage->Bind();

	// texture 2 will be the light projection texture
	GL_SelectTextureNoClient( 2 );
	din->lightImage->Bind();

	// texture 3 is the per-surface diffuse map
	GL_SelectTextureNoClient( 3 );
	din->diffuseImage->Bind();

	if ( !din->ambientLight ) {
		// texture 4 is the per-surface specular map
		GL_SelectTextureNoClient( 4 );
		din->specularImage->Bind();
	}

	// draw it
	RB_DrawElementsWithCounters( din->surf->backendGeo );
}


/*
=============
RB_GLSL_CreateDrawInteractions
=============
*/
static void RB_GLSL_CreateDrawInteractions( const drawSurf_t *surf ) {
	if ( !surf ) {
		return;
	}

	// perform setup here that will be constant for all interactions
	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | backEnd.depthFunc );

	// bind the vertex and fragment program
	if ( backEnd.vLight->lightShader->IsAmbientLight() ) {
		qglUseProgram( ambientInteractionShader.program );
	} else {
		qglUseProgram( interactionShader.program );
	}

	// enable the vertex arrays
	qglEnableVertexAttribArrayARB( 8 );
	qglEnableVertexAttribArrayARB( 9 );
	qglEnableVertexAttribArrayARB( 10 );
	qglEnableVertexAttribArrayARB( 11 );
	//qglEnableClientState( GL_COLOR_ARRAY );
	qglEnableVertexAttribArrayARB(3);

	for ( ; surf ; surf=surf->nextOnLight ) {
		// perform setup here that will not change over multiple interaction passes

		// set the vertex pointers
		idDrawVert	*ac = (idDrawVert *)vertexCache.Position( surf->backendGeo->ambientCache );
		//qglColorPointer( 4, GL_UNSIGNED_BYTE, sizeof( idDrawVert ), ac->color );
		qglVertexAttribPointerARB(3, 4, GL_UNSIGNED_BYTE, true, sizeof(idDrawVert), &ac->color);
		qglVertexAttribPointerARB(11, 3, GL_FLOAT, false, sizeof(idDrawVert), ac->normal.ToFloatPtr());
		qglVertexAttribPointerARB( 10, 3, GL_FLOAT, false, sizeof( idDrawVert ), ac->tangents[1].ToFloatPtr() );
		qglVertexAttribPointerARB( 9, 3, GL_FLOAT, false, sizeof( idDrawVert ), ac->tangents[0].ToFloatPtr() );
		qglVertexAttribPointerARB( 8, 2, GL_FLOAT, false, sizeof( idDrawVert ), ac->st.ToFloatPtr() );
		qglVertexAttribPointerARB( 0, 3, GL_FLOAT, false, sizeof( idDrawVert ), &ac->xyz );

		// set model matrix
		if ( backEnd.vLight->lightShader->IsAmbientLight() ) {
		} else {
			qglUniform1f( interactionShader.advanced, r_testARBProgram.GetFloat());
		}

		// this may cause RB_GLSL_DrawInteraction to be executed multiple
		// times with different colors and images if the surface or light have multiple layers
		RB_CreateSingleDrawInteractions( surf/*, RB_GLSL_DrawInteraction*/ );
	}

	qglDisableVertexAttribArrayARB( 8 );
	qglDisableVertexAttribArrayARB( 9 );
	qglDisableVertexAttribArrayARB( 10 );
	qglDisableVertexAttribArrayARB( 11 );
	//qglDisableClientState( GL_COLOR_ARRAY );
	qglDisableVertexAttribArrayARB(3);

	// disable features
	GL_SelectTextureNoClient( 4 );
	globalImages->BindNull();

	GL_SelectTextureNoClient( 3 );
	globalImages->BindNull();

	GL_SelectTextureNoClient( 2 );
	globalImages->BindNull();

	GL_SelectTextureNoClient( 1 );
	globalImages->BindNull();

	backEnd.glState.currenttmu = -1;
	GL_SelectTexture( 0 );

	qglUseProgram( 0 );
}


/*
==================
RB_GLSL_DrawInteractions
==================
*/
void RB_GLSL_DrawInteractions( void ) {
	viewLight_t		*vLight;

	GL_SelectTexture( 0 );

	//
	// for each light, perform adding and shadowing
	//
	for ( vLight = backEnd.viewDef->viewLights ; vLight ; vLight = vLight->next ) {
		backEnd.vLight = vLight;

		// do fogging later
		if ( vLight->lightShader->IsFogLight() ) {
			continue;
		}
		if ( vLight->lightShader->IsBlendLight() ) {
			continue;
		}

		// if there are no interactions, get out!
		if ( !vLight->localInteractions && !vLight->globalInteractions && 
			!vLight->translucentInteractions ) {
			continue;
		}

		// clear the stencil buffer if needed
		if ( vLight->globalShadows || vLight->localShadows ) {
			backEnd.currentScissor = vLight->scissorRect;
			if ( r_useScissor.GetBool() ) {
				qglScissor( backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1, 
					backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
					backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
					backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1 );
			}
			qglClear( GL_STENCIL_BUFFER_BIT );
		} else {
			// no shadows, so no need to read or write the stencil buffer
			// we might in theory want to use GL_ALWAYS instead of disabling
			// completely, to satisfy the invarience rules
			qglStencilFunc( GL_ALWAYS, 128, 255 );
		}

		qglUseProgram( stencilShadowShader.program );

		RB_StencilShadowPass( vLight->globalShadows );
		RB_GLSL_CreateDrawInteractions( vLight->localInteractions );

		qglUseProgram( stencilShadowShader.program );
		RB_StencilShadowPass( vLight->localShadows );
		RB_GLSL_CreateDrawInteractions( vLight->globalInteractions );

		qglUseProgram( 0 );	// if there weren't any globalInteractions, it would have stayed on

		// translucent surfaces never get stencil shadowed
		if ( r_skipTranslucent.GetBool() ) {
			continue;
		}

		qglStencilFunc( GL_ALWAYS, 128, 255 );

		backEnd.depthFunc = GLS_DEPTHFUNC_LESS;
		RB_GLSL_CreateDrawInteractions( vLight->translucentInteractions );
		backEnd.depthFunc = GLS_DEPTHFUNC_EQUAL;
	}

	// disable stencil shadow test
	qglStencilFunc( GL_ALWAYS, 128, 255 );

	GL_SelectTexture( 0 );
}

/*
==================
R_ReloadGLSLShaders_f
==================
*/
bool R_ReloadGLSLPrograms() {
	if ( !interactionShader.Load( "interaction" ) )
		return false;
	if ( !ambientInteractionShader.Load( "ambientInteraction" ) )
		return false;
	if ( !stencilShadowShader.Load( "stencilshadow" ) )
		return false;
	if ( !oldStageShader.Load( "oldStage" ) )
		return false;
	if ( !depthShader.Load( "depthAlpha" ) )
		return false;
	if ( !fogShader.Load( "fog" ) )
		return false;
	if ( !blendShader.Load( "blend" ) )
		return false;
	if ( !cubeMapShader.Load( "cubeMap" ) )
		return false;
	return true;
}

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
GLuint shaderProgram_t::CompileShader( GLint ShaderType, idStr &fileName ) {
	char *source;
	GLuint shader;
	GLint length, result;

	/* get shader source */
	char    *fileBuffer;

	// load the program even if we don't support it
	fileSystem->ReadFile( fileName, (void **)&fileBuffer, NULL );
	if ( !fileBuffer ) {
		common->Warning( "shaderCompileFromFile: \'%s\' not found", fileName.c_str() );
		return 0;
	}

	common->Printf( "%s\n", fileName.c_str() );

	source = fileBuffer;

	/* create shader object, set the source, and compile */
	shader = qglCreateShader( ShaderType );
	length = strlen( source );
	qglShaderSource( shader, 1, (const char **)&source, &length );
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
		common->Warning( "shaderCompileFromFile(): Unable to compile %s: %s\n", fileName.c_str(), log );
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
void shaderProgram_t::AttachShader( GLint ShaderType, char *fileName ) {
	/* compile the shader */
	GLuint shader = CompileShader( ShaderType, idStr( "glprogs/" ) + fileName + (ShaderType == GL_FRAGMENT_SHADER ? ".fs" : ".vs") );
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
bool shaderProgram_t::Load( char *fileName ) {
	if ( program )
		qglDeleteProgram( program );
	program = qglCreateProgram();
	AttachShader( GL_VERTEX_SHADER, fileName );
	AttachShader( GL_FRAGMENT_SHADER, fileName );
	qglBindAttribLocation( program, 3, "attr_Color" );
	qglBindAttribLocation( program, 8, "attr_TexCoord" );
	qglBindAttribLocation( program, 9, "attr_Tangent" );
	qglBindAttribLocation( program, 10, "attr_Bitangent" );
	qglBindAttribLocation( program, 11, "attr_Normal" );
	//qglBindAttribLocation( prog.genId, 3, "Color" );
	//qglBindAttribLocation( prog.genId, 8, "TexCoord0" );

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
		common->Warning( "Program linking failed: %s\n", log );
		delete log;

		/* delete the program */
		qglDeleteProgram( program );
		program = 0;
		return false;
	}

	GLint validProgram;
	qglValidateProgram( program );
	qglGetProgramiv( program, GL_OBJECT_VALIDATE_STATUS_ARB, &validProgram );
	if ( !validProgram ) {
		common->Printf( "R_ValidateGLSLProgram: program invalid\n" );
		return false;
	}

	AfterLoad();
	return true;
}

void shaderProgram_t::AfterLoad() {

}

void oldStageProgram_t::AfterLoad() {
	texPlaneS = qglGetUniformLocation( program, "texPlaneS" );
	texPlaneT = qglGetUniformLocation( program, "texPlaneT" );
	texPlaneQ = qglGetUniformLocation( program, "texPlaneQ" );
	screenTex = qglGetUniformLocation( program, "screenTex" );
	colorMul = qglGetUniformLocation( program, "colorMul" );
	colorAdd = qglGetUniformLocation( program, "colorAdd" );
}

void depthProgram_t::AfterLoad() {
	clipPlane = qglGetUniformLocation( program, "clipPlane" );
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
	tex1PlaneS = qglGetUniformLocation( program, "tex1PlaneS" );
	texture1 = qglGetUniformLocation( program, "texture1" );
	fogColor = qglGetUniformLocation( program, "fogColor" );
	fogEnter = qglGetUniformLocation( program, "fogEnter" );
}

void lightProgram_t::AfterLoad() {
	localLightOrigin = qglGetUniformLocation( program, "u_lightOrigin" );
}

void interactionProgram_t::AfterLoad() {
	lightProgram_t::AfterLoad();
	u_normalTexture = qglGetUniformLocation( program, "u_normalTexture" );
	u_lightFalloffTexture = qglGetUniformLocation( program, "u_lightFalloffTexture" );
	u_lightProjectionTexture = qglGetUniformLocation( program, "u_lightProjectionTexture" );
	u_diffuseTexture = qglGetUniformLocation( program, "u_diffuseTexture" );
	u_specularTexture = qglGetUniformLocation( program, "u_specularTexture" );

	localViewOrigin = qglGetUniformLocation( program, "u_viewOrigin" );
	lightProjectionS = qglGetUniformLocation( program, "u_lightProjectionS" );
	lightProjectionT = qglGetUniformLocation( program, "u_lightProjectionT" );
	lightProjectionQ = qglGetUniformLocation( program, "u_lightProjectionQ" );
	lightFalloff = qglGetUniformLocation( program, "u_lightFalloff" );
	advanced = qglGetUniformLocation( program, "u_advanced" );

	bumpMatrixS = qglGetUniformLocation( program, "u_bumpMatrixS" );
	bumpMatrixT = qglGetUniformLocation( program, "u_bumpMatrixT" );
	diffuseMatrixS = qglGetUniformLocation( program, "u_diffuseMatrixS" );
	diffuseMatrixT = qglGetUniformLocation( program, "u_diffuseMatrixT" );
	specularMatrixS = qglGetUniformLocation( program, "u_specularMatrixS" );
	specularMatrixT = qglGetUniformLocation( program, "u_specularMatrixT" );

	colorModulate = qglGetUniformLocation( program, "u_colorModulate" );
	colorAdd = qglGetUniformLocation( program, "u_colorAdd" );

	diffuseColor = qglGetUniformLocation( program, "u_diffuseColor" );
	specularColor = qglGetUniformLocation( program, "u_specularColor" );

	// set texture locations
	qglUseProgram( program );
	qglUniform1i( u_normalTexture, 0 );
	qglUniform1i( u_lightFalloffTexture, 1 );
	qglUniform1i( u_lightProjectionTexture, 2 );
	qglUniform1i( u_diffuseTexture, 3 );
	qglUniform1i( u_specularTexture, 4 );
	qglUseProgram( 0 );
}
