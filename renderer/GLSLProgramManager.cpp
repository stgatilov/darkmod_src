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

	interactionShader = nullptr;
	frobShader = nullptr;
	cubeMapShader = nullptr;
	depthShader = nullptr;
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

	void InitCubeMapShader( GLSLProgram *program ) {
		DefaultProgramInit( program, idDict(), "cubeMap.vs", "cubeMap.fs" );
		program->Activate();
		GLSLUniform_sampler( program, "u_normalTexture" ).Set( 1 );
		program->Validate();
	}

	void InitDepthShader( GLSLProgram *program ) {
		DefaultProgramInit( program, idDict(), "depthAlpha.vs", "depthAlpha.fs" );
		program->Activate();
		GLSLUniform_sampler( program, "u_tex0" ).Set( 0 );
		program->Validate();
	}
}

void GLSLProgramManager::Init() {
	interactionShader = LoadFromGenerator( "interaction", InitInteractionShader );
	cubeMapShader = LoadFromGenerator( "cubeMap", InitCubeMapShader );
	frobShader = Load( "frob" );
	depthShader = LoadFromGenerator( "depthAlpha" , InitDepthShader );
}

