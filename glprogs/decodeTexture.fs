#version 140
// !!ARBfp1.0 

in vec4 var_tc0;
out vec4 draw_Color;
uniform sampler2D u_texture0;

void main() {
	// OPTION ARB_precision_hint_fastest;
	
	//------------------------------------------
	vec4 TC, R1, R2;                                                                                    //TEMP	 TC, R1, R2;
	
	TC = var_tc0;                                                                                       //MOV		TC, fragment.texcoord[0];
	
	R2 = texture(u_texture0, TC.xy);                                                                    //TEX		R2, TC, texture[0], 2D;
	
	//---------------------------------------------------------
	// Decode three 8 bit integer values into one float
	//---------------------------------------------------------
	R1.x = (R2.x) * (255.0);                                                                            //MUL	R1.x, R2.x, 255.0;
	R1.x = (R1.x) * (0.00390625);                                                                       //MUL	R1.x, R1.x, 0.00390625;
	R1.y = (R2.y) * (255.0);                                                                            //MUL	R1.y, R2.y, 255.0;
	R1.y = (R1.y) * (0.00001525879);                                                                    //MUL	R1.y, R1.y, 0.00001525879;
	R1.z = (R2.z) * (255.0);                                                                            //MUL	R1.z, R2.z, 255.0;
	R1.z = (R1.z) * (0.00000005960464);                                                                 //MUL	R1.z, R1.z, 0.00000005960464;
	
	R2 = (R1.xxxx) + (R1.yyyy);                                                                         //ADD R2, R1.x, R1.y;
	R2 = (R2) + (R1.zzzz);                                                                              //ADD R2, R2, R1.z;
	
	//---------------------------------------------------------
	
	draw_Color = R2;                                                                                    //MOV		result.color,  R2;
	
}
