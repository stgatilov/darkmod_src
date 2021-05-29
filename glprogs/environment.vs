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
// !!ARBvp1.0 OPTION ARB_position_invariant ;

#pragma tdm_include "tdm_transform.glsl"

in vec3 attr_Normal;
in vec4 attr_Color;
INATTR_POSITION  //in vec4 attr_Position;
out vec4 var_color;
out vec4 var_tc0;
out vec4 var_tc1;
uniform vec4 u_viewOriginLocal;

void main() {
	
	// env[5] is the eye position in local coordinates
	// env[16-17] control if color is by vertex or global
	
	//MOV	result.texcoord[0], vertex.normal;
	var_tc0 = vec4(attr_Normal, 1);                                                                     //MOV	result.texcoord[0], vertex.attrib[2];
	var_tc1 = (u_viewOriginLocal) - (attr_Position);                                                    //SUB	result.texcoord[1], program.env[5], vertex.position;
	
	var_color = attr_Color;                                                                             //MOV	result.color, vertex.color;
	//MAD	result.color, vertex.color, program.env[16], program.env[17];
	
	gl_Position = tdm_transform(attr_Position);
}
