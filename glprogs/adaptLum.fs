#version 140
// !!ARBfp1.0 

in vec4 var_tc1;
out vec4 draw_Color;
uniform sampler2D u_texture0;
uniform sampler2D u_texture1;

void main() {
	// OPTION ARB_precision_hint_fastest;
	
	//------------------------------------------
	// offsets for sample points
	//------------------------------------------
	vec4 TC0 = vec4(0.000000, 0.000000, 0.0, 1.0);                                                      //PARAM	TC0 = { 0.000000, 0.000000 };
	vec4 TC1 = vec4(0.000000, 0.250000, 0.0, 1.0);                                                      //PARAM	TC1 = { 0.000000, 0.250000 };
	vec4 TC2 = vec4(0.000000, 0.500000, 0.0, 1.0);                                                      //PARAM	TC2 = { 0.000000, 0.500000 };
	vec4 TC3 = vec4(0.000000, 0.750000, 0.0, 1.0);                                                      //PARAM	TC3 = { 0.000000, 0.750000 };
	vec4 TC4 = vec4(0.250000, 0.000000, 0.0, 1.0);                                                      //PARAM	TC4 = { 0.250000, 0.000000 };
	vec4 TC5 = vec4(0.250000, 0.250000, 0.0, 1.0);                                                      //PARAM	TC5 = { 0.250000, 0.250000 };
	vec4 TC6 = vec4(0.250000, 0.500000, 0.0, 1.0);                                                      //PARAM	TC6 = { 0.250000, 0.500000 };
	vec4 TC7 = vec4(0.250000, 0.750000, 0.0, 1.0);                                                      //PARAM	TC7 = { 0.250000, 0.750000 };
	vec4 TC8 = vec4(0.500000, 0.000000, 0.0, 1.0);                                                      //PARAM	TC8 = { 0.500000, 0.000000 };
	vec4 TC9 = vec4(0.500000, 0.250000, 0.0, 1.0);                                                      //PARAM	TC9 = { 0.500000, 0.250000 };
	vec4 TC10 = vec4(0.500000, 0.500000, 0.0, 1.0);                                                     //PARAM	TC10 = { 0.500000, 0.500000 };
	vec4 TC11 = vec4(0.500000, 0.750000, 0.0, 1.0);                                                     //PARAM	TC11 = { 0.500000, 0.750000 };
	vec4 TC12 = vec4(0.750000, 0.000000, 0.0, 1.0);                                                     //PARAM	TC12 = { 0.750000, 0.000000 };
	vec4 TC13 = vec4(0.750000, 0.250000, 0.0, 1.0);                                                     //PARAM	TC13 = { 0.750000, 0.250000 };
	vec4 TC14 = vec4(0.750000, 0.500000, 0.0, 1.0);                                                     //PARAM	TC14 = { 0.750000, 0.500000 };
	vec4 TC15 = vec4(0.750000, 0.750000, 0.0, 1.0);                                                     //PARAM	TC15 = { 0.750000, 0.750000 };
	//------------------------------------------
	vec4 TC = vec4(0.5, 0.5, 0.0, 1.0);                                                                 //PARAM	TC = { 0.5, 0.5 };
	//PARAM lum_vec = { 0.2125, 0.7154, 0.0721 };
	//------------------------------------------
	
	vec4 color, lum, curLum, lumParms;                                                                  //TEMP	color, lum, curLum, lumParms;
	vec4 prevLum, R1, R2;                                                                               //TEMP	prevLum, R1, R2;
	
	lum = texture(u_texture0, TC0.xy);                                                                  //TEX		lum, TC0, texture[0], 2D;
	
	color = texture(u_texture0, TC1.xy);                                                                //TEX		color, TC1, texture[0], 2D;
	lum = (lum) + (color);                                                                              //ADD		lum, lum, color;
	color = texture(u_texture0, TC2.xy);                                                                //TEX		color, TC2, texture[0], 2D;
	lum = (lum) + (color);                                                                              //ADD		lum, lum, color;
	color = texture(u_texture0, TC3.xy);                                                                //TEX		color, TC3, texture[0], 2D;
	lum = (lum) + (color);                                                                              //ADD		lum, lum, color;
	color = texture(u_texture0, TC4.xy);                                                                //TEX		color, TC4, texture[0], 2D;
	lum = (lum) + (color);                                                                              //ADD		lum, lum, color;
	color = texture(u_texture0, TC5.xy);                                                                //TEX		color, TC5, texture[0], 2D;
	lum = (lum) + (color);                                                                              //ADD		lum, lum, color;
	color = texture(u_texture0, TC6.xy);                                                                //TEX		color, TC6, texture[0], 2D;
	lum = (lum) + (color);                                                                              //ADD		lum, lum, color;
	color = texture(u_texture0, TC7.xy);                                                                //TEX		color, TC7, texture[0], 2D;
	lum = (lum) + (color);                                                                              //ADD		lum, lum, color;
	color = texture(u_texture0, TC8.xy);                                                                //TEX		color, TC8, texture[0], 2D;
	lum = (lum) + (color);                                                                              //ADD		lum, lum, color;
	color = texture(u_texture0, TC9.xy);                                                                //TEX		color, TC9, texture[0], 2D;
	lum = (lum) + (color);                                                                              //ADD		lum, lum, color;
	color = texture(u_texture0, TC10.xy);                                                               //TEX		color, TC10, texture[0], 2D;
	lum = (lum) + (color);                                                                              //ADD		lum, lum, color;
	color = texture(u_texture0, TC11.xy);                                                               //TEX		color, TC11, texture[0], 2D;
	lum = (lum) + (color);                                                                              //ADD		lum, lum, color;
	color = texture(u_texture0, TC12.xy);                                                               //TEX		color, TC12, texture[0], 2D;
	lum = (lum) + (color);                                                                              //ADD		lum, lum, color;
	color = texture(u_texture0, TC13.xy);                                                               //TEX		color, TC13, texture[0], 2D;
	lum = (lum) + (color);                                                                              //ADD		lum, lum, color;
	color = texture(u_texture0, TC14.xy);                                                               //TEX		color, TC14, texture[0], 2D;
	lum = (lum) + (color);                                                                              //ADD		lum, lum, color;
	color = texture(u_texture0, TC15.xy);                                                               //TEX		color, TC15, texture[0], 2D;
	lum = (lum) + (color);                                                                              //ADD		lum, lum, color;
	
	
	// average the luminance
	//MUL		color, lum, 0.0625;
	curLum = (lum) * (vec4(0.0625));                                                                    //MUL		curLum, lum, 0.0625;
	
	//---------------------------------------------------------
	// Decode High Dynamic Range For Luminance
	//---------------------------------------------------------
	R1 = (vec4(1.0)) + (-curLum);                                                                       //ADD		R1, 1.0, -curLum;
	R1 = max(R1, vec4(0.0001));                                                                         //MAX		R1, R1, 0.0001;
	R1.x = 1.0 / R1.x;                                                                                  //RCP		R1.x, R1.x;
	curLum = (curLum) * (R1.xxxx);                                                                      //MUL		curLum, curLum, R1.x;
	//---------------------------------------------------------
	// fetch the previous luminance
	prevLum = texture(u_texture1, TC.xy);                                                               //TEX		prevLum, TC, texture[1], 2D;
	
	//---------------------------------------------------------
	// Decode two 8 bit integer values into one float
	//---------------------------------------------------------
	
	R1.x = (prevLum.x) * (0.99609375);                                                                  //MUL		R1.x, prevLum.x, 0.99609375;
	R1.y = (prevLum.y) * (0.0038909912109375);                                                          //MUL		R1.y, prevLum.y, 0.0038909912109375;
	
	prevLum = (R1.xxxx) + (R1.yyyy);                                                                    //ADD prevLum, R1.x, R1.y;
	//---------------------------------------------------------
	// Decode High Dynamic Range For Luminance
	//---------------------------------------------------------
	R1 = (vec4(1.0)) + (-prevLum);                                                                      //ADD		R1, 1.0, -prevLum;
	R1 = max(R1, vec4(0.0001));                                                                         //MAX		R1, R1, 0.0001;
	R1.x = 1.0 / R1.x;                                                                                  //RCP		R1.x, R1.x;
	prevLum = (prevLum) * (R1.xxxx);                                                                    //MUL		prevLum, prevLum, R1.x;
	//---------------------------------------------------------
	
	// fetch parameters
	lumParms = var_tc1;                                                                                 //MOV		lumParms, fragment.texcoord[1];
	
	//calculate adapted luminance
	
	//-----------------------------
	//MOV		lum.x, 0.98;
	//MUL		R1.x, lumParms.x, 10000.0;
	//POW		R1.x, lum.x, R1.x;
	//SUB		R1.x, 1, R1.x;
	//SUB		lum, prevLum, curLum;
	//MAD		lum, lum, R1.x, curLum;
	
	//-----------------------------
	//Use simplest formula that works ;)
	//-----------------------------
	lum = mix(prevLum.xxxx, curLum.xxxx, lumParms.xxxx);                                                //LRP			lum, lumParms.x, curLum.x, prevLum.x;
	
	// Clamp to Maximum allowed luminance
	lum = min(lumParms.yyyy, lum);                                                                      //MIN		lum, lumParms.y, lum;
	
	// Clamp to Minimum allowed luminance
	lum = max(lumParms.zzzz, lum);                                                                      //MAX		lum, lumParms.z, lum;
	
	//---------------------------------------------------------
	//	Compress HDR values to 0 - 1 range
	//---------------------------------------------------------
	R1 = (vec4(1.0)) + (lum);                                                                           //ADD		R1, 1.0, lum;
	R1.x = 1.0 / R1.x;                                                                                  //RCP		R1.x, R1.x;
	lum = (lum) * (R1.xxxx);                                                                            //MUL		lum, lum, R1.x;
	R1 = vec4(0);                                                                                       //MOV		R1, 0;
	//---------------------------------------------------------
	
	//------------------------------
	// Encode 24 bit float into two 8 bit integer values
	//------------------------------
	lum.x = (lum.x) * (256.0);                                                                          //MUL lum.x, lum.x, 256.0;
	R2.x = floor(lum.x);                                                                                //FLR R2.x, lum.x;
	R1.x = (R2.x) * (0.003921568627451);                                                                //MUL R1.x, R2.x, 0.003921568627451;
	lum.x = (lum.x) - (R2.x);                                                                           //SUB lum.x, lum.x, R2.x;
	lum.x = (lum.x) * (256.0);                                                                          //MUL lum.x, lum.x, 256.0;
	R2.x = floor(lum.x);                                                                                //FLR R2.x, lum.x;
	R1.y = (R2.x) * (0.003921568627451);                                                                //MUL R1.y, R2.x, 0.003921568627451;
	//-------------------------------
	draw_Color = R1;                                                                                    //MOV result.color, R1;
	//MOV result.color, lum.xxxx;
	
	
}
