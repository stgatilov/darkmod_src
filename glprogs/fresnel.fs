#version 140
// !!ARBfp1.0

in vec4 var_tc2;
out vec4 draw_Color;
uniform sampler2D u_texture0;
uniform vec4 u_scaleWindowToUnit;

void main() {
	// OPTION ARB_precision_hint_fastest;
	
	vec4 A, C;                                                                                          //TEMP A, C;
	
	A = (gl_FragCoord) * (u_scaleWindowToUnit);                                                         //MUL A, fragment.position, program.env[1];
	
	C = texture(u_texture0, A.xy);                                                                      //TEX C, A, texture[0], 2D;
	
	draw_Color = C;                                                                                     //MOV result.color, C;
	
	draw_Color.a = var_tc2.x;                                                                           //MOV result.color.a, fragment.texcoord[2].x;
	
}
