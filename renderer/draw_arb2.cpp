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

#include "tr_local.h"
#include "Profiling.h"

/*
=========================================================================================

GENERAL INTERACTION RENDERING

=========================================================================================
*/

/*
==================
RB_ARB2_DrawInteraction
==================
*/
void RB_ARB2_DrawInteraction( const drawInteraction_t *din ) {
	// load all the vertex program parameters
	qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_LIGHT_ORIGIN, din->localLightOrigin.ToFloatPtr() );
	qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_VIEW_ORIGIN, din->localViewOrigin.ToFloatPtr() );
	qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_LIGHT_PROJECT_S, din->lightProjection[0].ToFloatPtr() );
	qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_LIGHT_PROJECT_T, din->lightProjection[1].ToFloatPtr() );
	qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_LIGHT_PROJECT_Q, din->lightProjection[2].ToFloatPtr() );
	qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_LIGHT_FALLOFF_S, din->lightProjection[3].ToFloatPtr() );
	qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_BUMP_MATRIX_S, din->bumpMatrix[0].ToFloatPtr() );
	qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_BUMP_MATRIX_T, din->bumpMatrix[1].ToFloatPtr() );
	qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_DIFFUSE_MATRIX_S, din->diffuseMatrix[0].ToFloatPtr() );
	qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_DIFFUSE_MATRIX_T, din->diffuseMatrix[1].ToFloatPtr() );
	qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_SPECULAR_MATRIX_S, din->specularMatrix[0].ToFloatPtr() );
	qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_SPECULAR_MATRIX_T, din->specularMatrix[1].ToFloatPtr() );

	// rebb: pass world-up in local coords to fragment program for ambient lights
	// nbohr1more #3881: cubemap based lighting (copy rebb's changes for uniformity, may not be required depending on usage and testing )
	if ( din->ambientLight ) {
		qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_MISC_0, din->worldUpLocal.ToFloatPtr() );
	}

	// testing fragment based normal mapping
	if ( r_testARBProgram.GetBool() ) {
		qglProgramEnvParameter4fvARB( GL_FRAGMENT_PROGRAM_ARB, 2, din->localLightOrigin.ToFloatPtr() );
		qglProgramEnvParameter4fvARB( GL_FRAGMENT_PROGRAM_ARB, 3, din->localViewOrigin.ToFloatPtr() );
	}
	static const float zero[4] = { 0, 0, 0, 0 };
	static const float one[4] = { 1, 1, 1, 1 };
	static const float negOne[4] = { -1, -1, -1, -1 };

	switch ( din->vertexColor ) {
	case SVC_IGNORE:
		qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_COLOR_MODULATE, zero );
		qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_COLOR_ADD, one );
		break;
	case SVC_MODULATE:
		qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_COLOR_MODULATE, one );
		qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_COLOR_ADD, zero );
		break;
	case SVC_INVERSE_MODULATE:
		qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_COLOR_MODULATE, negOne );
		qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_COLOR_ADD, one );
		break;
	}

	// set the constant colors
	qglProgramEnvParameter4fvARB( GL_FRAGMENT_PROGRAM_ARB, 0, din->diffuseColor.ToFloatPtr() );
	qglProgramEnvParameter4fvARB( GL_FRAGMENT_PROGRAM_ARB, 1, din->specularColor.ToFloatPtr() );

	// set the textures

	// texture 1 will be the per-surface bump map
	GL_SelectTexture( 1 );
	din->bumpImage->Bind();

	// texture 2 will be the light falloff texture
	GL_SelectTexture( 2 );
	din->lightFalloffImage->Bind();

	// texture 3 will be the light projection texture
	GL_SelectTexture( 3 );
	din->lightImage->Bind();

	// texture 4 is the per-surface diffuse map
	GL_SelectTexture( 4 );
	din->diffuseImage->Bind();

	// texture 5 is the per-surface specular map
	GL_SelectTexture( 5 );
	din->specularImage->Bind();

	// draw it
	RB_DrawElementsWithCounters( din->surf );
}

