#version 140
// !!ARBvp1.0 OPTION ARB_position_invariant ;

#pragma tdm_include "tdm_transform.glsl"

INATTR_POSITION  //in vec4 attr_Position;
out vec4 var_tc2;

void main() {
	
	// input:
	//
	// texcoord[0] TEX0    texcoords
	//
	// output:
	//
	// texture 0 is _currentRender
	//
	// texCoord[2] is the copied deform magnitude
	
	vec4 R0, R1;                                                                                        //TEMP    R0, R1;
	
	var_tc2 = (R1) * (vec4(1.0));                                                                       //MUL        result.texcoord[2], R1, 1.0;
	
	gl_Position = tdm_transform(attr_Position);
}
