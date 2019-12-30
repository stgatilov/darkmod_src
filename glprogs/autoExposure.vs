#version 140

#pragma tdm_include "tdm_transform.glsl"

INATTR_POSITION  //in vec4 attr_Position;

out float overbright;

uniform sampler2D u_bloomTexture;
  
const int mip = 6;

void main( void ) {
	gl_Position = attr_Position;

	ivec2 tcMax = textureSize(u_bloomTexture, mip);
	overbright = 0;
	int clip = (tcMax.x - tcMax.y) / 2;
	ivec2 tci;
	for(tci.x=clip; tci.x<tcMax.x-clip; tci.x++) {
	for(tci.y=0; tci.y<tcMax.y; tci.y++) {
		float bright = dot(texelFetch(u_bloomTexture, tci, mip).rgb, vec3(0.3,0.5,0.2));
		if(bright > overbright)
			overbright += .1*(bright-overbright);
	}
	}
	overbright -= .5;
}