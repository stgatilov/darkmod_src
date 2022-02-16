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
#include "glsl.h"
#include "FrameBuffer.h"
#include "GLSLProgram.h"
#include "GLSLProgramManager.h"
#include "AmbientOcclusionStage.h"
#include "FrameBufferManager.h"

#if defined(_MSC_VER) && _MSC_VER >= 1800 && !defined(DEBUG)
//#pragma optimize("t", off) // duzenko: used in release to enforce breakpoints in inlineable code. Please do not remove
#endif

idCVarBool r_shadowMapCullFront( "r_shadowMapCullFront", "0", CVAR_ARCHIVE | CVAR_RENDERER, "Cull front faces in shadow maps" );
idCVarBool r_useMultiDraw( "r_useMultiDraw", "0", CVAR_RENDERER, "Use glMultiDrawElements to save on draw calls" );

struct ShadowMapUniforms : GLSLUniformGroup {
	UNIFORM_GROUP_DEF( ShadowMapUniforms )

	DEFINE_UNIFORM( vec4, lightOrigin )
	DEFINE_UNIFORM( float, alphaTest )
	DEFINE_UNIFORM( mat4, modelMatrix )
};

GLSLProgram *currrentInteractionShader; // dynamic, either pointInteractionShader or ambientInteractionShader

idCVarInt r_shadowMapSinglePass( "r_shadowMapSinglePass", "0", CVAR_ARCHIVE | CVAR_RENDERER, "1 - render shadow maps for all lights in a single pass; 2 - also render all light interactions in a single pass" );

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
	GL_CheckErrors();
}

static void BindShadowTexture() {
	if ( backEnd.vLight->shadowMapIndex ) {
		GL_SelectTexture( 6 );
		globalImages->shadowAtlas->Bind();
	} else {
		GL_SelectTexture( 6 );
		globalImages->currentDepthImage->Bind();
		GL_SelectTexture( 7 );

		globalImages->shadowDepthFbo->Bind();
		qglTexParameteri( GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_STENCIL_INDEX );
	}
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
		interactionUniforms->RGTC.Set( din->bumpImage->internalFormat == GL_COMPRESSED_RG_RGTC2 );
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

	if ( !backEnd.vLight->lightShader->IsAmbientLight() && ( r_softShadowsQuality.GetBool() && !backEnd.viewDef->IsLightGem() || backEnd.vLight->shadows == LS_MAPS ) )
		BindShadowTexture();

	// draw it
	RB_DrawElementsWithCounters( din->surf );
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
	TRACE_GL_SCOPE( "GLSL_CreateDrawInteractions" );

	// if using float buffers, alpha values are not clamped and can stack up quite high, since most interactions add 1 to its value
	// this in turn causes issues with some shader stage materials that use DST_ALPHA blending.
	// masking the alpha channel for interactions seems to fix those issues, but only do it for float buffers in case it has
	// unwanted side effects
	int alphaMask = r_fboColorBits.GetInteger() == 64 ? GLS_ALPHAMASK : 0;

	// perform setup here that will be constant for all interactions
	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | alphaMask | backEnd.depthFunc );
	backEnd.currentSpace = NULL; // ambient/interaction shaders conflict

	// bind the vertex and fragment program
	ChooseInteractionProgram();
	Uniforms::Interaction *interactionUniforms = currrentInteractionShader->GetUniformGroup<Uniforms::Interaction>();
	interactionUniforms->SetForShadows( surf == backEnd.vLight->translucentInteractions );
	if( backEnd.vLight->lightShader->IsAmbientLight() && ambientOcclusion->ShouldEnableForCurrentView() ) {
		ambientOcclusion->BindSSAOTexture( 6 );
	}

	for ( /**/; surf; surf = surf->nextOnLight ) {
		if ( surf->dsFlags & DSF_SHADOW_MAP_ONLY ) {
			continue;
		}
		if ( backEnd.currentSpace != surf->space ) {
			// FIXME needs a better integration with RB_CreateSingleDrawInteractions
			interactionUniforms->modelMatrix.Set( surf->space->modelMatrix );
		}

		// set the vertex pointers
		vertexCache.VertexPosition( surf->ambientCache );

		// this may cause RB_GLSL_DrawInteraction to be executed multiple
		// times with different colors and images if the surface or light have multiple layers
		RB_CreateSingleDrawInteractions( surf );
		GL_CheckErrors();
	}

	GL_SelectTexture( 0 );

	GLSLProgram::Deactivate();
	GL_CheckErrors();
}

