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
#include "FrobOutlineStage.h"

#include "../glsl.h"
#include "../GLSLProgram.h"
#include "../GLSLProgramManager.h"
#include "../tr_local.h"
#include "../FrameBuffer.h"
#include "../FrameBufferManager.h"

idCVar r_frobIgnoreDepth( "r_frobIgnoreDepth", "1", CVAR_BOOL|CVAR_RENDERER|CVAR_ARCHIVE, "Ignore depth when drawing frob outline" );
idCVar r_frobDepthOffset( "r_frobDepthOffset", "0.004", CVAR_FLOAT|CVAR_RENDERER|CVAR_ARCHIVE, "Extra depth offset for frob outline" );
idCVar r_frobOutline( "r_frobOutline", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "Work-in-progress outline around highlighted objects: 1 = image-based, 2 = geometric" );
idCVar r_frobOutlineColorR( "r_frobOutlineColorR", "1.0", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE , "Color of the frob outline - red component" );
idCVar r_frobOutlineColorG( "r_frobOutlineColorG", "1.0", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE , "Color of the frob outline - green component" );
idCVar r_frobOutlineColorB( "r_frobOutlineColorB", "1.0", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE , "Color of the frob outline - blue component" );
idCVar r_frobOutlineColorA( "r_frobOutlineColorA", "1.2", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE , "Color of the frob outline - alpha component" );
idCVar r_frobOutlineExtrusion( "r_frobOutlineExtrusion", "10", CVAR_FLOAT | CVAR_RENDERER | CVAR_ARCHIVE, "Thickness of geometric outline in pixels" );
idCVar r_frobHighlightColorMulR( "r_frobHighlightColorMulR", "0.3", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE , "Diffuse color of the frob highlight - red component" );
idCVar r_frobHighlightColorMulG( "r_frobHighlightColorMulG", "0.3", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE , "Diffuse color of the frob highlight - green component" );
idCVar r_frobHighlightColorMulB( "r_frobHighlightColorMulB", "0.3", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE , "Diffuse color of the frob highlight - blue component" );
idCVar r_frobHighlightColorAddR( "r_frobHighlightColorAddR", "0.02", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE , "Added color of the frob highlight - red component" );
idCVar r_frobHighlightColorAddG( "r_frobHighlightColorAddG", "0.02", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE , "Added color of the frob highlight - green component" );
idCVar r_frobHighlightColorAddB( "r_frobHighlightColorAddB", "0.02", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE , "Added color of the frob highlight - blue component" );
idCVar r_frobOutlineBlurPasses( "r_frobOutlineBlurPasses", "2", CVAR_RENDERER|CVAR_FLOAT|CVAR_ARCHIVE, "Thickness of the new frob outline" );

namespace {
	struct FrobOutlineUniforms : GLSLUniformGroup {
		UNIFORM_GROUP_DEF( FrobOutlineUniforms )

		DEFINE_UNIFORM( vec2, extrusion )
		DEFINE_UNIFORM( float, depth )
		DEFINE_UNIFORM( vec4, color )
		DEFINE_UNIFORM( vec4, colorAdd )
		DEFINE_UNIFORM( vec4, texMatrix )
		DEFINE_UNIFORM( sampler, diffuse )
		DEFINE_UNIFORM( float, alphaTest )
	};

	struct BlurUniforms : GLSLUniformGroup {
		UNIFORM_GROUP_DEF( BlurUniforms )

		DEFINE_UNIFORM( sampler, source )
		DEFINE_UNIFORM( vec2, axis )
	};

	struct ApplyUniforms : GLSLUniformGroup {
		UNIFORM_GROUP_DEF( ApplyUniforms )

		DEFINE_UNIFORM( sampler, source )
		DEFINE_UNIFORM( vec4, color )
	};
}

