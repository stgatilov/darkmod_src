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

shaderProgram_t		interactionShader, ambientInteractionShader, stencilShadowShader;

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
    
	shaderProgram_t *shader = din->ambientLight ? &ambientInteractionShader : &interactionShader;
    // load all the shader parameters
	qglUniform4fv( shader->localLightOrigin, 1, din->localLightOrigin.ToFloatPtr() );
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
			qglUniformMatrix4fv( ambientInteractionShader.modelMatrix, 1, false, surf->space->modelMatrix );
		} else {
			qglUniformMatrix4fv( interactionShader.modelMatrix, 1, false, surf->space->modelMatrix );
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
	//qglDisableClientState( GL_TEXTURE_COORD_ARRAY );

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

		if ( r_useShadowVertexProgram.GetBool() ) {
			qglUseProgram( stencilShadowShader.program );

			RB_StencilShadowPass( vLight->globalShadows );
			RB_GLSL_CreateDrawInteractions( vLight->localInteractions );

			qglUseProgram( stencilShadowShader.program );
			RB_StencilShadowPass( vLight->localShadows );
			RB_GLSL_CreateDrawInteractions( vLight->globalInteractions );

			qglUseProgram( 0 );	// if there weren't any globalInteractions, it would have stayed on
		} else {
			RB_StencilShadowPass( vLight->globalShadows );
			RB_GLSL_CreateDrawInteractions( vLight->localInteractions );

			RB_StencilShadowPass( vLight->localShadows );
			RB_GLSL_CreateDrawInteractions( vLight->globalInteractions );
		}

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
	//qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
}

/*
=================
R_LoadGLSLShader

loads GLSL vertex or fragment shaders
=================
*/
bool R_LoadGLSLShader( const char *name, shaderProgram_t *shaderProgram, GLenum type ) {
	idStr	fullPath = "glprogs/";
	fullPath += name;
	char	*fileBuffer;
	char	*buffer;

	common->Printf( "%s", fullPath.c_str() );

	// load the program even if we don't support it, so
	// fs_copyfiles can generate cross-platform data dumps
	fileSystem->ReadFile( fullPath.c_str(), (void **)&fileBuffer, NULL );
	if ( !fileBuffer ) {
		common->Printf( ": File not found\n" );
		return false;
	}

	// copy to stack memory and free
	buffer = (char *)_alloca( strlen( fileBuffer ) + 1 );
	strcpy( buffer, fileBuffer );
	fileSystem->FreeFile( fileBuffer );

	if ( !glConfig.isInitialized ) {
		return false;
	}

	GLhandleARB* shader;

	switch ( type ) {
	case GL_VERTEX_SHADER_ARB:
		// create and compile vertex shader
		shader = &shaderProgram->vertexShader;
		shaderProgram->vertexShader = qglCreateShader( GL_VERTEX_SHADER_ARB );
		break;
	case GL_FRAGMENT_SHADER_ARB:
		// create and copmpile fragment shader
		shader = &shaderProgram->fragmentShader;
		shaderProgram->fragmentShader = qglCreateShader( GL_FRAGMENT_SHADER_ARB );
		break;
	default:
		common->Printf( "R_LoadGLSLShader: no type\n" );
		return false;
	}
	qglShaderSource( *shader, 1, (const GLcharARB **)&buffer, 0 );
	qglCompileShader( *shader );

	/* make sure the compilation was successful */
	GLint length, result;
	qglGetShaderiv( *shader, GL_COMPILE_STATUS, &result );
	if ( result == GL_FALSE ) {
		char *log;

		/* get the shader info log */
		qglGetShaderiv( *shader, GL_INFO_LOG_LENGTH, &length );
		log = new char[length];
		qglGetShaderInfoLog( *shader, length, &result, log );

		/* print an error message and the info log */
		common->Warning( "shaderCompileFromFile(): Unable to compile %s: %s\n", fullPath.c_str(), log );
		delete log;

		qglDeleteShader( *shader );
		return 0;
	}

	common->Printf( "\n" );
	return true;
}

/*
=================
R_LinkGLSLShader

links the GLSL vertex and fragment shaders together to form a GLSL program
=================
*/
bool R_LinkGLSLShader( shaderProgram_t *shaderProgram, bool needsAttributes ) {
	GLint result;

	if ( shaderProgram->program )
		qglDeleteProgram( shaderProgram->program );
	shaderProgram->program = qglCreateProgram();

	qglAttachShader( shaderProgram->program, shaderProgram->vertexShader );
	qglAttachShader( shaderProgram->program, shaderProgram->fragmentShader );

	if ( needsAttributes ) {
		qglBindAttribLocation(shaderProgram->program, 3, "attr_Color");
		qglBindAttribLocation(shaderProgram->program, 8, "attr_TexCoord");
		qglBindAttribLocation( shaderProgram->program, 9, "attr_Tangent" );
		qglBindAttribLocation( shaderProgram->program, 10, "attr_Bitangent" );
		qglBindAttribLocation( shaderProgram->program, 11, "attr_Normal" );
	}

	qglLinkProgram( shaderProgram->program );

	qglGetProgramiv( shaderProgram->program, GL_LINK_STATUS, &result );
	if ( !result ) {
		GLint length;
		char *log;

		/* get the program info log */
		qglGetProgramiv( shaderProgram->program, GL_INFO_LOG_LENGTH, &length );
		log = new char[length];
		qglGetProgramInfoLog( shaderProgram->program, length, &result, log );

		/* print an error message and the info log */
		common->Warning( "Program linking failed: %s\n", log );
		delete log;

		/* delete the program */
		qglDeleteProgram( shaderProgram->program );
		shaderProgram->program = 0;
		return false;
	}

	return true;
}

