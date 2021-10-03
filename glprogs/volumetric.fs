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

in vec4 csThis;
in vec4 lightProject;
in vec4 worldPosition;

out vec4 fragColor;

// #define SHOW_PRECISION_ISSUE 1

#ifdef SHOW_PRECISION_ISSUE

void main() {
	float minDist = 0; 
	for(int i=0; i<6; i++) {
		float dist = dot(u_lightFrustum[i], worldPosition);
		minDist = min(dist, dist);
	}
	gl_FragColor.rg = minDist * vec2(1, -1) * 1e-1;
	gl_FragColor.b = .5;
}

#else

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

mat4 projMatrixInv = inverse(u_MVP[1]);

// this is supposed to get the world position from the depth buffer
vec3 ViewPosFromDepth(float depth) {
    float z = depth * 2.0 - 1.0;

	vec2 TexCoord = gl_FragCoord.xy / textureSize(s_depth, 0);
    vec4 clipSpacePosition = vec4(TexCoord * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePosition = projMatrixInv * clipSpacePosition;

    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;
	return viewSpacePosition.xyz;
}

float calcCylinder(vec4 startPos, vec4 exitPoint) {
	// return .93;
	vec4 p1 = startPos * u_lightProject;
	vec4 p2 = exitPoint * u_lightProject;
	p1.xy /= p1.z;
	p2.xy /= p2.z;
	vec4 mid = (p1+p2)/2;
	float fromCenter = distance(mid.xy, vec2(0.5));
	float cap = 0.3 - fromCenter;
	return cap > 0 ? pow(cap * 1e-3, .3) * 3e1 : 0;
} 

// get N samples from the fragment-view ray inside the frustum
float calcWithShadows(vec4 startPos, vec4 exitPoint) {
	float color = 0;
	for(float i=0; i<u_sampleCount; i++) {
		vec4 samplePos = mix(startPos, exitPoint, i/u_sampleCount);
			// shadow test
			vec4 depthSamples;
			vec2 sampleWeights;
			vec3 light2fragment = samplePos.xyz - u_lightOrigin;
			ShadowAtlasForVector(normalize(light2fragment), depthSamples, sampleWeights);
			vec3 absL = abs(light2fragment);
			float maxAbsL = max(absL.x, max(absL.y, absL.z));
			vec4 lit4 = vec4(lessThan(vec4(maxAbsL), depthSamples));
			float lit = mix(mix(lit4.w, lit4.z, sampleWeights.x),
                	mix(lit4.x, lit4.y, sampleWeights.x),
                   	sampleWeights.y);
			vec4 lightProject = samplePos * u_lightProject;
			vec4 t0 = texture2DProj(s_projection, lightProject.xyz );
			vec4 t1 = texture2D(s_falloff, vec2(lightProject.w, 0.5) );
			color += t0.r * t1.r * lit;
	}
	return color / u_sampleCount;
}

void main() {
	fragColor = vec4(0);
	// if(int(gl_FragCoord.x / 38) % 7 == 6)
	// 	fragColor.b = .3;
	vec2 wrCoord = csThis.xy/csThis.w * .5 + .5;
	float depth = texture2D(s_depth, wrCoord ).r;
	
	// possible error - fix world position to be inside frustum
	vec3 fixedWorldPos = worldPosition.xyz;
	// float minVF = 1e11;
	for(int i=0; i<6; i++) {
		float distance = dot(u_lightFrustum[i], vec4(fixedWorldPos, 1));
		// minVF = min(minVF, abs(dot(u_lightFrustum[i], vec4(u_viewOrigin, 1))));
		if(distance > 0) {
			fixedWorldPos -= 1.1*distance*u_lightFrustum[i].xyz;
			// fragColor.rg = (distance)*1e1*vec2(-1,1);
			// return;
		} 
		distance = dot(u_lightFrustum[i], vec4(fixedWorldPos, 1));
		if(distance > 0) {
			fixedWorldPos -= 1.1*distance*u_lightFrustum[i].xyz;
			fragColor.rg = (distance)*1e1*vec2(-1,1);
			return;
		} 		
	}
	// gl_FragColor.r = minVF*1e-2;
	// return;

	vec3 dirToViewer = normalize(u_viewOrigin - fixedWorldPos);

	// where does the fragment-viewer ray leave the light frustum?
	float minRayCoord = length(u_viewOrigin - fixedWorldPos); 
	for(int i=0; i<6; i++) {  // https://stackoverflow.com/questions/23975555/how-to-do-ray-plane-intersection
		// float this = dot(u_lightFrustum[i], vec4(u_viewOrigin, 1));
		// if(this < 0)
		// 	continue;
		float dotnp = dot(u_lightFrustum[i], vec4(fixedWorldPos, 1));
		float dotnv = dot(u_lightFrustum[i].xyz, -dirToViewer);
		float rayCoord = dotnp/dotnv;
		// if(int(gl_FragCoord.x / 38) % 7 == i) {
		// if(3 == i)
			// fragColor.rg = (rayCoord - minRayCoord)*1e-1*vec2(-1,1);
			// fragColor.rg = (dotnp)*1e1*vec2(-1,1);
			// fragColor.rg = (dot(u_lightFrustum[i], vec4(u_viewOrigin, 1)))*1e-2*vec2(-1,1);
		// }
		// if(rayCoord < 1e-1) { // negative should mean that the intersection is behind the fragment - ignored
		// 	continue;         // at least one of these must be zero-ish (the plane being rendered now)
		// }
		if(rayCoord < 0) continue;
		if(rayCoord < minRayCoord) {// the intersection closest to the fragment is the exit point
			minRayCoord = rayCoord;
			// if(int(gl_FragCoord.x / 38) % 7 == i) {
			// 	fragColor.rg = (minRayCoord)*1e2*vec2(-1,1);
			// }
		}
	}
	// fragColor.r = minRayCoord*1e-1;
	vec4 exitPoint = vec4(minRayCoord * dirToViewer + fixedWorldPos, 1);

	// get the nearest solid surface
	float solidDistance = length(ViewPosFromDepth(depth));
	// if(int(gl_FragCoord.x / 38) % 7 == 0)
	// 	fragColor.rg = (solidDistance-distance(exitPoint.xyz, u_viewOrigin))*1e-1*vec2(-1,1);
	// occluder outside of frustum
	// fragColor.r = distance(exitPoint.xyz, u_viewOrigin)*1e-2;
	// return;
	if(distance(exitPoint.xyz, u_viewOrigin) >= solidDistance)
		return;
	// gl_FragColor.r = distance(exitPoint.xyz, u_viewOrigin);
	// return;
	// start position lies on the light frustum	
	float fragmentDistance = distance(u_viewOrigin, fixedWorldPos);
	vec4 startPos = vec4(fixedWorldPos, 1);
	if(solidDistance < fragmentDistance) // frustum occluded, need to shift
		startPos = vec4(mix(u_viewOrigin, fixedWorldPos, solidDistance / fragmentDistance), 1);

	float color = 0;
	switch (u_sampleCount) {
	case 1:
		color = calcCylinder(startPos, exitPoint);
		break;
	default:
		color = calcWithShadows(startPos, exitPoint);
	}

	fragColor.rgb = u_lightColor.rgb * vec3(color) * 3e-1;
}

#endif