#pragma tdm_define "MAX_SHADER_PARAMS"

struct ShaderParams {
    mat4 modelViewMatrix;
	mat4 textureMatrix;
	vec4 color;
	vec4 scissor;
	vec2 alphaTest;
	uvec2 texture;
};

layout (std140) uniform ShaderParamsBlock {
    ShaderParams params[MAX_SHADER_PARAMS];
};
