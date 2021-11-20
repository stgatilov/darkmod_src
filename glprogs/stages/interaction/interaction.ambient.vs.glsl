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
#version 330 core

#pragma tdm_include "stages/interaction/interaction.params.glsl"
#pragma tdm_include "tdm_lightproject.glsl"

in vec4 attr_Position;
in vec4 attr_TexCoord;
in vec3 attr_Tangent;
in vec3 attr_Bitangent;
in vec3 attr_Normal;
in vec4 attr_Color;
in int attr_DrawId;
 
out vec3 var_Position;
out vec2 var_TexDiffuse;
out vec2 var_TexSpecular;
out vec2 var_TexNormal;
out vec4 var_TexLight;
out mat3 var_TangentBinormalNormalMatrix;
out vec4 var_Color;
out vec3 var_tc0;
out vec3 var_localViewDir;
out vec4 var_ClipPosition;
flat out int var_DrawId;

void main( void ) {     
	// transform vertex position into homogenous clip-space  
	var_ClipPosition = u_projectionMatrix * (params[attr_DrawId].modelViewMatrix * attr_Position);
	gl_Position = var_ClipPosition;
	
	// transform vertex position into world space  
	var_Position = attr_Position.xyz;

	// normal map texgen   
	var_TexNormal.x = dot(attr_TexCoord, params[attr_DrawId].bumpMatrix[0]);
	var_TexNormal.y = dot(attr_TexCoord, params[attr_DrawId].bumpMatrix[1]);
 
	// diffuse map texgen      
	var_TexDiffuse.x = dot(attr_TexCoord, params[attr_DrawId].diffuseMatrix[0]);
	var_TexDiffuse.y = dot(attr_TexCoord, params[attr_DrawId].diffuseMatrix[1]);
 
	// specular map texgen  
	var_TexSpecular.x = dot(attr_TexCoord, params[attr_DrawId].specularMatrix[0]);
	var_TexSpecular.y = dot(attr_TexCoord, params[attr_DrawId].specularMatrix[1]);
 
	// light projection texgen
	var_TexLight = computeLightTex(params[attr_DrawId].lightProjectionFalloff, attr_Position);

	// construct tangent-binormal-normal 3x3 matrix    
	var_TangentBinormalNormalMatrix = mat3( attr_Tangent, attr_Bitangent, attr_Normal ); 
	//var_tc0 = u_lightOrigin.xyz.xyz * var_TangentBinormalNormalMatrix;
	var_localViewDir = (params[attr_DrawId].viewOrigin.xyz - var_Position).xyz;

	// primary color 
	var_Color = (attr_Color * params[attr_DrawId].colorModulate) + params[attr_DrawId].colorAdd;  
	
	var_DrawId = attr_DrawId;
}