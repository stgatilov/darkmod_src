#version 140

in vec4 attr_Position;
in vec2 attr_TexCoord;
out vec2 var_TexCoord;
out vec2 var_ViewRayXY;

uniform block {
	mat4 u_projectionMatrix;
};
vec2 halfTanFov = vec2(1 / u_projectionMatrix[0][0], 1 / u_projectionMatrix[1][1]);

void main() {
	gl_Position = attr_Position;
	var_TexCoord = attr_TexCoord;
	// prepare a part of the NDC to view space coordinate math here in the vertex shader to save instructions in the fragment shader
	var_ViewRayXY = -halfTanFov * (2 * attr_TexCoord - 1);
}
