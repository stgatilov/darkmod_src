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

#pragma tdm_define "BINDLESS_TEXTURES"

#ifdef BINDLESS_TEXTURES
#extension GL_ARB_bindless_texture : require
#endif

#pragma tdm_include "stages/interaction/interaction.params.glsl"
#pragma tdm_include "tdm_lightproject.glsl"

in vec3 var_Position;
in vec2 var_TexDiffuse;        
in vec2 var_TexSpecular;
in vec2 var_TexNormal;      
in vec4 var_TexLight;      
in mat3 var_TangentBinormalNormalMatrix;      
in vec4 var_Color;        
in vec3 var_tc0;  
in vec3 var_localViewDir;
in vec4 var_ClipPosition;
flat in int var_DrawId;

out vec4 FragColor;
	 
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

uniform sampler2D u_lightProjectionTexture;         
uniform samplerCube	u_lightProjectionCubemap;
uniform sampler2D u_lightFalloffTexture;         
uniform samplerCube	u_lightFalloffCubemap;
		 
uniform int u_cubic;
uniform float u_gamma, u_minLevel;
   
uniform sampler2D u_ssaoTexture;
uniform int u_ssaoEnabled;
float sampleSSAO() {
	return texture(u_ssaoTexture, 0.5 + 0.5 * var_ClipPosition.xy / var_ClipPosition.w).r;
}
   
void main() {         
	// compute the diffuse term     
	vec4 matDiffuse = textureDiffuse( var_TexDiffuse );
	vec3 matSpecular = textureSpecular( var_TexSpecular ).rgb;

	// compute normal from normal map, move from [0, 1] to [-1, 1] range, normalize    
	vec4 bumpTexel = textureNormal( var_TexNormal.st ) * 2. - 1.;
	vec3 localNormal = vec3(bumpTexel.x, bumpTexel.y, sqrt(max(1.-bumpTexel.x*bumpTexel.x-bumpTexel.y*bumpTexel.y, 0))); 
	vec3 N = var_TangentBinormalNormalMatrix * localNormal;
	//stgatilov: without normalization |N| > 1 is possible, which leads to |spec| > 1,
	//which causes white sparklies when antialiasing is enabled (http://forums.thedarkmod.com/topic/19134-aliasing-artefact-white-pixels-near-edges/)
	N = normalize(N);

	vec3 nViewDir = normalize(var_localViewDir);
	vec3 reflect = - (nViewDir - 2*N*dot(N, nViewDir));

	// compute lighting model     
	vec4 color = params[var_DrawId].diffuseColor * var_Color, light;
	if (u_cubic == 1) {
		//color.rgb = vec3(var_TexLight.z);
		vec3 tl = vec3(var_TexLight.xy/var_TexLight.w, var_TexLight.z) - .5;
		float a = .25 - tl.x*tl.x - tl.y*tl.y - tl.z*tl.z;
		light = vec4(vec3(a*2), 1); // FIXME pass r_lightScale as uniform
	} else {
		light.rgb = projFalloffOfNormalLight(u_lightProjectionTexture, u_lightFalloffTexture, params[var_DrawId].lightTextureMatrix, var_TexLight);
		light.a = 1;
	} 

	if (u_cubic == 1) {
		vec4 worldN = params[var_DrawId].modelMatrix * vec4(N, 0); // rotation only
		vec3 cubeTC = var_TexLight.xyz * 2.0 - 1.0;
		// diffuse
		vec4 light1 = texture(u_lightProjectionCubemap, worldN.xyz) * matDiffuse;
		// specualr
		light1.rgb += texture( u_lightFalloffCubemap, reflect, 2 ).rgb * matSpecular;
		light.rgb *= color.rgb * light1.rgb;
		light.a = light1.a;
	} else {
		vec3 light1 = vec3(.5); // directionless half
		light1 += max(dot(N, params[var_DrawId].lightOrigin.xyz) * (1. - matSpecular) * .5, 0);
		float spec = max(dot(reflect, params[var_DrawId].lightOrigin.xyz), 0);
		float specPow = clamp((spec*spec), 0.0, 1.1);
		light1 += vec3(spec*specPow*specPow) * matSpecular * 1.0;
		light.a = matDiffuse.a;

		light1.rgb *= color.rgb;
		if (u_minLevel != 0) // home-brewed "pretty" linear
			light1.rgb = light1.rgb * (1.0 - u_minLevel) + vec3(u_minLevel);
		light.rgb *= matDiffuse.rgb * light1;
	}

	light = max(light, vec4(0));  // avoid negative values, which with floating point render buffers can lead to NaN artefacts
	if(u_gamma != 1 ) // old-school exponential
		light.rgb = pow(light.rgb, vec3(1.0 / u_gamma));

	if(params[var_DrawId].ambientRimColor.a != 0) { // produces no visible speed difference on nVidia 1060, but maybe on some other hardware?..
		float NV = 1-abs(dot(N, nViewDir));
		NV *= NV;
		light.rgb += params[var_DrawId].ambientRimColor.rgb * NV * NV;
	}

	if (u_ssaoEnabled == 1) {
		light *= sampleSSAO();
	}

	FragColor = light;
}
