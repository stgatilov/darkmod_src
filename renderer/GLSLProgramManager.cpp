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
	void DefaultProgramInit( GLSLProgram *program, const idDict &defines, const char *vertexSource, const char *fragmentSource, const char *geometrySource = nullptr ) {
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
	for( auto it : programs ) {
		delete it.program;
	}
	programs.Clear();

	frobShader = nullptr;
	cubeMapShader = nullptr;
	depthShader = nullptr;
	fogShader = nullptr;
	oldStageShader = nullptr;
	blendShader = nullptr;
	stencilShadowShader = nullptr;
	shadowMapShader = nullptr;
	ambientInteractionShader = nullptr;
	stencilInteractionShader = nullptr;
	shadowMapInteractionShader = nullptr;
	multiLightInteractionShader = nullptr;
}

GLSLProgram * GLSLProgramManager::Load( const idStr &name, const idDict &defines ) {
	Generator generator = [=]( GLSLProgram *program ) {
		if( fileSystem->FindFile( idStr("glprogs/") + name + ".gs" ) != FIND_NO ) {
			DefaultProgramInit( program, defines, name + ".vs", name + ".fs", name + ".gs" );
		} else {
			DefaultProgramInit( program, defines, name + ".vs", name + ".fs", nullptr );
		}
	};
	return LoadFromGenerator( name, generator );	
}

GLSLProgram * GLSLProgramManager::LoadFromFiles( const idStr &name, const idStr &vertexSource, const idStr &fragmentSource, const idDict &defines ) {
	return LoadFromFiles( name, vertexSource, fragmentSource, nullptr, defines );
}

GLSLProgram * GLSLProgramManager::LoadFromFiles( const idStr &name, const idStr &vertexSource, const idStr &fragmentSource, const idStr &geometrySource, const idDict &defines ) {
	Generator generator = [=]( GLSLProgram *program ) {
		DefaultProgramInit( program, defines, vertexSource, fragmentSource, geometrySource );
	};
	return LoadFromGenerator( name, generator );
}

GLSLProgram * GLSLProgramManager::LoadFromGenerator( const char *name, const Generator &generator ) {
	programWithGenerator_t *entry = FindEntry( name );
	if( entry != nullptr ) {
		common->Warning( "Program %s already exists and is being overwritten.\n", name );
		entry->generator = generator;
		if( renderSystem->IsOpenGLRunning() ) {
			Reload( entry );
		}
		return entry->program;
	} else {
		GLSLProgram *program = new GLSLProgram( name );
		programs.Append( programWithGenerator_t { program, generator } );
		if( renderSystem->IsOpenGLRunning() ) {
			program->Init();
			generator(program);
		}
		return program;
	}
}

GLSLProgram * GLSLProgramManager::Find( const char *name ) {
	programWithGenerator_t *entry = FindEntry( name );
	return entry != nullptr ? entry->program : nullptr;
}

void GLSLProgramManager::Reload( const char *name ) {
	programWithGenerator_t *entry = FindEntry( name );
	if( entry != nullptr ) {
		Reload( entry );
	}
}

void GLSLProgramManager::ReloadAllPrograms() {
	for( auto &it : programs ) {
		Reload( &it );
	}
}

GLSLProgramManager::programWithGenerator_t * GLSLProgramManager::FindEntry( const char *name ) {
	for( auto &it : programs ) {
		if( it.program->GetName() == name ) {
			return &it;			
		}
	}
	return nullptr;
}

void GLSLProgramManager::Reload( programWithGenerator_t *entry ) {
	entry->program->Destroy();
	entry->program->Init();
	entry->generator( entry->program );
}



// INITIALIZE BUILTIN PROGRAMS HERE

namespace {
	void InitInteractionShader( GLSLProgram *program ) {
		idDict defines;
		// TODO: set some defines based on cvars
		defines.Set( "SOFT", "1" );
		DefaultProgramInit( program, defines, "interaction.vs", "interaction.fs" );
		program->Validate();
	}

