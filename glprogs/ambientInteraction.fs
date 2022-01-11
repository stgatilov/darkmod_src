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

out vec4 FragColor;
     
uniform sampler2D u_normalTexture;         
uniform sampler2D u_lightFalloffTexture;         
uniform sampler2D u_lightProjectionTexture;         
uniform sampler2D u_diffuseTexture;   
uniform sampler2D u_specularTexture;

uniform samplerCube	u_lightProjectionCubemap;
uniform samplerCube	u_lightFalloffCubemap;
         
uniform vec3 u_lightOrigin;   
uniform vec4 u_diffuseColor;    
uniform float u_cubic;
uniform float u_gamma, u_minLevel;
uniform mat4 u_modelMatrix;
uniform float u_RGTC;
uniform vec4 u_rimColor;   
uniform vec4 u_lightTextureMatrix[2];

uniform sampler2D u_ssaoTexture;
uniform int u_ssaoEnabled;
float sampleSSAO() {
	return texture(u_ssaoTexture, 0.5 + 0.5 * var_ClipPosition.xy / var_ClipPosition.w).r;
}

void main() {         
	// compute the diffuse term     
	vec4 matDiffuse = texture( u_diffuseTexture, var_TexDiffuse );
	vec3 matSpecular = texture( u_specularTexture, var_TexSpecular ).rgb;

	// compute normal from normal map, move from [0, 1] to [-1, 1] range, normalize    
	vec4 bumpTexel = texture ( u_normalTexture, var_TexNormal.st ) * 2. - 1.;
	vec3 localNormal = u_RGTC == 1. 
		? vec3(bumpTexel.x, bumpTexel.y, sqrt(max(1.-bumpTexel.x*bumpTexel.x-bumpTexel.y*bumpTexel.y, 0)))
		: normalize( bumpTexel.xyz ); 
	vec3 N = var_TangentBinormalNormalMatrix * localNormal;
	//stgatilov: without normalization |N| > 1 is possible, which leads to |spec| > 1,
	//which causes white sparklies when antialiasing is enabled (http://forums.thedarkmod.com/topic/19134-aliasing-artefact-white-pixels-near-edges/)
	N = normalize(N);

	vec3 nViewDir = normalize(var_localViewDir);
	vec3 reflect = - (nViewDir - 2*N*dot(N, nViewDir));

	// compute lighting model     
	vec4 color = u_diffuseColor * var_Color, light;
	if (u_cubic == 1.0) {
		//color.rgb = vec3(var_TexLight.z);
		vec3 tl = vec3(var_TexLight.xy/var_TexLight.w, var_TexLight.z) - .5;
		float a = .25 - tl.x*tl.x - tl.y*tl.y - tl.z*tl.z;
		light = vec4(vec3(a*2), 1); // FIXME pass r_lightScale as uniform
	} else {
		light.rgb = projFalloffOfNormalLight(u_lightProjectionTexture, u_lightFalloffTexture, u_lightTextureMatrix, var_TexLight);
		light.a = 1;
	} 

	if (u_cubic == 1.0) {
		vec4 worldN = u_modelMatrix * vec4(N, 0); // rotation only
		vec3 cubeTC = var_TexLight.xyz * 2.0 - 1.0;
		// diffuse
		vec4 light1 = texture(u_lightProjectionCubemap, worldN.xyz) * matDiffuse;
		// specualr
		light1.rgb += texture( u_lightFalloffCubemap, reflect, 2 ).rgb * matSpecular;
		light.rgb *= color.rgb * light1.rgb;
		light.a = light1.a;
	} else {
		vec3 light1 = vec3(.5); // directionless half
		light1 += max(dot(N, u_lightOrigin.xyz) * (1. - matSpecular) * .5, 0);
		float spec = max(dot(reflect, u_lightOrigin.xyz), 0);
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

	if(u_rimColor.a != 0) { // produces no visible speed difference on nVidia 1060, but maybe on some other hardware?..
		float NV = 1-abs(dot(N, nViewDir));
		NV *= NV;
		light.rgb += u_rimColor.rgb * NV * NV;
	}

    if (u_ssaoEnabled == 1) {
		light *= sampleSSAO();
	}
	FragColor = light;
}

// fresnel part, saved here for future revisit
/*	// load diffuse and specular maps
	vec4 R2 = texture2D(u_specularTexture, var_TexSpecular);

	// calculate specularity
	vec4 R1;
	R1.y = dot(toViewer, localNormal);
	R2.w = mix(lightParms.w, lightParms.z, R2.g);
	R1.x = pow(R1.y, R2.w);
	R1.x = R1.x * 0.65;

	// "diffuse" fresnel effect
	vec4 fresnelTerm;
	fresnelTerm.x = 1.0 - R1.y;
	fresnelTerm.x = pow(fresnelTerm.x, R2.w);
	R1.w = toViewer.z + toViewer.z;
	fresnelTerm.x = fresnelTerm.x * R1.w;
	light.xyz = fresnelTerm.x * 0.4 + light;

	// modulate diffuse and specular maps
	color.xyz = color * program.env[0];
	R2.xyz = R2 * program.env[1];
	diffuse *= light;*/
