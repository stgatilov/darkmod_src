#version 140
// !!ARBfp1.0 

in vec4 var_tc0;
in vec4 var_tc1;
out vec4 draw_Color;
uniform sampler2D u_texture0;

void main() {
	// OPTION ARB_precision_hint_fastest;
	
	//------------------------------------------
	// Parameters resp. vignette power, vignette exposure.
	vec4 vignetteParams = vec4(5, 4, 0.0, 1.0);                                                         //PARAM vignetteParams = { 5, 4 }; 
	
	vec4 TC, R1;                                                                                        //TEMP	 TC, R1;
	vec4 vignetteBias, color, rcpShiftScale;                                                            //TEMP	 vignetteBias, color, rcpShiftScale;
	
	//PARAM	centerCoord		= { -0.498046875, -0.498046875, 0.0, 0.0 };
	vec4 centerCoord = vec4(-0.5, -0.5, 0.0, 0.0);                                                      //PARAM	centerCoord		= { -0.5, -0.5, 0.0, 0.0 };
	vec4 scaleOne2D = vec4(1.0, 1.0, 0.0, 0.0);                                                         //PARAM	scaleOne2D		= { 1.0, 1.0, 0.0, 0.0 };
	
	TC = var_tc0;                                                                                       //MOV		TC, fragment.texcoord[0];
	
	//------------------------------------------
	//	Read Vignette Bias.
	//------------------------------------------
	rcpShiftScale = vec4(0);                                                                            //MOV		rcpShiftScale,	0;
	vignetteBias = var_tc1.xxxx;                                                                        //MOV		vignetteBias,		fragment.texcoord[1].x;
	rcpShiftScale.x = var_tc1.y;                                                                        //MOV		rcpShiftScale.x,	fragment.texcoord[1].y;
	rcpShiftScale.y = var_tc1.z;                                                                        //MOV		rcpShiftScale.y,	fragment.texcoord[1].z;
	
	//------------------------------------------
	//	Read cooked data in all previous passes.
	//------------------------------------------
	color = texture(u_texture0, TC.xy);                                                                 //TEX		color, TC, texture[0], 2D;
	
	TC = (TC) * (rcpShiftScale);                                                                        //MUL	TC, TC, rcpShiftScale;
	//---------------------------------------------------------
	//	vignette effect with bias 
	//---------------------------------------------------------
	TC = (TC) * (scaleOne2D) + (centerCoord);                                                           //MAD		TC, TC, scaleOne2D, centerCoord;		
	R1.x = clamp(dot(TC.xyz, TC.xyz), 0.0, 1.0);                                                        //DP3_SAT	R1.x, TC, TC;
	R1.x = (1) - (R1.x);                                                                                //SUB		R1.x, 1, R1.x;
	R1.x = pow(R1.x, vignetteParams.x);                                                                 //POW		R1.x, R1.x, vignetteParams.x;
	R1.x = clamp((R1.x) * (vignetteParams.y), 0.0, 1.0);                                                //MUL_SAT	R1.x, R1.x, vignetteParams.y;
	
	R1.y = (R1.x) * (R1.x);                                                                             //MUL		R1.y, R1.x, R1.x;
	R1.y = (R1.x) * (-3.0);                                                                             //MUL		R1.y, R1.x, -3.0;
	R1.y = clamp(pow(2.718282, R1.y), 0.0, 1.0);                                                        //POW_SAT	R1.y, 2.718282.x, R1.y;
	R1.y = (1) - (R1.y);                                                                                //SUB		R1.y, 1, R1.y;
	R1.x = mix(R1.y, R1.x, 0.5);                                                                        //LRP		R1.x, 0.5, R1.x, R1.y;
	
	color.w = 1;                                                                                        //MOV		color.w, 1;		
	
	color.z = mix(1.0, R1.x, vignetteBias.x);                                                           //LRP		color.z, vignetteBias.x, R1.x, 1.0;
	
	//ADD		R1, TC.y, 0;
	//MUL		color.z, R1.x, 1;
	//---------------------------------------------------------
	
	draw_Color = color;                                                                                 //MOV		result.color,  color;
	
}
