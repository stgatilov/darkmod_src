uniform ViewParamsBlock {
	uniform mat4 u_projectionMatrix;
};

#pragma tdm_define "MAX_SHADER_PARAMS"
#pragma tdm_define "MAX_LIGHTS"

struct PerDrawCallParams {
	mat4 modelMatrix;
	mat4 modelViewMatrix;
	vec4 bumpMatrix[2];
	vec4 diffuseMatrix[2];
	vec4 specularMatrix[2];
	vec4 colorModulate;
	vec4 colorAdd;
	vec4 diffuseColor;
	vec4 specularColor;
	vec4 hasTextureDNS;
	vec4 ambientRimColor;
    uint lightMask;
    int padding;
	// bindless texture handles - if supported
	uvec2 normalTexture;
	uvec2 diffuseTexture;
	uvec2 specularTexture;
};

struct PerLightParams {
    vec4 scissor;
    vec4 origin;
    vec4 shadowRect;
    vec4 color;
	mat4 projection;
    int shadows;
    int cubic;
    int ambient;
    int padding;
    // bindless texture handles - if supported
    uvec2 falloffTexture;
    uvec2 projectionTexture;
};

layout (std140) uniform PerDrawCallParamsBlock {
	PerDrawCallParams params[MAX_SHADER_PARAMS];
};

layout (std140) uniform PerLightParamsBlock {
    PerLightParams lights[MAX_LIGHTS];
};

layout (std140) uniform ShadowSamplesBlock {
	vec2 u_softShadowsSamples[150];
};
