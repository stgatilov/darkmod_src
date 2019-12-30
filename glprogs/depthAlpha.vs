#version 140

#pragma tdm_include "tdm_transform.glsl"

INATTR_POSITION  //in vec4 attr_Position;

uniform vec4 u_clipPlane;
uniform mat4 u_matViewRev;

in vec2 attr_TexCoord;

out float clipPlaneDist; 
out vec4 var_TexCoord0;

void main() {
	var_TexCoord0 = u_textureMatrix * vec4(attr_TexCoord, 0, 1);
	clipPlaneDist = dot(u_matViewRev * u_modelViewMatrix * attr_Position, u_clipPlane);
	gl_Position = tdm_transform(attr_Position);
}