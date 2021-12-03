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
#version 140

#define MAX_LIGHTS 16

uniform mat4 u_modelMatrix;

uniform int u_lightCount;
uniform float u_shadowTexelStep;
uniform vec3 u_lightOrigin[MAX_LIGHTS];
uniform vec4 u_shadowRect[MAX_LIGHTS];

in vec4 attr_Position;
in vec4 attr_TexCoord;

out vec2 texCoord;

const mat3 cubicTransformations[6] = mat3[6] (
	mat3(
		0, 0, -1,
		0, -1, 0,
		-1, 0, 0
	),
	mat3(
		0, 0, 1,
		0, -1, 0,
		1, 0, 0
	),
	mat3(
		1, 0, 0,
		0, 0, -1,
		0, 1, 0
	),
	mat3(
		1, 0, 0,
		0, 0, 1,
		0, -1, 0
	),
	mat3(
		1, 0, 0,
		0, -1, 0,
		0, 0, -1
	),
	mat3(
		-1, 0, 0,
		0, -1, 0,
		0, 0, 1
	)
);

const float clipEps = 0e-2;
const vec4 ClipPlanes[4] = vec4[4] (
	vec4(1, 0, -1, clipEps),
	vec4(-1, 0, -1, clipEps),
	vec4(0, 1, -1, clipEps),
	vec4(0, -1, -1, clipEps)
);
 
// I suspect that the right way is to cook the projection matrices for each page on CPU and pass as uniforms to avoid this mess
vec2 ShadowAtlasForVector(vec3 v, int lightNo, int targetFace) {
	vec2 shadow2d = v.xy*.5 + vec2(-v.z)*.5;
	float pageSize = u_shadowRect[lightNo].w + u_shadowTexelStep;
	vec2 pageOrigin = u_shadowRect[lightNo].xy;
	pageOrigin.x += pageSize * targetFace;
	pageOrigin = pageOrigin * 2 - 1;
	shadow2d *= u_shadowRect[lightNo].w*2 + u_shadowTexelStep*5e-3; // FIXME rasterization rules workaround
	shadow2d.xy += -v.z * pageOrigin;
	return vec2(shadow2d);
}

void main() {
	gl_Position = u_modelMatrix * attr_Position;
	//gl_Position = gl_Vertex;
	texCoord = (attr_TexCoord).st;

	int lightNo = gl_InstanceID / 6;
	vec3 inLightSpace = gl_Position.xyz - u_lightOrigin[lightNo];
    int j = gl_InstanceID % 6;

		vec4 clip;
		vec4 clipCount = vec4(0);
			vec4 inCubeFaceSpace = vec4(cubicTransformations[j] * inLightSpace, 1);
			for(int k=0; k<4; k++) {
				clip[k] = dot(inCubeFaceSpace, ClipPlanes[k]);
				clipCount[k] += float(clip[k] < 0);
			}
			gl_Position.xy = ShadowAtlasForVector(inCubeFaceSpace.xyz, lightNo, j);
            gl_Position.z = (-inCubeFaceSpace.z - 2);
            gl_Position.w = -inCubeFaceSpace.z;
			// clip the triangle to the atlas page 
			gl_ClipDistance[0] = clip.x;
			gl_ClipDistance[1] = clip.y;
			gl_ClipDistance[2] = clip.z;
			gl_ClipDistance[3] = clip.w;
}