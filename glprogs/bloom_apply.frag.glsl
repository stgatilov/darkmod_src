#version 140

in vec2 var_TexCoord;
out vec4 draw_Color;

uniform sampler2D u_texture;
uniform sampler2D u_bloomTex;
uniform float u_bloomWeight;

void main() {
	vec4 color = texture(u_texture, var_TexCoord.xy);
	vec4 bloom = texture(u_bloomTex, var_TexCoord.xy);
	draw_Color = color + u_bloomWeight * bloom;
}
