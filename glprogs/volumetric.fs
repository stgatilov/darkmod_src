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
#version 330

#pragma tdm_include "tdm_utils.glsl"
#pragma tdm_include "tdm_transform.glsl"
#pragma tdm_include "tdm_lightproject.glsl"
#pragma tdm_include "tdm_shadowmaps.glsl"

uniform sampler2D u_lightProjectionTexture;
uniform sampler2D u_lightFalloffTexture;
uniform sampler2D u_depthTexture;
uniform sampler2D u_shadowMap;

uniform vec3 u_viewOrigin;
uniform mat4 u_lightProject;
uniform vec4 u_lightFrustum[6];
uniform vec4 u_shadowRect;
uniform vec3 u_lightOrigin;
uniform int u_sampleCount;
uniform vec4 u_lightColor;
uniform int u_shadows;

in vec4 worldPosition;

out vec4 fragColor;

// get N samples from the fragment-view ray inside the frustum
vec3 calcWithSampling(vec3 rayStart, vec3 rayVec, float minParam, float maxParam) {
	vec3 color = vec3(0.0);
	for (int i = 0; i < u_sampleCount; i++) { 
		float ratio = (i + 0.5) / u_sampleCount;
		vec3 samplePos = rayStart + rayVec * mix(minParam, maxParam, ratio);
		// shadow test
		vec3 light2fragment = samplePos - u_lightOrigin;
		float lit = 1;
		if (u_shadows != 0) {
			float depth = ShadowAtlasForVector(u_shadowMap, u_shadowRect, light2fragment);
			vec3 absL = abs(light2fragment);
			float maxAbsL = max(absL.x, max(absL.y, absL.z));
			lit = float(maxAbsL < depth);
		}
		vec4 texCoord = computeLightTex(u_lightProject, vec4(samplePos, 1));
		vec3 texColor = projFalloffOfNormalLight(u_lightProjectionTexture, u_lightFalloffTexture, texCoord);
		color += lit * texColor;
	}
	return color / u_sampleCount;
}

vec3 calcAverage(vec3 rayStart, vec3 rayVec, float minParam, float maxParam) {
	// 1. suppose that nothing is shadowed
	// 2. take average of both projection and falloff textures
	vec4 avgProjection = textureLod(u_lightProjectionTexture, vec2(0.5), 10000);
	vec4 avgFalloff = textureLod(u_lightFalloffTexture, vec2(0.5), 10000);
	return avgProjection.xyz * avgFalloff.xyz;
}

void main() {
	//cast segment from viewer eye to the fragment
	vec3 rayStart = u_viewOrigin;
	vec3 rayVec = worldPosition.xyz - u_viewOrigin;
	
	//intersect the segment with light polytope
	float minParam = 0.0;
	float maxParam = 1.0;
	for (int i = 0; i < 6; i++) {
		float dotnp = dot(u_lightFrustum[i], vec4(rayStart, 1.0));
		float dotnv = dot(u_lightFrustum[i].xyz, rayVec);
		float param = -dotnp / dotnv;
		if (dotnv > 0)
			maxParam = min(maxParam, param);
		else
			minParam = max(minParam, param);
	}

	//only consider visible part (not occluded by opaque geometry)
	vec2 depthTexCoord = gl_FragCoord.xy / textureSize(u_depthTexture, 0);
	float depth = texture2D(u_depthTexture, depthTexCoord).r;
	float solidParam = depthToZ(u_projectionMatrix, depth) / depthToZ(u_projectionMatrix, gl_FragCoord.z);
	maxParam = min(maxParam, solidParam);

	if (minParam >= maxParam)
		discard;    //no intersection
	
	vec3 avgColor;
	if (u_sampleCount > 0)
		avgColor = calcWithSampling(rayStart, rayVec, minParam, maxParam);
	else
		avgColor = calcAverage(rayStart, rayVec, minParam, maxParam);

	float litDistance = (maxParam - minParam) * length(rayVec);
	float dustCoeff = 1e-3; //TODO: expose it from C++
	fragColor.rgb = u_lightColor.rgb * avgColor * litDistance * dustCoeff;
}