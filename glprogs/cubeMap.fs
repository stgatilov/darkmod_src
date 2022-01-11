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

uniform samplerCube texCube;
uniform sampler2D u_normalTexture;

uniform float u_reflective;
uniform float u_RGTC;

in mat3 var_TangentBinormalNormalMatrix;  
in vec3 var_viewDir;
in vec4 var_TexCoord0;

out vec4 FragColor;

void main() {
	vec3 texCoord = var_TexCoord0.xyz;
	if(u_reflective==1.0) {
		vec4 bumpTexel = texture ( u_normalTexture, var_TexCoord0.st ) * 2. - 1.;
		vec3 localNormal = u_RGTC == 1. 
			? vec3(bumpTexel.x, bumpTexel.y, sqrt(max(1.-bumpTexel.x*bumpTexel.x-bumpTexel.y*bumpTexel.y, 0)))
			: normalize( bumpTexel.xyz ); 
		vec3 N = var_TangentBinormalNormalMatrix * localNormal;
		N = normalize(N);
		vec3 nViewDir = normalize(var_viewDir);
		vec3 reflect = - (nViewDir - 2*N*dot(N, nViewDir));
		texCoord = reflect;
		//gl_FragColor.xyz = abs(reflect); return;
	}
	vec4 tex = texture(texCube, texCoord);
	FragColor = tex;	
}