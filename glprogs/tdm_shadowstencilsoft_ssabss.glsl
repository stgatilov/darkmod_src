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

// Screen-space blocker search for contact hardening of soft stencil shadows (SSABSS).
// Used both by the penumbra pre-pass (stencil_penumbra.frag.glsl) and by the interaction shader.

#pragma tdm_include "tdm_utils.glsl"

// penumbra when no blocker was found in the depth buffer (it may be off-screen or hidden), as fraction of the search distance:
// the result must not depend much on what the camera sees, so use a moderate value instead of the widest penumbra
#define SSABSS_NO_BLOCKER_DIST_FRAC 0.75
// contact curve: >1 hardens shadows more close to the blocker (blur ~ pow(distance to blocker / search distance, power)), 1 = linear
#define SSABSS_CONTACT_POWER 1.6
// contact hardening zone: blockers farther than this from the receiver are not searched for (world units, ~1.3m)
#define SSABSS_MAX_CONTACT_DIST 52.0
// fraction of blocker search taps which must hit for the blocker to fully count (fades the footprint edge)
#define SSABSS_HIT_FULL_FRAC 0.15

float interleavedGradientNoise(vec2 pixelPos) {
	vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
	return fract(magic.z * fract(dot(pixelPos, magic.xy)));
}

vec3 viewPosFromUvDepth(mat4 projectionMatrix, vec2 uv, float depth) {
	// depthToZ returns positive eye distance (-Z_view)
	float zEye = depthToZ(projectionMatrix, depth);
	vec2 ndc = uv * 2.0 - 1.0;
	return vec3(
		(ndc.x + projectionMatrix[2][0]) * zEye / projectionMatrix[0][0],
		(ndc.y + projectionMatrix[2][1]) * zEye / projectionMatrix[1][1],
		-zEye
	);
}

// Symmetric screen-space cone blocker search for contact hardening
// Returns (distance from the receiver to the closest blocker, distance to the farthest blocker found), or (-1, -1) if no blocker was found.
// Only the surfaces visible in the depth buffer can be found, so "not found" does not mean "no occluder".
vec3 estimateContactBlockerDist(
	sampler2D depthTexture, mat4 projectionMatrix,
	vec3 P_v, vec3 N_v, vec3 L_dir, float rawLightDist,
	float lightRadius, float maxSearchDist, float noise, int steps,
	vec2 uvMin, vec2 uvMax
) {
	vec3 up = abs(L_dir.z) < 0.99 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 w1 = normalize(cross(L_dir, up));
	vec3 w2 = cross(L_dir, w1);

	float minBlockerDist = 1e9;
	float maxBlockerDist = 0.0;
	float hits = 0.0;
	// Angular spread lets both shadowed pixels and adjacent outer-penumbra pixels detect contact blockers
	float coneTan = clamp(lightRadius / max(rawLightDist, 8.0), 0.04, 0.25);

	for (int i = 0; i < steps; i++) {
		float frac = (float(i) + noise) / float(steps);
		float t = maxSearchDist * frac * sqrt(frac); // frac^1.5: denser near contact seam

		float angle = 2.3999632 * float(i) + noise * 6.2831853;
		float spread = t * coneTan * (0.25 + 0.75 * frac);
		vec3 Q_v = P_v + t * L_dir + spread * (cos(angle) * w1 + sin(angle) * w2);

		vec4 clip = projectionMatrix * vec4(Q_v, 1.0);
		if (clip.w <= 0.0)
			continue;
		vec2 uv = (clip.xy / clip.w) * 0.5 + 0.5;
		if (any(lessThan(uv, uvMin)) || any(greaterThan(uv, uvMax)))
			continue;

		float sceneDepth = textureLod(depthTexture, uv, 0.0).r;
		vec3 B_v = viewPosFromUvDepth(projectionMatrix, uv, sceneDepth);

		float dz = (-Q_v.z) - (-B_v.z); // > 0 means ray stepped behind visible scene surface
		float bias = 0.012 * (-P_v.z) + 0.04 * t;
		vec3 diff = B_v - P_v;
		float dist3D = length(diff);

		if (dz > bias && dist3D < maxSearchDist) {
			// Require blocker to rise above the receiver's tangent plane (avoids curved self-hardening)
			// and to lie on the light's side of the receiver (a surface which is merely closer to the camera is no blocker)
			if (dot(N_v, diff) > 0.08 * dist3D && dot(L_dir, diff) > 0.5 * dist3D) {
				minBlockerDist = min(minBlockerDist, dist3D);
				maxBlockerDist = max(maxBlockerDist, dist3D);
				hits += 1.0;
			}
		}
	}

	return minBlockerDist < 1e8 ? vec3(minBlockerDist, maxBlockerDist, hits / float(steps)) : vec3(-1.0, -1.0, 0.0);
}

// penumbra size factor (0..1, relative to the far field penumbra) for a blocker at the given distance from the receiver
float ssabssContactFactor(float d_occ, float maxSearchDist, float rawLightDist) {
	// PCSS contact ratio normalized against far-field fallback (smoothly blends to 1.0 at maxSearchDist)
	float blockerRatio = (d_occ / max(maxSearchDist, 1e-3))
		* (rawLightDist / max(rawLightDist - d_occ, 0.25 * rawLightDist));
	float k = pow(clamp(blockerRatio, 0.0, 1.0), SSABSS_CONTACT_POWER);
	return mix(k, 1.0, smoothstep(0.55 * maxSearchDist, maxSearchDist, d_occ));
}

// Returns penumbra size factors (x = for the farthest blocker found, y = for the nearest one)
vec2 ssabssContactFactors(
	sampler2D depthTexture, mat4 projectionMatrix,
	vec3 P_v, vec3 N_v, vec3 L_dir, float rawLightDist,
	float lightRadius, int softQuality, float noise,
	vec2 uvMin, vec2 uvMax
) {
	float maxSearchDist = min(SSABSS_MAX_CONTACT_DIST, rawLightDist * 0.75);
	int blockerSteps = clamp(softQuality / 3, 4, 8);

	vec3 occ = estimateContactBlockerDist(
		depthTexture, projectionMatrix,
		P_v, N_v, L_dir, rawLightDist,
		lightRadius, maxSearchDist, noise, blockerSteps,
		uvMin, uvMax
	);
	float dNear = occ.x;
	float dFar = occ.y;
	if (occ.x < 0.0) {
		// the occluder may be invisible for the camera (off-screen, hidden): don't make the penumbra depend on that too much
		dNear = dFar = SSABSS_NO_BLOCKER_DIST_FRAC * maxSearchDist;
	}
	vec2 found = vec2(
		ssabssContactFactor(dFar, maxSearchDist, rawLightDist),
		ssabssContactFactor(dNear, maxSearchDist, rawLightDist)
	);
	// The edge of the search footprint is a straight line on screen: pixels just outside it see no blocker and just inside see it.
	// Fade between "no blocker" and "blocker found" by the fraction of search taps which hit it, and never let the
	// far (wide) penumbra shrink below what a partially seen blocker allows: partial hits mean the blocker is at the footprint edge.
	float fb = ssabssContactFactor(SSABSS_NO_BLOCKER_DIST_FRAC * maxSearchDist, maxSearchDist, rawLightDist);
	float w = occ.x < 0.0 ? 0.0 : smoothstep(0.0, SSABSS_HIT_FULL_FRAC, occ.z);
	return mix(vec2(fb), found, w);
}
