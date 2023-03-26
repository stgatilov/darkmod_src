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

// TODO: move bumpmap reading function to separate file
#pragma tdm_include "tdm_interaction.glsl"

uniform vec3 u_viewOriginLocal;
uniform mat4 u_modelMatrix;
uniform samplerCube u_environmentMap;
uniform sampler2D u_normalMap;
uniform bool u_RGTC;
uniform vec4 u_constant;
uniform vec4 u_fresnel;
uniform bool u_tonemapOutputColor;

in vec2 var_TexCoord;
in vec4 var_Color;
in vec3 var_PositionLocal;
in mat3 var_TangentToLocalMatrix;

out vec4 FragColor;

void main() {
	vec3 normalTangent = fetchSurfaceNormal(var_TexCoord, true, u_normalMap, u_RGTC);
	// transform the surface normal to model space
	vec3 normalLocal = normalize(var_TangentToLocalMatrix * normalTangent);

	// calculate reflection vector
	vec3 toEyeLocal = normalize(u_viewOriginLocal - var_PositionLocal);
	float dotEN = dot(toEyeLocal, normalLocal);
	vec3 reflectLocal = 2 * dotEN * normalLocal - toEyeLocal;
	// transform it to world space
	vec3 reflectWorld = mat3(u_modelMatrix) * reflectLocal;

	// read the environment map with the reflection vector
	vec4 reflectedColor = texture(u_environmentMap, reflectWorld);

	// calculate fresnel reflectance
	float q = 1 - dotEN;
	reflectedColor *= u_fresnel * (q * q * q * q) + u_constant;

	if (u_tonemapOutputColor) {
		// tonemap to convert HDR values to range 0.0 - 1.0
		reflectedColor.rgb = reflectedColor.rgb / (vec3(1.0) + reflectedColor.rgb);
	}
	FragColor = reflectedColor;
}
