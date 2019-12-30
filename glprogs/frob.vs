#version 140
// !!ARBvp1.0

#pragma tdm_include "tdm_transform.glsl"

in vec3 attr_Bitangent;
in vec3 attr_Normal;
in vec3 attr_Tangent;
in vec4 attr_Color;
INATTR_POSITION  //in vec4 attr_Position;
in vec4 attr_TexCoord;

out vec4 var_color;
out vec3 var_viewDir;
out vec4 var_tc5;
out vec3 var_normal;

uniform vec4 u_localParam0;
uniform vec4 u_viewOriginLocal;

void main() {
	var_viewDir = u_viewOriginLocal.xyz - attr_Position.xyz; 
	var_normal = attr_Normal;
	var_tc5 = u_localParam0;                                                                            //MOV		result.texcoord[5], program.local[0];
	
	var_color = attr_Color;                                                                             //MOV		result.color, vertex.color;
	
	gl_Position = tdm_transform(attr_Position);
}
