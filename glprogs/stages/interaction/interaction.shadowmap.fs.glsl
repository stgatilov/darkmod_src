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
#extension GL_ARB_texture_gather: enable

#define STGATILOV_OCCLUDER_SEARCH 1
#define STGATILOV_USEGATHER 1

#pragma tdm_include "stages/interaction/interaction.common.fs.glsl"
#define TDM_allow_ARB_texture_gather STGATILOV_USEGATHER
#pragma tdm_include "tdm_shadowmaps.glsl"

uniform bool 	u_shadowMapCullFront;
uniform vec4	u_shadowRect;
uniform sampler2D u_shadowMap;
in vec3 var_WorldLightDir;

out vec4 fragColor;


void UseShadowMap() {
	float shadowMapResolution = (textureSize(u_shadowMap, 0).x * u_shadowRect.w);

	//get unit direction from light to current fragment
	vec3 L = normalize(var_WorldLightDir);
	//find maximum absolute coordinate in light vector
	vec3 absL = abs(var_WorldLightDir);
	float maxAbsL = max(absL.x, max(absL.y, absL.z));

	//get interpolated normal / normal of triangle (using different normal affects near-tangent lighting greatly)
	//vec3 normal = normalize(cross(dFdx(var_WorldLightDir), dFdy(var_WorldLightDir)));
	vec3 normal = mat3(params[var_DrawId].modelMatrix) * N;//var_TangentBitangentNormalMatrix[2];
	float lightFallAngle = -dot(normal, L);

	//note: choosing normal and how to cap angles is the hardest question for now
	//this has large effect on near-tangent surfaces (mostly the curved ones)
	fragColor.rgb *= smoothstep(0.01, 0.05, lightFallAngle);
	//some very generic error estimation...
	float errorMargin = 5.0 * maxAbsL / ( shadowMapResolution * max(lightFallAngle, 0.1) );
	if(u_shadowMapCullFront)
	   errorMargin *= -.5;

	//process central shadow sample
	float centerFragZ = maxAbsL;
#if STGATILOV_USEGATHER && defined(GL_ARB_texture_gather)
	vec4 wgt;
	vec4 centerBlockerZ = ShadowAtlasForVector4(u_shadowMap, u_shadowRect, L, wgt);
	float lit = dot(wgt, step(centerFragZ - errorMargin, centerBlockerZ));
#else
	float centerBlockerZ = ShadowAtlasForVector(u_shadowMap, u_shadowRect, L);
	float lit = float(centerBlockerZ >= centerFragZ - errorMargin);
#endif

	if (u_softShadowsQuality == 0) {
		fragColor.rgb *= lit;
		return;
	}
	float lightDist = length(var_WorldLightDir);
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
		float blockerZ = ShadowAtlasForVector(u_shadowMap, u_shadowRect, perturbedLightDir);
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
		return;
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
		float blockerZ = ShadowAtlasForVector(u_shadowMap, u_shadowRect, perturbedLightDir);
		lit += float(blockerZ >= fragZ - errorMargin);
	}
	lit /= u_softShadowsQuality + 1;
	fragColor.rgb *= lit;
}

void main() {
	fetchDNS();
	fragColor.rgb = computeInteraction();
	if (u_shadows)
		UseShadowMap();
	fragColor.a = 1.0;
}
