#version 140
// !!ARBfp1.0 

in vec4 var_tc0;
out vec4 draw_Color;
uniform sampler2D u_texture0;

void main() {
	// OPTION ARB_precision_hint_fastest;
	
	//-----------------------------------
	// Params resp. ImageExposure, ImageGamma
	//-----------------------------------
	vec4 haloParams = vec4(1.0, 2.0, 0.0, 1.0);                                                         //PARAM	haloParams = { 1.0, 2.0 };
	//-----------------------------------
	
	vec4 colBrightPassImage;                                                                            //TEMP	colBrightPassImage;
	vec4 TC, color;                                                                                     //TEMP	TC, color;
	
	TC = var_tc0;                                                                                       //MOV		TC, fragment.texcoord[0];
	
	colBrightPassImage = texture(u_texture0, TC.xy);                                                    //TEX		colBrightPassImage, TC, texture[0], 2D;
	
	//MUL		colBrightPassImage, colBrightPassImage, haloParams.x;
	//POW		color.x, colBrightPassImage.x, haloParams.y; 
	//POW		color.y, colBrightPassImage.y, haloParams.y; 
	//POW		color.z, colBrightPassImage.z, haloParams.y; 
	
	//This is same as above but faster than POW.
	//MUL		color, colBrightPassImage, colBrightPassImage; 
	//MOV		color.w, 1.0; 
	
	
	draw_Color = colBrightPassImage;                                                                    //MOV		result.color, colBrightPassImage;
	
}
