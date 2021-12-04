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

vec3 CubeMapDirectionToUv(vec3 v, out int faceIdx) {
	vec3 v1 = abs(v);
	float maxV = max(v1.x, max(v1.y, v1.z));
	faceIdx = 0;
	if(maxV == v.x) {
		v1 = -v.zyx;
	}
	else if(maxV == -v.x) {
		v1 = v.zyx * vec3(1, -1, 1);
		faceIdx = 1;
	}
	else if(maxV == v.y) {
		v1 = v.xzy * vec3(1, 1, -1);
		faceIdx = 2;
	}
	else if(maxV == -v.y) {
		v1 = v.xzy * vec3(1, -1, 1);
		faceIdx = 3;
	}
	else if(maxV == v.z) {
		v1 = v.xyz * vec3(1, -1, -1);
		faceIdx = 4;
	}
	else { //if(maxV == -v.z) {
		v1 = v.xyz * vec3(-1, -1, 1);
		faceIdx = 5;
	}
	v1.xy /= -v1.z;
	return v1;
}

//shadowRect defines where one face of this light is within the map atlas:
//  (offsetX, offsetY, ?, size)     (all from 0..1)
//lightVec is world-space vector from the light origin to the fragment being considered
//returns positive distance along face normal of cube map (i.e. along coordinate where lightVec has maximum absolute value)
float ShadowAtlasForVector(in sampler2D shadowMapTexture, vec4 shadowRect, vec3 lightVec) {
	int faceIdx;
	vec3 v1 = CubeMapDirectionToUv(lightVec, faceIdx);
	vec2 texSize = textureSize(shadowMapTexture, 0);
	vec2 shadow2d = (v1.xy * .5 + vec2(.5) ) * shadowRect.ww + shadowRect.xy;
	shadow2d.x += (shadowRect.w + 1./texSize.x) * faceIdx;
	float d = textureLod(shadowMapTexture, shadow2d, 0).r;
	return 1 / (1 - d);
}
#if TDM_allow_ARB_texture_gather
vec4 ShadowAtlasForVector4(in sampler2D shadowMapTexture, vec4 shadowRect, vec3 lightVec, out vec4 sampleWeights) {
	int faceIdx;
	vec3 v1 = CubeMapDirectionToUv(lightVec, faceIdx);
	vec2 texSize = textureSize(shadowMapTexture, 0);
	vec2 shadow2d = (v1.xy * .5 + vec2(.5) ) * shadowRect.ww + shadowRect.xy;
	shadow2d.x += (shadowRect.w + 1./texSize.x) * faceIdx;
#if GL_ARB_texture_gather
	vec4 d = textureGather(shadowMapTexture, shadow2d);
#else
    vec4 d = textureLod(shadowMapTexture, shadow2d, 0).rrrr;
#endif
	vec2 wgt = fract(shadow2d * texSize - 0.5);
	vec2 mwgt = vec2(1) - wgt;
	sampleWeights = vec4(mwgt.x, wgt.x, wgt.x, mwgt.x) * vec4(wgt.y, wgt.y, mwgt.y, mwgt.y);
	return vec4(1) / (vec4(1) - d);
}
#endif
