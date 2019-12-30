#version 140

uniform sampler2D u_tex0;
uniform float u_alphaTest;
uniform vec4 u_color;

in vec2 texCoord;

void main() {   
	if (u_alphaTest >= 0) {
		vec4 tex = texture(u_tex0, texCoord);
		if (tex.a <= u_alphaTest)
			discard;
	}
//	gl_FragColor = u_color;
}
