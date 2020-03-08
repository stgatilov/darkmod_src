#version 140

// display the occlusion channel from the SSAO texture for debug purposes

out vec3 FragColor;

uniform sampler2D u_source;

void main() {
	ivec2 ssC = ivec2(gl_FragCoord.xy);

	vec4 ssao = texelFetch(u_source, ssC, 0);

	FragColor = vec3(ssao.r, ssao.r, ssao.r);
}
