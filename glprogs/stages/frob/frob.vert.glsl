#version 330

#pragma tdm_include "tdm_transform.glsl"

in vec3 attr_Normal;
INATTR_POSITION  //in vec4 attr_Position;

uniform float u_depth;

void main() {
	vec4 transformed = tdm_transform(attr_Position);
	transformed.z -= u_depth * transformed.w;
	gl_Position = transformed;
}
