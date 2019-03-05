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

#ifndef __SHADER_H__
#define __SHADER_H__
#include <unordered_map>

using ShaderDefines = idHashTable<idStr>;

class Shader {
public:
	~Shader();

private:
	GLuint program;
	idHashTable<int> activeUniformLocations;
	idHashIndex virtualLocationMap;
};

class GLSLProgramLoader {
public:
	GLSLProgramLoader();

	void AddVertexShader( const char *sourceFile, const ShaderDefines &defines = ShaderDefines() );
	void AddFragmentShader( const char *sourceFile, const ShaderDefines &defines = ShaderDefines() );
	void AddGeometryShader( const char *sourceFile, const ShaderDefines &defines = ShaderDefines() );

private:
	GLuint program;

	void LoadAndAttachShader( GLint shaderType, const char *sourceFile, const ShaderDefines &defines );
	GLuint CompileShader( GLint shaderType, const char *sourceFile, const ShaderDefines &defines );
};

#endif
