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

#pragma tdm_include "tdm_transform.glsl"

INATTR_POSITION  //in vec4 attr_Position;
in vec4 attr_TexCoord;
in vec3 attr_Tangent;
in vec3 attr_Bitangent;   
in vec3 attr_Normal;   
in vec4 attr_Color;   

uniform vec4[2]	u_bumpMatrix;
uniform vec4[2]	u_diffuseMatrix;
uniform vec4[2]	u_specularMatrix;   
uniform vec4 u_colorModulate; 
uniform vec4 u_colorAdd; 
uniform mat4 u_modelMatrix;

out vec4 var_Position;  
out vec4 var_PositionWorld;
out vec2 var_TexDiffuse;        
out vec2 var_TexNormal;  
out vec2 var_TexSpecular;  
out mat3 var_TangentBitangentNormalMatrix; 
out vec4 var_Color;  
 
void main() {
	gl_Position = tdm_transform(attr_Position);
	var_Position = u_modelMatrix*attr_Position;
	var_PositionWorld = u_modelMatrix * attr_Position;

	// normal map texgen   
	var_TexNormal.x = dot(attr_TexCoord, u_bumpMatrix[0]);
	var_TexNormal.y = dot(attr_TexCoord, u_bumpMatrix[1]);
 
	// diffuse map texgen      
	var_TexDiffuse.x = dot(attr_TexCoord, u_diffuseMatrix[0]);
	var_TexDiffuse.y = dot(attr_TexCoord, u_diffuseMatrix[1]);
 
	// specular map texgen  
	var_TexSpecular.x = dot(attr_TexCoord, u_specularMatrix[0]);
	var_TexSpecular.y = dot(attr_TexCoord, u_specularMatrix[1]);

	// construct tangent-bitangent-normal 3x3 matrix   
	//var_TangentBitangentNormalMatrix = mat3( clamp(attr_Tangent,-1,1), clamp(attr_Bitangent,-1,1), clamp(attr_Normal,-1,1) );
	var_TangentBitangentNormalMatrix[0] = mat3(u_modelMatrix)*attr_Tangent;
	var_TangentBitangentNormalMatrix[1] = mat3(u_modelMatrix)*attr_Bitangent;
	var_TangentBitangentNormalMatrix[2] = mat3(u_modelMatrix)*attr_Normal;

	// primary color 
	var_Color = (attr_Color * u_colorModulate) + u_colorAdd;  
}