#version 140
// !!ARBfp1.0 

out vec4 draw_Color;
uniform sampler2D u_texture0;
uniform sampler2D u_texture1;

void main() {
	// OPTION ARB_precision_hint_fastest;
	
	//##
	
	vec4 xero = vec4(0.0, 0.0, 0.0, 1.0);                                                               //PARAM	xero = { 0.0, 0.0, 0.0 };
	
	// measure points
	vec4 w1 = vec4(0.3, 0.5, 0.0, 1.0);                                                                 //PARAM	w1 = { 0.3, 0.5 };
	vec4 w2 = vec4(0.7, 0.5, 0.0, 1.0);                                                                 //PARAM	w2 = { 0.7, 0.5 };
	vec4 w3 = vec4(0.5, 0.7, 0.0, 1.0);                                                                 //PARAM	w3 = { 0.5, 0.7 };
	vec4 w4 = vec4(0.5, 0.3, 0.0, 1.0);                                                                 //PARAM	w4 = { 0.5, 0.3 };
	
	vec4 w5 = vec4(0.5, 0.5, 0.0, 1.0);                                                                 //PARAM	w5 = { 0.5, 0.5 }; 
	
	vec4 w6 = vec4(0.45, 0.55, 0.0, 1.0);                                                               //PARAM	w6 = { 0.45, 0.55 };
	vec4 w7 = vec4(0.55, 0.55, 0.0, 1.0);                                                               //PARAM	w7 = { 0.55, 0.55 };
	vec4 w8 = vec4(0.55, 0.45, 0.0, 1.0);                                                               //PARAM	w8 = { 0.55, 0.45 };
	vec4 w9 = vec4(0.45, 0.45, 0.0, 1.0);                                                               //PARAM	w9 = { 0.45, 0.45 };
	
	vec4 w10 = vec4(0.4, 0.4, 0.0, 1.0);                                                                //PARAM	w10 = { 0.4, 0.4 };
	vec4 w11 = vec4(0.6, 0.4, 0.0, 1.0);                                                                //PARAM	w11 = { 0.6, 0.4 };
	vec4 w12 = vec4(0.6, 0.6, 0.0, 1.0);                                                                //PARAM	w12 = { 0.6, 0.6 };
	vec4 w13 = vec4(0.4, 0.6, 0.0, 1.0);                                                                //PARAM	w13 = { 0.4, 0.6 };
	
	//##
	
	vec4 R1, R2, W;                                                                                     //TEMP R1, R2, W;
	
	W = texture(u_texture0, w1.xy);                                                                     //TEX	W, w1, texture[0], 2D;
	
	R1 = texture(u_texture0, w2.xy);                                                                    //TEX	R1, w2, texture[0], 2D;
	W = (W) + (R1);                                                                                     //ADD	W, W, R1;
	
	R1 = texture(u_texture0, w3.xy);                                                                    //TEX	R1, w3, texture[0], 2D;
	W = (W) + (R1);                                                                                     //ADD	W, W, R1;
	
	R1 = texture(u_texture0, w4.xy);                                                                    //TEX	R1, w4, texture[0], 2D;
	W = (W) + (R1);                                                                                     //ADD	W, W, R1;
	
	R1 = texture(u_texture0, w6.xy);                                                                    //TEX	R1, w6, texture[0], 2D;
	W = (W) + (R1);                                                                                     //ADD	W, W, R1;
	
	R1 = texture(u_texture0, w7.xy);                                                                    //TEX	R1, w7, texture[0], 2D;
	W = (W) + (R1);                                                                                     //ADD	W, W, R1;
	
	R1 = texture(u_texture0, w8.xy);                                                                    //TEX	R1, w8, texture[0], 2D;
	W = (W) + (R1);                                                                                     //ADD	W, W, R1;
	
	R1 = texture(u_texture0, w9.xy);                                                                    //TEX	R1, w9, texture[0], 2D;
	W = (W) + (R1);                                                                                     //ADD	W, W, R1;
	
	R1 = texture(u_texture0, w10.xy);                                                                   //TEX	R1, w10, texture[0], 2D;
	W = (W) + (R1);                                                                                     //ADD	W, W, R1;
	
	R1 = texture(u_texture0, w11.xy);                                                                   //TEX	R1, w11, texture[0], 2D;
	W = (W) + (R1);                                                                                     //ADD	W, W, R1;
	
	R1 = texture(u_texture0, w12.xy);                                                                   //TEX	R1, w12, texture[0], 2D;
	W = (W) + (R1);                                                                                     //ADD	W, W, R1;
	
	R1 = texture(u_texture0, w13.xy);                                                                   //TEX	R1, w13, texture[0], 2D;
	W = (W) + (R1);                                                                                     //ADD	W, W, R1;
	
	//##
	
	R1 = texture(u_texture0, w5.xy);                                                                    //TEX	R1, w5, texture[0], 2D;
	W = (W) + (R1);                                                                                     //ADD	W, W, R1;
	
	W = (W) * (vec4(0.1111111));                                                                        //MUL	W, W, 0.1111111; 
	
	// make monochromatic
	
	W.a = max(W.r, W.g);                                                                                //MAX	W.a, W.r, W.g;
	W.a = max(W.a, W.b);                                                                                //MAX	W.a, W.a, W.b;
	
	W.a = (W.a) * (W.a);                                                                                //MUL	W.a, W.a, W.a;
	
	//## fetch last weight
	
	R2 = texture(u_texture1, xero.xy);                                                                  //TEX	R2, xero, texture[1], 2D;
	
	//## 
	
	R1 = (vec4(1.0)) - (W.aaaa);                                                                        //SUB	R1, 1.0, W.aaaa;
	
	R1 = (R1) - (R2);                                                                                   //SUB	R1, R1, R2; 
	R1 = (R1) * (vec4(0.1));                                                                            //MUL	R1, R1, 0.1; 
	
	draw_Color = (R1) + (R2);                                                                           //ADD	result.color, R1, R2;
	
	//##
	
}
