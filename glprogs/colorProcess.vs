#version 140
// !!ARBvp1.0

#pragma tdm_include "tdm_transform.glsl"

INATTR_POSITION  //in vec4 attr_Position;
out vec4 var_tc0;
out vec4 var_tc1;
uniform vec4 u_localParam0;
uniform vec4 u_localParam1;

void main() {
	// OPTION ARB_position_invariant ;
	
	// parameter 0 is the fraction from the current hue to the target hue to map
	// parameter 1.rgb is the target hue
	// texture 0 is _currentRender
	
	// nothing to do but pass the parameters along
	
	// 1 - fraction
	var_tc0 = (vec4(1.0)) - (u_localParam0);                                                            //SUB		result.texcoord[0], 1.0, program.local[0];
	
	// fraction * target color
	var_tc1 = (u_localParam1) * (u_localParam0);                                                        //MUL		result.texcoord[1], program.local[1], program.local[0];
	
	gl_Position = tdm_transform(attr_Position);
}
