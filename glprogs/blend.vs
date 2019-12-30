#version 140

#pragma tdm_include "tdm_transform.glsl"

INATTR_POSITION  //in vec4 attr_Position;

uniform vec4 u_tex0PlaneS;
uniform vec4 u_tex0PlaneT;
uniform vec4 u_tex0PlaneQ;
uniform vec4 u_tex1PlaneS;

out vec4 var_TexCoord0;
out vec4 var_TexCoord1;

void main() {
	float s = dot(attr_Position, u_tex0PlaneS);
	float t = dot(attr_Position, u_tex0PlaneT);
	float q = dot(attr_Position, u_tex0PlaneQ);
	var_TexCoord0 = u_textureMatrix * vec4(s, t, 0, q);
	s = dot(attr_Position, u_tex1PlaneS);
	var_TexCoord1 = u_textureMatrix * vec4(s, 0.5, 0, 1);
	gl_Position = tdm_transform(attr_Position);
}