void FrobOutlineStage::Init() {
	silhouetteShader = programManager->LoadFromFiles( "frob_silhouette", "stages/frob/frob.vert.glsl", "stages/frob/frob_flat.frag.glsl" );
	highlightShader = programManager->LoadFromFiles( "frob_highlight", "stages/frob/frob.vert.glsl", "stages/frob/frob_highlight.frag.glsl" );
	extrudeShader = programManager->LoadFromFiles( "frob_extrude", "stages/frob/frob.vert.glsl", "stages/frob/frob_modalpha.frag.glsl", "stages/frob/frob_extrude.geom.glsl" );
	applyShader = programManager->LoadFromFiles( "frob_apply", "fullscreen_tri.vert.glsl", "stages/frob/frob_apply.frag.glsl" );
	colorTex[0] = globalImages->ImageFromFunction( "frob_color_0", FB_RenderTexture );
	colorTex[1] = globalImages->ImageFromFunction( "frob_color_1", FB_RenderTexture );
	depthTex = globalImages->ImageFromFunction( "frob_depth", FB_RenderTexture );
	fbo[0] = frameBuffers->CreateFromGenerator( "frob_0", [this](FrameBuffer *) { this->CreateFbo( 0 ); } );
	fbo[1] = frameBuffers->CreateFromGenerator( "frob_1", [this](FrameBuffer *) { this->CreateFbo( 1 ); } );
	drawFbo = frameBuffers->CreateFromGenerator( "frob_draw", [this](FrameBuffer *) { this->CreateDrawFbo(); } );
}

void FrobOutlineStage::Shutdown() {}

void FrobOutlineStage::DrawFrobOutline( drawSurf_t **drawSurfs, int numDrawSurfs ) {
	// find any surfaces that should be outlined
	idList<drawSurf_t *> outlineSurfs;
	for ( int i = 0; i < numDrawSurfs; ++i ) {
		drawSurf_t *surf = drawSurfs[i];

		if ( !surf->shaderRegisters[EXP_REG_PARM11] )
			continue;
		if ( !surf->material->HasAmbient() || !surf->numIndexes || !surf->ambientCache.IsValid() || !surf->space
			//stgatilov: some objects are fully transparent
			//I want to at least draw outline around them!
			/* || surf->material->GetSort() >= SS_PORTAL_SKY*/
		) {
			continue;
		}

		outlineSurfs.AddGrow( surf );
	}

	if ( outlineSurfs.Num() > 0 ) {
		TRACE_GL_SCOPE( "DrawFrobOutline" )

		GL_ScissorRelative( 0, 0, 1, 1 );

		MaskObjects( outlineSurfs );
		if ( r_frobOutline.GetInteger() == 2 ) {
			// old new implementation: extruded geometry, no image stuff
			DrawGeometricOutline( outlineSurfs );
		}
		else {
			if ( !r_frobIgnoreDepth.GetBool() ) {
				MaskOutlines( outlineSurfs );
			}
			if ( r_frobOutline.GetInteger() == 1 ) {
				DrawSoftOutline( outlineSurfs );
			}
		}
	}

	GL_State( GLS_DEPTHFUNC_EQUAL );
	qglStencilFunc( GL_ALWAYS, 255, 255 );
	GL_SelectTexture( 0 );
}

void FrobOutlineStage::CreateFbo( int idx ) {
	int width = frameBuffers->renderWidth / 4;
	int height = frameBuffers->renderHeight / 4;
	colorTex[idx]->GenerateAttachment( width, height, GL_R8 );
	fbo[idx]->Init( width, height );
	fbo[idx]->AddColorRenderTexture( 0, colorTex[idx] );
}

void FrobOutlineStage::CreateDrawFbo() {
	int width = frameBuffers->renderWidth / 4;
	int height = frameBuffers->renderHeight / 4;
	drawFbo->Init( width, height, 4 );
	drawFbo->AddColorRenderBuffer( 0, GL_R8 );
}

