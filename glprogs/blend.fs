#version 140

uniform sampler2D u_texture0;
uniform sampler2D u_texture1;
uniform vec4 u_blendColor;

in vec4 var_TexCoord0;
in vec4 var_TexCoord1;

out vec4 FragColor;

void main() {
	vec4 texel0 = texture(u_texture0, var_TexCoord0.xy);
	vec4 texel1 = texture(u_texture1, var_TexCoord1.xy);
	FragColor = u_blendColor*texel0*texel1;	
}