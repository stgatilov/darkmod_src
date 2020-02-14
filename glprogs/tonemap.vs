#version 140

#pragma tdm_include "tdm_transform.glsl"

INATTR_POSITION
in vec4 attr_TexCoord;
out vec4 var_TexCoord;

void main() {
	gl_Position = tdm_transform(attr_Position);
	var_TexCoord = attr_TexCoord;
}