void FrobOutlineStage::MaskObjects( idList<drawSurf_t *> &surfs ) {
	// mark surfaces in the stencil buffer
	qglClearStencil( 0 );
	qglClear( GL_STENCIL_BUFFER_BIT );
	qglStencilFunc( GL_ALWAYS, 255, 255 );
	qglStencilOp( GL_KEEP, GL_REPLACE, GL_REPLACE );
	GLSLProgram *shader;
	if ( r_newFrob.GetInteger() == 1 ) {
		shader = highlightShader;
		GL_State( GLS_DEPTHFUNC_LESS | GLS_DEPTHMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE );
	} else {
		shader = silhouetteShader;
		GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_DEPTHMASK | GLS_COLORMASK );
	}
	shader->Activate();
	FrobOutlineUniforms *frobUniforms = shader->GetUniformGroup<FrobOutlineUniforms>();
	frobUniforms->extrusion.Set( 0.f, 0.f );
	frobUniforms->depth.Set( 0.f );
	frobUniforms->color.Set( r_frobHighlightColorMulR.GetFloat(), r_frobHighlightColorMulG.GetFloat(), r_frobHighlightColorMulB.GetFloat(), 1 );
	frobUniforms->colorAdd.Set( r_frobHighlightColorAddR.GetFloat(), r_frobHighlightColorAddG.GetFloat(), r_frobHighlightColorAddB.GetFloat(), 1 );

	DrawObjects( surfs, shader, r_newFrob.GetInteger() == 1 );
}

void FrobOutlineStage::MaskOutlines( idList<drawSurf_t *> &surfs ) {
	extrudeShader->Activate();
	// mask triangle outlines where depth test fails
	qglStencilFunc( GL_NOTEQUAL, 255, 255 );
	qglStencilOp( GL_KEEP, GL_REPLACE, GL_KEEP );
	GL_State( GLS_DEPTHFUNC_LESS | GLS_DEPTHMASK | GLS_COLORMASK );
	auto *uniforms = extrudeShader->GetUniformGroup<FrobOutlineUniforms>();
	float ext = r_frobOutlineBlurPasses.GetFloat() * 0.02;
	uniforms->extrusion.Set( ext, ext );
	uniforms->depth.Set( r_frobDepthOffset.GetFloat() );

	DrawObjects( surfs, extrudeShader, false );
}

