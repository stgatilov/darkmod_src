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

uniform mat4 u_modelViewMatrix;
uniform mat4 u_projectionMatrix;

in vec4 attr_Position;
in vec2 attr_TexCoord;
in vec3 attr_Tangent;
in vec3 attr_Bitangent;
in vec3 attr_Normal;

out vec2 var_TexCoord;
out vec3 var_PositionLocal;
out mat3 var_TangentToLocalMatrix;

void main() {
	gl_Position = objectPosToClip(attr_Position, u_modelViewMatrix, u_projectionMatrix);

	var_TangentToLocalMatrix = mat3(
		clamp(attr_Tangent, -1, 1),
		clamp(attr_Bitangent, -1, 1),
		clamp(attr_Normal, -1, 1)
	);

	var_PositionLocal = vec3(attr_Position);
	var_TexCoord = attr_TexCoord;
}
