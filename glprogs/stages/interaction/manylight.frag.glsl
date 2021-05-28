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
#version 400 core
#extension GL_ARB_texture_gather: enable

#pragma tdm_define "BINDLESS_TEXTURES"

#ifdef BINDLESS_TEXTURES
#extension GL_ARB_bindless_texture : require
#endif

in vec3 var_WorldPosition;
in vec4 var_ClipPosition;
in vec4 var_Color;
in vec2 var_TexDiffuse;
in vec2 var_TexNormal;
in vec2 var_TexSpecular;
flat in int var_DrawId;

#pragma tdm_include "stages/interaction/manylight.params.glsl"

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

vec4 lightFalloff2D(int light, vec2 uv) {
    sampler2D falloffTexture = sampler2D(lights[light].falloffTexture);
    return texture(falloffTexture, uv);
}

vec4 lightProjection2D(int light, vec3 proj) {
    sampler2D projectionTexture = sampler2D(lights[light].projectionTexture);
    return textureProj(projectionTexture, proj);
}

vec4 lightFalloffCube(int light, vec3 uv) {
    samplerCube falloffTexture = samplerCube(lights[light].falloffTexture);
    return texture(falloffTexture, uv, 2);
}

vec4 lightProjectionCube(int light, vec3 proj) {
    samplerCube projectionTexture = samplerCube(lights[light].projectionTexture);
    return texture(projectionTexture, proj);
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

uniform sampler2D u_lightFalloffTexture[MAX_LIGHTS];
uniform samplerCube	u_lightFalloffCubemap[MAX_LIGHTS];
uniform sampler2D u_lightProjectionTexture[MAX_LIGHTS];
uniform samplerCube	u_lightProjectionCubemap[MAX_LIGHTS];

vec4 lightFalloff2D(int light, vec2 uv) {
    return texture(u_lightFalloffTexture[light], uv);
}

vec4 lightProjection2D(int light, vec3 proj) {
    return textureProj(u_lightProjectionTexture[light], proj);
}

vec4 lightFalloffCube(int light, vec3 uv) {
    return texture(u_lightFalloffCubemap[light], uv, 2);
}

vec4 lightProjectionCube(int light, vec3 proj) {
    return texture(u_lightProjectionCubemap[light], proj);
}

#endif


uniform int		u_useBumpmapLightTogglingFix;  //stgatilov #4825


uniform vec3 u_globalViewOrigin;

// output of fetchDNS
vec3 RawN, N;

// common variables
vec3 lightDir, viewDir;     //direction to light/eye in world coords
vec3 L, V, H;               //normalized light, view and half angle vectors 
float NdotH, NdotL, NdotV;
vec3 diffuse, specular;

in mat3 var_TangentBitangentNormalMatrix; 

void calcNormals() {
	// compute normal from normal map, move from [0, 1] to [-1, 1] range, normalize 
	if (params[var_DrawId].hasTextureDNS[1] != 0) {
		vec4 bumpTexel = textureNormal( var_TexNormal.st ) * 2. - 1.;
		RawN = vec3(bumpTexel.x, bumpTexel.y, sqrt(max(1.-bumpTexel.x*bumpTexel.x-bumpTexel.y*bumpTexel.y, 0))); 
		N = var_TangentBitangentNormalMatrix * RawN; 
	}
	else {
		RawN = vec3(0, 0, 1);
		N = var_TangentBitangentNormalMatrix[2];
	}
    N = normalize(N);
}
//fetch surface normal at fragment
void fetchDNS() {
	//initialize common variables (TODO: move somewhere else?)
	viewDir = u_globalViewOrigin - var_WorldPosition;
	V = normalize(viewDir);
	calcNormals();
	NdotV = clamp(dot(N, V), 0.0, 1.0);
}

uniform float u_gamma, u_minLevel;
   
uniform sampler2D u_ssaoTexture;
uniform int u_ssaoEnabled;
float sampleSSAO() {
	return texture(u_ssaoTexture, 0.5 + 0.5 * var_ClipPosition.xy / var_ClipPosition.w).r;
}
   
vec3 ambientLight(int lightNum) {
    vec3 up = vec3(0, 0, 1);
    vec3 reflectDir = - reflect(V, N);
    vec4 texLight = (vec4(var_WorldPosition, 1) * lights[lightNum].projection).xywz;

	// compute lighting model     
	vec4 color = params[var_DrawId].diffuseColor * var_Color * lights[lightNum].color, light;
	if (lights[lightNum].cubic == 1) {
		vec3 tl = vec3(texLight.xy/texLight.w, texLight.z) - .5;
		float a = .25 - tl.x*tl.x - tl.y*tl.y - tl.z*tl.z;
		light = vec4(vec3(a*2), 1);

		vec3 cubeTC = texLight.xyz * 2.0 - 1.0;
		// diffuse
		vec3 light1 = lightProjectionCube(lightNum, N).rgb * diffuse;
		// specualr
		light1.rgb += lightFalloffCube(lightNum, reflectDir).rgb * specular;
		light.rgb *= color.rgb * light1.rgb;
	} else {
		vec3 lightProjection = lightProjection2D( lightNum, texLight.xyw ).rgb; 
		vec3 lightFalloff = lightFalloff2D( lightNum, vec2( texLight.z, 0.5 ) ).rgb;
		light = vec4(lightProjection * lightFalloff, 1);
		vec3 light1 = vec3(.5); // directionless half
		light1 += max(dot(N, up) * (1. - specular) * .5, 0);
		float spec = max(dot(reflectDir, up), 0);
		float specPow = clamp((spec*spec), 0.0, 1.1);
		light1 += vec3(spec*specPow*specPow) * specular * 1.0;

		light1.rgb *= color.rgb;
		if (u_minLevel != 0) // home-brewed "pretty" linear
			light1.rgb = light1.rgb * (1.0 - u_minLevel) + vec3(u_minLevel);
		light.rgb *= diffuse.rgb * light1;
	} 

	light = max(light, vec4(0));  // avoid negative values, which with floating point render buffers can lead to NaN artefacts
	if(u_gamma != 1 ) // old-school exponential
		light.rgb = pow(light.rgb, vec3(1.0 / u_gamma));

	if(params[var_DrawId].ambientRimColor.a != 0) { // produces no visible speed difference on nVidia 1060, but maybe on some other hardware?..
		float NV = 1-abs(dot(N, u_globalViewOrigin));
		NV *= NV;
		light.rgb += params[var_DrawId].ambientRimColor.rgb * NV * NV;
	}

	if (u_ssaoEnabled == 1) {
		light *= sampleSSAO();
	}

	return light.rgb;
}

//fetch color of the light source
vec3 lightColor(int lightNum) {
    // compute light projection and falloff
    vec4 texLight = (vec4(var_WorldPosition, 1) * lights[lightNum].projection).xywz;

    vec3 lightColor;
    if (lights[lightNum].cubic == 1) {
        vec3 cubeTC = texLight.xyz * 2.0 - 1.0;
        lightColor = lightProjectionCube(lightNum, cubeTC).rgb;
        float att = clamp(1.0 - length(cubeTC), 0.0, 1.0);
        lightColor *= att * att;
    }
    else {
        vec3 lightProjection = lightProjection2D(lightNum, texLight.xyw).rgb;
        vec3 lightFalloff = lightFalloff2D(lightNum, vec2(texLight.z, 0.5)).rgb;
        lightColor = lightProjection * lightFalloff;
    }
    return lightColor * lights[lightNum].color.rgb;
}

#pragma tdm_include "stages/interaction/manylight.shadowmap.glsl"

vec4 fresnelParms = vec4(1.0, .23, .5, 1.0);
vec4 fresnelParms2 = vec4(.2, .023, 120.0, 4.0);
vec4 lightParms = vec4(.7, 1.8, 10.0, 30.0);

vec3 directLight(int i) {
    L = normalize(lights[i].origin.xyz - var_WorldPosition);
    H = normalize(L + V);
    NdotL = clamp(dot(N, L), 0.0, 1.0);
    NdotH = clamp(dot(N, H), 0.0, 1.0);

    // fresnel part, ported from test_direct.vfp
    float fresnelTerm = pow(1.0 - NdotV, fresnelParms2.w);
    float rimLight = fresnelTerm * clamp(NdotL - 0.3, 0.0, fresnelParms.z) * lightParms.y;
    float specularPower = mix(lightParms.z, lightParms.w, specular.z);
    float specularCoeff = pow(NdotH, specularPower) * fresnelParms2.z;
    float fresnelCoeff = fresnelTerm * fresnelParms.y + fresnelParms2.y;
    
    vec3 specularColor = specularCoeff * fresnelCoeff * specular * (diffuse * 0.25 + vec3(0.75));
    float R2f = clamp(dot(L, var_TangentBitangentNormalMatrix[2]) * 4.0, 0.0, 1.0);

    float NdotL_adjusted = NdotL;
    if (u_useBumpmapLightTogglingFix != 0) {
        //stgatilov: hacky coefficient to make lighting smooth when L is almost in surface tangent plane
        vec3 meshNormal = normalize(var_TangentBitangentNormalMatrix[2]);
        float MNdotL = max(dot(meshNormal, L), 0);
        if (MNdotL < min(0.25, NdotL))
            NdotL_adjusted = mix(MNdotL, NdotL, MNdotL / 0.25);
    }
    float light = rimLight * R2f + NdotL_adjusted;
    
    if (lights[i].shadows != 0) {
        light *= UseShadowMap(i);
    }
    
    return (specularColor * params[var_DrawId].specularColor.rgb * R2f + diffuse * params[var_DrawId].diffuseColor.rgb) * light * lightColor(i) * var_Color.rgb;
}

uniform int u_numLights;
vec3 computeInteractions() {
    diffuse = textureDiffuse(var_TexDiffuse).rgb;
    specular = vec3(0.026);	//default value if texture not set?...
    if (dot(params[var_DrawId].specularColor, params[var_DrawId].specularColor) > 0.0)
        specular = textureSpecular(var_TexSpecular).rgb;

    vec3 totalColor = vec3(0, 0, 0);
    for (int i = 0; i < u_numLights; ++i) {
        if ((params[var_DrawId].lightMask & (1 << i)) == 0) {
            continue;
        }
        vec4 scissor = lights[i].scissor;
        if (gl_FragCoord.x < scissor.x || gl_FragCoord.y < scissor.y || gl_FragCoord.x > scissor.z || gl_FragCoord.y > scissor.w) {
            continue;
        }
        if (lights[i].ambient == 1) {
            totalColor += ambientLight(i);
        } else {
            totalColor += directLight(i);
        }        
    }

    return totalColor;
}

out vec4 fragColor;

void main() {
	fetchDNS();
	fragColor.rgb = computeInteractions();
	fragColor.a = 1.0;
}
