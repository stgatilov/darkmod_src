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

#pragma once

#include "qgl.h"

struct shaderProgram_t {
	GLuint program;								// GPU program = vertex + fragment shader
	bool Load( const char *fileName );
	virtual void AfterLoad();
	virtual void Use();							// 	qglUseProgram( 0 ) to reset, maybe consider RAII?
private:
	void AttachShader( GLint ShaderType, const char *fileName );
	GLuint CompileShader( GLint ShaderType, const char *fileName );
};

struct oldStageProgram_t : shaderProgram_t {
	GLint			screenTex;
	GLint			colorMul;
	GLint			colorAdd;
	virtual	void AfterLoad();
};

struct depthProgram_t : shaderProgram_t {
	GLint			clipPlane, matViewRev;
	GLint			color;
	GLint			alphaTest;
	virtual	void AfterLoad();
};

struct blendProgram_t : shaderProgram_t {
	GLint			tex0PlaneS;
	GLint			tex0PlaneT;
	GLint			tex0PlaneQ;
	GLint			tex1PlaneS;
	GLint			texture1;
	GLint			blendColor;
	virtual	void AfterLoad();
};

struct fogProgram_t : shaderProgram_t {
	GLint			tex0PlaneS;
	GLint			tex1PlaneT;
	GLint			fogEnter;
	GLint			texture1;
	GLint			fogColor;
	virtual	void AfterLoad();
};

struct lightProgram_t : shaderProgram_t {
	GLint lightOrigin;
	GLint modelMatrix;
	virtual	void AfterLoad();
	virtual void UpdateUniforms( bool translucent ) {}
	virtual void UpdateUniforms( const drawInteraction_t *din ) {}
};

extern shaderProgram_t cubeMapShader;
extern oldStageProgram_t oldStageShader;
extern depthProgram_t depthShader;
extern fogProgram_t fogShader;
extern blendProgram_t blendShader;
extern lightProgram_t stencilShadowShader;

extern idCVar r_ambient_testadd;
extern idCVar r_useGLSL;
extern idCVar r_useMultiDraw;

void AddPoissonDiskSamples( idList<idVec2> &pts, float dist );
void GeneratePoissonDiskSampling( idList<idVec2> &pts, int wantedCount );
