#version 140
// !!ARBfp1.0 

in vec4 var_tc0;
out vec4 draw_Color;
uniform sampler2D u_texture0;
uniform sampler2D u_texture1;

void main() {
	// OPTION ARB_precision_hint_fastest;
	
	vec4 xero = vec4(0.0, 0.0, 0.0, 1.0);                                                               //PARAM 	xero = { 0.0, 0.0, 0.0 };
	
	vec4 R1, R2, W;                                                                                     //TEMP R1, R2, W;
	
	//### fetch _fsfx_input
	R1 = texture(u_texture0, var_tc0.xy);                                                               //TEX	R1, fragment.texcoord[0], texture[0], 2D; 
	
	//## _afxweight
	W = texture(u_texture1, xero.xy);                                                                   //TEX	W, xero, texture[1], 2D;
	W = (W) * (vec4(1.55));                                                                             //MUL	W, W, 1.55;
	
	R1 = (R1) * (W);                                                                                    //MUL	R1, R1, W;
	
	draw_Color = (R1) * (R1);                                                                           //MUL	result.color, R1, R1;
	
}
