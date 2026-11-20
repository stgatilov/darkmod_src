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

// Penumbra pre-pass for soft stencil shadows:
// for every pixel in light scissor, finds blockers in the depth buffer (screen-space) and stores penumbra size factors.
// The interaction shader reads them for the pixel itself and for every tap of its blur (so that taps of narrow
// penumbras do not influence wide penumbras and vice versa).

#pragma tdm_include "tdm_utils.glsl"
#pragma tdm_include "tdm_shadowstencilsoft_ssabss.glsl"

in vec2 var_TexCoord;
out vec4 FragColor;

uniform sampler2D u_depthTexture;
uniform mat4 u_projectionMatrix;
uniform vec3 u_lightPosView;	// light origin in view space
uniform float u_softRadius;
uniform int u_softQuality;

void main() {
	vec2 texSize = vec2(textureSize(u_depthTexture, 0));
	vec2 pixSize = vec2(1.0) / texSize;
	vec2 uv = gl_FragCoord.xy * pixSize;
	float depth = texelFetch(u_depthTexture, ivec2(gl_FragCoord.xy), 0).r;

	if (depth >= 1.0) {
		// no geometry (sky, hole): nothing is lit here
		FragColor = vec4(1.0, 1.0, 0.0, 1.0);
		return;
	}

	vec3 P_v = viewPosFromUvDepth(u_projectionMatrix, uv, depth);
	// geometric normal from depth derivatives (the interaction shader uses interpolated vertex normal, which is not available here)
	vec3 N_v = cross(dFdx(P_v), dFdy(P_v));
	float nLen = length(N_v);
	N_v = nLen > 1e-8 ? N_v / nLen : normalize(-P_v);
	if (dot(N_v, -P_v) < 0.0)
		N_v = -N_v;

	vec3 toLight = u_lightPosView - P_v;
	float rawLightDist = max(length(toLight), 1e-3);
	vec3 L_dir = toLight / rawLightDist;

	// the same noise as in the interaction shader
	float ign2 = interleavedGradientNoise(gl_FragCoord.xy + vec2(5.588238, 5.588238));
	vec2 k = ssabssContactFactors(
		u_depthTexture, u_projectionMatrix,
		P_v, N_v, L_dir, rawLightDist,
		u_softRadius, u_softQuality, ign2,
		0.5 * pixSize, vec2(1.0) - 0.5 * pixSize
	);
	FragColor = vec4(k, 0.0, 1.0);
}
