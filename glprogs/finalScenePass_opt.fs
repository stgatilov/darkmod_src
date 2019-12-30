#version 140
// !!ARBfp1.0 

in vec4 var_tc0;
in vec4 var_tc1;
out vec4 draw_Color;
uniform sampler2D u_texture0;
uniform sampler2D u_texture1;
uniform sampler2D u_texture2;

void main() {
	// OPTION ARB_precision_hint_fastest;
	
	//------------------------------------------
	vec4 X1_Y0_Z0 = vec4(1.0, 0.0, 0., 1.0);                                                            //PARAM X1_Y0_Z0 = {1.0, 0.0, 0.0};
	vec4 lum_vec = vec4(0.2125, 0.7154, 0.0721, 1.0);                                                   //PARAM lum_vec = { 0.2125, 0.7154, 0.0721 };
	//------------------------------------------
	vec4 TC, color, R1, R2, R3, bloom;                                                                  //TEMP	TC, color, R1, R2, R3, bloom;
	
	TC = var_tc0;                                                                                       //MOV		TC, fragment.texcoord[0];
	
	color = texture(u_texture0, TC.xy);                                                                 //TEX		color,		TC, texture[0], 2D;
	bloom = texture(u_texture1, TC.xy);                                                                 //TEX		bloom,		TC, texture[1], 2D;
	
	
	TC = (color.rrrr) * (X1_Y0_Z0);                                                                     //MUL		TC, color.r, X1_Y0_Z0;
	R1 = texture(u_texture2, TC.xy);                                                                    //TEX		R1, TC, texture[2], 2D;
	
	TC = (color.gggg) * (X1_Y0_Z0);                                                                     //MUL		TC, color.g, X1_Y0_Z0;
	R2 = texture(u_texture2, TC.xy);                                                                    //TEX		R2, TC, texture[2], 2D;
	
	TC = (color.bbbb) * (X1_Y0_Z0);                                                                     //MUL		TC, color.b, X1_Y0_Z0;
	R3 = texture(u_texture2, TC.xy);                                                                    //TEX		R3, TC, texture[2], 2D;
	
	R1.y = R2.x;                                                                                        //MOV		R1.y, R2.x;
	R1.z = R3.x;                                                                                        //MOV		R1.z, R3.x;
	R1.w = 1.0;                                                                                         //MOV		R1.w, 1.0;
	
	//Add bloom
	color = (bloom) * (var_tc1.xxxx) + (R1);                                                            //MAD		color, bloom, fragment.texcoord[1].x, R1;
	
	//Desaturate.
	R1 = clamp(vec4(dot(lum_vec.xyz, color.xyz)), 0.0, 1.0);                                            //DP3_SAT	R1, lum_vec,color; 
	
	draw_Color = mix(color, R1, var_tc1.yyyy);                                                          //LRP		result.color, fragment.texcoord[1].y, R1, color;
	
}
