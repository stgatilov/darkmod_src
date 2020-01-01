#version 140

#pragma tdm_include "tdm_interaction.vs.glsl"

out vec3 var_WorldLightDir;

void main( void ) {
	interactionProcessVertex();
	var_WorldLightDir = (u_modelMatrix * attr_Position).xyz - u_lightOrigin2;
}