/*
=============
RB_ARB2_CreateDrawInteractions
=============
*/
void RB_ARB2_CreateDrawInteractions( const drawSurf_t *surf ) {
	if ( !surf ) {
		return;
	}
	GL_PROFILE( "ARB2_CreateDrawInteractions" );

	// perform setup here that will be constant for all interactions
	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | backEnd.depthFunc );

	// bind the vertex program
	// rebb: support dedicated ambient - CVar and direct interactions can probably be removed, they're there mainly for performance testing
	// nbohr1more #3881: removed the direct interaction cvar toggle
	// nbohr1more #3881: dedicated cubemap lighting
	if ( backEnd.vLight->lightShader->IsCubicLight() ) {
		if ( backEnd.vLight->lightShader->IsAmbientLight() ) {
			qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, VPROG_AMBIENT_CUBE_LIGHT );
			qglBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, FPROG_AMBIENT_CUBE_LIGHT );
		} else { // nbohr1more #3881: dedicated cubemap lighting (further changes)
			if ( backEnd.vLight->lightDef->parms.pointLight ) {
				qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, VPROG_TEST_CUBIC_POINT );
				qglBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, FPROG_TEST_CUBIC_POINT );
			} else {
				qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, VPROG_TEST_CUBIC_PROJ );
				qglBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, FPROG_TEST_CUBIC_PROJ );
			}
		}
	} else if ( backEnd.vLight->lightShader->IsAmbientLight() ) {
		qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, VPROG_AMBIENT );
		qglBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, FPROG_AMBIENT );
	} else {
		qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, VPROG_TEST_DIRECT );
		qglBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, FPROG_TEST_DIRECT );
	}
	qglEnable( GL_VERTEX_PROGRAM_ARB );
	qglEnable( GL_FRAGMENT_PROGRAM_ARB );

	// enable the vertex arrays
	qglEnableVertexAttribArray( 8 );
	qglEnableVertexAttribArray( 9 );
	qglEnableVertexAttribArray( 10 );
	qglEnableVertexAttribArray( 11 );
	qglEnableVertexAttribArray( 3 );

	// texture 0 is the normalization cube map for the vector towards the light
	GL_SelectTexture( 0 );

	if ( backEnd.vLight->lightShader->IsAmbientLight() ) {
		globalImages->ambientNormalMap->Bind();
	} else {
		globalImages->normalCubeMapImage->Bind();
	}

	// texture 6 is the specular lookup table
	GL_SelectTexture( 6 );

	if ( r_testARBProgram.GetBool() ) {
		globalImages->specular2DTableImage->Bind();	// variable specularity in alpha channel
	} else {
		globalImages->specularTableImage->Bind();
	}

	for ( ; surf ; surf = surf->nextOnLight ) {
		// perform setup here that will not change over multiple interaction passes

		// set the vertex pointers
		idDrawVert	*ac = ( idDrawVert * )vertexCache.VertexPosition( surf->ambientCache );
		qglVertexAttribPointer( 3, 4, GL_UNSIGNED_BYTE, true, sizeof( idDrawVert ), &ac->color );
		qglVertexAttribPointer( 11, 3, GL_FLOAT, false, sizeof( idDrawVert ), ac->normal.ToFloatPtr() );
		qglVertexAttribPointer( 10, 3, GL_FLOAT, false, sizeof( idDrawVert ), ac->tangents[1].ToFloatPtr() );
		qglVertexAttribPointer( 9, 3, GL_FLOAT, false, sizeof( idDrawVert ), ac->tangents[0].ToFloatPtr() );
		qglVertexAttribPointer( 8, 2, GL_FLOAT, false, sizeof( idDrawVert ), ac->st.ToFloatPtr() );
		qglVertexAttribPointer( 0, 3, GL_FLOAT, false, sizeof( idDrawVert ), ac->xyz.ToFloatPtr() );

		// this may cause RB_ARB2_DrawInteraction to be exacuted multiple
		// times with different colors and images if the surface or light have multiple layers
		RB_CreateSingleDrawInteractions( surf );
	}
	qglDisableVertexAttribArray( 8 );
	qglDisableVertexAttribArray( 9 );
	qglDisableVertexAttribArray( 10 );
	qglDisableVertexAttribArray( 11 );
	qglDisableVertexAttribArray( 3 );

	// disable features
	GL_SelectTexture( 6 );
	globalImages->BindNull();

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

	backEnd.glState.currenttmu = -1;
	GL_SelectTexture( 0 );

	qglDisable( GL_VERTEX_PROGRAM_ARB );
	qglDisable( GL_FRAGMENT_PROGRAM_ARB );
}

