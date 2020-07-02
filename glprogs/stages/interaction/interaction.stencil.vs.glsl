#version 330 core

#pragma tdm_include "stages/interaction/interaction.common.vs.glsl"

void main( void ) {
	interactionProcessVertex();
}
