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

#include "RenderBackend.h"

#include "../AmbientOcclusionStage.h"
#include "../Profiling.h"
#include "../GLSLProgram.h"
#include "../GLSLProgramManager.h"
#include "../FrameBufferManager.h"

RenderBackend renderBackendImpl;
RenderBackend *renderBackend = &renderBackendImpl;

idCVar r_useNewBackend( "r_useNewBackend", "1", CVAR_BOOL|CVAR_RENDERER|CVAR_ARCHIVE, "Use experimental new backend" );
idCVar r_useBindlessTextures("r_useBindlessTextures", "0", CVAR_BOOL|CVAR_RENDERER|CVAR_ARCHIVE, "Use experimental bindless texturing to reduce drawcall overhead (if supported by hardware)");

RenderBackend::RenderBackend() 
	: depthStage( &shaderParamsBuffer, &drawBatchExecutor ),
	  interactionStage( &shaderParamsBuffer, &drawBatchExecutor ),
	  stencilShadowStage( &shaderParamsBuffer, &drawBatchExecutor )
{}

void RenderBackend::Init() {
	shaderParamsBuffer.Init();
	drawBatchExecutor.Init();
	depthStage.Init();
	interactionStage.Init();
	stencilShadowStage.Init();
}

void RenderBackend::Shutdown() {
	stencilShadowStage.Shutdown();
	interactionStage.Shutdown();
	depthStage.Shutdown();
	drawBatchExecutor.Destroy();
	shaderParamsBuffer.Destroy();
}

void RenderBackend::DrawView( const viewDef_t *viewDef ) {
	// we will need to do a new copyTexSubImage of the screen when a SS_POST_PROCESS material is used
	backEnd.currentRenderCopied = false;
	backEnd.afterFogRendered = false;

	GL_PROFILE( "DrawView" );

	// skip render bypasses everything that has models, assuming
	// them to be 3D views, but leaves 2D rendering visible
	if ( viewDef->viewEntitys && r_skipRender.GetBool() ) {
		return;
	}

	// skip render context sets the wgl context to NULL,
	// which should factor out the API cost, under the assumption
	// that all gl calls just return if the context isn't valid
	if ( viewDef->viewEntitys && r_skipRenderContext.GetBool() ) {
		GLimp_DeactivateContext();
	}
	backEnd.pc.c_surfaces += viewDef->numDrawSurfs;

	RB_ShowOverdraw();


	int processed;

	backEnd.depthFunc = GLS_DEPTHFUNC_EQUAL;

	// clear the z buffer, set the projection matrix, etc
	RB_BeginDrawingView();

	backEnd.lightScale = r_lightScale.GetFloat();
	backEnd.overBright = 1.0f;

	drawSurf_t **drawSurfs = ( drawSurf_t ** )&viewDef->drawSurfs[ 0 ];
	int numDrawSurfs = viewDef->numDrawSurfs;

	// if we are just doing 2D rendering, no need to fill the depth buffer
	if ( viewDef->viewEntitys ) {
		// fill the depth buffer and clear color buffer to black except on subviews
		depthStage.DrawDepth( viewDef, drawSurfs, numDrawSurfs );
		if( ambientOcclusion->ShouldEnableForCurrentView() ) {
			ambientOcclusion->ComputeSSAOFromDepth();
		}
		DrawShadowsAndInteractions( viewDef );
	}
		
	// now draw any non-light dependent shading passes
	int RB_STD_DrawShaderPasses( drawSurf_t **drawSurfs, int numDrawSurfs );
	processed = RB_STD_DrawShaderPasses( drawSurfs, numDrawSurfs );

	// fog and blend lights
	void RB_STD_FogAllLights( bool translucent );
	RB_STD_FogAllLights( false );

	// refresh fog and blend status 
	backEnd.afterFogRendered = true;

	// now draw any post-processing effects using _currentRender
	if ( processed < numDrawSurfs ) {
		RB_STD_DrawShaderPasses( drawSurfs + processed, numDrawSurfs - processed );
	}

	RB_STD_FogAllLights( true ); // 2.08: second fog pass, translucent only

	RB_RenderDebugTools( drawSurfs, numDrawSurfs );

	// restore the context for 2D drawing if we were stubbing it out
	if ( r_skipRenderContext.GetBool() && viewDef->viewEntitys ) {
		GLimp_ActivateContext();
		RB_SetDefaultGLState();
	}
}

void RenderBackend::EndFrame() {
	shaderParamsBuffer.Lock();
	drawBatchExecutor.Lock();
	if (GLAD_GL_ARB_bindless_texture) {
		globalImages->MakeUnusedImagesNonResident();
	}
}

