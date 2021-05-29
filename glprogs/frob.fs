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
// !!ARBfp1.0

in vec4 var_tc0;
in vec4 var_tc1;
in vec4 var_tc5;
in vec3 var_viewDir;
in vec3 var_normal;

out vec4 draw_Color;
uniform sampler2D u_texture0;
uniform sampler2D u_texture1;
uniform sampler2D u_texture2;

uniform float u_pulse;

void main() {
    vec3 viewDir = normalize(var_viewDir);
    vec3 normal = normalize(var_normal);
    float x = dot(normal, viewDir); 
	draw_Color.rgb = u_pulse * vec3(1-x);
    draw_Color.a = 1;
//	draw_Color.rgb = abs(var_normal);
	//draw_Color.rgb = vec3(1,0,0);
}
