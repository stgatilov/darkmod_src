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

uniform vec4 u_lightOrigin;
uniform mat4 u_modelMatrix;

in vec4 attr_Position;
in vec4 attr_TexCoord;

out vec2 texCoord;
out float gl_ClipDistance[4];

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

void main() {
	texCoord = (attr_TexCoord).st;
	gl_Position = u_modelMatrix * attr_Position - u_lightOrigin;
            vec4 frag_pos = vec4(cubicTransformations[gl_InstanceID] * gl_Position.xyz, 1);
            gl_Position.x = frag_pos.x / 6 + frag_pos.z * 5/6 - frag_pos.z / 3 * gl_InstanceID;
            gl_Position.y = frag_pos.y;
            gl_Position.z = -frag_pos.z - 2;
            gl_Position.w = -frag_pos.z;
			gl_ClipDistance[0] = dot(frag_pos, ClipPlanes[0]);
			gl_ClipDistance[1] = dot(frag_pos, ClipPlanes[1]);
			gl_ClipDistance[2] = dot(frag_pos, ClipPlanes[2]);
			gl_ClipDistance[3] = dot(frag_pos, ClipPlanes[3]);
}