/*
=================
R_ValidateGLSLProgram

makes sure GLSL program is valid
=================
*/
bool R_ValidateGLSLProgram( shaderProgram_t *shaderProgram ) {
	GLint validProgram;

	qglValidateProgram( shaderProgram->program );

	qglGetProgramiv( shaderProgram->program, GL_OBJECT_VALIDATE_STATUS_ARB, &validProgram );
	if ( !validProgram ) {
		common->Printf( "R_ValidateGLSLProgram: program invalid\n" );
		return false;
	}

	return true;
}

static bool RB_GLSL_InitShaders() {
	// load interation shaders
	R_LoadGLSLShader( "interaction.vs", &interactionShader, GL_VERTEX_SHADER_ARB );
	R_LoadGLSLShader( "interaction.fs", &interactionShader, GL_FRAGMENT_SHADER_ARB );
	if ( R_LinkGLSLShader( &interactionShader, true ) && R_ValidateGLSLProgram( &interactionShader ) ) {
		// set uniform locations
		interactionShader.u_normalTexture = qglGetUniformLocation( interactionShader.program, "u_normalTexture" );
		interactionShader.u_lightFalloffTexture = qglGetUniformLocation( interactionShader.program, "u_lightFalloffTexture" );
		interactionShader.u_lightProjectionTexture = qglGetUniformLocation( interactionShader.program, "u_lightProjectionTexture" );
		interactionShader.u_diffuseTexture = qglGetUniformLocation( interactionShader.program, "u_diffuseTexture" );
		interactionShader.u_specularTexture = qglGetUniformLocation( interactionShader.program, "u_specularTexture" );

		interactionShader.modelMatrix = qglGetUniformLocation( interactionShader.program, "u_modelMatrix" );

		interactionShader.localLightOrigin = qglGetUniformLocation( interactionShader.program, "u_lightOrigin" );
		interactionShader.localViewOrigin = qglGetUniformLocation( interactionShader.program, "u_viewOrigin" );
		interactionShader.lightProjectionS = qglGetUniformLocation( interactionShader.program, "u_lightProjectionS" );
		interactionShader.lightProjectionT = qglGetUniformLocation( interactionShader.program, "u_lightProjectionT" );
		interactionShader.lightProjectionQ = qglGetUniformLocation( interactionShader.program, "u_lightProjectionQ" );
		interactionShader.lightFalloff = qglGetUniformLocation( interactionShader.program, "u_lightFalloff" );
		interactionShader.advanced = qglGetUniformLocation( interactionShader.program, "u_advanced" );

		interactionShader.bumpMatrixS = qglGetUniformLocation( interactionShader.program, "u_bumpMatrixS" );
		interactionShader.bumpMatrixT = qglGetUniformLocation( interactionShader.program, "u_bumpMatrixT" );
		interactionShader.diffuseMatrixS = qglGetUniformLocation( interactionShader.program, "u_diffuseMatrixS" );
		interactionShader.diffuseMatrixT = qglGetUniformLocation( interactionShader.program, "u_diffuseMatrixT" );
		interactionShader.specularMatrixS = qglGetUniformLocation( interactionShader.program, "u_specularMatrixS" );
		interactionShader.specularMatrixT = qglGetUniformLocation( interactionShader.program, "u_specularMatrixT" );

		interactionShader.colorModulate = qglGetUniformLocation( interactionShader.program, "u_colorModulate" );
		interactionShader.colorAdd = qglGetUniformLocation( interactionShader.program, "u_colorAdd" );

		interactionShader.diffuseColor = qglGetUniformLocation( interactionShader.program, "u_diffuseColor" );
		interactionShader.specularColor = qglGetUniformLocation( interactionShader.program, "u_specularColor" );

		// set texture locations
		qglUseProgram( interactionShader.program );
		qglUniform1i( interactionShader.u_normalTexture, 0 );
		qglUniform1i( interactionShader.u_lightFalloffTexture, 1 );
		qglUniform1i( interactionShader.u_lightProjectionTexture, 2 );
		qglUniform1i( interactionShader.u_diffuseTexture, 3 );
		qglUniform1i( interactionShader.u_specularTexture, 4 );
		qglUseProgram( 0 );
    } else {
        common->Printf( "GLSL interactionShader failed to init.\n" );
        return false;
	}

	// load ambient interation shaders
	R_LoadGLSLShader( "ambientInteraction.vs", &ambientInteractionShader, GL_VERTEX_SHADER_ARB );
	R_LoadGLSLShader( "ambientInteraction.fs", &ambientInteractionShader, GL_FRAGMENT_SHADER_ARB );
	if ( R_LinkGLSLShader( &ambientInteractionShader, true ) && R_ValidateGLSLProgram( &ambientInteractionShader ) ) {
		// set uniform locations
		ambientInteractionShader.u_normalTexture = qglGetUniformLocation( ambientInteractionShader.program, "u_normalTexture" );
		ambientInteractionShader.u_lightFalloffTexture = qglGetUniformLocation( ambientInteractionShader.program, "u_lightFalloffTexture" );
		ambientInteractionShader.u_lightProjectionTexture = qglGetUniformLocation( ambientInteractionShader.program, "u_lightProjectionTexture" );
		ambientInteractionShader.u_diffuseTexture = qglGetUniformLocation( ambientInteractionShader.program, "u_diffuseTexture" );

		ambientInteractionShader.modelMatrix = qglGetUniformLocation( ambientInteractionShader.program, "u_modelMatrix" );

		ambientInteractionShader.localLightOrigin = qglGetUniformLocation( ambientInteractionShader.program, "u_lightOrigin" );
		ambientInteractionShader.lightProjectionS = qglGetUniformLocation( ambientInteractionShader.program, "u_lightProjectionS" );
		ambientInteractionShader.lightProjectionT = qglGetUniformLocation( ambientInteractionShader.program, "u_lightProjectionT" );
		ambientInteractionShader.lightProjectionQ = qglGetUniformLocation( ambientInteractionShader.program, "u_lightProjectionQ" );
		ambientInteractionShader.lightFalloff = qglGetUniformLocation( ambientInteractionShader.program, "u_lightFalloff" );

		ambientInteractionShader.bumpMatrixS = qglGetUniformLocation( ambientInteractionShader.program, "u_bumpMatrixS" );
		ambientInteractionShader.bumpMatrixT = qglGetUniformLocation( ambientInteractionShader.program, "u_bumpMatrixT" );
		ambientInteractionShader.diffuseMatrixS = qglGetUniformLocation( ambientInteractionShader.program, "u_diffuseMatrixS" );
		ambientInteractionShader.diffuseMatrixT = qglGetUniformLocation( ambientInteractionShader.program, "u_diffuseMatrixT" );

		ambientInteractionShader.colorModulate = qglGetUniformLocation( ambientInteractionShader.program, "u_colorModulate" );
		ambientInteractionShader.colorAdd = qglGetUniformLocation( ambientInteractionShader.program, "u_colorAdd" );

		ambientInteractionShader.diffuseColor = qglGetUniformLocation( ambientInteractionShader.program, "u_diffuseColor" );

		// set texture locations
		qglUseProgram( ambientInteractionShader.program );
		qglUniform1i( ambientInteractionShader.u_normalTexture, 0 );
		qglUniform1i( ambientInteractionShader.u_lightFalloffTexture, 1 );
		qglUniform1i( ambientInteractionShader.u_lightProjectionTexture, 2 );
		qglUniform1i( ambientInteractionShader.u_diffuseTexture, 3 );
		qglUseProgram( 0 );
    } else {
        common->Printf( "GLSL ambientInteractionShader failed to init.\n" );
        return false;
	}

	// load stencil shadow extrusion shaders
	R_LoadGLSLShader( "stencilshadow.vs", &stencilShadowShader, GL_VERTEX_SHADER_ARB );
	R_LoadGLSLShader( "stencilshadow.fs", &stencilShadowShader, GL_FRAGMENT_SHADER_ARB );
	if ( R_LinkGLSLShader( &stencilShadowShader, false ) && R_ValidateGLSLProgram( &stencilShadowShader ) ) {
		// set uniform locations
		stencilShadowShader.localLightOrigin = qglGetUniformLocation( stencilShadowShader.program, "u_lightOrigin" );
    } else {
        common->Printf( "GLSL stencilShadowShader failed to init.\n" );
        return false;
    }

	return true;
}

/*
==================
R_ReloadGLSLShaders_f
==================
*/
void R_ReloadGLSLShaders_f( const idCmdArgs &args ) {
	RB_GLSL_InitShaders();
}

/*
 ==================
 R_GLSL_Init
 ==================
 */
void R_GLSL_Init( void ) {
	//glConfig.allowGLSLPath = false;

	common->Printf( "---------- R_GLSL_Init -----------\n" );

	/*if ( !glConfig.GLSLAvailable ) {
		common->Printf( "Not available.\n" );
		return;
	} else */if ( !RB_GLSL_InitShaders() ) {
		common->Printf( "GLSL shaders failed to init.\n" );
		return;
	}
	
	common->Printf( "Available.\n" );

	common->Printf( "---------------------------------\n" );

	//glConfig.allowGLSLPath = true;
}

