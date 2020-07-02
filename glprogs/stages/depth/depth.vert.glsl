#version 330 core

in vec4 attr_Position;
in vec2 attr_TexCoord;
in int attr_DrawId;

uniform ViewParamsBlock {
	uniform mat4 u_projectionMatrix;
};

#pragma tdm_include "stages/depth/depth.params.glsl"

uniform vec4 u_clipPlane;
uniform mat4 u_inverseView;

out float clipPlaneDist; 
out vec4 var_TexCoord0;
flat out int var_DrawId;

void main() {
	var_TexCoord0 = params[attr_DrawId].textureMatrix * vec4(attr_TexCoord, 0, 1);
	vec4 viewPos = params[attr_DrawId].modelViewMatrix * attr_Position;
	clipPlaneDist = dot(u_inverseView * viewPos, u_clipPlane);
	gl_Position = u_projectionMatrix * viewPos;
	var_DrawId = attr_DrawId;
}