#version 140
// !!ARBfp1.0 

in vec4 var_tc0;
in vec4 var_tc1;
out vec4 draw_Color;
uniform sampler2D u_texture0;

void main() {
	// OPTION ARB_precision_hint_fastest;
	
	//------------------------------------------
	vec4 TC, R1, R2, blendParams;                                                                       //TEMP	 TC, R1, R2, blendParams;
	vec4 color, brightPassThreshold, brightPassOffset, brightPassColor, colorCorrection, colorCorrectionBias;//TEMP	 color, brightPassThreshold, brightPassOffset, brightPassColor, colorCorrection, colorCorrectionBias;
	
	vec4 brightPassGamma = vec4(1.0, 0.0, 0.0, 1.0);                                                    //PARAM	brightPassGamma = { 1.0 };
	
	TC = var_tc0;                                                                                       //MOV		TC, fragment.texcoord[0];
	
	//------------------------------------------
	//	Variables for Cooking: X = scene color texture, Y = None.
	//------------------------------------------
	color = TC.xxxx;                                                                                    //MOV		color,		TC.x;
	
	//------------------------------------------
	//	Rest of the parameters are constant.
	//------------------------------------------
	R1 = var_tc1;                                                                                       //MOV		R1, fragment.texcoord[1];
	
	brightPassThreshold = R1.xxxx;                                                                      //MOV		brightPassThreshold,	R1.x;
	brightPassOffset = R1.yyyy;                                                                         //MOV		brightPassOffset,		R1.y;
	colorCorrection = R1.zzzz;                                                                          //MOV		colorCorrection,		R1.z;
	colorCorrectionBias = R1.wwww;                                                                      //MOV		colorCorrectionBias,	R1.w;
	
	//----------------------------------------
	// Cook Bright pass data 
	//----------------------------------------
	brightPassColor = (color) - (brightPassThreshold);                                                  //SUB		brightPassColor, color, brightPassThreshold;
	
	brightPassColor = max(brightPassColor, vec4(0.0));                                                  //MAX		brightPassColor, brightPassColor, 0.0;
	
	brightPassColor.x = pow(brightPassColor.x, brightPassGamma.x);                                      //POW		brightPassColor.x, brightPassColor.x, brightPassGamma.x;
	
	R1 = (brightPassOffset) + (brightPassColor);                                                        //ADD		R1, brightPassOffset, brightPassColor;
	R1.x = 1.0 / R1.x;                                                                                  //RCP		R1.x, R1.x;
	
	brightPassColor = (brightPassColor) * (R1.xxxx);                                                    //MUL		brightPassColor, brightPassColor, R1.x;
	
	//----------------------------------------
	// Pass over the previously cooked data to x 
	// and store the now cooked data to y.
	//----------------------------------------
	color = texture(u_texture0, TC.xy);                                                                 //TEX		color, TC, texture[0], 2D;
	
	//---------------------------------------------------------
	//	Apply Smooth Exponential color correction with a bias
	//---------------------------------------------------------
	R1.x = (color.x) * (-colorCorrection.x);                                                            //MUL		R1.x, color.x, -colorCorrection;
	R1.x = pow(2.718282, R1.x);                                                                         //POW		R1.x, 2.718282.x, R1.x;
	R1.x = (1) - (R1.x);                                                                                //SUB		R1.x, 1, R1.x;
	color.x = mix(color.x, R1.x, colorCorrectionBias.x);                                                //LRP		color.x, colorCorrectionBias, R1.x, color.x;
	//---------------------------------------------------------
	
	
	color.y = brightPassColor.x;                                                                        //MOV		color.y, brightPassColor.x;
	draw_Color = color;                                                                                 //MOV		result.color,  color;
	
}
