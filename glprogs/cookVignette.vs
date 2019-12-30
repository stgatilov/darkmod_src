#version 140
// !!ARBvp1.0 OPTION ARB_position_invariant;

#pragma tdm_include "tdm_transform.glsl"

INATTR_POSITION  //in vec4 attr_Position;
in vec4 attr_TexCoord;
out vec4 var_tc0;
out vec4 var_tc1;
uniform vec4 u_localParam0;

void main() {
	//---------------------------------------------------------
	//	drawVignette.vfp
	//	Author:		JC Denton
	//	Draws a vignette.
	//---------------------------------------------------------
	
	vec4 texCoordFix = vec4(1.0, 1, 0.0, 0.0);                                                          //PARAM texCoordFix = { 1.0, 1, 0.0, 0.0 };
	
	// basic texcoord
	var_tc0 = (attr_TexCoord) * (texCoordFix);                                                          //MUL 	result.texcoord[0], vertex.texcoord[0], texCoordFix;
	//MOV 	result.texcoord[0], vertex.texcoord[0];
	
	//Store Parameters sent to this shader to Result.
	var_tc1 = u_localParam0;                                                                            //MOV 	result.texcoord[1], program.local[0];
	
	gl_Position = tdm_transform(attr_Position);
}
