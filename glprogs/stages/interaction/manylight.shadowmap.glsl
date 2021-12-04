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
#define STGATILOV_OCCLUDER_SEARCH 1
#define STGATILOV_USEGATHER 1

#define TDM_allow_ARB_texture_gather STGATILOV_USEGATHER
#pragma tdm_include "tdm_shadowmaps.glsl"

uniform int		u_softShadowsQuality;
uniform float	u_softShadowsRadius;
uniform bool 	u_shadowMapCullFront;
uniform sampler2D u_shadowMap;

float UseShadowMap(int lightNum) {
	float shadowMapResolution = (textureSize(u_shadowMap, 0).x * lights[lightNum].shadowRect.w);

    vec3 worldLightDir = var_WorldPosition - lights[lightNum].origin.xyz;
    vec3 L = normalize(worldLightDir);
	//find maximum absolute coordinate in light vector
	vec3 absL = abs(worldLightDir);
	float maxAbsL = max(absL.x, max(absL.y, absL.z));

	//get interpolated normal / normal of triangle (using different normal affects near-tangent lighting greatly)
	//vec3 normal = normalize(cross(dFdx(var_WorldLightDir), dFdy(var_WorldLightDir)));
	vec3 normal = N;
	float lightFallAngle = -dot(normal, L);

	//note: choosing normal and how to cap angles is the hardest question for now
	//this has large effect on near-tangent surfaces (mostly the curved ones)
	float lit = smoothstep(0.01, 0.05, lightFallAngle);
	//some very generic error estimation...
	float errorMargin = 5.0 * maxAbsL / ( shadowMapResolution * max(lightFallAngle, 0.1) );
	if(u_shadowMapCullFront)
	   errorMargin *= -.5;

	//process central shadow sample
	float centerFragZ = maxAbsL;
#if STGATILOV_USEGATHER
	vec4 wgt;
	vec4 centerBlockerZ = ShadowAtlasForVector4(u_shadowMap, lights[lightNum].shadowRect, L, wgt);
	lit *= dot(wgt, step(centerFragZ - errorMargin, centerBlockerZ));
#else
	float centerBlockerZ = ShadowAtlasForVector(u_shadowMap, lights[lightNum].shadowRect, lightNum, L);
	lit *= float(centerBlockerZ >= centerFragZ - errorMargin);
#endif

	if (u_softShadowsQuality == 0) {
		return lit;
	}
	float lightDist = length(worldLightDir);
	//this is (1 / cos(phi)), where phi is angle between light direction and normal of the pierced cube face
	float secFallAngle = lightDist / maxAbsL;
	//find two unit directions orthogonal to light direction
	vec3 nonCollinear = vec3(1, 0, 0);
	if (absL.x == maxAbsL)
		nonCollinear = vec3(0, 1, 0);
	vec3 orthoAxisX = normalize(cross(L, nonCollinear));
	vec3 orthoAxisY = cross(L, orthoAxisX);

	#if STGATILOV_OCCLUDER_SEARCH
	//search for blockers in a cone with rather large angle
	float searchAngle = 0.03 * u_softShadowsRadius;    //TODO: this option is probably very important
	float avgBlockerZ = 0;
	int blockerCnt = 0;
	for (int i = 0; i < u_softShadowsQuality; i++) {
		//note: copy/paste from sampling code below
		vec3 perturbedLightDir = normalize(L + searchAngle * (u_softShadowsSamples[i].x * orthoAxisX + u_softShadowsSamples[i].y * orthoAxisY));
		float blockerZ = ShadowAtlasForVector(u_shadowMap, lights[lightNum].shadowRect, perturbedLightDir);
		float dotDpL = max(max(abs(perturbedLightDir.x), abs(perturbedLightDir.y)), abs(perturbedLightDir.z));
		float distCoeff = lightFallAngle / max(-dot(normal, perturbedLightDir), 1e-3) * (dotDpL * secFallAngle);
		float fragZ = centerFragZ * distCoeff;
		//note: only things which may potentially occlude are averaged
		if (blockerZ < fragZ - errorMargin) {
			avgBlockerZ += blockerZ;
			blockerCnt++;
		}
	}
	//shortcut: no blockers in search angle => fully lit
	if (blockerCnt == 0)
		return 1;
	/* Bad optimization!
	 * Go to St. Alban's Collateral and execute:
	 *   setviewpos  -114.57 1021.61 130.95   -1.0 147.8 0.0
	 * and you'll notice artefacts if you enable this piece of code.
	//shortcut: all blockers in search angle => fully occluded
	if (blockerCnt == u_softShadowsQuality && lit == 0) {
		fragColor.rgb *= 0.0f;
		return;
	}
	*/
	avgBlockerZ /= blockerCnt;
	#endif

	//radius of light source
	float lightRadius = u_softShadowsRadius;
	//radius of one-point penumbra at the considered point (in world coordinates)
	//note that proper formula is:  lightRadius * (lightDist - occlDist) / occlDist;
	float blurRadiusWorld = lightRadius * lightDist / 200.0;
	#if STGATILOV_OCCLUDER_SEARCH
	blurRadiusWorld = lightRadius * (centerFragZ - avgBlockerZ) / avgBlockerZ;
	#endif
	//blur radius relative to light distance
	float blurRadiusRel = blurRadiusWorld / lightDist;
	#if STGATILOV_OCCLUDER_SEARCH
	//note: it is very important to limit blur angle <= search angle !
	blurRadiusRel = min(blurRadiusRel, searchAngle);
	#endif
	//minor radius of the ellipse, which is: the intersection of penumbra cone with cube's face
	float blurRadiusCube = blurRadiusRel * secFallAngle;
	//the same radius in shadowmap pixels
	float blurRadiusPixels = blurRadiusCube * shadowMapResolution;

	//limit blur radius from below: blur must cover at least (2*M) pixels in any direction
	//otherwise user will see the shitty pixelated shadows, which is VERY ugly
	//we prefer blurring shadows to hell instead of showing pixelation...
	const float minBlurInShadowPixels = 5.0;
	if (blurRadiusPixels < minBlurInShadowPixels) {
		float coeff = minBlurInShadowPixels / (blurRadiusPixels + 1e-7);
		//note: other versions of blurRadius are not used below
		blurRadiusRel *= coeff;
	}
	for (int i = 0; i < u_softShadowsQuality; i++) {
		//unit vector L' = perturbed version of L
		vec3 perturbedLightDir = normalize(L + blurRadiusRel * (u_softShadowsSamples[i].x * orthoAxisX + u_softShadowsSamples[i].y * orthoAxisY));
		//this is:  (cos(D ^ L') / cos(N ^ L')) / (cos(D ^ L) / cos(N ^ L))
		//where L and L' is central/perturbed light direction, D is normal of active cubemap face, and N is normal of tangent plane
		float dotDpL = max(max(abs(perturbedLightDir.x), abs(perturbedLightDir.y)), abs(perturbedLightDir.z));
		float distCoeff = lightFallAngle / max(-dot(normal, perturbedLightDir), 1e-3) * (dotDpL * secFallAngle);
		float fragZ = centerFragZ * distCoeff;
		float blockerZ = ShadowAtlasForVector(u_shadowMap, lights[lightNum].shadowRect, perturbedLightDir);
		lit += float(blockerZ >= fragZ - errorMargin);
	}
	lit /= u_softShadowsQuality + 1;
	return lit;
}
