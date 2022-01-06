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

extern idCVar r_newFrob;
extern idCVar r_frobOutline;
extern idCVarBool r_shadowMapCullFront;

void AddPoissonDiskSamples( idList<idVec2> &pts, float dist );
void GeneratePoissonDiskSampling( idList<idVec2> &pts, int wantedCount );
float GetEffectiveLightRadius();
void RB_SingleSurfaceToDepthBuffer( GLSLProgram *program, const drawSurf_t *surf );

GLSLProgram *R_FindGLSLProgram( const char *name );

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
			DrawId    = 15,
		};
		void Bind(GLSLProgram *program);
		//startOffset is byte offset of first idDrawVert in current VBO
		//arrayMask is a bitmask with attributes fetched from vertex array (arrays are disabled for unset attributes)
		//void SetDrawVert(size_t startOffset, int arrayMask);
	}
};

namespace Uniforms {
	//pack of uniforms defined in every shader program
	struct Global : public GLSLUniformGroup {
		UNIFORM_GROUP_DEF(Global)

		//DEFINE_UNIFORM( mat4, projectionMatrix )
		DEFINE_UNIFORM( mat4, modelMatrix )
		DEFINE_UNIFORM( mat4, modelViewMatrix )
		DEFINE_UNIFORM( vec4, viewOriginLocal )
		DEFINE_UNIFORM( mat4, textureMatrix )

		//TODO: is space necessary as argument, or we can take backEnd->currentSpace ?
		void Set( const viewEntity_t *space );
	};

	struct Depth: GLSLUniformGroup {
		UNIFORM_GROUP_DEF( Depth)

		DEFINE_UNIFORM( float, alphaTest )
		DEFINE_UNIFORM( vec4, clipPlane )
		DEFINE_UNIFORM( mat4, matViewRev )
		DEFINE_UNIFORM( vec4, color )

		int instances = 0;
		bool acceptsTranslucent = false;
	};

	struct SoftParticle : GLSLUniformGroup {
		UNIFORM_GROUP_DEF( SoftParticle )

		DEFINE_UNIFORM( sampler, texture0 )
		DEFINE_UNIFORM( sampler, texture1 )
		DEFINE_UNIFORM( vec4, softParticleBlend )
		DEFINE_UNIFORM( vec4, softParticleParams )

		int instances = 0;
		bool acceptsTranslucent = false;
	};

	struct Interaction: GLSLUniformGroup {
		UNIFORM_GROUP_DEF( Interaction )

		DEFINE_UNIFORM( mat4, modelMatrix )
		DEFINE_UNIFORM( vec4, diffuseMatrix )
		DEFINE_UNIFORM( vec4, bumpMatrix )
		DEFINE_UNIFORM( vec4, specularMatrix )
		DEFINE_UNIFORM( vec4, colorModulate )
		DEFINE_UNIFORM( vec4, colorAdd )

		DEFINE_UNIFORM( mat4, lightProjectionFalloff )
		DEFINE_UNIFORM( vec4, lightTextureMatrix )
		DEFINE_UNIFORM( vec4, diffuseColor )
		DEFINE_UNIFORM( vec4, specularColor )
		DEFINE_UNIFORM( float, cubic )
		DEFINE_UNIFORM( sampler, normalTexture )
		DEFINE_UNIFORM( sampler, diffuseTexture )
		DEFINE_UNIFORM( sampler, specularTexture )
		DEFINE_UNIFORM( sampler, lightProjectionTexture )
		DEFINE_UNIFORM( sampler, lightProjectionCubemap )
		DEFINE_UNIFORM( sampler, lightFalloffTexture )
		DEFINE_UNIFORM( sampler, lightFalloffCubemap )
		DEFINE_UNIFORM( vec4, viewOrigin )

		DEFINE_UNIFORM( vec3, lightOrigin )
		DEFINE_UNIFORM( vec3, lightOrigin2 )

		DEFINE_UNIFORM( float, minLevel )
		DEFINE_UNIFORM( float, gamma )
		DEFINE_UNIFORM( vec4, rimColor )

