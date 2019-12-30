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
