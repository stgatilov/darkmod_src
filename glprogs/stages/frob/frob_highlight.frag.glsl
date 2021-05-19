#version 330

in vec2 var_TexCoord;
out vec4 draw_Color;

uniform sampler2D u_diffuse;
uniform vec4 u_color;
uniform vec4 u_colorAdd;

void main() {
	vec3 diffuse = texture(u_diffuse, var_TexCoord).rgb;
	draw_Color.rgb = u_color.rgb * diffuse + u_colorAdd.rgb;
	draw_Color.a = 1;
}
