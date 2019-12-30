#version 140
// !!ARBfp1.0 

in vec4 var_tc0;
out vec4 draw_Color;
uniform sampler2D u_texture0;
uniform sampler2D u_texture1;
uniform vec4 u_scalePotToWindow;
uniform vec4 u_scaleWindowToUnit;

void main() {
	// OPTION ARB_precision_hint_fastest;
	
	vec4 xero = vec4(0.0, 0.0, 0.0, 1.0);                                                               //PARAM 	xero = { 0.0, 0.0, 0.0 };
	
	vec4 R1, R2, W, TC1;                                                                                //TEMP R1, R2, W, TC1;
	
	//## convert pixel position to texcoord
	TC1 = (gl_FragCoord) * (u_scaleWindowToUnit);                                                       //MUL	TC1, fragment.position, program.env[1];
	TC1 = (TC1) * (u_scalePotToWindow);                                                                 //MUL	TC1, TC1, program.env[0];
	
	//## fetch _fsfx_input
	R1 = texture(u_texture0, var_tc0.xy);                                                               //TEX	R1, fragment.texcoord[0], texture[0], 2D; 
	
	//## _afxweight
	W = texture(u_texture1, xero.xy);                                                                   //TEX	W, xero, texture[1], 2D;
	W = (W) * (vec4(1.55));                                                                             //MUL	W, W, 1.55;
	R1 = (R1) * (W);                                                                                    //MUL	R1, R1, W;
	R2 = (R1) * (vec4(0.5));                                                                            //MUL	R2, R1, 0.5;
	
	//## modulate
	R1 = (R1) * (R1);                                                                                   //MUL	R1, R1, R1;
	//ADD	R1, R1, R2;
	draw_Color = (R1) + (R2);                                                                           //ADD	result.color, R1, R2;
	//MUL	result.color, R1, W;
	
}
