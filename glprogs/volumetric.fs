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
#version 430

#pragma tdm_include "tdm_lightproject.glsl"

layout(binding=0) uniform sampler2D s_projection;
layout(binding=1) uniform sampler2D s_falloff;
layout(binding=2) uniform sampler2D s_depth;
layout(binding=5) uniform sampler2D u_shadowMap;

layout (location = 0) uniform mat4[2] u_MVP;
layout (location = 2) uniform vec3 u_viewOrigin;
layout (location = 3) uniform mat4 u_lightProject;
layout (location = 4) uniform vec4 u_lightFrustum[6];
layout (location = 10) uniform vec4 u_shadowRect;
layout (location = 11) uniform vec3 u_lightOrigin;
layout (location = 12) uniform int u_sampleCount;
layout (location = 13) uniform float u_lightRadius;
layout (location = 14) uniform vec4 u_lightColor;
layout (location = 15) uniform bool u_shadows;

in vec4 csThis;
in vec4 lightProject;
in vec4 worldPosition;

out vec4 fragColor;

void ShadowAtlasForVector(vec3 v, out vec4 depthSamples, out vec2 sampleWeights) {
	vec3 v1 = abs(v);
	float maxV = max(v1.x, max(v1.y, v1.z));
	int cubeSide = 0;
	if(maxV == v.x) {
		v1 = -v.zyx;
	} 
	else if(maxV == -v.x) {
		v1 = v.zyx * vec3(1, -1, 1);
		cubeSide = 1;
	}
	else if(maxV == v.y) {
		v1 = v.xzy * vec3(1, 1, -1);
		cubeSide = 2;
	}
	else if(maxV == -v.y) {
		v1 = v.xzy * vec3(1, -1, 1);
		cubeSide = 3;
	}
	else if(maxV == v.z) {
		v1 = v.xyz * vec3(1, -1, -1);
		cubeSide = 4;
	}
	else { //if(maxV == -v.z) {
		v1 = v.xyz * vec3(-1, -1, 1);
		cubeSide = 5;
	}
	v1.xy /= -v1.z;
	vec2 texSize = textureSize(u_shadowMap, 0);
	vec2 shadow2d = (v1.xy * (.5-.0/texSize.x) + vec2(.5) ) * u_shadowRect.ww + u_shadowRect.xy;
	shadow2d.x += u_shadowRect.w * cubeSide;
	vec4 d = textureGather(u_shadowMap, shadow2d);
	vec4 one = vec4(1.000001);
	depthSamples = u_lightRadius / (one - d);
	sampleWeights = fract(shadow2d * texSize + -0.5);
}

//returns eye Z coordinate with reversed sign (monotonically increasing with depth)
//TODO: move this to common include?...
float depthToZ(float depth) {
	float clipZ = 2.0 * depth - 1.0;
	float A = u_MVP[1][2].z;
	float B = u_MVP[1][3].z;
	return B / (A + clipZ);
}

// get N samples from the fragment-view ray inside the frustum
vec3 calcWithShadows(vec3 rayStart, vec3 rayVec, float minParam, float maxParam) {
	vec3 color = vec3(0.0);
	for (int i = 0; i < u_sampleCount; i++) { 
		float ratio = (i + 0.5) / u_sampleCount;
		vec3 samplePos = rayStart + rayVec * mix(minParam, maxParam, ratio);
		// shadow test
		vec3 light2fragment = samplePos - u_lightOrigin;
		float lit = 1;
		if (u_shadows) {
			vec4 depthSamples;
			vec2 sampleWeights;
			ShadowAtlasForVector(normalize(light2fragment), depthSamples, sampleWeights);
			vec3 absL = abs(light2fragment);
			float maxAbsL = max(absL.x, max(absL.y, absL.z));
			vec4 lit4 = vec4(lessThan(vec4(maxAbsL), depthSamples));
			lit = mix(mix(lit4.w, lit4.z, sampleWeights.x),
					mix(lit4.x, lit4.y, sampleWeights.x),
					sampleWeights.y);
		}
		vec4 texCoord = computeLightTex(u_lightProject, vec4(samplePos, 1));
		vec3 texColor = projFalloffOfNormalLight(s_projection, s_falloff, texCoord);
		color += lit * texColor;
	}
	return color / u_sampleCount;
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
	vec2 depthTexCoord = gl_FragCoord.xy / textureSize(s_depth, 0);
	float depth = texture2D(s_depth, depthTexCoord).r;
	float solidParam = depthToZ(depth) / depthToZ(gl_FragCoord.z);
	maxParam = min(maxParam, solidParam);

	if (minParam >= maxParam)
		discard;    //no intersection
	
	vec3 avgColor;
	if (u_sampleCount > 0)
		avgColor = calcWithShadows(rayStart, rayVec, minParam, maxParam);
	else
		avgColor = vec3(1.0);	//full-white

	float litDistance = (maxParam - minParam) * length(rayVec);
	float dustCoeff = 1e-3; //TODO: expose it from C++
	fragColor.rgb = u_lightColor.rgb * avgColor * litDistance * dustCoeff;
}