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
out vec4 var_tc0;
out vec4 var_tc1;
out vec4 var_tc2;
out vec4 var_tc3;
out vec4 var_tc4;
out vec4 var_tc5;
uniform vec4 u_localParam0;
uniform vec4 u_modelMatrixRow0;
uniform vec4 u_modelMatrixRow1;
uniform vec4 u_modelMatrixRow2;
uniform vec4 u_viewOriginLocal;

void main() {
	// OPTION ARB_position_invariant;
	
	vec4 R0;                                                                                            //TEMP R0;
	
	// texture 0 takes the unodified texture coordinates
	var_tc0 = attr_TexCoord;                                                                            //MOV		result.texcoord[0], vertex.texcoord[0];
	
	// texture 1 is the vector to the eye in global coordinates
	R0 = (u_viewOriginLocal) - (attr_Position);                                                         //SUB		R0, program.env[5], vertex.position;
	var_tc1.x = dot(R0.xyz, u_modelMatrixRow0.xyz);                                                     //DP3		result.texcoord[1].x, R0, program.env[6];
	var_tc1.y = dot(R0.xyz, u_modelMatrixRow1.xyz);                                                     //DP3		result.texcoord[1].y, R0, program.env[7];
	var_tc1.z = dot(R0.xyz, u_modelMatrixRow2.xyz);                                                     //DP3		result.texcoord[1].z, R0, program.env[8];
	
	// texture 2 gets the transformed tangent
	var_tc2.x = dot(attr_Tangent.xyz, u_modelMatrixRow0.xyz);                                           //DP3		result.texcoord[2].x, vertex.attrib[9], program.env[6];
	var_tc3.x = dot(attr_Tangent.xyz, u_modelMatrixRow1.xyz);                                           //DP3		result.texcoord[3].x, vertex.attrib[9], program.env[7];
	var_tc4.x = dot(attr_Tangent.xyz, u_modelMatrixRow2.xyz);                                           //DP3		result.texcoord[4].x, vertex.attrib[9], program.env[8];
	
	// texture 3 gets the transformed tangent
	var_tc2.y = dot(attr_Bitangent.xyz, u_modelMatrixRow0.xyz);                                         //DP3		result.texcoord[2].y, vertex.attrib[10], program.env[6];
	var_tc3.y = dot(attr_Bitangent.xyz, u_modelMatrixRow1.xyz);                                         //DP3		result.texcoord[3].y, vertex.attrib[10], program.env[7];
	var_tc4.y = dot(attr_Bitangent.xyz, u_modelMatrixRow2.xyz);                                         //DP3		result.texcoord[4].y, vertex.attrib[10], program.env[8];
	
	// texture 4 gets the transformed tangent
	var_tc2.z = dot(vec4(attr_Normal, 1).xyz, u_modelMatrixRow0.xyz);                                   //DP3		result.texcoord[2].z, vertex.normal, program.env[6];
	var_tc3.z = dot(vec4(attr_Normal, 1).xyz, u_modelMatrixRow1.xyz);                                   //DP3		result.texcoord[3].z, vertex.normal, program.env[7];
	var_tc4.z = dot(vec4(attr_Normal, 1).xyz, u_modelMatrixRow2.xyz);                                   //DP3		result.texcoord[4].z, vertex.normal,program.env[8];
	
	var_tc5 = u_localParam0;                                                                            //MOV		result.texcoord[5], program.local[0];
	
	var_color = attr_Color;                                                                             //MOV		result.color, vertex.color;
	
	gl_Position = tdm_transform(attr_Position);
}
