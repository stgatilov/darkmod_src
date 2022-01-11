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
uniform ViewParamsBlock {
	uniform mat4 u_projectionMatrix;
};

#pragma tdm_define "MAX_SHADER_PARAMS"

struct PerDrawCallParams {
	mat4 modelMatrix;
	mat4 modelViewMatrix;
	vec4 bumpMatrix[2];
	vec4 diffuseMatrix[2];
	vec4 specularMatrix[2];
	mat4 lightProjectionFalloff;
	vec4 lightTextureMatrix[2];
	vec4 colorModulate;
	vec4 colorAdd;
	vec4 lightOrigin;
	vec4 viewOrigin;
	vec4 diffuseColor;
	vec4 specularColor;
	vec4 hasTextureDNS;
	vec4 ambientRimColor;
	int useBumpmapLightTogglingFix;
	float RGTC;
	vec2 padding_2;
	// bindless texture handles - if supported
	uvec2 normalTexture;
	uvec2 padding;
	uvec2 diffuseTexture;
	uvec2 specularTexture;
};

layout (std140) uniform PerDrawCallParamsBlock {
	PerDrawCallParams params[MAX_SHADER_PARAMS];
};

layout (std140) uniform ShadowSamplesBlock {
	vec2 u_softShadowsSamples[150];
};