/*
=============
RB_ARB2_CreateDrawInteractions_simple
// nbohr1more #3881: simplify test verses default shader toggle
=============
*/
void RB_ARB2_CreateDrawInteractions_simple( const drawSurf_t *surf ) {
	if ( !surf ) {
		return;
	}
	GL_PROFILE( "ARB2_CreateDrawInteractions_simple" );

	// perform setup here that will be constant for all interactions
	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | backEnd.depthFunc );

	// bind the vertex program
	// rebb: support dedicated ambient - CVar and direct interactions can probably be removed, they're there mainly for performance testing
	// nbohr1more #3881: removed direct interaction cvar toggle
	// nbohr1more #3881: dedicated cubemap lighting
	if ( backEnd.vLight->lightShader->IsCubicLight() ) {
		if ( backEnd.vLight->lightShader->IsAmbientLight() ) {
			qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, VPROG_AMBIENT_CUBE_LIGHT );
			qglBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, FPROG_AMBIENT_CUBE_LIGHT );
		} else { // nbohr1more #3881: dedicated cubemap lighting (further changes)
			if ( backEnd.vLight->lightDef->parms.pointLight ) {
				qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, VPROG_CUBIC_LIGHT_POINT );
				qglBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, FPROG_CUBIC_LIGHT_POINT );
			} else {
				qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, VPROG_CUBIC_LIGHT_PROJ );
				qglBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, FPROG_CUBIC_LIGHT_PROJ );
			}
		}
	} else if ( backEnd.vLight->lightShader->IsAmbientLight() ) {
		qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, VPROG_AMBIENT );
		qglBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, FPROG_AMBIENT );
	} else {
		qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, VPROG_INTERACTION_DIRECT );
		qglBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, FPROG_INTERACTION_DIRECT );
	}
	qglEnable( GL_VERTEX_PROGRAM_ARB );
	qglEnable( GL_FRAGMENT_PROGRAM_ARB );

	// enable the vertex arrays
	qglEnableVertexAttribArray( 8 );
	qglEnableVertexAttribArray( 9 );
	qglEnableVertexAttribArray( 10 );
	qglEnableVertexAttribArray( 11 );
	qglEnableVertexAttribArray( 3 );

	// texture 0 is the normalization cube map for the vector towards the light
	GL_SelectTexture( 0 );

	if ( backEnd.vLight->lightShader->IsAmbientLight() ) {
		globalImages->ambientNormalMap->Bind();
	} else {
		globalImages->normalCubeMapImage->Bind();
	}

	// texture 6 is the specular lookup table
	GL_SelectTexture( 6 );

	if ( r_testARBProgram.GetBool() ) {
		globalImages->specular2DTableImage->Bind();	// variable specularity in alpha channel
	} else {
		globalImages->specularTableImage->Bind();
	}

	for ( ; surf ; surf = surf->nextOnLight ) {
		// perform setup here that will not change over multiple interaction passes

		// set the vertex pointers
		idDrawVert	*ac = ( idDrawVert * )vertexCache.VertexPosition( surf->ambientCache );
		qglVertexAttribPointer( 3, 4, GL_UNSIGNED_BYTE, true, sizeof( idDrawVert ), &ac->color );
		qglVertexAttribPointer( 11, 3, GL_FLOAT, false, sizeof( idDrawVert ), ac->normal.ToFloatPtr() );
		qglVertexAttribPointer( 10, 3, GL_FLOAT, false, sizeof( idDrawVert ), ac->tangents[1].ToFloatPtr() );
		qglVertexAttribPointer( 9, 3, GL_FLOAT, false, sizeof( idDrawVert ), ac->tangents[0].ToFloatPtr() );
		qglVertexAttribPointer( 8, 2, GL_FLOAT, false, sizeof( idDrawVert ), ac->st.ToFloatPtr() );
		qglVertexAttribPointer( 0, 3, GL_FLOAT, false, sizeof( idDrawVert ), ac->xyz.ToFloatPtr() );

		// this may cause RB_ARB2_DrawInteraction to be exacuted multiple
		// times with different colors and images if the surface or light have multiple layers
		RB_CreateSingleDrawInteractions( surf );
	}
	qglDisableVertexAttribArray( 8 );
	qglDisableVertexAttribArray( 9 );
	qglDisableVertexAttribArray( 10 );
	qglDisableVertexAttribArray( 11 );
	qglDisableVertexAttribArray( 3 );

	// disable features
	GL_SelectTexture( 6 );
	globalImages->BindNull();

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

	backEnd.glState.currenttmu = -1;
	GL_SelectTexture( 0 );

	qglDisable( GL_VERTEX_PROGRAM_ARB );
	qglDisable( GL_FRAGMENT_PROGRAM_ARB );
}

