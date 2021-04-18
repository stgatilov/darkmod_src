#version 330

out vec4 draw_Color;

uniform vec4 u_color;

void main() {
    draw_Color = u_color;
}
