#version 140
// !!ARBfp1.0 

in vec4 var_tc0;
in vec4 var_tc1;
out vec4 draw_Color;

void main() {
	// OPTION ARB_precision_hint_fastest;
	
	//------------------------------------------
	vec4 TC, R1, R2;                                                                                    //TEMP	 TC, R1, R2;
	vec4 colorCurveBias, gamma, sceneExposure, color;                                                   //TEMP	 colorCurveBias, gamma, sceneExposure, color;
	
	vec4 R1_G0_B0 = vec4(1, 0, 0, 1.0);                                                                 //PARAM	R1_G0_B0 = { 1, 0, 0 };
	
	TC = var_tc0;                                                                                       //MOV		TC, fragment.texcoord[0];
	
	//------------------------------------------
	//	Variables for Cooking: X = scene color texture, Y = None.
	//------------------------------------------
	color = TC.xxxx;                                                                                    //MOV		color,		TC.x;
	
	//------------------------------------------
	//	Rest of the parameters are constant.
	//------------------------------------------
	R1 = var_tc1;                                                                                       //MOV		R1, fragment.texcoord[1];
	
	colorCurveBias = R1.xxxx;                                                                           //MOV		colorCurveBias,			R1.x; 
	gamma = R1.yyyy;                                                                                    //MOV		gamma,					R1.y;
	sceneExposure = R1.zzzz;                                                                            //MOV		sceneExposure,			R1.z;
	//------------------------------------------
	color = (color) * (sceneExposure);                                                                  //MUL		color, color, sceneExposure;
	
	color.x = pow(color.x, gamma.x);                                                                    //POW		color.x, color.x, gamma.x;
	//---------------------------------------------------------
	//	Apply Smooth Exponential color falloff on cooked data of pass 1
	//---------------------------------------------------------
	R1.x = (color.x) * (color.x);                                                                       //MUL		R1.x, color.x, color.x;
	R1.x = (R1.x) * (-3.0);                                                                             //MUL		R1.x, R1.x, -3.0;
	R1.x = pow(2.718282, R1.x);                                                                         //POW		R1.x, 2.718282.x, R1.x;
	R1.x = (1) - (R1.x);                                                                                //SUB		R1.x, 1, R1.x;
	color.x = mix(color.x, R1.x, colorCurveBias.x);                                                     //LRP		color.x, colorCurveBias, R1.x, color.x;
	//---------------------------------------------------------
	
	draw_Color = (color) * (R1_G0_B0);                                                                  //MUL		result.color,  color, R1_G0_B0;
	
}
