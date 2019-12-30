#version 430

layout(location = 0) in vec4 attr_Vertex;

layout (location = 0) uniform mat4[2] u_MVP;

out vec4 csThis;
out vec4 lightProject;
out vec4 worldPosition;

void main() {
	gl_Position = u_MVP[1] * u_MVP[0] * attr_Vertex;
	worldPosition = attr_Vertex;
	// fragment position in clip space
	csThis = u_MVP[1] * u_MVP[0] * attr_Vertex;
}