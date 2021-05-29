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
#version 150

#define MAX_LIGHTS 16

uniform mat4 u_modelMatrix;

uniform int u_lightCount;
uniform mat4 u_lightProjectionFalloff[MAX_LIGHTS];

in vec4 attr_Position;
in vec4 attr_TexCoord;

out VertOutput {
	vec2 texCoord;
	ivec2 clipX;
	ivec2 clipY;
} vert;

void main() {
	gl_Position = u_modelMatrix * attr_Position;
	vert.clipX = vert.clipY = ivec2(0);
	for(int i = 0; i<u_lightCount; i++) {
		vec3 b0 = u_lightProjectionFalloff[i][0].xyz;
		vec3 b1 = u_lightProjectionFalloff[i][1].xyz;
		vec3 v = gl_Position.xyz;
		if(v.x < b0.x)
			vert.clipX.x |= 1 << i;
		if(v.x > b1.x)
			vert.clipX.y |= 1 << i;
		if(v.y < b0.y)
			vert.clipY.x |= 1 << i;
		if(v.y > b1.y)
			vert.clipX.y |= 1 << i;
	}
	vert.texCoord = (attr_TexCoord).st;
}