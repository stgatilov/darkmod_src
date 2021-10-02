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
#include "GLSLProgramManager.h"
#include "GLSLProgram.h"
#include "glsl.h"

GLSLProgramManager programManagerInstance;
GLSLProgramManager *programManager = &programManagerInstance;

namespace {
	void DefaultProgramInit( GLSLProgram *program, idHashMapDict defines, const char *vertexSource, const char *fragmentSource = nullptr, const char *geometrySource = nullptr ) {
		program->Init();
		if( vertexSource != nullptr ) {
			program->AttachVertexShader( vertexSource, defines );
		}
		if( fragmentSource != nullptr ) {
			program->AttachFragmentShader( fragmentSource, defines );
		}
		if( geometrySource != nullptr ) {
			program->AttachGeometryShader( geometrySource, defines );
		}
		Attributes::Default::Bind( program );
		program->Link();
		program->Activate();
		int mv = program->GetUniformLocation( "u_modelViewMatrix" );
		if ( mv >= 0 )
			qglUniformMatrix4fv( mv, 1, false, mat4_identity.ToFloatPtr() );
		program->Deactivate();
	}
}


GLSLProgramManager::GLSLProgramManager() {
	// init all global program references to null
	Shutdown();
}

GLSLProgramManager::~GLSLProgramManager() {
	Shutdown();	
}

void GLSLProgramManager::Shutdown() {
	for( GLSLProgram *program : programs ) {
		delete program;
	}
	programs.ClearFree();

	frobShader = nullptr;
	cubeMapShader = nullptr;
	depthShader = nullptr;
	fogShader = nullptr;
	oldStageShader = nullptr;
	blendShader = nullptr;
	stencilShadowShader = nullptr;
	shadowMapShader = nullptr;
	shadowMapMultiShader = nullptr;
	ambientInteractionShader = nullptr;
	stencilInteractionShader = nullptr;
	shadowMapInteractionShader = nullptr;
	multiLightInteractionShader = nullptr;
}

GLSLProgram * GLSLProgramManager::Load( const idStr &name, const idHashMapDict &defines ) {
	Generator generator = [=]( GLSLProgram *program ) {
		if( fileSystem->FindFile( idStr("glprogs/") + name + ".gs" ) != FIND_NO ) {
			DefaultProgramInit( program, defines, name + ".vs", name + ".fs", name + ".gs" );
		} else {
			DefaultProgramInit( program, defines, name + ".vs", name + ".fs", nullptr );
		}
	};
	return LoadFromGenerator( name, generator );	
}

GLSLProgram * GLSLProgramManager::LoadFromFiles( const idStr &name, const idStr &vertexSource, const idHashMapDict &defines ) {
	Generator generator = [=]( GLSLProgram *program ) {
		DefaultProgramInit( program, defines, vertexSource );
	};
	return LoadFromGenerator( name, generator );
}

GLSLProgram * GLSLProgramManager::LoadFromFiles( const idStr &name, const idStr &vertexSource, const idStr &fragmentSource, const idHashMapDict &defines ) {
	Generator generator = [=]( GLSLProgram *program ) {
		DefaultProgramInit( program, defines, vertexSource, fragmentSource );
	};
	return LoadFromGenerator( name, generator );
}

GLSLProgram * GLSLProgramManager::LoadFromFiles( const idStr &name, const idStr &vertexSource, const idStr &fragmentSource, const idStr &geometrySource, const idHashMapDict &defines ) {
	Generator generator = [=]( GLSLProgram *program ) {
		DefaultProgramInit( program, defines, vertexSource, fragmentSource, geometrySource );
	};
	return LoadFromGenerator( name, generator );
}

GLSLProgram * GLSLProgramManager::LoadFromGenerator( const char *name, const Generator &generator ) {
	GLSLProgram *program = Find( name );
	if( program != nullptr ) {
		program->SetGenerator( generator );
		if( renderSystem->IsOpenGLRunning() ) {
			program->Destroy();
		}
		return program;
	} 

	program = new GLSLProgram( name, generator );
	programs.Append( program );
	return program;
}

