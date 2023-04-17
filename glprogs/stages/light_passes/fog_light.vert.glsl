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

in vec4 attr_Position;

out vec2 var_falloffTexcoord;
out vec2 var_enterTexcoord;

uniform mat4 u_modelViewMatrix;
uniform mat4 u_projectionMatrix;
uniform vec4 u_texPlaneFalloffS;
uniform vec4 u_texPlaneFalloffT;
uniform vec4 u_texPlaneEnterS;
uniform vec4 u_texPlaneEnterT;

void main() {
	gl_Position = objectPosToClip(attr_Position, u_modelViewMatrix, u_projectionMatrix);
	float s, t;

	s = dot(attr_Position, u_texPlaneFalloffS);
	t = dot(attr_Position, u_texPlaneFalloffT);
	var_falloffTexcoord = vec2(s, t);

	s = dot(attr_Position, u_texPlaneEnterS);
	t = dot(attr_Position, u_texPlaneEnterT);
	var_enterTexcoord = vec2(s, t);
}
