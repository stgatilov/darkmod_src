#version 140

in vec4 attr_Position;
in vec2 attr_TexCoord;
out vec2 var_TexCoord;

void main() {
	gl_Position = attr_Position;
	var_TexCoord = attr_TexCoord;
}
