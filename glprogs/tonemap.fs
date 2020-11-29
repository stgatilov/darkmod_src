#version 140

in vec4 var_TexCoord;
out vec4 draw_Color;
uniform sampler2D u_texture;

uniform float u_gamma, u_brightness;
uniform float u_desaturation;
uniform float u_colorCurveBias;
uniform float u_colorCorrection, u_colorCorrectBias;

uniform int u_sharpen;
uniform float u_sharpness;

/**
 * Contrast-adaptive sharpening from AMD's FidelityFX.
 * Adapted from Marty McFly's port for Reshade:
 * https://gist.github.com/martymcmodding/30304c4bffa6e2bd2eb59ff8bb09d135
 *
 * Note this is only the most basic form of CAS. The AMD original
 * can do more, including up- and downscaling. As that's harder to implement,
 * we're keeping it simple here.
 */
vec3 sharpen(vec2 texcoord) {
	// fetch a 3x3 neighborhood around the pixel 'e',
	//  a b c
	//  d(e)f
	//  g h i
	vec3 a = textureOffset(u_texture, texcoord, ivec2(-1, -1)).rgb;
	vec3 b = textureOffset(u_texture, texcoord, ivec2(0, -1)).rgb;
	vec3 c = textureOffset(u_texture, texcoord, ivec2(1, -1)).rgb;
	vec3 d = textureOffset(u_texture, texcoord, ivec2(-1, 0)).rgb;
	vec3 e = textureOffset(u_texture, texcoord, ivec2(0, 0)).rgb;
	vec3 f = textureOffset(u_texture, texcoord, ivec2(1, 0)).rgb;
	vec3 g = textureOffset(u_texture, texcoord, ivec2(-1, 1)).rgb;
	vec3 h = textureOffset(u_texture, texcoord, ivec2(0, 1)).rgb;
	vec3 i = textureOffset(u_texture, texcoord, ivec2(1, 1)).rgb;

	// Soft min and max.
	//  a b c             b
	//  d e f * 0.5  +  d e f * 0.5
	//  g h i             h
	// These are 2.0x bigger (factored out the extra multiply).
	vec3 mnRGB = min(min(min(d, e), min(f, b)), h);
	vec3 mnRGB2 = min(mnRGB, min(min(a, c), min(g, i)));
	mnRGB += mnRGB2;

	vec3 mxRGB = max(max(max(d, e), max(f, b)), h);
	vec3 mxRGB2 = max(mxRGB, max(max(a, c), max(g, i)));
	mxRGB += mxRGB2;

	// Smooth minimum distance to signal limit divided by smooth max.
	vec3 rcpMRGB = vec3(1) / mxRGB;
	vec3 ampRGB = clamp(min(mnRGB, 2.0 - mxRGB) * rcpMRGB, 0, 1);

	// Shaping amount of sharpening.
	ampRGB = inversesqrt(ampRGB);

	float peak = 8.0 - 3.0 * u_sharpness;
	vec3 wRGB = -vec3(1) / (ampRGB * peak);

	vec3 rcpWeightRGB = vec3(1) / (1.0 + 4.0 * wRGB);

	//                          0 w 0
	//  Filter shape:           w 1 w
	//                          0 w 0
	vec3 window = (b + d) + (f + h);
	vec3 outColor = clamp((window * wRGB + e) * rcpWeightRGB, 0, 1);

	return outColor;
}

float mapColorComponent(float value) {
	float color = value;

	//stgatilov: apply traditional gamma/brightness settings
	color = pow(color, 1.0/u_gamma);
	color *= u_brightness;

	if (u_colorCurveBias != 0.0) {
		//---------------------------------------------------------
		//  Apply Smooth Exponential color falloff
		//---------------------------------------------------------
		float reduced1 = 1.0 - pow(2.718282, -3.0 * color * color);
		color = mix(color, reduced1, u_colorCurveBias);
	}
	if (u_colorCorrectBias != 0.0) {
		//---------------------------------------------------------
		//  Apply Smooth Exponential color correction with a bias
		//---------------------------------------------------------
		float reduced2 = 1.0 - pow(2.718282, -u_colorCorrection * color);
		color = mix(color, reduced2, u_colorCorrectBias);
	}
	return color;
}

void main() {
	vec3 color;
	if (u_sharpen != 0) {
		color = sharpen(var_TexCoord.xy);
	} else {
		color = texture(u_texture, var_TexCoord.xy).rgb;
	}

	color.r = mapColorComponent(color.r);
	color.g = mapColorComponent(color.g);
	color.b = mapColorComponent(color.b);

	if (u_desaturation != 0.0) {
		float luma = clamp(dot(vec3(0.2125, 0.7154, 0.0721), color.rgb), 0.0, 1.0);
		color = mix(color, vec3(luma), u_desaturation);
	}

	draw_Color = vec4(color, 1);
}
