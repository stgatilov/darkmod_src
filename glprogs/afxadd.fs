#version 140
// !!ARBfp1.0 

in vec4 var_tc0;
in vec4 var_tc1;
in vec4 var_tc2;
out vec4 draw_Color;
uniform sampler2D u_texture0;
uniform sampler2D u_texture1;
uniform vec4 u_scalePotToWindow;
uniform vec4 u_scaleWindowToUnit;

void main() {
	// OPTION ARB_precision_hint_fastest;
	
	vec4 R1, R2, TC1;                                                                                   //TEMP R1, R2, TC1;
	
	//## convert pixel position to texcoord
	TC1 = (gl_FragCoord) * (u_scaleWindowToUnit);                                                       //MUL	TC1, fragment.position, program.env[1];
	TC1 = (TC1) * (u_scalePotToWindow);                                                                 //MUL	TC1, TC1, program.env[0];
	
	//## fetch _fsfx_input
	R2 = texture(u_texture1, var_tc0.xy);                                                               //TEX	R2, fragment.texcoord[0], texture[1], 2D; 
	
	//## fetch _currentRender
	R1 = texture(u_texture0, TC1.xy);                                                                   //TEX	R1, TC1, texture[0], 2D;
	
	//multiply _fsfx_input with blur multiplier
	R2 = (R2) * (var_tc1);                                                                              //MUL	R2, R2, fragment.texcoord[1];
	
	//multiply _currentrender with blur multiplier
	R1 = (R1) * (var_tc2);                                                                              //MUL	R1, R1, fragment.texcoord[2];
	
	draw_Color = (R1) + (R2);                                                                           //ADD	result.color, R1, R2;
	
}
