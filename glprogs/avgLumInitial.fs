#version 140
// !!ARBfp1.0 

in vec4 var_tc0;
in vec4 var_tc1;
out vec4 draw_Color;
uniform sampler2D u_texture0;

void main() {
	// OPTION ARB_precision_hint_fastest;
	
	//------------------------------------------
	// offsets for sample points
	//------------------------------------------
	vec4 offset1 = vec4(-1.0, 	-1.0, 0.0, 1.0);                                                         //PARAM	offset1 = { -1.0,	-1.0 };
	vec4 offset2 = vec4(0.0, 	-1.0, 0.0, 1.0);                                                          //PARAM	offset2 = {  0.0,	-1.0 };
	vec4 offset3 = vec4(1.0, 	-1.0, 0.0, 1.0);                                                          //PARAM	offset3 = {  1.0,	-1.0 };
	
	vec4 offset4 = vec4(-1.0, 	0.0, 0.0, 1.0);                                                          //PARAM	offset4 = { -1.0,	0.0 };
	vec4 offset5 = vec4(1.0, 	0.0, 0.0, 1.0);                                                           //PARAM	offset5 = {  1.0,	0.0 };
	
	vec4 offset6 = vec4(-1.0, 	1.0, 0.0, 1.0);                                                          //PARAM	offset6 = { -1.0,	1.0 };
	vec4 offset7 = vec4(0.0, 	1.0, 0.0, 1.0);                                                           //PARAM	offset7 = {  0.0,	1.0 };
	vec4 offset8 = vec4(1.0, 	1.0, 0.0, 1.0);                                                           //PARAM	offset8 = {  1.0,	1.0 };
	//------------------------------------------
	vec4 lum_vec = vec4(0.2125, 0.7154, 0.0721, 1.0);                                                   //PARAM lum_vec = { 0.2125, 0.7154, 0.0721 };
	vec4 lum_black = vec4(0.0001, 0.0, 0.0, 1.0);                                                       //PARAM lum_black = { 0.0001 };
	//------------------------------------------
	
	vec4 color0, color1, color2, color3, color4, color5, color6, color7, color8;                        //TEMP	color0, color1, color2, color3, color4, color5, color6, color7, color8;
	vec4 TC, R1;                                                                                        //TEMP	TC, R1;
	
	
	TC = var_tc1;                                                                                       //MOV		TC, fragment.texcoord[1];
	
	color0 = var_tc0;                                                                                   //MOV color0, fragment.texcoord[0];
	color1 = (offset1) * (TC) + (color0);                                                               //MAD	color1, offset1, TC, color0; 
	color2 = (offset2) * (TC) + (color0);                                                               //MAD	color2, offset2, TC, color0; 
	color3 = (offset3) * (TC) + (color0);                                                               //MAD	color3, offset3, TC, color0; 
	color4 = (offset4) * (TC) + (color0);                                                               //MAD	color4, offset4, TC, color0; 
	color5 = (offset5) * (TC) + (color0);                                                               //MAD	color5, offset5, TC, color0; 
	color6 = (offset6) * (TC) + (color0);                                                               //MAD	color6, offset6, TC, color0; 
	color7 = (offset7) * (TC) + (color0);                                                               //MAD	color7, offset7, TC, color0; 
	color8 = (offset8) * (TC) + (color0);                                                               //MAD	color8, offset8, TC, color0; 
	
	color0 = texture(u_texture0, color0.xy);                                                            //TEX color0, color0, texture[0], 2D;
	color1 = texture(u_texture0, color1.xy);                                                            //TEX color1, color1, texture[0], 2D;
	color2 = texture(u_texture0, color2.xy);                                                            //TEX color2, color2, texture[0], 2D;
	color3 = texture(u_texture0, color3.xy);                                                            //TEX color3, color3, texture[0], 2D;
	color4 = texture(u_texture0, color4.xy);                                                            //TEX color4, color4, texture[0], 2D;
	color5 = texture(u_texture0, color5.xy);                                                            //TEX color5, color5, texture[0], 2D;
	color6 = texture(u_texture0, color6.xy);                                                            //TEX color6, color6, texture[0], 2D;
	color7 = texture(u_texture0, color7.xy);                                                            //TEX color7, color7, texture[0], 2D;
	color8 = texture(u_texture0, color8.xy);                                                            //TEX color8, color8, texture[0], 2D;
	
	color0 = clamp(vec4(dot(color0.xyz, lum_vec.xyz)), 0.0, 1.0);                                       //DP3_SAT	color0, color0, lum_vec; 
	//MAX	color0, color0, lum_black; 
	color1 = clamp(vec4(dot(color1.xyz, lum_vec.xyz)), 0.0, 1.0);                                       //DP3_SAT	color1, color1, lum_vec; 
	//MAX	color1, color1, lum_black; 
	color2 = clamp(vec4(dot(color2.xyz, lum_vec.xyz)), 0.0, 1.0);                                       //DP3_SAT	color2, color2, lum_vec; 
	//MAX	color2, color2, lum_black; 
	color3 = clamp(vec4(dot(color3.xyz, lum_vec.xyz)), 0.0, 1.0);                                       //DP3_SAT	color3, color3, lum_vec; 
	//MAX	color3, color3, lum_black; 
	color4 = clamp(vec4(dot(color4.xyz, lum_vec.xyz)), 0.0, 1.0);                                       //DP3_SAT	color4, color4, lum_vec; 
	//MAX	color4, color4, lum_black; 
	color5 = clamp(vec4(dot(color5.xyz, lum_vec.xyz)), 0.0, 1.0);                                       //DP3_SAT	color5, color5, lum_vec; 
	//MAX	color5, color5, lum_black; 
	color6 = clamp(vec4(dot(color6.xyz, lum_vec.xyz)), 0.0, 1.0);                                       //DP3_SAT	color6, color6, lum_vec; 
	//MAX	color6, color6, lum_black; 
	color7 = clamp(vec4(dot(color7.xyz, lum_vec.xyz)), 0.0, 1.0);                                       //DP3_SAT	color7, color7, lum_vec; 
	//MAX	color7, color7, lum_black; 
	color8 = clamp(vec4(dot(color8.xyz, lum_vec.xyz)), 0.0, 1.0);                                       //DP3_SAT	color8, color8, lum_vec; 
	//MAX	color8, color8, lum_black; 
	
	
	color0 = (color0) + (color1);                                                                       //ADD	color0, color0, color1; 
	color0 = (color0) + (color2);                                                                       //ADD	color0, color0, color2; 
	color0 = (color0) + (color3);                                                                       //ADD	color0, color0, color3; 
	color0 = (color0) + (color4);                                                                       //ADD	color0, color0, color4; 
	color0 = (color0) + (color5);                                                                       //ADD	color0, color0, color5; 
	color0 = (color0) + (color6);                                                                       //ADD	color0, color0, color6; 
	color0 = (color0) + (color7);                                                                       //ADD	color0, color0, color7; 
	color0 = (color0) + (color8);                                                                       //ADD	color0, color0, color8; 
	
	
	color0 = (color0) * (vec4(0.11111111));                                                             //MUL color0, color0, 0.11111111;
	
	//------------------------------
	// Encode 24 bit float into three 8 bit integer values
	//------------------------------
	color0.x = (color0.x) * (256.0);                                                                    //MUL color0.x, color0.x, 256.0;
	color2.x = floor(color0.x);                                                                         //FLR color2.x, color0.x;
	color1.x = (color2.x) * (0.003921568627451);                                                        //MUL color1.x, color2.x, 0.003921568627451;
	color0.x = (color0.x) - (color2.x);                                                                 //SUB color0.x, color0.x, color2.x;
	color0.x = (color0.x) * (256.0);                                                                    //MUL color0.x, color0.x, 256.0;
	color2.x = floor(color0.x);                                                                         //FLR color2.x, color0.x;
	color1.y = (color2.x) * (0.003921568627451);                                                        //MUL color1.y, color2.x, 0.003921568627451;
	//-------------------------------
	draw_Color = color1;                                                                                //MOV result.color, color1;
	//MOV result.color, color0;
	
}
