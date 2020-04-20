#version 140
#pragma tdm_define "BLOOM_BRIGHTPASS"

/**
 * This is the downsampling portion of the "dual filtering" blur as suggested in the Siggraph 2015 talk
 * "Bandwidth-efficient Rendering" by Marius Bjorge.
 */

uniform sampler2D u_sourceTexture;
uniform int u_sourceMipLevel;

in vec2 var_TexCoord;
out vec4 FragColor;

#ifdef BLOOM_BRIGHTPASS
const vec3 toGrayscale = vec3(0.2126, 0.7152, 0.0722);
uniform float u_brightnessThreshold;
uniform float u_thresholdFalloff;

float brightpass(vec3 color) {
	float brightness = dot(color.rgb, toGrayscale);
	return clamp(pow(brightness / u_brightnessThreshold, u_thresholdFalloff), 0, 1);
}
#endif

vec4 sampleTexture(vec2 offset) {
	return textureLod(u_sourceTexture, var_TexCoord + offset, u_sourceMipLevel);
}

void main() {
	// query previous mipmap level by a full-pixel offset (corresponds to half-pixel in our output framebuffer)
	vec2 offset = vec2(1, 1) / textureSize(u_sourceTexture, u_sourceMipLevel);
	vec4 sum = sampleTexture(vec2(0, 0)) * 4;
	sum += sampleTexture(-offset);
	sum += sampleTexture(offset);
	sum += sampleTexture(vec2(offset.x, -offset.y));
	sum += sampleTexture(vec2(-offset.x, offset.y));
	FragColor = sum / 8;
#ifdef BLOOM_BRIGHTPASS
	FragColor.rgb *= brightpass(FragColor.rgb);
#endif
}
