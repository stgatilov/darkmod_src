#version 140

uniform sampler2D u_bloomTexture;

in float overbright;

out vec4 FragColor;

void main() {
	vec2 tc = gl_FragCoord.xy / textureSize(u_bloomTexture, 0);
	FragColor = texture(u_bloomTexture, tc);
	if(overbright > 0) {
		FragColor.rgb *= mix(1,dot(FragColor.rgb,vec3(0.3,0.5,0.2)),sqrt(overbright));
	}
	if(overbright < 0) {
		FragColor.rgb *= 1-overbright;
	}
	//FragColor = texelFetch(u_bloomTexture, ivec2(tcMax*tc), mip);
	if(gl_FragCoord.x < 99*abs(overbright))
		FragColor[overbright>0?0:1] = 1;
//	FragColor.rgb = vec3(overbright.g);
}