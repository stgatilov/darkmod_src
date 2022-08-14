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
// Contains common formulas for computing interaction.
// Includes: illumination model, fetching surface and light properties
// Excludes: shadows

#pragma tdm_include "tdm_lightproject.glsl"
#pragma tdm_include "stages/interaction/interaction.params.glsl"
#pragma tdm_include "tdm_interaction.glsl"

flat in int var_DrawId;
in vec2 var_TexDiffuse;
in vec2 var_TexNormal;
in vec2 var_TexSpecular;
in vec4 var_TexLight;
in vec4 var_Color;
in mat3 var_TangentBitangentNormalMatrix; 
in vec3 var_LightDirLocal;
in vec3 var_ViewDirLocal;

uniform sampler2D u_normalTexture;
uniform sampler2D u_diffuseTexture;
uniform sampler2D u_specularTexture;

uniform sampler2D u_lightFalloffTexture;
uniform sampler2D u_lightProjectionTexture;
uniform samplerCube	u_lightProjectionCubemap;

uniform int 	u_cubic;

uniform bool	u_shadows;
uniform int		u_softShadowsQuality;
uniform float	u_softShadowsRadius;


vec3 computeInteraction(out InteractionGeometry props) {
	vec3 lightColor;
	if (u_cubic == 1.0)
		lightColor = projFalloffOfCubicLight(u_lightProjectionCubemap, var_TexLight);
	else
		lightColor = projFalloffOfNormalLight(u_lightProjectionTexture, u_lightFalloffTexture, params[var_DrawId].lightTextureMatrix, var_TexLight);

	vec3 localNormal = fetchSurfaceNormal(var_TexNormal, params[var_DrawId].hasTextureDNS[1] != 0.0, u_normalTexture, params[var_DrawId].RGTC != 0.0);
	props = computeInteractionGeometry(var_LightDirLocal, var_ViewDirLocal, localNormal);

	vec3 interactionColor = computeAdvancedInteraction(
		props,
		u_diffuseTexture, params[var_DrawId].diffuseColor.rgb, var_TexDiffuse,
		u_specularTexture, params[var_DrawId].specularColor.rgb, var_TexSpecular,
		var_Color.rgb,
		params[var_DrawId].useBumpmapLightTogglingFix != 0
	);

	return interactionColor * lightColor;
}