/*
==================
RB_ARB2_DrawInteractions
==================
*/
void RB_ARB2_DrawInteractions( void ) {
	GL_PROFILE( "ARB2_DrawInteractions" );

	viewLight_t		*vLight;
	const idMaterial	*lightShader;

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

		if ( !vLight->localInteractions &&
		     !vLight->globalInteractions &&
		     !vLight->translucentInteractions ) {
			continue;
		}
		lightShader = vLight->lightShader;

		//anon begin
		// set the depth bounds for the whole light
		if ( backEnd.useLightDepthBounds ) {
			GL_DepthBoundsTest( vLight->scissorRect.zmin, vLight->scissorRect.zmax );
		}
		//anon end

		// clear the stencil buffer if needed
		if ( vLight->globalShadows || vLight->localShadows ) {
			backEnd.currentScissor = vLight->scissorRect;

			if ( r_useScissor.GetBool() ) {
				GL_Scissor( backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
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

		// nbohr1more #3881: toggle test verse direct shaders further up the tree to reduce bind complexity
		if ( r_testARBProgram.GetBool() ) {
			qglEnable( GL_VERTEX_PROGRAM_ARB );
			qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, VPROG_STENCIL_SHADOW );
			RB_StencilShadowPass( vLight->globalShadows );
			RB_ARB2_CreateDrawInteractions( vLight->localInteractions );
			qglEnable( GL_VERTEX_PROGRAM_ARB );
			qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, VPROG_STENCIL_SHADOW );
			RB_StencilShadowPass( vLight->localShadows );
			RB_ARB2_CreateDrawInteractions( vLight->globalInteractions );
			qglDisable( GL_VERTEX_PROGRAM_ARB );	// if there weren't any globalInteractions, it would have stayed on
		} else {
			qglEnable( GL_VERTEX_PROGRAM_ARB );
			qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, VPROG_STENCIL_SHADOW );
			RB_StencilShadowPass( vLight->globalShadows );
			RB_ARB2_CreateDrawInteractions_simple( vLight->localInteractions );
			qglEnable( GL_VERTEX_PROGRAM_ARB );
			qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, VPROG_STENCIL_SHADOW );
			RB_StencilShadowPass( vLight->localShadows );
			RB_ARB2_CreateDrawInteractions_simple( vLight->globalInteractions );
			qglDisable( GL_VERTEX_PROGRAM_ARB );	// if there weren't any globalInteractions, it would have stayed on
		}

		//anon begin
		// reset depth bounds
		if ( backEnd.useLightDepthBounds ) {
			GL_DepthBoundsTest( 0.0f, 0.0f );
		}
		//anon end

		// translucent surfaces never get stencil shadowed
		if ( r_skipTranslucent.GetBool() ) {
			continue;
		}
		qglStencilFunc( GL_ALWAYS, 128, 255 );

		backEnd.depthFunc = GLS_DEPTHFUNC_LESS;
		RB_ARB2_CreateDrawInteractions( vLight->translucentInteractions );
		backEnd.depthFunc = GLS_DEPTHFUNC_EQUAL;
	}

	// disable stencil shadow test
	qglStencilFunc( GL_ALWAYS, 128, 255 );

	GL_SelectTexture( 0 );
}

//===================================================================================


typedef struct {
	GLenum			target;
	GLuint			ident;
	const char		*name;
	GLuint          genId; // glsl program id
} progDef_t;

#define MAX_GLPROGS			512