/*
==================
RB_GLSL_DrawLight_Stencil
==================
*/
void RB_GLSL_DrawLight_Stencil() {
	TRACE_GL_SCOPE( "GLSL_DrawLight_Stencil" );

	bool useShadowFbo = r_softShadowsQuality.GetBool() && !backEnd.viewDef->IsLightGem();// && (r_shadows.GetInteger() != 2);

	// set depth bounds for the whole light
	const DepthBoundsTest depthBoundsTest( backEnd.vLight->scissorRect );

	// clear the stencil buffer if needed
	if ( backEnd.vLight->globalShadows || backEnd.vLight->localShadows ) {
		backEnd.currentScissor = backEnd.vLight->scissorRect;

		if ( r_useScissor.GetBool() ) {
			GL_ScissorVidSize( backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
			            backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
			            backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
			            backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1 );
		}

		if ( useShadowFbo ) {
			frameBuffers->EnterShadowStencil();
		}
		qglClear( GL_STENCIL_BUFFER_BIT );
	} else {
		// no shadows, so no need to read or write the stencil buffer
		qglStencilFunc( GL_ALWAYS, 128, 255 );
	}
	programManager->stencilShadowShader->Activate();

	if ( backEnd.vLight->globalShadows ) {
		RB_StencilShadowPass( backEnd.vLight->globalShadows );
		if ( useShadowFbo && r_multiSamples.GetInteger() > 1 && r_softShadowsQuality.GetInteger() >= 0 ) {
			frameBuffers->ResolveShadowStencilAA();
		}
	}

	const bool NoSelfShadows = true; // don't delete - debug check for low-poly "round" models casting ugly shadows on themselves

	if ( NoSelfShadows ) {
		if ( useShadowFbo ) {
			frameBuffers->LeaveShadowStencil();
		}
		RB_GLSL_CreateDrawInteractions( backEnd.vLight->localInteractions );

		if ( useShadowFbo ) {
			frameBuffers->EnterShadowStencil();
		}
	}
	programManager->stencilShadowShader->Activate();

	if ( backEnd.vLight->localShadows ) {
		RB_StencilShadowPass( backEnd.vLight->localShadows );
		if ( useShadowFbo && r_multiSamples.GetInteger() > 1 && r_softShadowsQuality.GetInteger() >= 0 ) {
			frameBuffers->ResolveShadowStencilAA();
		}
	}

	if ( useShadowFbo ) {
		frameBuffers->LeaveShadowStencil();
	}

	if ( !NoSelfShadows ) {
		RB_GLSL_CreateDrawInteractions( backEnd.vLight->localInteractions );
	}
	RB_GLSL_CreateDrawInteractions( backEnd.vLight->globalInteractions );

	GLSLProgram::Deactivate();	// if there weren't any globalInteractions, it would have stayed on
}

float GetEffectiveLightRadius() {
	return r_softShadowsRadius.GetFloat();	//default value
}

