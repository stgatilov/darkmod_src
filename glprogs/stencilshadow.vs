#version 140

#pragma tdm_include "tdm_transform.glsl"

INATTR_POSITION  //in vec4 attr_Position;
in vec4 attr_Color;

out vec4 var_Color;
  
uniform vec4 u_lightOrigin;

void main( void ) {
	if( attr_Position.w == 1.0 ) {
		// mvp transform into clip space     
		gl_Position = tdm_transform(attr_Position); 
	} else {
		// project vertex position to infinity
		vec4 vertex = attr_Position - u_lightOrigin;
		mat4 modelViewProjectionMatrix = u_projectionMatrix*u_modelViewMatrix;
		gl_Position = modelViewProjectionMatrix * vertex;
	}

	// primary color
	var_Color = attr_Color; 
}