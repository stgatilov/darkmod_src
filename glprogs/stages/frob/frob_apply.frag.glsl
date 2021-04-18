#version 330

in vec2 var_TexCoord;
out vec4 FragColor;

uniform sampler2D u_source;
uniform vec4 u_color;

void main() {
    float outline = texture(u_source, var_TexCoord).r;
	FragColor = vec4(u_color.rgb, u_color.a * outline);
}
