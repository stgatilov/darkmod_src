#version 330 core

#pragma tdm_define "BINDLESS_TEXTURES"

#ifdef BINDLESS_TEXTURES
#extension GL_ARB_bindless_texture : require
#endif

#pragma tdm_include "stages/depth/depth.params.glsl"

in float clipPlaneDist; 
in vec4 var_TexCoord0;
flat in int var_DrawId;
out vec4 FragColor;

#ifdef BINDLESS_TEXTURES
vec4 textureAlpha() {
	sampler2D tex = sampler2D(params[var_DrawId].texture);
	return texture(tex, var_TexCoord0.st);
}
#else
uniform sampler2D u_texture;
vec4 textureAlpha() {
	return texture(u_texture, var_TexCoord0.st);
}
#endif

void main() {
	if (clipPlaneDist < 0.0)
		discard;
	uvec4 scissor = params[var_DrawId].scissor;
	// manual scissor test to avoid GL state change; needed to cull invisible parts of surfaces around visportals
	if (gl_FragCoord.x < scissor.x || gl_FragCoord.y < scissor.y || gl_FragCoord.x >= scissor.x + scissor.z || gl_FragCoord.y >= scissor.y + scissor.w)
		discard;
	
	if (params[var_DrawId].alphaTest < 0) {
		FragColor = params[var_DrawId].color;
	}
	else {
		vec4 tex = textureAlpha();
		if (tex.a <= params[var_DrawId].alphaTest)
			discard;
		FragColor = tex * params[var_DrawId].color;
	}
}
