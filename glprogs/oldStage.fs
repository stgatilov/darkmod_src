#version 140

uniform float u_screenTex;
uniform sampler2D u_tex0;

in vec4 var_TexCoord0;
in vec4 var_Color;

out vec4 FragColor;

void main() {
	vec4 tex;
	if (u_screenTex == 1.0) {
		tex = var_TexCoord0;
		tex.xy /= tex.w;
		tex = tex * 0.5 + 0.5;
		tex = texture(u_tex0, tex.xy);
	} else
		tex = textureProj(u_tex0, var_TexCoord0);
	FragColor = tex*var_Color;
}