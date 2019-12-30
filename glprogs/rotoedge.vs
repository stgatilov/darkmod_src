#version 140
// !!ARBvp1.0 OPTION ARB_position_invariant ;

#pragma tdm_include "tdm_transform.glsl"

INATTR_POSITION  //in vec4 attr_Position;
out vec4 var_tc0;
uniform vec4 u_localParam0;

void main() {
	
	//from sobel filter by John Rittenhouse
	vec4 size = vec4(0.0016);                                                                           //PARAM size = 0.0016;
	var_tc0 = (size) * (u_localParam0);                                                                 //MUL	result.texcoord[0], size, program.local[0];
	
	gl_Position = tdm_transform(attr_Position);
}
