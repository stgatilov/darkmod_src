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
// !!ARBvp1.0 OPTION ARB_position_invariant ;

#pragma tdm_include "tdm_transform.glsl"

in vec3 attr_Bitangent;
in vec3 attr_Normal;
in vec3 attr_Tangent;
in vec4 attr_Color;
INATTR_POSITION  //in vec4 attr_Position;
in vec4 attr_TexCoord;
out vec4 var_color;
out vec4 var_tc0;
out vec4 var_tc1;
out vec4 var_tc2;
out vec4 var_tc3;
out vec4 var_tc4;
uniform vec4 u_colorAdd;
uniform vec4 u_colorModulate;
uniform mat4 u_modelMatrix;
vec4 u_modelMatrixRow0 = transpose(u_modelMatrix)[0];
vec4 u_modelMatrixRow1 = transpose(u_modelMatrix)[1];
vec4 u_modelMatrixRow2 = transpose(u_modelMatrix)[2];
uniform vec4 u_viewOriginLocal;

void main() {
	
	// input:
	// 
	// attrib[8] (former texcoord[0]) TEX0	texcoords
	// attrib[9] TEX1			tangent[0]
	// attrib[10] TEX2			tangent[1]
	// attrib[2] (former normal) TEX3	normal
	//
	// c[5]	localViewOrigin
	// c[6]	modelMatrix[0]
	// c[7]	modelMatrix[1]
	// c[8]	modelMatrix[2]
	// env[16-17] control if color is by vertex or global
	// 
	// output:
	// 
	// texture 0 is the environment cube map
	// texture 1 is the normal map
	//
	// texCoord[0] is the normal map texcoord
	// texCoord[1] is the vector to the eye in global space
	// texCoord[2] is the surface tangent to global coordiantes
	// texCoord[3] is the surface bitangent to global coordiantes
	// texCoord[4] is the surface normal to global coordiantes
	
	vec4 R0, R1, R2;                                                                                    //TEMP	R0, R1, R2;
	
	// texture 0 takes the unodified texture coordinates
	var_tc0 = attr_TexCoord;                                                                            //MOV		result.texcoord[0], vertex.attrib[8];
	
	// texture 1 is the vector to the eye in global coordinates
	R0 = (u_viewOriginLocal) - (attr_Position);                                                         //SUB		R0, program.env[5], vertex.position;
	var_tc1.x = dot(R0.xyz, u_modelMatrixRow0.xyz);                                                     //DP3		result.texcoord[1].x, R0, program.env[6];
	var_tc1.y = dot(R0.xyz, u_modelMatrixRow1.xyz);                                                     //DP3		result.texcoord[1].y, R0, program.env[7];
	var_tc1.z = dot(R0.xyz, u_modelMatrixRow2.xyz);                                                     //DP3		result.texcoord[1].z, R0, program.env[8];
	
	// texture 2 gets the transformed tangent
	var_tc2.x = dot(attr_Tangent.xyz, u_modelMatrixRow0.xyz);                                           //DP3		result.texcoord[2].x, vertex.attrib[9], program.env[6];
	var_tc3.x = dot(attr_Tangent.xyz, u_modelMatrixRow1.xyz);                                           //DP3		result.texcoord[3].x, vertex.attrib[9], program.env[7];
	var_tc4.x = dot(attr_Tangent.xyz, u_modelMatrixRow2.xyz);                                           //DP3		result.texcoord[4].x, vertex.attrib[9], program.env[8];
	
	// texture 3 gets the transformed tangent
	var_tc2.y = dot(attr_Bitangent.xyz, u_modelMatrixRow0.xyz);                                         //DP3		result.texcoord[2].y, vertex.attrib[10], program.env[6];
	var_tc3.y = dot(attr_Bitangent.xyz, u_modelMatrixRow1.xyz);                                         //DP3		result.texcoord[3].y, vertex.attrib[10], program.env[7];
	var_tc4.y = dot(attr_Bitangent.xyz, u_modelMatrixRow2.xyz);                                         //DP3		result.texcoord[4].y, vertex.attrib[10], program.env[8];
	
	// texture 4 gets the transformed tangent
	var_tc2.z = dot(vec4(attr_Normal, 1).xyz, u_modelMatrixRow0.xyz);                                   //DP3		result.texcoord[2].z, vertex.attrib[2], program.env[6];
	var_tc3.z = dot(vec4(attr_Normal, 1).xyz, u_modelMatrixRow1.xyz);                                   //DP3		result.texcoord[3].z, vertex.attrib[2], program.env[7];
	var_tc4.z = dot(vec4(attr_Normal, 1).xyz, u_modelMatrixRow2.xyz);                                   //DP3		result.texcoord[4].z, vertex.attrib[2], program.env[8];
	
	//MOV		result.color, vertex.color;
	var_color = (attr_Color) * (u_colorModulate) + (u_colorAdd);                                        //MAD	result.color, vertex.color, program.env[16], program.env[17];
	
	gl_Position = tdm_transform(attr_Position);
}
