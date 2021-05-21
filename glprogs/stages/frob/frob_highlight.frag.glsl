#version 330

in vec2 var_TexCoord;
out vec4 draw_Color;

uniform sampler2D u_diffuse;
uniform vec4 u_color;
uniform vec4 u_colorAdd;

void main() {
	vec4 diffuse = texture(u_diffuse, var_TexCoord);
	draw_Color.rgb = u_color.rgb * diffuse.rgb + u_colorAdd.rgb;
	draw_Color.a = diffuse.a;
}
