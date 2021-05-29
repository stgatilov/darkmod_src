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

#pragma tdm_include "tdm_transform.glsl"

INATTR_POSITION  //in vec4 attr_Position;
in vec4 attr_Color;

out vec4 var_Color;
  
uniform vec4 u_lightOrigin;

void main( void ) {
	if( attr_Position.w == 1.0 ) {
		// mvp transform into clip space     
		gl_Position = tdm_transform(attr_Position); 
	} else {
		// project vertex position to infinity
		vec4 vertex = attr_Position - u_lightOrigin;
		mat4 modelViewProjectionMatrix = u_projectionMatrix*u_modelViewMatrix;
		gl_Position = modelViewProjectionMatrix * vertex;
	}

	// primary color
	var_Color = attr_Color; 
}