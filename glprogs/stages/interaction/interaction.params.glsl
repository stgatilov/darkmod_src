uniform ViewParamsBlock {
	uniform mat4 u_projectionMatrix;
};

#pragma tdm_define "MAX_SHADER_PARAMS"

struct PerDrawCallParams {
	mat4 modelMatrix;
	mat4 modelViewMatrix;
	vec4 bumpMatrix[2];
	vec4 diffuseMatrix[2];
	vec4 specularMatrix[2];
	mat4 lightProjectionFalloff;
	vec4 colorModulate;
	vec4 colorAdd;
	vec4 lightOrigin;
	vec4 viewOrigin;
	vec4 diffuseColor;
	vec4 specularColor;
	vec4 hasTextureDNS;
	vec4 ambientRimColor;
	// bindless texture handles - if supported
	uvec2 normalTexture;
	uvec2 diffuseTexture;
	uvec2 specularTexture;
	uvec2 padding;
};

layout (std140) uniform PerDrawCallParamsBlock {
	PerDrawCallParams params[MAX_SHADER_PARAMS];
};

layout (std140) uniform ShadowSamplesBlock {
	vec2 u_softShadowsSamples[150];
};
