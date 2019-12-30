#version 140

#pragma tdm_include "tdm_transform.glsl"

INATTR_POSITION  //in vec4 attr_Position;

uniform vec4 u_tex0PlaneS;
uniform vec4 u_tex1PlaneT;
uniform float u_fogEnter;

uniform mat4 u_modelMatrix;

out vec4 var_TexCoord0;
out vec4 var_TexCoord1;
out vec4 var_worldPosition;

void main() {
	float s = dot(attr_Position, u_tex0PlaneS);
	float t = 0.5;
	var_TexCoord0 = u_textureMatrix * vec4(s, t, 0, 1);
	s = u_fogEnter;
	t = dot(attr_Position, u_tex1PlaneT);
	var_TexCoord1 = u_textureMatrix * vec4(s, t, 0, 1);
	gl_Position = tdm_transform(attr_Position);
	var_worldPosition = u_modelMatrix * attr_Position;
}