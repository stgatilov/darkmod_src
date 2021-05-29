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

uniform ViewParamsBlock {
	uniform mat4 u_projectionMatrix;
};

#pragma tdm_define "MAX_SHADER_PARAMS"

struct PerDrawCallParams {
	mat4 modelViewMatrix;
	vec4 localLightOrigin;
};

layout (std140) uniform PerDrawCallParamsBlock {
	PerDrawCallParams params[MAX_SHADER_PARAMS];
};

in vec4 attr_Position;
in vec4 attr_Color;
in int attr_DrawId;

out vec4 var_Color;
  
void main( void ) {
	vec4 projectedPosition = attr_Position;
	if( attr_Position.w != 1.0 ) {
		// project vertex position to infinity
		projectedPosition -= params[attr_DrawId].localLightOrigin;
	}

	gl_Position = u_projectionMatrix * (params[attr_DrawId].modelViewMatrix * projectedPosition);

	// primary color
	var_Color = attr_Color; 
}