GLSLProgram * GLSLProgramManager::Find( const char *name ) {
	for ( GLSLProgram *program : programs ) {
		if ( program->GetName() == name ) {
			return program;
		}
	}
	return nullptr;
}

void GLSLProgramManager::Reload( const char *name ) {
	GLSLProgram *program = Find( name );
	if( program != nullptr ) {
		program->Regenerate();
	}
}

void GLSLProgramManager::ReloadAllPrograms() {
	if ( uboHandle ) 
		qglDeleteBuffers( 1, &uboHandle );
	qglGenBuffers( 1, &uboHandle );
	qglBindBuffer( GL_UNIFORM_BUFFER, uboHandle );
	qglBindBufferBase( GL_UNIFORM_BUFFER, 0, uboHandle );
	for( GLSLProgram *program : programs ) {
		program->Regenerate();
	}
}


// INITIALIZE BUILTIN PROGRAMS HERE

namespace {
	void InitInteractionShader( GLSLProgram *program ) {
		idHashMapDict defines;
		// TODO: set some defines based on cvars
		defines.Set( "SOFT", "1" );
		DefaultProgramInit( program, defines, "interaction.vs", "interaction.fs" );
		program->Validate();
	}

	void InitDepthShader( GLSLProgram *program ) {
		DefaultProgramInit( program, {}, "depthAlpha.vs", "depthAlpha.fs" );
		program->Activate();
		GLSLUniform_sampler( program, "u_tex0" ).Set( 0 );
		program->GetUniformGroup<Uniforms::Global>()->textureMatrix.Set( mat4_identity );
		program->Validate();
	}

	void InitFogShader( GLSLProgram *program ) {
		DefaultProgramInit( program, {}, "fog.vs", "fog.fs" );
		program->Activate();
		GLSLUniform_sampler( program, "u_texture0" ).Set( 0 );
		GLSLUniform_sampler( program, "u_texture1" ).Set( 1 );
		program->GetUniformGroup<Uniforms::Global>()->textureMatrix.Set( mat4_identity );
		program->Validate();
	}

	void InitOldStageShader( GLSLProgram *program ) {
		DefaultProgramInit( program, {}, "oldStage.vs", "oldStage.fs" );
		program->Activate();
		GLSLUniform_sampler( program, "u_tex0" ).Set( 0 );
		program->GetUniformGroup<Uniforms::Global>()->textureMatrix.Set( mat4_identity );
		program->Validate();
	}

	void InitBlendShader( GLSLProgram *program ) {
		DefaultProgramInit( program, {}, "blend.vs", "blend.fs" );
		program->Activate();
		GLSLUniform_sampler( program, "u_texture0" ).Set( 0 );
		GLSLUniform_sampler( program, "u_texture1" ).Set( 1 );
		program->GetUniformGroup<Uniforms::Global>()->textureMatrix.Set( mat4_identity );
		program->Validate();
	}

	void InitShadowMapShader( GLSLProgram *program ) {
		DefaultProgramInit( program, {}, program->GetName() + ".vs", program->GetName() + ".fs"/*, program->GetName() + ".gs"*/ );
		Uniforms::Depth *depthUniforms = program->GetUniformGroup<Uniforms::Depth>();
		depthUniforms->instances = 6;
		depthUniforms->acceptsTranslucent = true; //duzenko: wait, what?
		program->Validate();
	}

	void InitSoftParticleShader( GLSLProgram *program ) {
		DefaultProgramInit( program, {}, program->GetName() + ".vs", program->GetName() + ".fs" );
		program->Activate();
		GLSLUniform_sampler( program, "u_texture0" ).Set( 0 );
		GLSLUniform_sampler( program, "u_texture1" ).Set( 1 );
		program->Validate();
	}