	void InitDepthShader( GLSLProgram *program ) {
		DefaultProgramInit( program, idDict(), "depthAlpha.vs", "depthAlpha.fs" );
		program->Activate();
		GLSLUniform_sampler( program, "u_tex0" ).Set( 0 );
		program->Validate();
	}

	void InitFogShader( GLSLProgram *program ) {
		DefaultProgramInit( program, idDict(), "fog.vs", "fog.fs" );
		program->Activate();
		GLSLUniform_sampler( program, "u_texture0" ).Set( 0 );
		GLSLUniform_sampler( program, "u_texture1" ).Set( 1 );
		program->Validate();
	}

	void InitOldStageShader( GLSLProgram *program ) {
		DefaultProgramInit( program, idDict(), "oldStage.vs", "oldStage.fs" );
		program->Activate();
		GLSLUniform_sampler( program, "u_tex0" ).Set( 0 );
		program->Validate();
	}

	void InitBlendShader( GLSLProgram *program ) {
		DefaultProgramInit( program, idDict(), "blend.vs", "blend.fs" );
		program->Activate();
		GLSLUniform_sampler( program, "u_texture0" ).Set( 0 );
		GLSLUniform_sampler( program, "u_texture1" ).Set( 1 );
		program->Validate();
	}

	void InitShadowMapShader( GLSLProgram *program ) {
		DefaultProgramInit( program, idDict(), "shadowMapA.vs", "shadowMapA.fs" );
		Uniforms::Depth *depthUniforms = program->GetUniformGroup<Uniforms::Depth>();
		depthUniforms->instances = 6;
		depthUniforms->acceptsTranslucent = true;
		program->Validate();
	}

	void InitSamplerBindingsForBumpShaders( GLSLProgram *program ) {
		GLSLUniform_sampler( program, "u_normalTexture" ).Set( 1 );
}

	GLSLProgram *LoadFromBaseNameWithCustomizer( const idStr &baseName, const std::function<void(GLSLProgram*)> customizer) {
		return programManager->LoadFromGenerator( baseName, [=]( GLSLProgram *program ) {
			idStr geometrySource = baseName + ".gs";
			if( fileSystem->FindFile( idStr( "glprogs/" ) + baseName + ".gs" ) != FIND_NO ) {
				DefaultProgramInit( program, idDict(), baseName + ".vs", baseName + ".fs", baseName + ".gs" );
			} else {
				DefaultProgramInit( program, idDict(), baseName + ".vs", baseName + ".fs", nullptr );
			}
			program->Activate();
			customizer( program );
			program->Validate();
		});		
	}

	GLSLProgram *LoadInteractionShader( const idStr &name, const idStr &baseName, bool ambient ) {
		return programManager->LoadFromGenerator( name, [=]( GLSLProgram *program ) {
			idDict defines;
			if( glConfig.gpuShader4Available ) {
				defines.Set( "EXT_gpu_shader4", "1" );
			}
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
	frobShader = Load( "frob" );
	bumpyEnvironment = LoadFromBaseNameWithCustomizer( "bumpyEnvironment", InitSamplerBindingsForBumpShaders );
	depthShader = LoadFromGenerator( "depthAlpha" , InitDepthShader );
	fogShader = LoadFromGenerator( "fog", InitFogShader );
	oldStageShader = LoadFromGenerator( "oldStage", InitOldStageShader );
	blendShader = LoadFromGenerator( "blend", InitBlendShader );
	stencilShadowShader = Load( "stencilshadow" );
	shadowMapShader = LoadFromGenerator( "shadowMap", InitShadowMapShader );
	ambientInteractionShader = LoadInteractionShader( "ambientInteraction", "ambientInteraction", true );
	stencilInteractionShader = LoadInteractionShader( "stencilInteraction", "interaction", false );
	shadowMapInteractionShader = LoadInteractionShader( "shadowMapInteraction", "interactionA", false );
	multiLightInteractionShader = LoadInteractionShader( "multiLightInteraction", "interactionN", false );
}

