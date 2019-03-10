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
#include "tr_local.h"

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
	int instances;
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

struct cubeMapProgram_t : shaderProgram_t {
	GLint reflective, rgtc, viewOrigin, modelMatrix;
	virtual	void AfterLoad();
};

struct lightProgram_t : shaderProgram_t {
	GLint lightOrigin;
	GLint modelMatrix;
	virtual	void AfterLoad();
};

struct basicInteractionProgram_t : lightProgram_t {
	GLint lightProjectionFalloff, bumpMatrix, diffuseMatrix, specularMatrix;
	GLint colorModulate, colorAdd;

	virtual	void AfterLoad();
	virtual void UpdateUniforms( bool translucent ) {}
	virtual void UpdateUniforms( const drawInteraction_t *din );
};

struct shadowMapProgram_t : basicDepthProgram_t {
	GLint lightOrigin, lightRadius, modelMatrix;
	GLint lightCount, shadowRect, shadowTexelStep, lightFrustum; // multi-light stuff
	virtual	void AfterLoad();
	void RenderAllLights();
	void RenderAllLights( drawSurf_t *surf );
};

struct multiLightInteractionProgram_t : basicInteractionProgram_t {
	static const uint MAX_LIGHTS = 16;
	GLint lightCount, lightOrigin, lightColor, shadowRect, softShadowsRadius;
	GLint minLevel, gamma;
	virtual	void AfterLoad();
	virtual void Draw( const drawInteraction_t *din );
};

extern cubeMapProgram_t cubeMapShader;
extern oldStageProgram_t oldStageShader;
extern depthProgram_t depthShader;
extern fogProgram_t fogShader;
extern blendProgram_t blendShader;
extern lightProgram_t stencilShadowShader;
extern multiLightInteractionProgram_t multiLightShader;

extern idCVarBool r_useGLSL;
extern idCVarBool r_newFrob;

void AddPoissonDiskSamples( idList<idVec2> &pts, float dist );
void GeneratePoissonDiskSampling( idList<idVec2> &pts, int wantedCount );
float GetEffectiveLightRadius();

//=============================================================================
// Below goes the suggested new way of handling GLSL parameters.

#include "GLSLUniforms.h"


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

namespace Uniforms {
	//pack of uniforms defined in every shader program
	struct Global : public GLSLUniformGroup {
		UNIFORM_GROUP_DEF(Global);

		DEFINE_UNIFORM( mat4, projectionMatrix );
		DEFINE_UNIFORM( mat4, modelMatrix );
		DEFINE_UNIFORM( mat4, modelViewMatrix );

		//TODO: is space necessary as argument, or we can take backEnd->currentSpace ?
		void Set( const viewEntity_t *space );
	};

	//pack of uniforms defined in a shader attached to "new" stage of a material
	struct MaterialStage : public GLSLUniformGroup {
		UNIFORM_GROUP_DEF( MaterialStage );

		DEFINE_UNIFORM( vec4, scalePotToWindow );
		DEFINE_UNIFORM( vec4, scaleWindowToUnit );
		DEFINE_UNIFORM( vec4, scaleDepthCoords );
		DEFINE_UNIFORM( vec4, viewOriginGlobal );
		DEFINE_UNIFORM( vec4, viewOriginLocal );
		DEFINE_UNIFORM( vec4, modelMatrixRow0 );
		DEFINE_UNIFORM( vec4, modelMatrixRow1 );
		DEFINE_UNIFORM( vec4, modelMatrixRow2 );

		DEFINE_UNIFORM( vec4, localParam0 );
		DEFINE_UNIFORM( vec4, localParam1 );
		DEFINE_UNIFORM( vec4, localParam2 );
		DEFINE_UNIFORM( vec4, localParam3 );

		GLSLUniform_vec4 *localParams[4] = { &localParam0, &localParam1, &localParam2, &localParam3 };

		DEFINE_UNIFORM( sampler, texture0 );
		DEFINE_UNIFORM( sampler, texture1 );
		DEFINE_UNIFORM( sampler, texture2 );
		DEFINE_UNIFORM( sampler, texture3 );
		DEFINE_UNIFORM( sampler, texture4 );
		DEFINE_UNIFORM( sampler, texture5 );
		DEFINE_UNIFORM( sampler, texture6 );
		DEFINE_UNIFORM( sampler, texture7 );

		GLSLUniform_sampler *textures[8] = { &texture0, &texture1, &texture2, &texture3, &texture4, &texture5, &texture6, &texture7 };

		//note: also binds fragmentProgramImages to texture units
		void Set( const shaderStage_t *pStage, const drawSurf_t *surf );
	};
};

GLSLProgram* GLSL_LoadMaterialStageProgram(const char *name);
