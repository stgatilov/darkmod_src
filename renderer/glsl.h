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
	void Use();									// 	qglUseProgram( 0 ) to reset, maybe consider RAII?
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

struct basicDepthProgram_t : shaderProgram_t {
	GLint alphaTest, color;
	bool acceptsTranslucent;
	virtual	void AfterLoad();
	void FillDepthBuffer( const drawSurf_t *surf );
};

struct depthProgram_t : basicDepthProgram_t {
	GLint clipPlane, matViewRev;
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
};

struct cubeMapProgram_t : shaderProgram_t {
	GLint reflective, rgtc, viewOrigin, modelMatrix;
	virtual	void AfterLoad();
};

extern cubeMapProgram_t cubeMapShader;
extern oldStageProgram_t oldStageShader;
extern depthProgram_t depthShader;
extern fogProgram_t fogShader;
extern blendProgram_t blendShader;
extern lightProgram_t stencilShadowShader;

extern idCVarBool r_useGLSL;
extern idCVarBool r_newFrob;
void AddPoissonDiskSamples( idList<idVec2> &pts, float dist );
void GeneratePoissonDiskSampling( idList<idVec2> &pts, int wantedCount );


//=============================================================================
// Below goes the suggested new way of handling GLSL parameters.

#include "glsl_def.h"
#include "GLSLProgram.h"


//pack of attributes used (almost) everywhere
namespace Attributes {
	namespace Default {
		enum Names {
			Position  = 0,
			Normal	  = 2,
			Color	  = 3,
			TexCoord  = 8,
			Tangent	  = 9,
			Bitangent = 10,
		};
		void Bind(GLSLProgram *program);
		//startOffset is byte offset of first idDrawVert in current VBO
		//arrayMask is a bitmask with attributes fetched from vertex array (arrays are disabled for unset attributes)
		void SetDrawVert(size_t startOffset, int arrayMask);
	}
};


//pack of uniforms defined in every shader program
#define UNIFORMS_GLOBAL(BEGIN, DEF, END) \
	BEGIN(Global, 10000) \
		DEF(projectionMatrix) \
		DEF(modelMatrix) \
		DEF(modelViewMatrix) \
		/*DEF(viewMatrix)*/ \
		/*DEF(modelViewProjectionMatrix)*/ \
	END()
UNIFORMS_GLOBAL(UNIFORMS_DECLARE_BEGIN, UNIFORMS_DECLARE_DEF, UNIFORMS_DECLARE_END)
namespace Uniforms {
	namespace Global {
		//TODO: is space necessary as argument, or we can take backEnd->currentSpace ?
		void Set(GLSLProgram *program, const viewEntity_t *space);
	}
}

//pack of uniforms defined in a shader attached to "new" stage of a material
#define UNIFORMS_MATERIALSTAGE(BEGIN, DEF, END) \
	BEGIN(MaterialStage, 100) \
		DEF(scalePotToWindow) \
		DEF(scaleWindowToUnit) \
		DEF(scaleDepthCoords) \
		DEF(viewOriginGlobal) \
		DEF(viewOriginLocal) \
		DEF(modelMatrixRow0) \
		DEF(modelMatrixRow1) \
		DEF(modelMatrixRow2) \
		\
		DEF(localParam0) \
		DEF(localParam1) \
		DEF(localParam2) \
		DEF(localParam3) \
		\
		DEF(texture0) \
		DEF(texture1) \
		DEF(texture2) \
		DEF(texture3) \
		DEF(texture4) \
		DEF(texture5) \
		DEF(texture6) \
		DEF(texture7) \
	END()
UNIFORMS_MATERIALSTAGE(UNIFORMS_DECLARE_BEGIN, UNIFORMS_DECLARE_DEF, UNIFORMS_DECLARE_END)
namespace Uniforms {
	namespace MaterialStage {
		//note: also binds fragmentProgramImages to texture units
		void Set(GLSLProgram *program, const shaderStage_t *pStage, const drawSurf_t *surf);
	}
}


struct globalPrograms_t {
	GLSLProgram *interaction;
	GLSLProgram *frob;
	//... all other static shaders here

	//this list includes all programs, even dynamically created ones
	idList<GLSLProgram*> allList;
};

extern globalPrograms_t globalPrograms;

void GLSL_InitPrograms();
void GLSL_DestroyPrograms();
void GLSL_ReloadPrograms();

GLSLProgram* GLSL_LoadMaterialStageProgram(const char *name);