	void InitSamplerBindingsForBumpShaders( GLSLProgram *program ) {
		GLSLUniform_sampler( program, "u_normalTexture" ).Set( 1 );
		program->GetUniformGroup<Uniforms::Global>()->textureMatrix.Set( mat4_identity );
}

	GLSLProgram *LoadFromBaseNameWithCustomizer( const idStr &baseName, const std::function<void(GLSLProgram*)> customizer) {
		return programManager->LoadFromGenerator( baseName, [=]( GLSLProgram *program ) {
			idStr geometrySource = baseName + ".gs";
			if( fileSystem->FindFile( idStr( "glprogs/" ) + baseName + ".gs" ) != FIND_NO ) {
				DefaultProgramInit( program, {}, baseName + ".vs", baseName + ".fs", baseName + ".gs" );
			} else {
				DefaultProgramInit( program, {}, baseName + ".vs", baseName + ".fs", nullptr );
			}
			program->Activate();
			customizer( program );
			program->Validate();
		});		
	}

	GLSLProgram *LoadInteractionShader( const idStr &name, const idStr &baseName, bool ambient ) {
		return programManager->LoadFromGenerator( name, [=]( GLSLProgram *program ) {
			idHashMapDict defines;
			DefaultProgramInit( program, defines, baseName + ".vs", baseName + ".fs" );
			program->Activate();
			Uniforms::Interaction *interactionUniforms = program->GetUniformGroup<Uniforms::Interaction>();
			interactionUniforms->ambient = ambient;
			// static bindings
			interactionUniforms->normalTexture.Set( 0 );
			interactionUniforms->lightFalloffTexture.Set( 1 );
			interactionUniforms->lightProjectionTexture.Set( 2 );
			interactionUniforms->diffuseTexture.Set( 3 );
			interactionUniforms->specularTexture.Set( 4 );

			// can't have sampler2D, usampler2D, samplerCube have the same TMU index
			interactionUniforms->lightProjectionCubemap.Set( 5 );
			interactionUniforms->shadowMap.Set( 6 );
			interactionUniforms->stencilTexture.Set( 7 );
			interactionUniforms->lightFalloffCubemap.Set( 8 );
			program->Validate();
		} );
	}
}

void GLSLProgramManager::Init() {
	cubeMapShader = LoadFromBaseNameWithCustomizer( "cubeMap", InitSamplerBindingsForBumpShaders );
	bumpyEnvironment = LoadFromBaseNameWithCustomizer( "bumpyEnvironment", InitSamplerBindingsForBumpShaders );
	depthShader = LoadFromGenerator( "depthAlpha" , InitDepthShader );
	fogShader = LoadFromGenerator( "fog", InitFogShader );
	oldStageShader = LoadFromGenerator( "oldStage", InitOldStageShader );
	blendShader = LoadFromGenerator( "blend", InitBlendShader );
	stencilShadowShader = Load( "stencilshadow" );
	shadowMapShader = LoadFromGenerator( "shadowMapA", InitShadowMapShader );
	shadowMapMultiShader = LoadFromGenerator( "shadowMapN", InitShadowMapShader );
	shadowMapMultiGShader = LoadFromGenerator( "shadowMapNG", InitShadowMapShader );
	ambientInteractionShader = LoadInteractionShader( "ambientInteraction", "ambientInteraction", true );
	stencilInteractionShader = LoadInteractionShader( "interactionStencil", "interactionStencil", false );
	shadowMapInteractionShader = LoadInteractionShader( "interactionShadowMaps", "interactionShadowMaps", false );
	multiLightInteractionShader = LoadInteractionShader( "interactionMultiLight", "interactionMultiLight", false );
	frobShader = Load( "frob" );
	softParticleShader = LoadFromGenerator( "soft_particle", InitSoftParticleShader );
	toneMapShader = Load( "tonemap" );
	gaussianBlurShader = LoadFromFiles( "gaussian_blur", "fullscreen_tri.vert.glsl", "gaussian_blur.frag.glsl" );
	volumetricLightShader = Load( "volumetric" );
}