// a single file can have both a vertex program and a fragment program
static progDef_t	progs[MAX_GLPROGS] = {
	{ GL_VERTEX_PROGRAM_ARB, VPROG_TEST, "test.vfp" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_TEST, "test.vfp" },
	{ GL_VERTEX_PROGRAM_ARB, VPROG_INTERACTION, "interaction.vfp" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_INTERACTION, "interaction.vfp" },
	{ GL_VERTEX_PROGRAM_ARB, VPROG_BUMPY_ENVIRONMENT, "bumpyEnvironment.vfp" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_BUMPY_ENVIRONMENT, "bumpyEnvironment.vfp" },
	{ GL_VERTEX_PROGRAM_ARB, VPROG_AMBIENT, "ambientLight.vfp" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_AMBIENT, "ambientLight.vfp" },
	{ GL_VERTEX_PROGRAM_ARB, VPROG_STENCIL_SHADOW, "shadow.vp" },
	{ GL_VERTEX_PROGRAM_ARB, VPROG_ENVIRONMENT, "environment.vfp" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_ENVIRONMENT, "environment.vfp" },

	// rebb: direct light interaction files for performance testing
	{ GL_VERTEX_PROGRAM_ARB, VPROG_TEST_DIRECT, "test_direct.vfp" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_TEST_DIRECT, "test_direct.vfp" },
	{ GL_VERTEX_PROGRAM_ARB, VPROG_INTERACTION_DIRECT, "interaction_direct.vfp" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_INTERACTION_DIRECT, "interaction_direct.vfp" },

	// SteveL #3878: Particle softening applied by the engine
	{ GL_VERTEX_PROGRAM_ARB, VPROG_SOFT_PARTICLE, "soft_particle.vfp" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_SOFT_PARTICLE, "soft_particle.vfp" },

	// nbohr1more #3881: cubicLight interactions
	{ GL_VERTEX_PROGRAM_ARB, VPROG_CUBIC_LIGHT_POINT, "cubic_light_point.vfp" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_CUBIC_LIGHT_POINT, "cubic_light_point.vfp" },
	{ GL_VERTEX_PROGRAM_ARB, VPROG_CUBIC_LIGHT_PROJ, "cubic_light_proj.vfp" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_CUBIC_LIGHT_PROJ, "cubic_light_proj.vfp" },


	// nbohr1more #3881: cubemap based lighting further changes
	{ GL_VERTEX_PROGRAM_ARB, VPROG_TEST_CUBIC_POINT, "test_cubic_light_point.vfp" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_TEST_CUBIC_POINT, "test_cubic_light_point.vfp" },
	{ GL_VERTEX_PROGRAM_ARB, VPROG_TEST_CUBIC_PROJ, "test_cubic_light_proj.vfp" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_TEST_CUBIC_PROJ, "test_cubic_light_proj.vfp" },

	{ GL_VERTEX_PROGRAM_ARB, VPROG_AMBIENT_CUBE_LIGHT, "ambient_cubic_light.vfp" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_AMBIENT_CUBE_LIGHT, "ambient_cubic_light.vfp" },

	// duzenko: backend bloom
	{ GL_VERTEX_PROGRAM_ARB, VPROG_BLOOM_COOK_MATH1, "cookMath_pass1.vfp" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_BLOOM_COOK_MATH1, "cookMath_pass1.vfp" },
	{ GL_VERTEX_PROGRAM_ARB, VPROG_BLOOM_COOK_MATH2, "cookMath_pass2.vfp" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_BLOOM_COOK_MATH2, "cookMath_pass2.vfp" },
	{ GL_VERTEX_PROGRAM_ARB, VPROG_BLOOM_BRIGHTNESS, "brightPass_opt.vfp" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_BLOOM_BRIGHTNESS, "brightPass_opt.vfp" },
	{ GL_VERTEX_PROGRAM_ARB, VPROG_BLOOM_GAUSS_BLRX, "blurx.vfp" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_BLOOM_GAUSS_BLRX, "blurx.vfp" },
	{ GL_VERTEX_PROGRAM_ARB, VPROG_BLOOM_GAUSS_BLRY, "blury.vfp" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_BLOOM_GAUSS_BLRY, "blury.vfp" },
	{ GL_VERTEX_PROGRAM_ARB, VPROG_BLOOM_FINAL_PASS, "finalScenePass_opt.vfp" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_BLOOM_FINAL_PASS, "finalScenePass_opt.vfp" },

	// additional programs can be dynamically specified in materials
};

