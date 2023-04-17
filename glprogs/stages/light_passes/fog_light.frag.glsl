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

uniform sampler2D u_fogFalloffImage;
uniform sampler2D u_fogEnterImage;
uniform vec3 u_fogColor;
uniform float u_fogAlpha;

in vec2 var_falloffTexcoord;
in vec2 var_enterTexcoord;

out vec4 FragColor;

void main() {
	vec4 falloffColor = texture(u_fogFalloffImage, var_falloffTexcoord);
	vec4 enterColor = texture(u_fogEnterImage, var_enterTexcoord);
	vec4 result = enterColor * falloffColor * vec4(u_fogColor, 1);
	result.a *= u_fogAlpha;
	FragColor = result;
}
