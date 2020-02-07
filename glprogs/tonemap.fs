#version 140

in vec4 var_TexCoord;
out vec4 draw_Color;
uniform sampler2D u_texture;
uniform float u_gamma;
uniform float u_brightness;

void main() {
	vec4 color = texture(u_texture, var_TexCoord.xy);
	color.r = pow(color.r, 1.0/u_gamma) * u_brightness;
	color.g = pow(color.g, 1.0/u_gamma) * u_brightness;
	color.b = pow(color.b, 1.0/u_gamma) * u_brightness;
	draw_Color = color;
}
