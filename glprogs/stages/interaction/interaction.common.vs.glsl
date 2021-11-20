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

in vec4 attr_Position;
in vec4 attr_TexCoord;
in vec3 attr_Tangent;
in vec3 attr_Bitangent;
in vec3 attr_Normal;
in vec4 attr_Color;
in int attr_DrawId;

#pragma tdm_include "stages/interaction/interaction.params.glsl"

out vec3 var_Position;
out vec2 var_TexDiffuse;
out vec2 var_TexNormal;
out vec2 var_TexSpecular;
out vec4 var_TexLight;
out vec4 var_Color;
out mat3 var_TangentBitangentNormalMatrix; 
out vec3 var_LightDirLocal;  
out vec3 var_ViewDirLocal;  
flat out int var_DrawId;

void sendTBN() {
	// construct tangent-bitangent-normal 3x3 matrix   
	var_TangentBitangentNormalMatrix = mat3( clamp(attr_Tangent,-1,1), clamp(attr_Bitangent,-1,1), clamp(attr_Normal,-1,1) );
	var_LightDirLocal = (params[attr_DrawId].lightOrigin.xyz - var_Position).xyz * var_TangentBitangentNormalMatrix;
	var_ViewDirLocal = (params[attr_DrawId].viewOrigin.xyz - var_Position).xyz * var_TangentBitangentNormalMatrix;
}

uniform vec3 u_globalLightOrigin;
out vec3 var_WorldLightDir;


void interactionProcessVertex() {
	// transform vertex position into homogenous clip-space
	gl_Position = u_projectionMatrix * (params[attr_DrawId].modelViewMatrix * attr_Position);

	var_Position = attr_Position.xyz;

	// normal map texgen
	var_TexNormal.x = dot(attr_TexCoord, params[attr_DrawId].bumpMatrix[0]);
	var_TexNormal.y = dot(attr_TexCoord, params[attr_DrawId].bumpMatrix[1]);

	// diffuse map texgen
	var_TexDiffuse.x = dot(attr_TexCoord, params[attr_DrawId].diffuseMatrix[0]);
	var_TexDiffuse.y = dot(attr_TexCoord, params[attr_DrawId].diffuseMatrix[1]);

	// specular map texgen
	var_TexSpecular.x = dot(attr_TexCoord, params[attr_DrawId].specularMatrix[0]);
	var_TexSpecular.y = dot(attr_TexCoord, params[attr_DrawId].specularMatrix[1]);

	// light projection texgen
	var_TexLight = computeLightTex(params[attr_DrawId].lightProjectionFalloff, attr_Position);

	// construct tangent-bitangent-normal 3x3 matrix
	sendTBN();

	// primary color
	var_Color = (attr_Color * params[attr_DrawId].colorModulate) + params[attr_DrawId].colorAdd;

	// light->fragment vector in world coordinates
	var_WorldLightDir = (params[attr_DrawId].modelMatrix * attr_Position).xyz - u_globalLightOrigin;
	var_DrawId = attr_DrawId;
}
