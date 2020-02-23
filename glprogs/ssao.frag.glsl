#version 140

// This is an SSAO implementation working purely from the depth buffer.
// The implementation is inspired by: https://learnopengl.com/Advanced-Lighting/SSAO
//
// For each texel in the output, its view space position and normal are inferred
// from the depth buffer, then the surrounding geometry is probed with a small
// kernel of samples distributed over a hemisphere aligned with the normal.
// Each sample is compared with the actual depth in the depth buffer, and if it is
// occluded, it contributes to this texel's AO term.

in vec2 var_TexCoord;
out float FragColor;

uniform sampler2D u_depthTexture;
uniform sampler2D u_noiseTexture;

// Adjustable SSAO parameters
uniform float u_sampleRadius;
uniform float u_depthCutoff;
uniform float u_depthBias;
uniform float u_baseValue;
uniform float u_power;

uniform block {
	mat4 u_projectionMatrix;
};

float nearZ = -0.5 * u_projectionMatrix[3][2];
vec2 halfTanFov = vec2(1 / u_projectionMatrix[0][0], 1 / u_projectionMatrix[1][1]);

float depthToZ(vec2 texCoord) {
	float depth = texture(u_depthTexture, texCoord).r;
	return nearZ / (depth + 0.5 * (u_projectionMatrix[2][2] - 1));
}

// map a texel in the depth texture back to view space coordinates by reversing the projection
vec3 texCoordToViewPos(vec2 texCoord) {
	vec3 viewPos;
	viewPos.z = depthToZ(texCoord);
	viewPos.xy = -halfTanFov * (2 * texCoord - 1) * viewPos.z;
	return viewPos;
}

// determine the actual occluding depth value in view space for a given view space position
float occluderZAtViewPos(vec3 viewPos) {
	vec4 clipPos = u_projectionMatrix * vec4(viewPos, 1);
	vec2 texCoord = 0.5 + 0.5 * (clipPos.xy / clipPos.w);
	return depthToZ(texCoord);
}

// the actual sample kernel, samples should be distributed over the unit hemisphere with z >= 0
uniform vec3 u_sampleKernel[128];
uniform int u_kernelSize;

void main() {
	// calculate the position and normal of the current texel in view space
	vec3 position = texCoordToViewPos(var_TexCoord);
	vec3 dx = dFdx(position);
	vec3 dy = dFdy(position);
	vec3 normal = normalize(cross(dx, dy));

	// query a small noise texture to acquire a random rotation vector
	vec2 noiseScale = vec2(textureSize(u_depthTexture, 0)) / 4;
	vec3 random = normalize(-1 + 2 * texture(u_noiseTexture, var_TexCoord * noiseScale).rgb);

	// use the random vector to build a randomly rotated tangent space for the current texel
	// this is done to require fewer samples per texel
	vec3 tangent = normalize(random - normal * dot(random, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);

	// calculate actual occlusion value
	float occlusion = 0.0;
	for (int i = 0; i < u_kernelSize; i++) {
		// determine sample position in view space
		vec3 samplePos = position + u_sampleRadius * (TBN * u_sampleKernel[i]);

		// determine actual depth at sample position and compare to sample
		float occluderZ = occluderZAtViewPos(samplePos);
		float difference = occluderZ - samplePos.z;

		// introduce a cut-off factor if the depth difference is larger than the cutoff to avoid unwanted
		// shadow halos for objects that are actually a distance apart
		float rangeCheck = smoothstep(0.0, 1.0, u_depthCutoff / abs(position.z - occluderZ));
		occlusion += step(u_depthBias, difference) * rangeCheck;
	}

	float ao = clamp(u_baseValue + 1.0 - occlusion / u_kernelSize, 0, 1);
	FragColor = pow(ao, u_power);
}
