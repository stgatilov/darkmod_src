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
#version 400

#define TDM_allow_ARB_texture_gather 1
#pragma tdm_include "tdm_shadowmaps.glsl"

// vertex shader output
in vec4 var_Position;
in vec4 var_PositionWorld;
in vec2 var_TexDiffuse;
in vec2 var_TexNormal;
in vec2 var_TexSpecular;
in mat3 var_TangentBitangentNormalMatrix;
in vec4 var_Color;        

out vec4 FragColor;

//uniform mat4 u_modelMatrix;
uniform float u_minLevel, u_gamma;

// lights
const int MAX_LIGHTS = 16;
#define ShadowExperiment true

uniform int u_lightCount;
uniform float u_softShadowsRadius[MAX_LIGHTS];
uniform vec3 u_lightOrigin[MAX_LIGHTS];
uniform vec3 u_lightColor[MAX_LIGHTS];
uniform mat4 u_lightProjectionFalloff[MAX_LIGHTS];
uniform vec4 u_shadowRect[MAX_LIGHTS];

// textures
uniform sampler2D u_diffuseTexture;
uniform sampler2D u_normalTexture;
uniform sampler2D u_shadowMap;

// compute normal from normal map, move from [0, 1] to [-1, 1] range, normalize
vec4 diffuse = texture( u_diffuseTexture, var_TexDiffuse );
vec4 bumpTexel = texture ( u_normalTexture, var_TexNormal ) * 2. - 1.;
//vec3 RawN = normalize( bumpTexel.xyz );
vec3 RawN = bumpTexel.xyz;
vec3 N = var_TangentBitangentNormalMatrix * RawN;
//float NdotH = clamp( dot( N, H ), 0.0, 1.0 );

vec2 ShadowTexSize = textureSize(u_shadowMap, 0);
vec2 ShadowTexelStep = 1/ShadowTexSize;

vec3 Barycentric(vec2 p, int tri) { // https://gamedev.stackexchange.com/questions/23743/whats-the-most-efficient-way-to-find-barycentric-coordinates
	vec2 t[4] = vec2[4](
		vec2(0,0),
		vec2(0,1),
		vec2(1,0),
		vec2(1,1)
	);
	vec2 a = t[tri+0];
	vec2 b = t[tri+1];
	vec2 c = t[tri+2];
	vec2 v0 = b - a, v1 = c - a, v2 = p - a;
    float den = v0.x * v1.y - v1.x * v0.y;
    float v = (v2.x * v1.y - v1.x * v2.y) / den;
    float w = (v0.x * v2.y - v2.x * v0.y) / den;
    float u = 1.0f - v - w;
	return vec3(u,v,w);
}

vec3 debugColor;
float OutsideShadowVolume(vec3 v, int i) {
	int faceIdx;
	vec3 v1 = CubeMapDirectionToUv(v, faceIdx);
//	v1.xy += sin(gl_FragCoord.xy)*1e-3;
	vec2 shadow2d = (v1.xy * .5 + .5 ) * u_shadowRect[i].ww + u_shadowRect[i].xy;
	shadow2d.x += (u_shadowRect[i].w + ShadowTexelStep.x) * faceIdx; // u_shadowRect[i].w is passed as width minus one - need to add one to get the original width value
	vec4 d = textureGather(u_shadowMap, shadow2d);
	vec4 blockerZ4 = u_softShadowsRadius[i] / (1 - d);
	float testZ = -v1.z;

	vec2 texels = u_shadowRect[i].w * ShadowTexSize;
	vec2 x0y0 = v1.xy * .5 + .5;
	vec2 wgt = fract(x0y0 * texels - 0.0);
	vec2 mwgt = vec2(1) - wgt;
	vec4 sampleWeights = vec4(mwgt.x, wgt.x, wgt.x, mwgt.x) * vec4(wgt.y, wgt.y, mwgt.y, mwgt.y);
#ifdef ALGO_TOGGLE
	float localZ = dot(blockerZ4, sampleWeights);
	if(isinf(localZ))
		localZ = min(min(blockerZ4[0], blockerZ4[1]),min(blockerZ4[2],blockerZ4[3]));
#else	
	int tri = (wgt.y + wgt.x < 1) ? 0 : 1;
	vec3 bariCoord = Barycentric(wgt, tri);
	vec3 blockerZ3 = tri == 0 ? blockerZ4.wxz : blockerZ4.xzy;
	float localZ = dot(blockerZ4, sampleWeights);
	localZ = dot(blockerZ3, bariCoord);
	if(isinf(localZ))
		localZ = min(min(blockerZ3[0], blockerZ3[1]),blockerZ3[2]);
#endif
	debugColor = sampleWeights.xyz;
//	return clamp((localZ - testZ)*1e-1, 0, 1);
	return float(localZ > testZ + 0e-0);
}

