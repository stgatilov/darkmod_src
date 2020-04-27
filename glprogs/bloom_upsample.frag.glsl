#version 140

/**
 * This is the upsampling portion of the "dual filtering" blur as suggested in the Siggraph 2015 talk
 * "Bandwidth-efficient Rendering" by Marius Bjorge.
 */

uniform sampler2D u_blurredTexture;
uniform sampler2D u_detailTexture;
uniform int u_mipLevel;
uniform float u_detailBlendWeight;

in vec2 var_TexCoord;
out vec4 FragColor;

// sample at half-pixel offsets in the lower mipmap level
vec2 offset = vec2(0.5, 0.5) / textureSize(u_blurredTexture, u_mipLevel + 1);

vec4 sampleBlurred(vec2 offset) {
	return textureLod(u_blurredTexture, var_TexCoord + offset, u_mipLevel + 1);
}

vec4 sampleDetail() {
	return textureLod(u_detailTexture, var_TexCoord, u_mipLevel);
}

void main() {
	vec4 sum = sampleBlurred(vec2(-offset.x * 2, 0));
	sum += sampleBlurred(vec2(-offset.x, offset.y)) * 2;
	sum += sampleBlurred(vec2(0, offset.y * 2));
	sum += sampleBlurred(vec2(offset.x, offset.y)) * 2;
	sum += sampleBlurred(vec2(offset.x * 2, 0));
	sum += sampleBlurred(vec2(offset.x, -offset.y)) * 2;
	sum += sampleBlurred(vec2(0, -offset.y * 2));
	sum += sampleBlurred(vec2(-offset.x, -offset.y)) * 2;
	vec4 upsampled = sum / 12;

	vec4 detail = sampleDetail();

	FragColor = mix(upsampled, detail, u_detailBlendWeight);
}

