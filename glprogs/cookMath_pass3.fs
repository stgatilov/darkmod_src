#version 140
// !!ARBfp1.0 

in vec4 var_tc0;
in vec4 var_tc1;
out vec4 draw_Color;
uniform sampler2D u_texture0;

void main() {
	// OPTION ARB_precision_hint_fastest;
	
	//------------------------------------------
	vec4 TC, R1;                                                                                        //TEMP	 TC, R1;
	vec4 colorCurveBias, gamma, color;                                                                  //TEMP	 colorCurveBias, gamma, color;
	
	vec4 brightPassGamma = vec4(2.0, 0.0, 0.0, 1.0);                                                    //PARAM	brightPassGamma = { 2.0 };
	
	TC = var_tc0;                                                                                       //MOV		TC, fragment.texcoord[0];
	
	//------------------------------------------
	//	Read constant parameters.
	//------------------------------------------
	R1 = var_tc1;                                                                                       //MOV		R1, fragment.texcoord[1];
	
	colorCurveBias = R1.xxxx;                                                                           //MOV		colorCurveBias,		R1.x; 
	gamma = R1.yyyy;                                                                                    //MOV		gamma,				R1.y;
	
	//----------------------------------------
	// Read cooked data from pass 1 & 2
	//----------------------------------------
	color = texture(u_texture0, TC.xy);                                                                 //TEX		color, TC, texture[0], 2D;
	
	//---------------------------------------------------------
	//	Apply Smooth Exponential color falloff on cooked data of pass 1
	//---------------------------------------------------------
	R1.x = (color.x) * (color.x);                                                                       //MUL		R1.x, color.x, color.x;
	R1.x = (R1.x) * (-3.0);                                                                             //MUL		R1.x, R1.x, -3.0;
	R1.x = pow(2.718282, R1.x);                                                                         //POW		R1.x, 2.718282.x, R1.x;
	R1.x = (1) - (R1.x);                                                                                //SUB		R1.x, 1, R1.x;
	color.x = mix(color.x, R1.x, colorCurveBias.x);                                                     //LRP		color.x, colorCurveBias, R1.x, color.x;
	//---------------------------------------------------------
	//	Apply Gamma correction on cooked data of pass 1
	//---------------------------------------------------------
	color.x = pow(color.x, gamma.x);                                                                    //POW		color.x, color.x, gamma.x;
	
	//---------------------------------------------------------
	//	Pass on previously & now cooked data 
	//---------------------------------------------------------
	
	draw_Color = color;                                                                                 //MOV		result.color,  color;
	
}