bool RenderBackend::ShouldUseBindlessTextures() const {
	return GLAD_GL_ARB_bindless_texture && r_useBindlessTextures.GetBool();
}

void RenderBackend::DrawInteractionsWithShadowMapping(viewLight_t *vLight) {
	void RB_GLSL_DrawInteractions_ShadowMap( const drawSurf_t *surf, bool clear = false );

	GL_PROFILE( "DrawLight_ShadowMap" );

	if ( vLight->lightShader->LightCastsShadows() ) {
		RB_GLSL_DrawInteractions_ShadowMap( vLight->globalInteractions, true );
		interactionStage.DrawInteractions( vLight, vLight->localInteractions );
		RB_GLSL_DrawInteractions_ShadowMap( vLight->localInteractions );
	} else {
		interactionStage.DrawInteractions( vLight, vLight->localInteractions );
	}
	interactionStage.DrawInteractions( vLight, vLight->globalInteractions );

	GLSLProgram::Deactivate();
}

void RenderBackend::DrawInteractionsWithStencilShadows( const viewDef_t *viewDef, viewLight_t *vLight ) {
	GL_PROFILE( "DrawLight_Stencil" );

	bool useShadowFbo = r_softShadowsQuality.GetBool() && !backEnd.viewDef->IsLightGem();// && (r_shadows.GetInteger() != 2);

	// set depth bounds for the whole light
	const DepthBoundsTest depthBoundsTest( vLight->scissorRect );

	// clear the stencil buffer if needed
	if ( vLight->globalShadows || vLight->localShadows ) {
		backEnd.currentScissor = vLight->scissorRect;

		if ( r_useScissor.GetBool() ) {
			GL_ScissorVidSize( viewDef->viewport.x1 + backEnd.currentScissor.x1,
			            viewDef->viewport.y1 + backEnd.currentScissor.y1,
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

	stencilShadowStage.DrawStencilShadows( vLight, vLight->globalShadows );
	if ( useShadowFbo && r_multiSamples.GetInteger() > 1 && r_softShadowsQuality.GetInteger() >= 0 ) {
		frameBuffers->ResolveShadowStencilAA();
	}

	if ( useShadowFbo ) {
		frameBuffers->LeaveShadowStencil();
	}
	interactionStage.DrawInteractions( vLight, vLight->localInteractions );

	if ( useShadowFbo ) {
		frameBuffers->EnterShadowStencil();
	}

	stencilShadowStage.DrawStencilShadows( vLight, vLight->localShadows );
	if ( useShadowFbo && r_multiSamples.GetInteger() > 1 && r_softShadowsQuality.GetInteger() >= 0 ) {
		frameBuffers->ResolveShadowStencilAA();
	}


	if ( useShadowFbo ) {
		frameBuffers->LeaveShadowStencil();
	}

	interactionStage.DrawInteractions( vLight, vLight->globalInteractions );

	GLSLProgram::Deactivate();
}

void RenderBackend::DrawShadowsAndInteractions( const viewDef_t *viewDef ) {
	GL_PROFILE( "LightInteractions" );

	if ( r_shadows.GetInteger() == 2 ) {
		if ( r_shadowMapSinglePass.GetBool() ) {
			void RB_ShadowMap_RenderAllLights();
			RB_ShadowMap_RenderAllLights();
		}
	}

	if ( r_shadows.GetInteger() != 1 && r_interactionProgram.GetInteger() == 2 ) {
		extern void RB_GLSL_DrawInteractions_MultiLight();
		RB_GLSL_DrawInteractions_MultiLight();
		return;
	}

	// for each light, perform adding and shadowing
	for ( viewLight_t *vLight = viewDef->viewLights; vLight; vLight = vLight->next ) {
		if ( vLight->lightShader->IsFogLight() || vLight->lightShader->IsBlendLight() ) {
			continue;
		}

		// if there are no interactions, get out!
		if ( !vLight->localInteractions && !vLight->globalInteractions && !vLight->translucentInteractions )
			continue;

		backEnd.vLight = vLight;
		if ( vLight->shadows == LS_MAPS ) {
			DrawInteractionsWithShadowMapping( vLight );
		} else {
			DrawInteractionsWithStencilShadows( viewDef, vLight );
		}

		if ( r_skipTranslucent.GetBool() ) {
			continue;
		}
		qglStencilFunc( GL_ALWAYS, 128, 255 );
		backEnd.depthFunc = GLS_DEPTHFUNC_LESS;
		interactionStage.DrawInteractions( vLight, vLight->translucentInteractions );
		backEnd.depthFunc = GLS_DEPTHFUNC_EQUAL;
	}

	// disable stencil shadow test
	qglStencilFunc( GL_ALWAYS, 128, 255 );
	GL_SelectTexture( 0 );
}
