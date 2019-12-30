#version 140

uniform sampler2D u_tex0;
uniform float u_alphaTest;
uniform vec4 u_color;

in float clipPlaneDist; 
in vec4 var_TexCoord0;

out vec4 FragColor;

void main() {
	if (clipPlaneDist < 0.0)
		discard;
	if (u_alphaTest < 0)
		FragColor = u_color;
	else {
		vec4 tex = texture(u_tex0, var_TexCoord0.st);
		if (tex.a <= u_alphaTest)
			discard;
		FragColor = tex*u_color;	
	}
}