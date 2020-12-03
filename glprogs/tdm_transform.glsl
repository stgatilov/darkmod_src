#pragma tdm_define "VERTEX_SHADER"

uniform mat4 u_textureMatrix;

uniform block {
	uniform mat4 u_projectionMatrix;
};
uniform mat4 u_modelViewMatrix;

vec4 tdm_transform(vec4 position) {
    return u_projectionMatrix * (u_modelViewMatrix * position);
}

#define INATTR_POSITION in vec4 attr_Position;