void FrobOutlineStage::DrawGeometricOutline( idList<drawSurf_t*> &surfs ) {
	// keep marked object pixels unmodified
	qglStencilFunc( GL_NOTEQUAL, 255, 255 );
	qglStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
	// enable geometry shader which produces extruded geometry
	extrudeShader->Activate();
	// outline 1) can be occluded by objects in the front, 2) is translucent
	GL_State( GLS_DEPTHFUNC_LESS | GLS_DEPTHMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
	auto *uniforms = extrudeShader->GetUniformGroup<FrobOutlineUniforms>();
	idVec2 extr = idVec2(
		1.0f / idMath::Fmax( frameBuffers->defaultFbo->Width(), 800.0f ),
		1.0f / idMath::Fmax( frameBuffers->defaultFbo->Height(), 600.0f )
	) * r_frobOutlineExtrusion.GetFloat();
	uniforms->extrusion.Set( extr );
	uniforms->depth.Set( r_frobDepthOffset.GetFloat() );
	uniforms->color.Set( r_frobOutlineColorR.GetFloat(), r_frobOutlineColorG.GetFloat(), r_frobOutlineColorB.GetFloat(), r_frobOutlineColorA.GetFloat() );

	DrawObjects( surfs, extrudeShader, true );
}

void FrobOutlineStage::DrawSoftOutline( idList<drawSurf_t *> &surfs ) {
	silhouetteShader->Activate();
	auto *silhouetteUniforms = silhouetteShader->GetUniformGroup<FrobOutlineUniforms>();
	silhouetteUniforms->color.Set( 1, 1, 1, 1 );
	// draw to small anti-aliased color buffer
	FrameBuffer *previousFbo = frameBuffers->activeFbo;
	drawFbo->Bind();
	GL_ViewportRelative( 0, 0, 1, 1 );
	GL_ScissorRelative( 0, 0, 1, 1 );
	GL_State( GLS_DEPTHFUNC_ALWAYS );
	qglClearColor( 0, 0, 0, 0 );
	qglClear( GL_COLOR_BUFFER_BIT );

	DrawObjects( surfs, silhouetteShader, false );

	// resolve color buffer
	drawFbo->BlitTo( fbo[0], GL_COLOR_BUFFER_BIT );

	// apply blur
	for ( int i = 0; i < r_frobOutlineBlurPasses.GetFloat(); ++i )
		ApplyBlur();

	previousFbo->Bind();
	GL_ViewportRelative( 0, 0, 1, 1 );
	GL_ScissorRelative( 0, 0, 1, 1 );

	GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_DEPTHMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
	qglStencilFunc( GL_NOTEQUAL, 255, 255 );
	GL_Cull( CT_FRONT_SIDED );
	applyShader->Activate();
	ApplyUniforms *applyUniforms = applyShader->GetUniformGroup<ApplyUniforms>();
	applyUniforms->source.Set( 0 );
	GL_SelectTexture( 0 );
	colorTex[0]->Bind();
	applyUniforms->color.Set( r_frobOutlineColorR.GetFloat(), r_frobOutlineColorG.GetFloat(), r_frobOutlineColorB.GetFloat(), r_frobOutlineColorA.GetFloat() );
	RB_DrawFullScreenTri();
}

extern void R_SetDrawInteraction( const shaderStage_t *surfaceStage, const float *surfaceRegs,
                           idImage **image, idVec4 matrix[2], float color[4] );

void FrobOutlineStage::DrawObjects( idList<drawSurf_t *> &surfs, GLSLProgram  *shader, bool bindDiffuseTexture ) {
	if ( bindDiffuseTexture ) {
		GL_SelectTexture( 1 );
		shader->GetUniformGroup<FrobOutlineUniforms>()->diffuse.Set( 1 );
	}

	for ( drawSurf_t *surf : surfs ) {
		GL_Cull( surf->material->GetCullType() );
		shader->GetUniformGroup<Uniforms::Global>()->Set( surf->space );
		vertexCache.VertexPosition( surf->ambientCache );

		if ( bindDiffuseTexture ) {
			//stgatilov: some transparent objects have no diffuse map
			//then using white results in very strong surface highlighting
			//better stay conservative and don't highlight them (almost)
			idImage *diffuse = globalImages->blackImage;

			const idMaterial *material = surf->material;
			for ( int i = 0; i < material->GetNumStages(); ++i ) {
				const shaderStage_t *stage = material->GetStage( i );
				if ( stage->lighting == SL_DIFFUSE && stage->texture.image ) {
					idVec4 textureMatrix[2];
					R_SetDrawInteraction( stage, surf->shaderRegisters, &diffuse, textureMatrix, nullptr );
					auto *uniforms = shader->GetUniformGroup<FrobOutlineUniforms>();
					uniforms->texMatrix.SetArray( 2, textureMatrix[0].ToFloatPtr() );
					uniforms->alphaTest.Set( stage->hasAlphaTest ? surf->shaderRegisters[stage->alphaTestRegister] : -1.0f );
					break;
				}
			}
			diffuse->Bind();
		}

		RB_DrawElementsWithCounters( surf );
	}
}

void FrobOutlineStage::ApplyBlur() {
	programManager->gaussianBlurShader->Activate();
	BlurUniforms *uniforms = programManager->gaussianBlurShader->GetUniformGroup<BlurUniforms>();
	uniforms->source.Set( 0 );

	GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_DEPTHMASK | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
	GL_SelectTexture( 0 );
	colorTex[0]->Bind();
	uniforms->axis.Set( 1, 0 );
	fbo[1]->Bind();
	qglClear(GL_COLOR_BUFFER_BIT);
	RB_DrawFullScreenTri();

	uniforms->axis.Set( 0, 1 );
	fbo[0]->Bind();
	colorTex[1]->Bind();
	qglClear(GL_COLOR_BUFFER_BIT);
	RB_DrawFullScreenTri();
}
