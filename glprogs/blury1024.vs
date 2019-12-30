#version 140
// !!ARBvp1.0 OPTION ARB_position_invariant;

#pragma tdm_include "tdm_transform.glsl"

INATTR_POSITION  //in vec4 attr_Position;
in vec4 attr_TexCoord;
out vec4 var_tc0;

void main() {
	
	var_tc0 = attr_TexCoord;                                                                            //MOV result.texcoord[0], vertex.texcoord[0];
	
	gl_Position = tdm_transform(attr_Position);
}
