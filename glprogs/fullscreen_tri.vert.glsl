#version 330

out vec2 var_TexCoord;

void main() {
    var_TexCoord.x = gl_VertexID == 1 ? 2 : 0;
    var_TexCoord.y = gl_VertexID == 2 ? 2 : 0;
    
    gl_Position = vec4(var_TexCoord * 2 - 1, 1, 1);
}
