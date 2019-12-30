#pragma tdm_define "UNIFORM_TRANSFORMS"
#pragma tdm_define "VERTEX_SHADER"

uniform mat4 u_textureMatrix;

#ifdef UNIFORM_TRANSFORMS

uniform block {
	uniform mat4 u_projectionMatrix;
};
//	uniform mat4 u_projectionMatrix;
	uniform mat4 u_modelViewMatrix;

    vec4 tdm_transform(vec4 position) {
        return u_projectionMatrix * (u_modelViewMatrix * position);
    }

    #define INATTR_POSITION in vec4 attr_Position;

#else

    #if __VERSION__ >= 140
    #extension GL_ARB_compatibility: enable
    #endif

	mat4 u_projectionMatrix = gl_ProjectionMatrix;
	mat4 u_modelViewMatrix = gl_ModelViewMatrix;

	#ifdef VERTEX_SHADER
    vec4 tdm_transform(vec4 position) {
        return ftransform();
    }
    #endif

    #define INATTR_POSITION vec4 attr_Position = gl_Vertex;

#endif
