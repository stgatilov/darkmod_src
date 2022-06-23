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

#pragma tdm_include "stages/depth/depth.params.glsl"

in float clipPlaneDist; 
in vec4 var_TexCoord0;
flat in int var_DrawId;
out vec4 FragColor;

uniform sampler2D u_texture;
vec4 textureAlpha() {
	return texture(u_texture, var_TexCoord0.st);
}

void main() {
	if (clipPlaneDist < 0.0)
		discard;
	uvec4 scissor = params[var_DrawId].scissor;
	// manual scissor test to avoid GL state change; needed to cull invisible parts of surfaces around visportals
	if (gl_FragCoord.x < scissor.x || gl_FragCoord.y < scissor.y || gl_FragCoord.x >= scissor.x + scissor.z || gl_FragCoord.y >= scissor.y + scissor.w)
		discard;
	
	if (params[var_DrawId].alphaTest < 0) {
		FragColor = params[var_DrawId].color;
	}
	else {
		vec4 tex = textureAlpha();
		if (tex.a <= params[var_DrawId].alphaTest)
			discard;
		FragColor = tex * params[var_DrawId].color;
	}
}
