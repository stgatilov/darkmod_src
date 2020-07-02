#pragma tdm_define "MAX_SHADER_PARAMS"

struct PerDrawCallParams {
	mat4 modelViewMatrix;
	mat4 textureMatrix;
	vec4 color;
	uvec4 scissor;
	uvec2 texture;
	float alphaTest;
	float padding;
};

layout (std140) uniform PerDrawCallParamsBlock {
	PerDrawCallParams params[MAX_SHADER_PARAMS];
};
