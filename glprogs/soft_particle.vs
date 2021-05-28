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
// !!ARBvp1.0

#pragma tdm_include "tdm_transform.glsl"

in vec4 attr_Color;
INATTR_POSITION  //in vec4 attr_Position;
in vec4 attr_TexCoord;
out vec4 var_color;
out vec4 var_tc0;

void main() {
	// OPTION ARB_position_invariant;
	var_tc0 = attr_TexCoord;                                                                            //MOV	   result.texcoord, vertex.attrib[8];
	var_color = attr_Color;                                                                             //MOV	   result.color, vertex.attrib[3];
	gl_Position = tdm_transform(attr_Position);
}
