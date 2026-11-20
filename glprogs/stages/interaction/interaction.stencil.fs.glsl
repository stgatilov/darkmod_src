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
#version 330 core
#extension GL_ARB_texture_query_lod: enable
#extension GL_ARB_texture_gather: enable

#define STGATILOV_USEGATHER 1

out vec4 FragColor;

#pragma tdm_include "tdm_utils.glsl"
#pragma tdm_include "stages/interaction/interaction.common.fs.glsl"
#pragma tdm_include "tdm_shadowstencilsoft.glsl"
#define TDM_allow_ARB_texture_gather STGATILOV_USEGATHER
#define SHADOWMAP_NO_FALL_ANGLE_LIMITER 1
#pragma tdm_include "tdm_shadowmaps.glsl"

uniform vec4 u_shadowRect;
uniform sampler2D u_shadowMap;
uniform int u_alphaSoftShadowsQuality;

in vec3 var_LightDirWorld;

uniform usampler2D u_stencilTexture;
uniform sampler2D u_depthTexture;

uniform sampler2D u_stencilMipmapsTexture;
uniform ivec2 u_stencilMipmapsLevel;
uniform vec4 u_stencilMipmapsScissor;

void main() {
	InteractionGeometry props;
	vec3 worldParallax;
	FragColor.rgb = computeInteraction(props, worldParallax);

	vec3 objectToLight = var_TangentBitangentNormalMatrix * var_LightDirLocal;
	vec3 objectNormal = var_TangentBitangentNormalMatrix[2];

	if (checkFlag(u_flags, SFL_INTERACTION_SHADOWS) && u_softShadowsQuality > 0) {
		FragColor.rgb *= computeStencilSoftShadow(
			u_stencilTexture, u_depthTexture,
			objectToLight, objectNormal,
			u_modelViewMatrix, u_projectionMatrix,
			u_softShadowsQuality, u_softShadowsRadius,
			u_stencilMipmapsTexture, u_stencilMipmapsLevel, u_stencilMipmapsScissor
		);
	}
	// alpha-tested casters (foliage, grates) are not in stencil: they are in a small shadow map
	// (skipped if the pixel is already unlit: facing away, or in the stencil shadow)
	if (checkFlag(u_flags, SFL_INTERACTION_SHADOWS) && checkFlag(u_flags, SFL_INTERACTION_ALPHA_SHADOW) && any(greaterThan(FragColor.rgb, vec3(0.0)))) {
		vec3 worldNormal = mat3(u_modelMatrix) * var_TangentBitangentNormalMatrix[2];
		if (props.localN.z < 0.0)	// #5862: reverse-sided material with Z < 0 normalmap
			worldNormal = -worldNormal;
		// thin two-sided casters (grates) are lit from both sides: the normal must face the light,
		// otherwise the shadow map code treats the back side as unlit (fallAngleLimiter = 0)
		if (dot(worldNormal, var_LightDirWorld) < 0.0)
			worldNormal = -worldNormal;
		float alphaCoeff = computeShadowMapCoefficient(
			-var_LightDirWorld, worldNormal,
			u_shadowMap, u_shadowRect,
			u_alphaSoftShadowsQuality, u_softShadowsRadius,	// PCSS (contact hardening) of the shadow map code, independent of stencil softness
			false	// alpha page is rendered two-sided, no cull-front hack
		);
		FragColor.rgb *= alphaCoeff;
	}
	FragColor.a = 1.0;
}
