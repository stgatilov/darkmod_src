#version 140
// !!ARBfp1.0

in vec4 var_tc0;
in vec4 var_tc1;
in vec4 var_tc5;
in vec3 var_viewDir;
in vec3 var_normal;

out vec4 draw_Color;
uniform sampler2D u_texture0;
uniform sampler2D u_texture1;
uniform sampler2D u_texture2;

uniform float u_pulse;

void main() {
    vec3 viewDir = normalize(var_viewDir);
    vec3 normal = normalize(var_normal);
    float x = dot(normal, viewDir); 
	draw_Color.rgb = u_pulse * vec3(1-x);
    draw_Color.a = 1;    
	draw_Color.rgb = abs(var_normal);
	//draw_Color.rgb = vec3(1,0,0);
}
