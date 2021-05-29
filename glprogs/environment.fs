/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/
#version 140
// !!ARBfp1.0 

in vec4 var_color;
in vec4 var_tc0;
in vec4 var_tc1;
out vec4 draw_Color;
uniform samplerCube u_texture0;

void main() {
	// OPTION ARB_precision_hint_fastest;
	
	// per-pixel cubic reflextion map calculation
	
	// texture 0 is the cube map
	// texCord[0] is the surface normal
	// texCord[1] is toEye, the eye XYZ - the surface XYZ
	
	vec4 toEye, normal, R0;                                                                             //TEMP	toEye, normal, R0;
	
	vec4 scaleTwo = vec4(2, 2, 2, 2);                                                                   //PARAM	scaleTwo = { 2, 2, 2, 2 };
	
	// normalize surface normal
	R0 = vec4(dot(var_tc0.xyz, var_tc0.xyz));                                                           //DP3	R0, fragment.texcoord[0], fragment.texcoord[0];
	R0 = vec4(1.0 / sqrt(R0.x));                                                                        //RSQ	R0, R0.x;
	normal = (var_tc0) * (R0);                                                                          //MUL	normal, fragment.texcoord[0], R0;
	
	// normalize vector to eye
	R0 = vec4(dot(var_tc1.xyz, var_tc1.xyz));                                                           //DP3	R0, fragment.texcoord[1], fragment.texcoord[1];
	R0 = vec4(1.0 / sqrt(R0.x));                                                                        //RSQ	R0, R0.x;
	toEye = (var_tc1) * (R0);                                                                           //MUL	toEye, fragment.texcoord[1], R0;
	
	// calculate reflection vector
	R0 = vec4(dot(toEye.xyz, normal.xyz));                                                              //DP3 	R0, toEye, normal;
	R0 = (R0) * (normal);                                                                               //MUL	R0, R0, normal;
	R0 = (R0) * (scaleTwo) + (-toEye);                                                                  //MAD	R0, R0, scaleTwo, -toEye;
	
	R0 = texture(u_texture0, R0.xyz);                                                                   //TEX	R0, R0, texture[0], CUBE;
	
	// this should be better on future hardware, but current drivers make it slower
	//MUL	result.color.xyz, R0, fragment.color;
	
	draw_Color = (R0) * (var_color);                                                                    //MUL	result.color, R0, fragment.color;
	//MOV	result.color, fragment.texcoord[1];
	
}
