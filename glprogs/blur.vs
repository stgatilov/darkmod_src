#version 140
// !!ARBvp1.0

#pragma tdm_include "tdm_transform.glsl"

INATTR_POSITION  //in vec4 attr_Position;

void main() {
	// OPTION ARB_position_invariant;
	
	gl_Position = tdm_transform(attr_Position);
}
