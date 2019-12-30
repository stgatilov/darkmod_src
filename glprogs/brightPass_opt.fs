#version 140
// !!ARBfp1.0 

in vec4 var_tc0;
out vec4 draw_Color;
uniform sampler2D u_texture0;
uniform sampler2D u_texture1;

void main() {
	// OPTION ARB_precision_hint_fastest;
	
	vec4 X1_Y0_Z0 = vec4(1.0, 0.0, 0., 1.0);                                                            //PARAM X1_Y0_Z0 = {1.0, 0.0, 0.0};
	
	vec4 TC, color, R1, R2, R3;                                                                         //TEMP	TC, color, R1, R2, R3;
	//---------------------------------------------------------
	color = texture(u_texture0, var_tc0.xy);                                                            //TEX		color, fragment.texcoord[0], texture[0], 2D;
	//---------------------------------------------------------
	
	TC = (color.rrrr) * (X1_Y0_Z0);                                                                     //MUL		TC, color.r, X1_Y0_Z0;
	R1 = texture(u_texture1, TC.xy);                                                                    //TEX		R1, TC, texture[1], 2D;
	
	TC = (color.gggg) * (X1_Y0_Z0);                                                                     //MUL		TC, color.g, X1_Y0_Z0;
	draw_Color = texture(u_texture1, TC.xy);                                                            //TEX		result.color, TC, texture[1], 2D;
	
	TC = (color.bbbb) * (X1_Y0_Z0);                                                                     //MUL		TC, color.b, X1_Y0_Z0;
	R2 = texture(u_texture1, TC.xy);                                                                    //TEX		R2, TC, texture[1], 2D;
	
	draw_Color.x = R1.y;                                                                                //MOV		result.color.x, R1.y;
	draw_Color.z = R2.y;                                                                                //MOV		result.color.z, R2.y;
	draw_Color.w = 1.0;                                                                                 //MOV		result.color.w, 1.0;
	
}