/*
=============
RB_GLSL_CreateDrawInteractions
=============
*/
void RB_GLSL_DrawInteractions_ShadowMap( const drawSurf_t *surf, bool clear = false ) {
	if ( backEnd.vLight->shadowMapIndex > 42 )
		return;
	TRACE_GL_SCOPE( "GLSL_DrawInteractions_ShadowMap" );

	GL_CheckErrors();
	frameBuffers->EnterShadowMap();

	programManager->shadowMapShader->Activate();
	GL_SelectTexture( 0 );

	ShadowMapUniforms *shadowMapUniforms = programManager->shadowMapShader->GetUniformGroup<ShadowMapUniforms>();
	idVec4 lightOrigin;
	lightOrigin.x = backEnd.vLight->globalLightOrigin.x;
	lightOrigin.y = backEnd.vLight->globalLightOrigin.y;
	lightOrigin.z = backEnd.vLight->globalLightOrigin.z;
	lightOrigin.w = 0;
	shadowMapUniforms->lightOrigin.Set( lightOrigin );
	shadowMapUniforms->alphaTest.Set( -1 );
	backEnd.currentSpace = NULL;

	GL_Cull( r_shadowMapCullFront ? CT_BACK_SIDED : CT_TWO_SIDED );
	qglPolygonOffset( 0, 0 );
	qglEnable( GL_POLYGON_OFFSET_FILL );

	auto &page = ShadowAtlasPages[backEnd.vLight->shadowMapIndex-1];
	qglViewport( page.x, page.y, 6*page.width, page.width );
	if ( r_useScissor.GetBool() )
		qglScissor( page.x, page.y, 6*page.width, page.width );
	if ( clear )
		qglClear( GL_DEPTH_BUFFER_BIT );
	for ( int i = 0; i < 4; i++ )
		qglEnable( GL_CLIP_DISTANCE0 + i );
	for ( ; surf; surf = surf->nextOnLight ) {
		if ( surf->dsFlags & DSF_SHADOW_MAP_IGNORE ) 
			continue;    // this flag is set by entities with parms.noShadow in R_PrepareLightSurf (candles, torches, etc)

		/*float customOffset = surf->space->entityDef->parms.shadowMapOffset + surf->material->GetShadowMapOffset();
		if ( customOffset != 0 )
			qglPolygonOffset( customOffset, 0 );*/

		if ( backEnd.currentSpace != surf->space ) {
			shadowMapUniforms->modelMatrix.Set( surf->space->modelMatrix );
			backEnd.currentSpace = surf->space;
			backEnd.pc.c_matrixLoads++;
		}

		RB_SingleSurfaceToDepthBuffer( programManager->shadowMapShader, surf );
		backEnd.pc.c_shadowIndexes += surf->numIndexes;
		backEnd.pc.c_drawIndexes -= surf->numIndexes;

		/*if ( customOffset != 0 )
			qglPolygonOffset( 0, 0 );*/
	}
	for ( int i = 0; i < 4; i++ )
		qglDisable( GL_CLIP_DISTANCE0 + i );

	qglDisable( GL_POLYGON_OFFSET_FILL );
	GL_Cull( CT_FRONT_SIDED );

	backEnd.currentSpace = NULL; // or else conflicts with qglLoadMatrixf
	GLSLProgram::Deactivate();

	frameBuffers->LeaveShadowMap();

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
	TRACE_GL_SCOPE( "GLSL_DrawLight_ShadowMap" );

	GL_CheckErrors();

	if ( backEnd.vLight->lightShader->LightCastsShadows() && !r_shadowMapSinglePass ) {
		RB_GLSL_DrawInteractions_ShadowMap( backEnd.vLight->globalInteractions, true );
		RB_GLSL_CreateDrawInteractions( backEnd.vLight->localInteractions );
		RB_GLSL_DrawInteractions_ShadowMap( backEnd.vLight->localInteractions );
	} else {
		RB_GLSL_CreateDrawInteractions( backEnd.vLight->localInteractions );
	}
	RB_GLSL_CreateDrawInteractions( backEnd.vLight->globalInteractions );

	GLSLProgram::Deactivate();

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
void RB_ShadowMap_RenderAllLights();

void RB_GLSL_DrawInteractions() {
	TRACE_GL_SCOPE( "GLSL_DrawInteractions" );
	GL_SelectTexture( 0 );

	if ( r_shadows.GetInteger() == 2 ) 
		if ( r_shadowMapSinglePass.GetBool() )
			RB_ShadowMap_RenderAllLights();
	if ( r_shadows.GetInteger() != 1 )
		if ( r_interactionProgram.GetInteger() == 2 ) {
			extern void RB_GLSL_DrawInteractions_MultiLight();
			RB_GLSL_DrawInteractions_MultiLight();
			return;
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

filenames hardcoded here since they're not used elsewhere
FIXME split the stencil and shadowmap interactions in separate shaders as the latter might not compile on DX10 and older hardware
==================
*/
ID_NOINLINE bool R_ReloadGLSLPrograms(const char *programName) { 
	// incorporate new shader interface:
	if ( programName )
		programManager->Reload( programName );
	else
		programManager->ReloadAllPrograms();

	return true;
}

/*
==================
R_ReloadGLSLPrograms_f
==================
*/
void R_ReloadGLSLPrograms_f( const idCmdArgs &args ) {
	common->Printf( "---------- R_ReloadGLSLPrograms_f -----------\n" );

	const char *programName = args.Argc() > 1 ? args.Argv( 1 ) : nullptr;
	if ( !R_ReloadGLSLPrograms( programName ) ) {
		common->Error( "GLSL shaders failed to init.\n" );
		return;
	}
	common->Printf( "---------------------------------\n" );
}

GLSLProgram *R_FindGLSLProgram( const char *name ) {
	GLSLProgram *program = programManager->Find( name );
	if( program == nullptr ) {
		program = programManager->Load( name );
	}
	return program;
}

idCVarInt r_depthColor("r_depthColor", "0", 0, "Depth pass debug color");
idVec4 colorMagentaHalf = idVec4( 1.00f, 0.00f, 1.00f, 1.00f ) * 0.3f;

void RB_SingleSurfaceToDepthBuffer( GLSLProgram *program, const drawSurf_t *surf ) {

	idVec4& depthColor = r_depthColor == 0 ? colorBlack : colorMagentaHalf;
	idVec4 color;
	const idMaterial		*shader = surf->material;
	int						stage;
	const shaderStage_t		*pStage;
	const float				*regs = surf->shaderRegisters;

	GL_CheckErrors();
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
	vertexCache.VertexPosition( surf->ambientCache );

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

		GL_CheckErrors();

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
			GL_CheckErrors();
			depthUniforms->color.Set( color );
			depthUniforms->alphaTest.Set( regs[pStage->alphaTestRegister] );

			// bind the texture
			pStage->texture.image->Bind();
			RB_LoadShaderTextureMatrix( surf->shaderRegisters, pStage );
			GL_CheckErrors();

			// draw it
			if ( depthUniforms->instances )
				RB_DrawElementsInstanced( surf, depthUniforms->instances );
			else
				RB_DrawElementsWithCounters( surf );
			GL_CheckErrors();

			RB_LoadShaderTextureMatrix( NULL, pStage );
			/*if ( pStage->texture.hasMatrix ) {
				qglMatrixMode( GL_TEXTURE );
				qglLoadIdentity();
				qglMatrixMode( GL_MODELVIEW );
			}*/

			depthUniforms->alphaTest.Set( -1 ); // hint the glsl to skip texturing
			GL_CheckErrors();
		}
		depthUniforms->color.Set( depthColor );
		GL_CheckErrors();

		if ( !didDraw ) {
			drawSolid = true;
		}
	}

	if ( drawSolid ) {  // draw the entire surface solid
		if ( depthUniforms->instances )
			RB_DrawElementsInstanced( surf, depthUniforms->instances );
		else
			if ( r_useMultiDraw && surf->indexCache.IsValid()
				// && !depthUniforms->instances //&& !memcmp( backEnd.viewDef->worldSpace.modelViewMatrix, surf->space->modelViewMatrix, 64) 
				) {
				RB_Multi_AddSurf( surf );
			} else
				RB_DrawElementsWithCounters( surf );
	}
	GL_CheckErrors();

	// reset blending
	if ( shader->GetSort() == SS_SUBVIEW ) {
		depthUniforms->color.Set( depthColor );
		GL_State( GLS_DEPTHFUNC_LESS );
		GL_CheckErrors();
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
	program->BindAttribLocation(DrawId, "attr_DrawId");
}

//I expect this function should be enough for setting up vertex attrib arrays in most cases..
//But I am not sure in it =)
/*void Attributes::Default::SetDrawVert(size_t startOffset, int arrayMask) {
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
}*/

void Uniforms::Global::Set(const viewEntity_t *space) {
	modelMatrix.Set( space->modelMatrix );
	//projectionMatrix.Set( backEnd.viewDef->projectionMatrix );
	modelViewMatrix.Set( space->modelViewMatrix );
	if ( viewOriginLocal.IsPresent() ) {
		idVec4 vol;
		R_GlobalPointToLocal( space->modelMatrix, backEnd.viewDef->renderView.vieworg, vol.ToVec3() );
		vol[3] = 1.0;
		viewOriginLocal.Set( vol );
	}
}

void Uniforms::MaterialStage::Set(const shaderStage_t *pStage, const drawSurf_t *surf) {
	//============================================================================
	//note: copied from RB_SetProgramEnvironment and RB_SetProgramEnvironmentSpace
	//============================================================================

	idVec4 parm;
	// screen power of two correction factor, assuming the copy to _currentRender
	// also copied an extra row and column for the bilerp
	parm[0] = 1;
	parm[1] = 1;
	parm[2] = 0;
	parm[3] = 1;
 	scalePotToWindow.Set( parm );

	// window coord to 0.0 to 1.0 conversion
	parm[0] = 1.0 / frameBuffers->activeFbo->Width();
	parm[1] = 1.0 / frameBuffers->activeFbo->Height();
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

	const struct viewEntity_s *space = surf->space;
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
	if (newStage) {
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
	lightTextureMatrix.SetArray( 2, din->lightTextureMatrix[0].ToFloatPtr() );
	// set the constant color
	diffuseColor.Set( din->diffuseColor );
	specularColor.Set( din->specularColor );
	viewOrigin.Set( din->localViewOrigin );

	if( ambient ) {
		lightOrigin.Set( din->worldUpLocal.ToVec3() );
		rimColor.Set( din->ambientRimColor );
	} else {
		lightOrigin.Set( din->localLightOrigin.ToVec3() );
		lightOrigin2.Set( backEnd.vLight->globalLightOrigin );
	}

	// stgatilov #4825: make translation "lit tangentially" -> "unlit" smoother
	// #5862 unless surface has "twosided" material
	useBumpmapLightTogglingFix.Set( r_useBumpmapLightTogglingFix.GetBool() && !din->surf->material->ShouldCreateBackSides() );

	GL_CheckErrors();
}

void Uniforms::Interaction::SetForShadows( bool translucent ) {
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

	if( ambient ) {
		minLevel.Set( backEnd.viewDef->IsLightGem() ? 0 : r_ambientMinLevel.GetFloat() );
		gamma.Set( backEnd.viewDef->IsLightGem() ? 1 : r_ambientGamma.GetFloat() );
		if ( backEnd.vLight->lightShader->IsCubicLight() ) {
			lightFalloffCubemap.Set( 1 );
		} else {
			lightFalloffCubemap.Set( MAX_MULTITEXTURE_UNITS + 1 );
		}
		ssaoTexture.Set( 6 );
		ssaoEnabled.Set( ambientOcclusion->ShouldEnableForCurrentView() );
		return;
	}

	advanced.Set( r_interactionProgram.GetFloat() );

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
	shadowMapCullFront.Set( r_shadowMapCullFront );

	if ( !translucent && ( backEnd.vLight->globalShadows || backEnd.vLight->localShadows || r_shadows.GetInteger() == 2 ) && !backEnd.viewDef->IsLightGem() ) {
		softShadowsQuality.Set( r_softShadowsQuality.GetInteger() );

		int sampleK = r_softShadowsQuality.GetInteger();
		if ( sampleK > 0 ) { // texcoords for screen-space softener filter
			if ( poissonSamples.Num() != sampleK || poissonSamples.Num() == 0 ) {
				GeneratePoissonDiskSampling( poissonSamples, sampleK );
				softShadowsSamples.SetArray( sampleK, ( float * )poissonSamples.Ptr() );
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
		//renderResolution.Set( glConfig.vidWidth, glConfig.vidHeight );
		renderResolution.Set( globalImages->currentDepthImage->uploadWidth, globalImages->currentDepthImage->uploadHeight ); // 5055 respect r_fboResolution
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
