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

#pragma tdm_include "tdm_utils.glsl"
#pragma tdm_include "stages/interaction/interaction.common.fs.glsl"

uniform usampler2D u_stencilTexture;
uniform sampler2D u_depthTexture;

in vec3 var_WorldLightDir;

out vec4 FragColor;

void StencilSoftShadow() {
	vec2 texSize = vec2(textureSize(u_depthTexture, 0));
	vec2 pixSize = vec2(1.0, 1.0) / texSize;
	vec2 baseTC = gl_FragCoord.xy * pixSize;

	float StLevel = 129.0;

	float StTex = float(texture( u_stencilTexture, baseTC ).r);
	float stencil = clamp( StLevel - StTex, 0., 1.);
	float sumWeight = 1.;

	float LightDist = min(length(lightDir), 1e3); // crutch !
	//radius of light source
	float lightRadius = u_softShadowsRadius;
	//radius of one-point penumbra at the consided point (in world coordinates)
	//note that proper formula is:  lightRadius * (LightDist - OcclDist) / OcclDist;
	float blurRadiusWorld = lightRadius * LightDist / 66.6666;  //TODO: revert?!

	//project direction to light onto surface
	vec3 normal = var_TangentBitangentNormalMatrix[2];
	vec3 alongDirW = normalize(lightDir - dot(lightDir, normal) * normal);
	//get orthogonal direction on surface
	vec3 orthoDirW = cross(normal, alongDirW);
	//multiply the two axes by penumbra radius
	alongDirW *= blurRadiusWorld / max(NdotL, 0.2);  //penumbra is longer by (1/cos(a)) in light direction
	orthoDirW *= blurRadiusWorld;

	//convert both vectors into clip space (get only X and Y components)
	mat4 modelViewProjectionMatrix = u_projectionMatrix * params[var_DrawId].modelViewMatrix;
	vec2 alongDir = (mat3(modelViewProjectionMatrix) * alongDirW).xy;
	vec2 orthoDir = (mat3(modelViewProjectionMatrix) * orthoDirW).xy;
	//now also get W component from multiplication by gl_ModelViewProjectionMatrix
	vec3 mvpRow3 = vec3(modelViewProjectionMatrix[0][3], modelViewProjectionMatrix[1][3], modelViewProjectionMatrix[2][3]);
	float along_w = dot(mvpRow3, alongDirW);
	float ortho_w = dot(mvpRow3, orthoDirW);
	//this is perspective correction: it is necessary because W component in clip space also varies
	//if you remove it and look horizontally parallel to a wall, then vertical shadow boundaries on this wall won't be blurred
	vec2 thisNdc = (2 * baseTC - vec2(1));
	alongDir -= thisNdc * along_w;
	orthoDir -= thisNdc * ortho_w;
	//divide by clip W to get NDC coords (screen coords are half of them)
	alongDir *= gl_FragCoord.w / 2;
	orthoDir *= gl_FragCoord.w / 2;
	//Note: if you want to check the math just above, consider how screen position changes when a point moves in specified direction:
	//  F(t) = divideByW(gl_ModelViewProjectionMatrix * (var_Position + dir_world * t)).xy
	//the converted vector must be equal to the derivative by parameter:
	//  dir_screen = dF/dt (0)
	//(here [dir_world, dir_screen] are either [alongDirW, alongDir] or [orthoDirW, orthoDir])

	//estimate the length of spot ellipse vectors (in pixels)
	float lenX = length(alongDir * texSize);
	float lenY = length(orthoDir * texSize);
	//make sure vectors are sufficiently sampled
	float avgSampleDistInPixels = 2 * max(1e-3 * texSize.y, 1.0);
	float oversize = max(lenX, lenY) / (avgSampleDistInPixels * sqrt(0.0 + u_softShadowsQuality));
	if (oversize > 1) {
		alongDir /= oversize;
		orthoDir /= oversize;
	}

	//compute partial derivatives of eye -Z by screen X and Y (normalized)
	float Z00 = depthToZ(u_projectionMatrix, gl_FragCoord.z);
	vec2 dzdxy = vec2(dFdx(Z00), dFdy(Z00));
	//rescale to derivatives by texture coordinates (not pixels)
	dzdxy *= texSize;
	//compute Z derivatives on a theoretical wall visible under 45-degree angle
	vec2 tanFovHalf = vec2(1.0 / u_projectionMatrix[0][0], 1.0 / u_projectionMatrix[1][1]);
	vec2 canonDerivs = 2.0 * Z00 * tanFovHalf;

	for( int i = 0; i < u_softShadowsQuality; i++ ) {
		vec2 delta = u_softShadowsSamples[i].x * alongDir + u_softShadowsSamples[i].y * orthoDir;
		vec2 StTc = baseTC + delta;
		float Zdiff = depthToZ(u_projectionMatrix, texture(u_depthTexture, StTc).r) - Z00;
		float tangentZdiff = dot(dzdxy, delta);
		float deg45diff = dot(canonDerivs, abs(delta));
		float weight = float(abs(Zdiff - tangentZdiff) <= abs(tangentZdiff) * 0.5 + deg45diff * 0.2);
		float StTex = float(texture( u_stencilTexture, StTc ).r);
		stencil += clamp( StLevel - StTex, 0., 1. ) * weight;
		sumWeight += weight;
	}
	FragColor.rgb *= stencil / sumWeight;
}

void main() {
	fetchDNS();
	FragColor.rgb = computeInteraction();
	if (u_shadows && u_softShadowsQuality > 0)
		StencilSoftShadow();
	FragColor.a = 1.0;
}
