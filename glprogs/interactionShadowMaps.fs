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
#version 140
#extension GL_ARB_texture_gather: enable

#define STGATILOV_OCCLUDER_SEARCH 1
#define STGATILOV_USEGATHER 1

#pragma tdm_include "interactionCommon.fs"
#define TDM_allow_ARB_texture_gather STGATILOV_USEGATHER
#pragma tdm_include "tdm_shadowmaps.glsl"


uniform bool 	u_shadows;
uniform bool 	u_shadowMapCullFront;
uniform int		u_softShadowsQuality;
uniform float	u_softShadowsRadius;
uniform vec4	u_shadowRect;
uniform mat4	u_modelMatrix;
uniform sampler2D u_shadowMap;
in vec3 var_WorldLightDir;

out vec4 fragColor;


void main() {
	InteractionGeometry props;
	fragColor.rgb = computeInteraction(props);

	vec3 worldNormal = mat3(u_modelMatrix) * (var_TangentBitangentNormalMatrix * props.localN);

	if (u_shadows) {
		float shadowsCoeff = computeShadowMapCoefficient(
			var_WorldLightDir, worldNormal,
			u_shadowMap, u_shadowRect,
			u_softShadowsQuality, u_softShadowsRadius, u_shadowMapCullFront
		);
		fragColor.rgb *= shadowsCoeff;
	}
	fragColor.a = 1.0;
}
