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

in vec4 attr_Position;
in vec2 attr_TexCoord;
in int attr_DrawId;

uniform ViewParamsBlock {
	uniform mat4 u_projectionMatrix;
};

#pragma tdm_include "stages/depth/depth.params.glsl"

uniform vec4 u_clipPlane;
uniform mat4 u_inverseView;

out float clipPlaneDist; 
out vec4 var_TexCoord0;
flat out int var_DrawId;

void main() {
	var_TexCoord0 = params[attr_DrawId].textureMatrix * vec4(attr_TexCoord, 0, 1);
	vec4 viewPos = params[attr_DrawId].modelViewMatrix * attr_Position;
	clipPlaneDist = dot(u_inverseView * viewPos, u_clipPlane);
	gl_Position = u_projectionMatrix * viewPos;
	var_DrawId = attr_DrawId;
}