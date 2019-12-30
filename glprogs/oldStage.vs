#version 140

#pragma tdm_include "tdm_transform.glsl"

INATTR_POSITION  //in vec4 attr_Position;
in vec2 attr_TexCoord;
in vec4 attr_Color;

uniform vec4 u_colorMul;
uniform vec4 u_colorAdd;
uniform float u_screenTex;

out vec4 var_TexCoord0;
out vec4 var_Color;

void main() {
	gl_Position = tdm_transform(attr_Position);
	var_Color = attr_Color * u_colorMul + u_colorAdd;
	if (u_screenTex == 1.0) 
		var_TexCoord0 = gl_Position;
	else
		var_TexCoord0 = u_textureMatrix * vec4(attr_TexCoord, 0, 1);
}