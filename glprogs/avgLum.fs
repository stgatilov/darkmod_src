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
	vec4 offset01 = vec4(-1.5, -1.5, 0.0, 1.0);                                                         //PARAM	offset01 = { -1.5, -1.5 };
	vec4 offset02 = vec4(-0.5, -1.5, 0.0, 1.0);                                                         //PARAM	offset02 = { -0.5, -1.5 };
	vec4 offset03 = vec4(0.5, -1.5, 0.0, 1.0);                                                          //PARAM	offset03 = {  0.5, -1.5 };
	vec4 offset04 = vec4(1.5, -1.5, 0.0, 1.0);                                                          //PARAM	offset04 = {  1.5, -1.5 };
	vec4 offset05 = vec4(-1.5, -0.5, 0.0, 1.0);                                                         //PARAM	offset05 = { -1.5, -0.5 };
	vec4 offset06 = vec4(-0.5, -0.5, 0.0, 1.0);                                                         //PARAM	offset06 = { -0.5, -0.5 };
	vec4 offset07 = vec4(0.5, -0.5, 0.0, 1.0);                                                          //PARAM	offset07 = {  0.5, -0.5 };
	vec4 offset08 = vec4(1.5, -0.5, 0.0, 1.0);                                                          //PARAM	offset08 = {  1.5, -0.5 };
	vec4 offset09 = vec4(-1.5, 0.5, 0.0, 1.0);                                                          //PARAM	offset09 = { -1.5,  0.5 };
	vec4 offset10 = vec4(-0.5, 0.5, 0.0, 1.0);                                                          //PARAM	offset10 = { -0.5,  0.5 };
	vec4 offset11 = vec4(0.5, 0.5, 0.0, 1.0);                                                           //PARAM	offset11 = {  0.5,  0.5 };
	vec4 offset12 = vec4(1.5, 0.5, 0.0, 1.0);                                                           //PARAM	offset12 = {  1.5,  0.5 };
	vec4 offset13 = vec4(-1.5, 1.5, 0.0, 1.0);                                                          //PARAM	offset13 = { -1.5,  1.5 };
	vec4 offset14 = vec4(-0.5, 1.5, 0.0, 1.0);                                                          //PARAM	offset14 = { -0.5,  1.5 };
	vec4 offset15 = vec4(0.5, 1.5, 0.0, 1.0);                                                           //PARAM	offset15 = {  0.5,  1.5 };
	vec4 offset16 = vec4(1.5, 1.5, 0.0, 1.0);                                                           //PARAM	offset16 = {  1.5,  1.5 };
	//------------------------------------------
	
	vec4 TexCoord, color0, color1, color2, color3, color4, color5, color6, color7, color8;              //TEMP	TexCoord, color0, color1, color2, color3, color4, color5, color6, color7, color8;
	vec4 color9, color10, color11, color12, color13, color14, color15;                                  //TEMP	color9, color10, color11, color12, color13, color14, color15;
	vec4 TC;                                                                                            //TEMP	TC;
	
	TC = var_tc1;                                                                                       //MOV		TC, fragment.texcoord[1];
	TexCoord = var_tc0;                                                                                 //MOV TexCoord, fragment.texcoord[0];
	
	color0 = (offset01) * (TC) + (TexCoord);                                                            //MAD	color0, offset01, TC, TexCoord; 
	color1 = (offset02) * (TC) + (TexCoord);                                                            //MAD	color1, offset02, TC, TexCoord; 
	color2 = (offset03) * (TC) + (TexCoord);                                                            //MAD	color2, offset03, TC, TexCoord; 
	color3 = (offset04) * (TC) + (TexCoord);                                                            //MAD	color3, offset04, TC, TexCoord; 
	color4 = (offset05) * (TC) + (TexCoord);                                                            //MAD	color4, offset05, TC, TexCoord; 
	color5 = (offset06) * (TC) + (TexCoord);                                                            //MAD	color5, offset06, TC, TexCoord; 
	color6 = (offset07) * (TC) + (TexCoord);                                                            //MAD	color6, offset07, TC, TexCoord; 
	color7 = (offset08) * (TC) + (TexCoord);                                                            //MAD	color7, offset08, TC, TexCoord; 
	color8 = (offset09) * (TC) + (TexCoord);                                                            //MAD	color8, offset09, TC, TexCoord; 
	color9 = (offset10) * (TC) + (TexCoord);                                                            //MAD	color9, offset10, TC, TexCoord; 
	color10 = (offset11) * (TC) + (TexCoord);                                                           //MAD	color10, offset11, TC, TexCoord; 
	color11 = (offset12) * (TC) + (TexCoord);                                                           //MAD	color11, offset12, TC, TexCoord; 
	color12 = (offset13) * (TC) + (TexCoord);                                                           //MAD	color12, offset13, TC, TexCoord; 
	color13 = (offset14) * (TC) + (TexCoord);                                                           //MAD	color13, offset14, TC, TexCoord; 
	color14 = (offset15) * (TC) + (TexCoord);                                                           //MAD	color14, offset15, TC, TexCoord; 
	color15 = (offset16) * (TC) + (TexCoord);                                                           //MAD	color15, offset16, TC, TexCoord; 
	
	color0 = texture(u_texture0, color0.xy);                                                            //TEX color0, color0, texture[0], 2D;
	color1 = texture(u_texture0, color1.xy);                                                            //TEX color1, color1, texture[0], 2D;
	color2 = texture(u_texture0, color2.xy);                                                            //TEX color2, color2, texture[0], 2D;
	color3 = texture(u_texture0, color3.xy);                                                            //TEX color3, color3, texture[0], 2D;
	color4 = texture(u_texture0, color4.xy);                                                            //TEX color4, color4, texture[0], 2D;
	color5 = texture(u_texture0, color5.xy);                                                            //TEX color5, color5, texture[0], 2D;
	color6 = texture(u_texture0, color6.xy);                                                            //TEX color6, color6, texture[0], 2D;
	color7 = texture(u_texture0, color7.xy);                                                            //TEX color7, color7, texture[0], 2D;
	color8 = texture(u_texture0, color8.xy);                                                            //TEX color8, color8, texture[0], 2D;
	color9 = texture(u_texture0, color9.xy);                                                            //TEX color9, color9, texture[0], 2D;
	color10 = texture(u_texture0, color10.xy);                                                          //TEX color10, color10, texture[0], 2D;
	color11 = texture(u_texture0, color11.xy);                                                          //TEX color11, color11, texture[0], 2D;
	color12 = texture(u_texture0, color12.xy);                                                          //TEX color12, color12, texture[0], 2D;
	color13 = texture(u_texture0, color13.xy);                                                          //TEX color13, color13, texture[0], 2D;
	color14 = texture(u_texture0, color14.xy);                                                          //TEX color14, color14, texture[0], 2D;
	color15 = texture(u_texture0, color15.xy);                                                          //TEX color15, color15, texture[0], 2D;
	
	color0 = (color0) + (color1);                                                                       //ADD	color0, color0, color1; 
	color0 = (color0) + (color2);                                                                       //ADD	color0, color0, color2; 
	color0 = (color0) + (color3);                                                                       //ADD	color0, color0, color3; 
	color0 = (color0) + (color4);                                                                       //ADD	color0, color0, color4; 
	color0 = (color0) + (color5);                                                                       //ADD	color0, color0, color5; 
	color0 = (color0) + (color6);                                                                       //ADD	color0, color0, color6; 
	color0 = (color0) + (color7);                                                                       //ADD	color0, color0, color7; 
	color0 = (color0) + (color8);                                                                       //ADD	color0, color0, color8; 
	color0 = (color0) + (color9);                                                                       //ADD	color0, color0, color9; 
	color0 = (color0) + (color10);                                                                      //ADD	color0, color0, color10; 
	color0 = (color0) + (color11);                                                                      //ADD	color0, color0, color11; 
	color0 = (color0) + (color12);                                                                      //ADD	color0, color0, color12; 
	color0 = (color0) + (color13);                                                                      //ADD	color0, color0, color13; 
	color0 = (color0) + (color14);                                                                      //ADD	color0, color0, color14; 
	color0 = (color0) + (color15);                                                                      //ADD	color0, color0, color15; 
	
	draw_Color = (color0) * (vec4(0.0625));                                                             //MUL	result.color, color0, 0.0625;
	
}
