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
in vec3 attr_TexCoord;
in vec3 attr_Tangent;     
in vec3 attr_Bitangent;     
in vec3 attr_Normal;  

uniform mat4 u_textureMatrix;
uniform vec3 u_viewOrigin; 
uniform mat4 u_modelMatrix;
uniform bool u_skybox;

out mat3 var_TangentBinormalNormalMatrix;  
out vec3 var_viewDir;
out vec4 var_TexCoord0;

void main() {
	if(u_skybox) {
		var_TexCoord0 = attr_Position - vec4(u_viewOrigin, 1);
	} else
		var_TexCoord0 = u_textureMatrix * vec4(attr_TexCoord, 1);
	gl_Position = tdm_transform(attr_Position);
	var_TangentBinormalNormalMatrix = mat3( attr_Tangent, attr_Bitangent, attr_Normal ); 
	var_viewDir = u_viewOrigin - (attr_Position * u_modelMatrix).xyz;
}