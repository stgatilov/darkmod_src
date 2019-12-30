#version 140
// !!ARBvp1.0 OPTION ARB_position_invariant;

#pragma tdm_include "tdm_transform.glsl"

INATTR_POSITION  //in vec4 attr_Position;
in vec4 attr_TexCoord;
out vec4 var_tc0;
out vec4 var_tc1;
out vec4 var_tc2;
out vec4 var_tc3;
out vec4 var_tc4;
uniform vec4 u_localParam0;
uniform vec4 u_localParam1;
uniform vec4 u_localParam2;
uniform vec4 u_localParam3;

void main() {
	
	// basic texcoord
	var_tc0 = attr_TexCoord;                                                                            //MOV 	result.texcoord[0], vertex.texcoord[0];
	
	//input vertex parameter 0: middle Gray value.
	var_tc1 = u_localParam0;                                                                            //MOV 	result.texcoord[1], program.local[0];
	
	//input vertex parameter 1: minimum luminance.
	var_tc2 = u_localParam1;                                                                            //MOV 	result.texcoord[2], program.local[1];
	
	//input vertex parameter 2: brightpass threshold.
	var_tc3 = u_localParam2;                                                                            //MOV 	result.texcoord[3], program.local[2];
	
	//input vertex parameter 3: brightpass offset.
	var_tc4 = u_localParam3;                                                                            //MOV 	result.texcoord[4], program.local[3];
	gl_Position = tdm_transform(attr_Position);
}
