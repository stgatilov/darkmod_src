#version 330

#pragma tdm_include "tdm_transform.glsl"

in vec3 attr_Normal;
in vec4 attr_TexCoord;
INATTR_POSITION  //in vec4 attr_Position;

out vec2 var_TexCoord;

uniform float u_depth;
uniform vec4 u_texMatrix[2];

void main() {
	vec4 transformed = tdm_transform(attr_Position);
	transformed.z -= u_depth * transformed.w;
	gl_Position = transformed;
	var_TexCoord.x = dot(attr_TexCoord, u_texMatrix[0]);
	var_TexCoord.y = dot(attr_TexCoord, u_texMatrix[1]);
}
