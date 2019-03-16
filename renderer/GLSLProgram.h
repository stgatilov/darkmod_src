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

#ifndef __GLSL_PROGRAM_H__
#define __GLSL_PROGRAM_H__

#include <unordered_map>
#include <typeindex>

class GLSLUniformGroup;

class GLSLProgram {
public:
	explicit GLSLProgram(const char *name);
	~GLSLProgram();

	void Init();
	void Destroy();

	void AttachVertexShader( const char *sourceFile, const idDict &defines = idDict() );
	void AttachGeometryShader( const char *sourceFile, const idDict &defines = idDict() );
	void AttachFragmentShader( const char *sourceFile, const idDict &defines = idDict() );

	void BindAttribLocation( unsigned int location, const char *attribName );
	void BindDefaultAttribLocations();

	bool Link();
	bool Validate();

	void Activate();
	static void Deactivate();

	int GetUniformLocation( const char *uniformName );

	template<typename Group>
	Group *GetUniformGroup() {
		GLSLUniformGroup *& group = uniformGroups[typeid(Group)];
		if( group == nullptr ) {
			Activate();
			group = new Group( this );
		}
		return static_cast<Group*>(group);
	}

	const idStr &GetName() const { return name; }

	GLSLProgram( const GLSLProgram &other ) = delete;
	GLSLProgram & operator=( const GLSLProgram &other ) = delete;
	GLSLProgram( GLSLProgram &&other ) = delete;
	GLSLProgram & operator=( GLSLProgram &&other ) = delete;

private:
	static GLuint currentProgram;

	idStr name;
	GLuint program;
	std::unordered_map<std::type_index, GLSLUniformGroup*> uniformGroups;

	void LoadAndAttachShader( GLint shaderType, const char *sourceFile, const idDict &defines );
	GLuint CompileShader( GLint shaderType, const char *sourceFile, const idDict &defines );
};

#endif
