#version 140
// !!ARBfp1.0 

in vec4 var_tc0;
in vec4 var_tc1;
in vec4 var_tc2;
in vec4 var_tc3;
in vec4 var_tc4;
out vec4 draw_Color;
uniform sampler2D u_texture0;
uniform sampler2D u_texture1;

void main() {
	// OPTION ARB_precision_hint_fastest;
	
	//------------------------------------------
	vec4 Gamma = vec4(2., 0.0, 0.0, 1.0);                                                               //PARAM	Gamma = { 2. };
	vec4 lum_vec = vec4(0.2125, 0.7154, 0.0721, 1.0);                                                   //PARAM	lum_vec = { 0.2125, 0.7154, 0.0721 };
	vec4 TC_lum = vec4(0.5, 0.5, 0.0, 1.0);                                                             //PARAM	TC_lum = { 0.5, 0.5 };
	//------------------------------------------
	vec4 minLuminance, middleGray, brightPassThreshold, brightPassOffset;                               //TEMP	minLuminance, middleGray, brightPassThreshold, brightPassOffset;
	vec4 TC, adaptedLum, color, R1, R2;                                                                 //TEMP	TC, adaptedLum, color, R1, R2;
	
	TC = var_tc0;                                                                                       //MOV		TC, fragment.texcoord[0];
	middleGray = var_tc1;                                                                               //MOV		middleGray, fragment.texcoord[1];
	minLuminance = var_tc2;                                                                             //MOV		minLuminance, fragment.texcoord[2];
	brightPassThreshold = var_tc3;                                                                      //MOV		brightPassThreshold, fragment.texcoord[3];
	brightPassOffset = var_tc4;                                                                         //MOV		brightPassOffset, fragment.texcoord[4];
	
	adaptedLum = texture(u_texture0, TC_lum.xy);                                                        //TEX		adaptedLum, TC_lum, texture[0], 2D;
	color = texture(u_texture1, TC.xy);                                                                 //TEX		color, TC, texture[1], 2D;
	
	//---------------------------------------------------------
	// Decode three 8 bit integer values into one float
	//---------------------------------------------------------
	R1.x = (adaptedLum.x) * (255.0);                                                                    //MUL	R1.x, adaptedLum.x, 255.0;
	R1.x = (R1.x) * (0.00390625);                                                                       //MUL	R1.x, R1.x, 0.00390625;
	R1.y = (adaptedLum.y) * (255.0);                                                                    //MUL	R1.y, adaptedLum.y, 255.0;
	R1.y = (R1.y) * (0.00001525879);                                                                    //MUL R1.y, R1.y, 0.00001525879;
	//MUL	R1.z, adaptedLum.z, 255.0;
	//MUL R1.z, R1.z, 0.00000005960464;
	
	adaptedLum = (R1.xxxx) + (R1.yyyy);                                                                 //ADD adaptedLum, R1.x, R1.y;
	//ADD adaptedLum, adaptedLum, R1.z;
	//---------------------------------------------------------
	// Decode High Dynamic Range from encoded Luminance
	//---------------------------------------------------------
	R1 = (vec4(1.0)) + (-adaptedLum);                                                                   //ADD		R1, 1.0, -adaptedLum;
	R1 = max(R1, vec4(0.0001));                                                                         //MAX		R1, R1, 0.0001;
	R1.x = 1.0 / R1.x;                                                                                  //RCP		R1.x, R1.x;
	adaptedLum = (adaptedLum) * (R1.xxxx);                                                              //MUL		adaptedLum, adaptedLum, R1.x;
	//---------------------------------------------------------
	    
	//---------------------------------------------------------
	// Decode High Dynamic Range for Color
	//---------------------------------------------------------
	R1 = (vec4(1.0)) + (-color);                                                                        //ADD		R1, 1.0, -color;
	R1 = max(R1, vec4(0.0001));                                                                         //MAX		R1, R1, 0.0001;
	R1.x = 1.0 / R1.x;                                                                                  //RCP		R1.x, R1.x;
	R1.y = 1.0 / R1.y;                                                                                  //RCP		R1.y, R1.y;
	R1.z = 1.0 / R1.z;                                                                                  //RCP		R1.z, R1.z;
	color = (color) * (R1);                                                                             //MUL		color, color, R1;
	//---------------------------------------------------------
	
	//MAX		R1, minLuminance, adaptedLum;
	R1 = max(vec4(0.0001), adaptedLum);                                                                 //MAX		R1, 0.0001, adaptedLum;
	R1.x = 1.0 / R1.x;                                                                                  //RCP		R1.x, R1.x;
	
	R1 = (R1.xxxx) * (middleGray);                                                                      //MUL		R1, R1.x, middleGray;
	R1 = (color) * (R1) + (-brightPassThreshold);                                                       //MAD		R1, color, R1, -brightPassThreshold;
	
	color = max(R1, vec4(0.0));                                                                         //MAX		color, R1, 0.0;
	
	//POW		color.x, color.x, Gamma.x; 
	//POW		color.y, color.y, Gamma.x; 
	//POW		color.z, color.z, Gamma.x; 
	
	// Since Gamma is a fixed value, we can optimize above three lines into 1.
	 
	color = (color) * (color);                                                                          //MUL		color, color, color; 
	
	R1 = (brightPassOffset) + (color);                                                                  //ADD		R1, brightPassOffset, color;
	R1.x = 1.0 / R1.x;                                                                                  //RCP		R1.x, R1.x;
	R1.y = 1.0 / R1.y;                                                                                  //RCP		R1.y, R1.y;
	R1.z = 1.0 / R1.z;                                                                                  //RCP		R1.z, R1.z;
	
	draw_Color = (color) * (R1);                                                                        //MUL		result.color, color, R1;
	
}