		DEFINE_UNIFORM( float, advanced )
		DEFINE_UNIFORM( int, shadows )
		DEFINE_UNIFORM( int, shadowMapCullFront )
		DEFINE_UNIFORM( vec4, shadowRect )
		DEFINE_UNIFORM( int, softShadowsQuality )
		DEFINE_UNIFORM( vec2, softShadowsSamples )
		DEFINE_UNIFORM( float, softShadowsRadius )
		DEFINE_UNIFORM( int, shadowMap )
		DEFINE_UNIFORM( sampler, depthTexture )
		DEFINE_UNIFORM( sampler, stencilTexture )
		DEFINE_UNIFORM( vec2, renderResolution )

		DEFINE_UNIFORM( float, RGTC )
		DEFINE_UNIFORM( vec3, hasTextureDNS )
		DEFINE_UNIFORM( int, useBumpmapLightTogglingFix )

		DEFINE_UNIFORM( int, lightCount )
		DEFINE_UNIFORM( vec3, lightColor )

		DEFINE_UNIFORM( sampler, ssaoTexture )
		DEFINE_UNIFORM( int, ssaoEnabled )

		// temp
		DEFINE_UNIFORM( int, shadowMapHistory )
		DEFINE_UNIFORM( int, frameCount )
		DEFINE_UNIFORM( vec3, lightSamples )


		bool ambient = false;

		idList<idVec2> poissonSamples;

		void SetForInteractionBasic( const drawInteraction_t *din );
		void SetForInteraction( const drawInteraction_t *din );
		void SetForShadows( bool translucent );
	};

	//pack of uniforms defined in a shader attached to "new" stage of a material
	struct MaterialStage : public GLSLUniformGroup {
		UNIFORM_GROUP_DEF( MaterialStage )

		DEFINE_UNIFORM( vec4, scalePotToWindow )
		DEFINE_UNIFORM( vec4, scaleWindowToUnit )
		DEFINE_UNIFORM( vec4, scaleDepthCoords )
		DEFINE_UNIFORM( vec4, viewOriginGlobal )
		DEFINE_UNIFORM( vec4, viewOriginLocal )
		DEFINE_UNIFORM( vec4, modelMatrixRow0 )
		DEFINE_UNIFORM( vec4, modelMatrixRow1 )
		DEFINE_UNIFORM( vec4, modelMatrixRow2 )

		DEFINE_UNIFORM( vec4, localParam0 )
		DEFINE_UNIFORM( vec4, localParam1 )
		DEFINE_UNIFORM( vec4, localParam2 )
		DEFINE_UNIFORM( vec4, localParam3 )

		GLSLUniform_vec4 *localParams[4] = { &localParam0, &localParam1, &localParam2, &localParam3 };

		DEFINE_UNIFORM( sampler, texture0 )
		DEFINE_UNIFORM( sampler, texture1 )
		DEFINE_UNIFORM( sampler, texture2 )
		DEFINE_UNIFORM( sampler, texture3 )
		DEFINE_UNIFORM( sampler, texture4 )
		DEFINE_UNIFORM( sampler, texture5 )
		DEFINE_UNIFORM( sampler, texture6 )
		DEFINE_UNIFORM( sampler, texture7 )

		GLSLUniform_sampler *textures[8] = { &texture0, &texture1, &texture2, &texture3, &texture4, &texture5, &texture6, &texture7 };

		//note: also binds fragmentProgramImages to texture units
		void Set( const shaderStage_t *pStage, const drawSurf_t *surf );
	};
};

struct OldStageUniforms : GLSLUniformGroup {
	UNIFORM_GROUP_DEF( OldStageUniforms );

	DEFINE_UNIFORM( float, screenTex );
	DEFINE_UNIFORM( vec4, colorMul );
	DEFINE_UNIFORM( vec4, colorAdd );
};

GLSLProgram* GLSL_LoadMaterialStageProgram(const char *name);
