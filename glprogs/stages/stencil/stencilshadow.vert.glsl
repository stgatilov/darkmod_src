#version 330 core

uniform ViewParamsBlock {
	uniform mat4 u_projectionMatrix;
};

#pragma tdm_define "MAX_SHADER_PARAMS"

struct ShaderParams {
    mat4 modelViewMatrix;
    vec4 localLightOrigin;
};

layout (std140) uniform ShaderParamsBlock {
    ShaderParams params[MAX_SHADER_PARAMS];
};

in vec4 attr_Position;
in vec4 attr_Color;
in int attr_DrawId;

out vec4 var_Color;
  
void main( void ) {
    vec4 projectedPosition = attr_Position;
	if( attr_Position.w != 1.0 ) {
		// project vertex position to infinity
        projectedPosition -= params[attr_DrawId].localLightOrigin;
    }

	gl_Position = u_projectionMatrix * (params[attr_DrawId].modelViewMatrix * projectedPosition);

	// primary color
	var_Color = attr_Color; 
}
