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

class GLSLProgram {
public:
	static GLSLProgram * Load( const char *vertexSourceFile, const char *fragmentSourceFile, const char *geometrySourceFile = nullptr );
	static GLSLProgram * Load( const idDict &defines, const char *vertexSourceFile, const char *fragmentSourceFile, const char *geometrySourceFile = nullptr );
	static GLSLProgram * Load( const char *programFileName, const idDict *defines = nullptr );

	~GLSLProgram();

	const char* GetFileName(GLint shaderType) const;
	const idDict &GetDefines() const { return defines; }

	void Activate();
	static void Deactivate();

	void BindAttribLocation( GLuint index, const char *name );

	void AddUniformAlias( int alias, const char *uniformName );

	// use these functions to bind a value directly to a uniform location
	void Uniform1fL( int location, GLfloat value );
	void Uniform2fL( int location, GLfloat v1, GLfloat v2 );
	void Uniform3fL( int location, GLfloat v1, GLfloat v2, GLfloat v3 );
	void Uniform4fL( int location, GLfloat v1, GLfloat v2, GLfloat v3, GLfloat v4 );
	void Uniform1iL( int location, GLint value );
	void Uniform2iL( int location, GLint v1, GLint v2 );
	void Uniform3iL( int location, GLint v1, GLint v2, GLint v3 );
	void Uniform4iL( int location, GLint v1, GLint v2, GLint v3, GLint v4 );
	void Uniform2fL( int location, const idVec2 &value );
	void Uniform3fL( int location, const idVec3 &value );
	void Uniform4fL( int location, const idVec4 &value );
	void Uniform4fvL( int location, GLfloat *value );
	void UniformMatrix4L( int location, const GLfloat *matrix );

	// use these functions to bind a value by an alias defined for a uniform name via AddUniformAlias
	void Uniform1f( int alias, GLfloat value ) { Uniform1fL( AliasToLocation( alias ), value ); }
	void Uniform2f( int alias, GLfloat v1, GLfloat v2 ) { Uniform2fL( AliasToLocation( alias ), v1, v2 ); }
	void Uniform3f( int alias, GLfloat v1, GLfloat v2, GLfloat v3 ) { Uniform3fL( AliasToLocation( alias ), v1, v2, v3 ); }
	void Uniform4f( int alias, GLfloat v1, GLfloat v2, GLfloat v3, GLfloat v4 ) { Uniform4fL( AliasToLocation( alias ), v1, v2, v3, v4 ); }
	void Uniform1i( int alias, GLint value ) { Uniform1iL( AliasToLocation( alias ), value ); }
	void Uniform2i( int alias, GLint v1, GLint v2 ) { Uniform2iL( AliasToLocation( alias ), v1, v2 ); }
	void Uniform3i( int alias, GLint v1, GLint v2, GLint v3 ) { Uniform3iL( AliasToLocation( alias ), v1, v2, v3 ); }
	void Uniform4i( int alias, GLint v1, GLint v2, GLint v3, GLint v4 ) { Uniform4iL( AliasToLocation( alias ), v1, v2, v3, v4 ); }
	void Uniform2f( int alias, const idVec2 &value ) { Uniform2fL( AliasToLocation( alias ), value ); }
	void Uniform3f( int alias, const idVec3 &value ) { Uniform3fL( AliasToLocation( alias ), value ); }
	void Uniform4f( int alias, const idVec4 &value ) { Uniform4fL( AliasToLocation( alias ), value ); }
	void Uniform4fv( int alias, GLfloat *value ) { Uniform4fvL( AliasToLocation( alias ), value ); }
	void UniformMatrix4( int alias, const GLfloat *matrix ) { UniformMatrix4L( AliasToLocation( alias ), matrix ); }

	GLSLProgram( const GLSLProgram &other ) = delete;
	GLSLProgram & operator=( const GLSLProgram &other ) = delete;
	GLSLProgram( GLSLProgram &&other ) = delete;
	GLSLProgram & operator=( GLSLProgram &&other ) = delete;

private:
	static GLuint currentProgram;

	idStr filenames[3];
	idDict defines;

	GLuint program;

	struct aliasLocation_t {
		int alias;
		int location;
	};
	idList<aliasLocation_t> aliasLocationMap;
	idList<idStr> aliasNames;

	struct bindAttribute_t {
		int index;
		idStr name;
	};
	idList<bindAttribute_t> boundAttributes;

	explicit GLSLProgram( GLuint program );

	int AliasToLocation( int alias ) const {
		for( int i = 0; i < aliasLocationMap.Num(); ++i ) {
			if( aliasLocationMap[i].alias == alias ) {
				return aliasLocationMap[i].location;
			}
		}
		common->Warning( "Missing uniform alias %d\n", alias );
		return -1;
	}

	friend class GLSLProgramLoader;
};

class GLSLProgramLoader {
public:
	GLSLProgramLoader();
	~GLSLProgramLoader();

	void AddVertexShader( const char *sourceFile, const idDict &defines = idDict() );
	void AddFragmentShader( const char *sourceFile, const idDict &defines = idDict() );
	void AddGeometryShader( const char *sourceFile, const idDict &defines = idDict() );

	GLSLProgram * LinkProgram();

private:
	GLuint program;

	void LoadAndAttachShader( GLint shaderType, const char *sourceFile, const idDict &defines );
	GLuint CompileShader( GLint shaderType, const char *sourceFile, const idDict &defines );
};

enum glslUniformAlias_t {
	PROJECTION_MATRIX,
	MODELVIEW_MATRIX,
	MVP_MATRIX,
};

struct globalPrograms_t {
	GLSLProgram *interactionShader;
};

extern globalPrograms_t globalPrograms;

void GLSL_InitPrograms();
void GLSL_DestroyPrograms();

#endif
