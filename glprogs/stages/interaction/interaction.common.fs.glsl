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

#pragma tdm_define "BINDLESS_TEXTURES"
#pragma tdm_include "tdm_lightproject.glsl"

#ifdef BINDLESS_TEXTURES
#extension GL_ARB_bindless_texture : require
#endif

in vec3 var_Position;
in vec4 var_Color;
in vec2 var_TexDiffuse;
in vec2 var_TexNormal;
in vec2 var_TexSpecular;
in vec4 var_TexLight;
flat in int var_DrawId;

#pragma tdm_include "stages/interaction/interaction.params.glsl"

#ifdef BINDLESS_TEXTURES
vec4 textureNormal(vec2 uv) {
	sampler2D normalTexture = sampler2D(params[var_DrawId].normalTexture);
	return texture(normalTexture, uv);
}

vec4 textureDiffuse(vec2 uv) {
	sampler2D diffuseTexture = sampler2D(params[var_DrawId].diffuseTexture);
	return texture(diffuseTexture, uv);
}

vec4 textureSpecular(vec2 uv) {
	sampler2D specularTexture = sampler2D(params[var_DrawId].specularTexture);
	return texture(specularTexture, uv);
}
#else
uniform sampler2D u_normalTexture;
uniform sampler2D u_diffuseTexture;
uniform sampler2D u_specularTexture;

vec4 textureNormal(vec2 uv) {
	return texture(u_normalTexture, uv);
}

vec4 textureDiffuse(vec2 uv) {
	return texture(u_diffuseTexture, uv);
}

vec4 textureSpecular(vec2 uv) {
	return texture(u_specularTexture, uv);
}
#endif

uniform sampler2D u_lightFalloffTexture;
uniform sampler2D u_lightProjectionTexture;
uniform samplerCube	u_lightProjectionCubemap;

uniform int	    u_advanced;
uniform int 	u_cubic;

uniform bool	u_shadows;
uniform int		u_softShadowsQuality;
uniform float	u_softShadowsRadius;

// output of fetchDNS
vec3 RawN, N;

// common variables
vec3 lightDir, viewDir;     //direction to light/eye in model coords
vec3 L, V, H;               //normalized light, view and half angle vectors 
float NdotH, NdotL, NdotV;

in mat3 var_TangentBitangentNormalMatrix; 
in vec3 var_LightDirLocal;  
in vec3 var_ViewDirLocal;

void calcNormals() {
	// compute normal from normal map, move from [0, 1] to [-1, 1] range, normalize 
	if (params[var_DrawId].hasTextureDNS[1] != 0) {
		vec4 bumpTexel = textureNormal( var_TexNormal.st ) * 2. - 1.;
		RawN = params[var_DrawId].RGTC == 1. 
			? vec3(bumpTexel.x, bumpTexel.y, sqrt(max(1.-bumpTexel.x*bumpTexel.x-bumpTexel.y*bumpTexel.y, 0)))
			: normalize( bumpTexel.xyz ); 
		N = var_TangentBitangentNormalMatrix * RawN; 
	}
	else {
		RawN = vec3(0, 0, 1);
		N = var_TangentBitangentNormalMatrix[2];
	}
}
//fetch surface normal at fragment
void fetchDNS() {
	//initialize common variables (TODO: move somewhere else?)
	lightDir = params[var_DrawId].lightOrigin.xyz - var_Position;
	viewDir = params[var_DrawId].viewOrigin.xyz - var_Position;
	L = normalize(lightDir);
	V = normalize(viewDir);
	H = normalize(L + V);
	calcNormals();
	NdotH = clamp(dot(N, H), 0.0, 1.0);
	NdotL = clamp(dot(N, L), 0.0, 1.0);
	NdotV = clamp(dot(N, V), 0.0, 1.0);
}

//fetch color of the light source (light projection and falloff)
vec3 lightColor() {
	if (u_cubic == 1.0)
		return projFalloffOfCubicLight(u_lightProjectionCubemap, var_TexLight);
	else
		return projFalloffOfNormalLight(u_lightProjectionTexture, u_lightFalloffTexture, params[var_DrawId].lightTextureMatrix, var_TexLight);
}

//illumination model with "simple interaction" setting
vec3 simpleInteraction() {
	// compute the diffuse term    
	vec3 diffuse = textureDiffuse(var_TexDiffuse).rgb * params[var_DrawId].diffuseColor.rgb;

	// compute the specular term
	float specularPower = 10.0;
	float specularContribution = pow(NdotH, specularPower);
	vec3 specular = textureSpecular(var_TexSpecular).rgb * specularContribution * params[var_DrawId].specularColor.rgb;

	// compute lighting model
	vec3 finalColor = (diffuse + specular) * NdotL * lightColor() * var_Color.rgb;

	return finalColor;
}

//illumination model with "enhanced interaction" setting
vec3 advancedInteraction() {
	vec4 fresnelParms = vec4(1.0, .23, .5, 1.0);
	vec4 fresnelParms2 = vec4(.2, .023, 120.0, 4.0);
	vec4 lightParms = vec4(.7, 1.8, 10.0, 30.0);

	vec3 diffuse = textureDiffuse(var_TexDiffuse).rgb;

	vec3 specular = vec3(0.026);	//default value if texture not set?...
	if (dot(params[var_DrawId].specularColor, params[var_DrawId].specularColor) > 0.0)
		specular = textureSpecular(var_TexSpecular).rgb;

	vec3 localL = normalize(var_LightDirLocal);
	vec3 localV = normalize(var_ViewDirLocal);
	//must be done in tangent space, otherwise smoothing will suffer (see #4958)
	float NdotL = clamp(dot(RawN, localL), 0.0, 1.0);
	float NdotV = clamp(dot(RawN, localV), 0.0, 1.0);
	float NdotH = clamp(dot(RawN, normalize(localV + localL)), 0.0, 1.0);

	// fresnel part, ported from test_direct.vfp
	float fresnelTerm = pow(1.0 - NdotV, fresnelParms2.w);
	float rimLight = fresnelTerm * clamp(NdotL - 0.3, 0.0, fresnelParms.z) * lightParms.y;
	float specularPower = mix(lightParms.z, lightParms.w, specular.z);
	float specularCoeff = pow(NdotH, specularPower) * fresnelParms2.z;
	float fresnelCoeff = fresnelTerm * fresnelParms.y + fresnelParms2.y;

	vec3 specularColor = specularCoeff * fresnelCoeff * specular * (diffuse * 0.25 + vec3(0.75));
	float R2f = clamp(localL.z * 4.0, 0.0, 1.0);

	float NdotL_adjusted = NdotL;
	if (params[var_DrawId].useBumpmapLightTogglingFix != 0) {
		//stgatilov: hacky coefficient to make lighting smooth when L is almost in surface tangent plane
		vec3 meshNormal = normalize(var_TangentBitangentNormalMatrix[2]);
		float MNdotL = max(dot(meshNormal, L), 0);
		if (MNdotL < min(0.25, NdotL))
			NdotL_adjusted = mix(MNdotL, NdotL, MNdotL / 0.25);
	}
	float light = rimLight * R2f + NdotL_adjusted;

	vec3 totalColor = (specularColor * params[var_DrawId].specularColor.rgb * R2f + diffuse * params[var_DrawId].diffuseColor.rgb) * light * lightColor() * var_Color.rgb;

	return totalColor;
}

vec3 computeInteraction() {
	vec3 res;
	if (u_advanced == 1)
		res = advancedInteraction();
	else
		res = simpleInteraction();
	return res;
}
