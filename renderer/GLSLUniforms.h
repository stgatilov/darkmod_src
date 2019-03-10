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

#ifndef __GLSL_UNIFORMS_H__
#define __GLSL_UNIFORMS_H__

#include "GLSLProgram.h"
#include "qgl.h"

class GLSLUniformGroup {
public:
	GLSLUniformGroup( GLSLProgram *program ) : program( program ) {}
	virtual ~GLSLUniformGroup() {}

protected:
	GLSLProgram *program;
};

class GLSLUniform_int {
public:
	GLSLUniform_int(GLSLProgram *program, const char *uniformName)
			: paramLocation(program->GetUniformLocation(uniformName)) {}

	void Set(int value) {
		qglUniform1i(paramLocation, value);
	}

private:
	int paramLocation;
};

typedef GLSLUniform_int GLSLUniform_sampler;

class GLSLUniform_float {
public:
	GLSLUniform_float(GLSLProgram *program, const char *uniformName)
			: paramLocation(program->GetUniformLocation(uniformName)) {}

	void Set(float value) {
		qglUniform1f(paramLocation, value);
	}

private:
	int paramLocation;
};

class GLSLUniform_vec2 {
public:
	GLSLUniform_vec2(GLSLProgram *program, const char *uniformName)
			: paramLocation(program->GetUniformLocation(uniformName)) {}

	void Set(float v1, float v2) {
		qglUniform2f(paramLocation, v1, v2);
	}

	void Set(const idVec2 &value) {
		qglUniform2fv(paramLocation, 1, value.ToFloatPtr());
	}

private:
	int paramLocation;
};

class GLSLUniform_vec3 {
public:
	GLSLUniform_vec3(GLSLProgram *program, const char *uniformName)
			: paramLocation(program->GetUniformLocation(uniformName)) {}

	void Set(float v1, float v2, float v3) {
		qglUniform3f(paramLocation, v1, v2, v3);
	}

	void Set(const idVec3 &value) {
		qglUniform3fv(paramLocation, 1, value.ToFloatPtr());
	}

private:
	int paramLocation;
};

class GLSLUniform_vec4 {
public:
	GLSLUniform_vec4(GLSLProgram *program, const char *uniformName)
			: paramLocation(program->GetUniformLocation(uniformName)) {}

	void Set(float v1, float v2, float v3, float v4) {
		qglUniform4f(paramLocation, v1, v2, v3, v4);
	}

	void Set(const idVec4 &value) {
		qglUniform4fv(paramLocation, 1, value.ToFloatPtr());
	}

private:
	int paramLocation;
};

class GLSLUniform_mat4 {
public:
	GLSLUniform_mat4(GLSLProgram *program, const char *uniformName)
			: paramLocation(program->GetUniformLocation(uniformName)) {}

	void Set(const float *value) {
		qglUniformMatrix4fv(paramLocation, 1, GL_FALSE, value);
	}

private:
	int paramLocation;
};

#define DEFINE_UNIFORM(type, name) GLSLUniform_##type name = GLSLUniform_##type(program, "u_" #name)
#define UNIFORM_GROUP_DEF(type) explicit type(GLSLProgram *program) : GLSLUniformGroup(program) {}

#endif