float Unshadowed(vec3 lightDir, int i) { // 0 - fully occluded, 1 - fully lit
	//lightDir = (u_modelMatrix*vec4(lightDir, 0)).xyz;
	vec3 L = normalize( -lightDir );
	L = ( -lightDir );
#ifdef ShadowExperiment
	return OutsideShadowVolume(L, i);
#endif
	vec3 absL = abs(lightDir);
	float maxAbsL = max(absL.x, max(absL.y, absL.z));

	float shadowMapResolution = (textureSize(u_shadowMap, 0).x * u_shadowRect[i].w);
	vec3 normal = /*mat3(u_modelMatrix) */ var_TangentBitangentNormalMatrix[2];
	float lightFallAngle = -dot(normal, L);
	float errorMargin = 2.0 * maxAbsL / ( shadowMapResolution * max(lightFallAngle, 0.1) );

	//process central shadow sample
	float centerFragZ = maxAbsL;
	vec4 sampleWeights;

	vec4 blockerZ = ShadowAtlasForVector4(u_shadowMap, u_shadowRect[i], L, sampleWeights);

	vec4 lit4 = step(centerFragZ - errorMargin, blockerZ);
	float lit = dot(sampleWeights, lit4);
	return lit;
}

float ShadowStage(int i) {
	vec3 lightDir = u_lightOrigin[i] - var_Position.xyz;
	return Unshadowed(lightDir, i);
}

vec3 PointLight(int i) {
	vec3 lightDir = u_lightOrigin[i] - var_Position.xyz;
	vec3 L = normalize( lightDir );
	float NdotL = dot( N, L );
	float thisLit = NdotL;
	vec4 lightTC = var_Position * u_lightProjectionFalloff[i];
	{ // quadratic falloff/projection texture replacement
		vec3 pt = lightTC.xyw - vec3(0.5);
		float dist2 = dot(pt, pt);
		thisLit *= max(0.25 - dist2, 0) * 4;
	}
	if(thisLit <= 0)
		return vec3(0);
	float sm = u_shadowRect[i].z;
	if(sm >= 0)
		thisLit *= ShadowStage(i);
	return thisLit * u_lightColor[i];
}

vec3 AmbientLight(int i) {
	vec3 thisLit = u_lightColor[i];
	
	// light boundary cut off
	vec4 lightTC = var_Position * u_lightProjectionFalloff[i];
	vec4 cutOff = 1 - step(0.5, abs(0.5 - lightTC));
	//thisLit *= cutOff.x * cutOff.y * cutOff.z;
	thisLit *= cutOff.x * cutOff.y;
	
	return thisLit;
}

void main() {
	vec3 lit = vec3(0);
	for(int i=0; i < u_lightCount; i++) {
		float sm = u_shadowRect[i].z;
		if(sm == -2)
			lit += AmbientLight(i);
		else
			lit += PointLight(i);
		//FragColor.rgb = lit; return;
	}
	// local gamma replacement
	if (u_minLevel != 0 ) // home-brewed "pretty" linear
		lit = lit * (1.0 - u_minLevel) + vec3(u_minLevel);
	FragColor.rgb = lit * diffuse.rgb * var_Color.rgb;
	if(u_gamma != 1 ) // old-school exponential
		FragColor.rgb = pow(FragColor.rgb, vec3(1.0 / u_gamma));
	//FragColor.rgb = mix(FragColor.rgb, debugColor, .7);
	FragColor.a = diffuse.a;
}