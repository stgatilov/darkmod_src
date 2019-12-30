#version 140
// !!ARBvp1.0 OPTION ARB_position_invariant;

#pragma tdm_include "tdm_transform.glsl"

INATTR_POSITION  //in vec4 attr_Position;
in vec4 attr_TexCoord;
out vec4 var_tc0;
out vec4 var_tc1;
out vec4 var_tc2;
uniform vec4 u_localParam0;
uniform vec4 u_localParam1;

void main() {
	
	// basic texcoord
	var_tc0 = attr_TexCoord;                                                                            //MOV 	result.texcoord[0], vertex.texcoord[0];
	
	//input vertex parameter 0: for constrast multiplier
	var_tc1 = u_localParam0;                                                                            //MOV 	result.texcoord[1], program.local[0];
	
	//input vertex parameter 1: for minimum constrast
	var_tc2 = u_localParam1;                                                                            //MOV 	result.texcoord[2], program.local[1];
	
	gl_Position = tdm_transform(attr_Position);
}
