#version 140
// !!ARBfp1.0 OPTION ARB_precision_hint_fastest;

in vec4 var_tc0;
out vec4 draw_Color;
uniform sampler2D u_texture0;

void main() {
	
	// gaussian distribution in 1D, standard deviation of 3
	vec4 x0 = vec4(0.132981);                                                                           //PARAM x0 = 0.132981;
	vec4 x1 = vec4(0.125794);                                                                           //PARAM x1 = 0.125794;
	vec4 x2 = vec4(0.106483);                                                                           //PARAM x2 = 0.106483;
	vec4 x3 = vec4(0.080657);                                                                           //PARAM x3 = 0.080657;
	vec4 x4 = vec4(0.054670);                                                                           //PARAM x4 = 0.054670;
	vec4 x5 = vec4(0.033159);                                                                           //PARAM x5 = 0.033159;
	vec4 x6 = vec4(0.017997);                                                                           //PARAM x6 = 0.017997;
	vec4 x7 = vec4(0.008741);                                                                           //PARAM x7 = 0.008741;
	vec4 x8 = vec4(0.003799);                                                                           //PARAM x8 = 0.003799;
	
	// pixel locations, assuming texture width of 512
	vec4 c0 = vec4(0, 0, 0, 1.0);                                                                       //PARAM c0 = {0, 0, 0, 0};
	vec4 c1 = vec4(0.00195313, 0, 0, 1.0);                                                              //PARAM c1 = {0.00195313, 0, 0, 0};
	vec4 c2 = vec4(0.00390625, 0, 0, 1.0);                                                              //PARAM c2 = {0.00390625, 0, 0, 0};
	vec4 c3 = vec4(0.00585938, 0, 0, 1.0);                                                              //PARAM c3 = {0.00585938, 0, 0, 0};
	vec4 c4 = vec4(0.0078125, 0, 0, 1.0);                                                               //PARAM c4 = {0.0078125, 0, 0, 0};
	vec4 c5 = vec4(0.00976563, 0, 0, 1.0);                                                              //PARAM c5 = {0.00976563, 0, 0, 0};
	vec4 c6 = vec4(0.0117188, 0, 0, 1.0);                                                               //PARAM c6 = {0.0117188, 0, 0, 0};
	vec4 c7 = vec4(0.0136719, 0, 0, 1.0);                                                               //PARAM c7 = {0.0136719, 0, 0, 0};
	vec4 c8 = vec4(0.015625, 0, 0, 1.0);                                                                //PARAM c8 = {0.015625, 0, 0, 0};
	
	vec4 color0, color1, color2, color3, color4, color5, color6, color7, color8, final;                 //TEMP color0, color1, color2, color3, color4, color5, color6, color7, color8, final;
	
	color0 = var_tc0;                                                                                   //MOV color0, fragment.texcoord[0];
	color1 = (color0) + (c1);                                                                           //ADD color1, color0, c1;
	color2 = (color0) + (c2);                                                                           //ADD color2, color0, c2;
	color3 = (color0) + (c3);                                                                           //ADD color3, color0, c3;
	color4 = (color0) + (c4);                                                                           //ADD color4, color0, c4;
	color5 = (color0) + (c5);                                                                           //ADD color5, color0, c5;
	color6 = (color0) + (c6);                                                                           //ADD color6, color0, c6;
	color7 = (color0) + (c7);                                                                           //ADD color7, color0, c7;
	color8 = (color0) + (c8);                                                                           //ADD color8, color0, c8;
	
	color1 = texture(u_texture0, color1.xy);                                                            //TEX color1, color1, texture[0], 2D;
	color2 = texture(u_texture0, color2.xy);                                                            //TEX color2, color2, texture[0], 2D;
	color3 = texture(u_texture0, color3.xy);                                                            //TEX color3, color3, texture[0], 2D;
	color4 = texture(u_texture0, color4.xy);                                                            //TEX color4, color4, texture[0], 2D;
	color5 = texture(u_texture0, color5.xy);                                                            //TEX color5, color5, texture[0], 2D;
	color6 = texture(u_texture0, color6.xy);                                                            //TEX color6, color6, texture[0], 2D;
	color7 = texture(u_texture0, color7.xy);                                                            //TEX color7, color7, texture[0], 2D;
	color8 = texture(u_texture0, color8.xy);                                                            //TEX color8, color8, texture[0], 2D;
	
	final = (color1) * (x1);                                                                            //MUL final, color1, x1;
	final = (color2) * (x2) + (final);                                                                  //MAD final, color2, x2, final;
	final = (color3) * (x3) + (final);                                                                  //MAD final, color3, x3, final;
	final = (color4) * (x4) + (final);                                                                  //MAD final, color4, x4, final;
	final = (color5) * (x5) + (final);                                                                  //MAD final, color5, x5, final;
	final = (color6) * (x6) + (final);                                                                  //MAD final, color6, x6, final;
	final = (color7) * (x7) + (final);                                                                  //MAD final, color7, x7, final;
	final = (color8) * (x8) + (final);                                                                  //MAD final, color8, x8, final;
	
	color1 = (color0) - (c1);                                                                           //SUB color1, color0, c1;
	color2 = (color0) - (c2);                                                                           //SUB color2, color0, c2;
	color3 = (color0) - (c3);                                                                           //SUB color3, color0, c3;
	color4 = (color0) - (c4);                                                                           //SUB color4, color0, c4;
	color5 = (color0) - (c5);                                                                           //SUB color5, color0, c5;
	color6 = (color0) - (c6);                                                                           //SUB color6, color0, c6;
	color7 = (color0) - (c7);                                                                           //SUB color7, color0, c7;
	color8 = (color0) - (c8);                                                                           //SUB color8, color0, c8;
	
	color0 = texture(u_texture0, color0.xy);                                                            //TEX color0, color0, texture[0], 2D;
	color1 = texture(u_texture0, color1.xy);                                                            //TEX color1, color1, texture[0], 2D;
	color2 = texture(u_texture0, color2.xy);                                                            //TEX color2, color2, texture[0], 2D;
	color3 = texture(u_texture0, color3.xy);                                                            //TEX color3, color3, texture[0], 2D;
	color4 = texture(u_texture0, color4.xy);                                                            //TEX color4, color4, texture[0], 2D;
	color5 = texture(u_texture0, color5.xy);                                                            //TEX color5, color5, texture[0], 2D;
	color6 = texture(u_texture0, color6.xy);                                                            //TEX color6, color6, texture[0], 2D;
	color7 = texture(u_texture0, color7.xy);                                                            //TEX color7, color7, texture[0], 2D;
	color8 = texture(u_texture0, color8.xy);                                                            //TEX color8, color8, texture[0], 2D;
	
	final = (color0) * (x0) + (final);                                                                  //MAD final, color0, x0, final;
	final = (color1) * (x1) + (final);                                                                  //MAD final, color1, x1, final;
	final = (color2) * (x2) + (final);                                                                  //MAD final, color2, x2, final;
	final = (color3) * (x3) + (final);                                                                  //MAD final, color3, x3, final;
	final = (color4) * (x4) + (final);                                                                  //MAD final, color4, x4, final;
	final = (color5) * (x5) + (final);                                                                  //MAD final, color5, x5, final;
	final = (color6) * (x6) + (final);                                                                  //MAD final, color6, x6, final;
	final = (color7) * (x7) + (final);                                                                  //MAD final, color7, x7, final;
	final = (color8) * (x8) + (final);                                                                  //MAD final, color8, x8, final;
	
	draw_Color = final;                                                                                 //MOV result.color, final;
	
}