/*
=================
R_LoadARBProgram
=================
*/
void R_LoadARBProgram( int progIndex ) {
	idStr	fullPath = "glprogs/";
	fullPath += progs[progIndex].name;
	char	*fileBuffer;
	char	*buffer;
	char	*start = NULL, *end;

	// load the program even if we don't support it
	fileSystem->ReadFile( fullPath.c_str(), ( void ** )&fileBuffer, NULL );

	if ( !fileBuffer ) {
		common->Warning( "LoadARBProgram: \'%s\' not found", fullPath.c_str() );
		return;
	}
	common->Printf( "%s", fullPath.c_str() );

	// copy to stack memory and free
	buffer = ( char * )_alloca( strlen( fileBuffer ) + 1 );
	strcpy( buffer, fileBuffer );
	fileSystem->FreeFile( fileBuffer );

	if ( !glConfig.isInitialized ) {
		return;
	}

	//
	// submit the program string at start to GL
	//
	if ( progs[progIndex].ident == 0 ) {
		// allocate a new identifier for this program
		progs[progIndex].ident = PROG_USER + progIndex;
	}
	common->Printf( " %d", progs[progIndex].ident );

	// vertex and fragment programs can both be present in a single file, so
	// scan for the proper header to be the start point, and stamp a 0 in after the end
	start = NULL;
	if ( progs[progIndex].target == GL_VERTEX_PROGRAM_ARB ) {
		start = strstr( ( char * )buffer, "!!ARBvp" );
	}
	if ( progs[progIndex].target == GL_FRAGMENT_PROGRAM_ARB ) {
		start = strstr( ( char * )buffer, "!!ARBfp" );
	}
	if ( !start ) {
		common->Printf( S_COLOR_RED ": !!ARB not found\n"  S_COLOR_DEFAULT );
		return;
	}
	end = strstr( start, "END" );

	if ( !end ) {
		common->Printf( S_COLOR_RED ": END not found\n"  S_COLOR_DEFAULT );
		return;
	}
	end[3] = 0;

	qglBindProgramARB( progs[progIndex].target, progs[progIndex].ident );
	qglProgramStringARB( progs[progIndex].target, GL_PROGRAM_FORMAT_ASCII_ARB,
	                     static_cast<GLsizei>( strlen( start ) ), ( unsigned char * )start );

	// this is pretty important for quick shader debugging, better have it in always
	int err = qglGetError();
	int	ofs;
	qglGetIntegerv( GL_PROGRAM_ERROR_POSITION_ARB, ( GLint * )&ofs );
	if ( err == GL_INVALID_OPERATION ) {
		const GLubyte *str = qglGetString( GL_PROGRAM_ERROR_STRING_ARB );
		common->Printf( "\nGL_PROGRAM_ERROR_STRING_ARB: %s\n", str );
		if ( ofs < 0 ) {
			common->Printf( "GL_PROGRAM_ERROR_POSITION_ARB < 0 with error\n" );
		} else if ( ofs >= ( int )strlen( ( char * )start ) ) {
			common->Printf( "error at end of program\n" );
		} else {
			common->Printf( "error at %i:\n%s", ofs, start + ofs );
		}
		return;
	}

	if ( ofs != -1 ) {
		common->Printf( "\nGL_PROGRAM_ERROR_POSITION_ARB != -1 without error\n" );
		return;
	}
	common->Printf( "\n" );
}

/*
==================
R_FindARBProgram

Returns a GL identifier that can be bound to the given target, parsing
a text file if it hasn't already been loaded.
==================
*/
int R_FindARBProgram( GLenum target, const char *program ) {
	int		i;
	idStr	stripped = program;

	stripped.StripFileExtension();

	// see if it is already loaded
	for ( i = 0 ; progs[i].name ; i++ ) {
		if ( progs[i].target != target ) {
			continue;
		}
		idStr	compare = progs[i].name;

		compare.StripFileExtension();

		if ( !idStr::Icmp( stripped.c_str(), compare.c_str() ) ) {
			return progs[i].ident;
		}
	}

	if ( i == MAX_GLPROGS ) {
		common->Error( "R_FindARBProgram: MAX_GLPROGS" );
	}

	// add it to the list and load it
	progs[i].ident = ( program_t )0;	// will be gen'd by R_LoadARBProgram
	progs[i].target = target;
	char *progName = new char[strlen( program ) + 1];
	progs[i].name = progName;
	strcpy( progName, program );
	R_LoadARBProgram( i );

	return progs[i].ident;
}

/*
==================
R_UseProgramARB

One-liner for qglBindProgramARB+qglEnable frag+vert
Important: fprog needs to go straight after vprog in program_t
==================
*/
void R_UseProgramARB( int vProg ) {
	GL_CheckErrors();

	if ( vProg == PROG_INVALID ) {
		qglDisable( GL_VERTEX_PROGRAM_ARB );
		qglDisable( GL_FRAGMENT_PROGRAM_ARB );
	} else {
		RB_LogComment( "R_UseProgram %d\n", vProg );
		qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, vProg );
		qglBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, vProg + 1 ); // as defined by program_t
		qglEnable( GL_VERTEX_PROGRAM_ARB );
		qglEnable( GL_FRAGMENT_PROGRAM_ARB );
		GL_CheckErrors();
	}
}

/*
==================
R_ReloadARBPrograms_f
==================
*/
void R_ReloadARBPrograms_f( const idCmdArgs &args ) {
	int		i;

	common->Printf( "----- R_ReloadARBPrograms -----\n" );

	for ( i = 0; progs[i].name && progs[i].name[0]; i++ ) {
		R_LoadARBProgram( i );
	}
	common->Printf( "-------------------------------\n" );
}

