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

vec4 computeLightTex(mat4 lightProjectionFalloff, vec4 position) {
	//in: divisor in Z, falloff in W
	return ( position * lightProjectionFalloff ).xywz;
	//out: divisor in W, falloff in Z
}

vec3 projFalloffOfNormalLight(in sampler2D lightProjectionTexture, in sampler2D lightFalloffTexture, vec4 texLight) {
	vec3 projCoords = texLight.xyw;     //divided by last component
	float falloffCoord = texLight.z;

	if (
		projCoords.z <= 0 ||                                            //anything with inversed W
		projCoords.x < 0 || projCoords.x > projCoords.z ||              //proj U outside [0..1]
		projCoords.y < 0 || projCoords.y > projCoords.z ||              //proj V outside [0..1]
		falloffCoord < 0 || falloffCoord > 1.0                          //falloff outside [0..1]
	) {
		return vec3(0);
	}

	vec3 lightProjection = textureProj(lightProjectionTexture, projCoords).rgb;
	vec3 lightFalloff = texture(lightFalloffTexture, vec2(falloffCoord, 0.5)).rgb;
	return lightProjection * lightFalloff;
}

vec3 projFalloffOfCubicLight(in samplerCube	lightProjectionCubemap, vec4 texLight) {
	vec3 cubeTC = texLight.xyz * 2.0 - 1.0;
	vec3 lightColor = texture(lightProjectionCubemap, cubeTC).rgb;
	float att = clamp(1.0 - length(cubeTC), 0.0, 1.0);
	lightColor *= att * att;
	return lightColor;
}
