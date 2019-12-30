#version 140
// !!ARBfp1.0 

in vec4 var_tc0;
in vec4 var_tc1;
in vec4 var_tc2;
out vec4 draw_Color;
uniform sampler2D u_texture0;
uniform sampler2D u_texture1;

void main() {
	// OPTION ARB_precision_hint_fastest;
	
	vec4 xero = vec4(0.0, 0.0, 0.0, 1.0);                                                               //PARAM 	xero = { 0.0, 0.0, 0.0 };
	
	vec4 R1, W;                                                                                         //TEMP R1, W;
	
	//### fetch _fsfx_input
	R1 = texture(u_texture0, var_tc0.xy);                                                               //TEX	R1, fragment.texcoord[0], texture[0], 2D; 
	
	//## _afxweight
	W = texture(u_texture1, xero.xy);                                                                   //TEX	W, xero, texture[1], 2D;
	// Multiply the weight with constrast multiplier.
	W = (W) * (var_tc1);                                                                                //MUL	W, W, fragment.texcoord[1];
	// Make Sure the weight is greater than our minimum expected value.
	W = max(W, var_tc2);                                                                                //MAX	W, W, fragment.texcoord[2];
	R1 = (R1) * (W);                                                                                    //MUL	R1, R1, W;
	
	draw_Color = (R1) * (R1);                                                                           //MUL	result.color, R1, R1;
	
}
