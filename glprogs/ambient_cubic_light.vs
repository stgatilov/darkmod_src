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
out vec4 var_tc6;
out vec4 var_tc7;
uniform vec4 u_bumpMatrixS;
uniform vec4 u_bumpMatrixT;
uniform vec4 u_colorAdd;
uniform vec4 u_colorModulate;
uniform vec4 u_lightFalloffS;
uniform vec4 u_lightOriginLocal;
uniform vec4 u_lightProjectQ;
uniform vec4 u_lightProjectS;
uniform vec4 u_lightProjectT;
uniform vec4 u_viewOriginLocal;
uniform vec4 u_worldUpLocal;

void main() {
	// OPTION ARB_position_invariant;
	
	// Instruction Count: 25
	
	vec4 defaultTexCoord = vec4(0.0, 0.5, 0.0, 1.0);                                                    //PARAM	defaultTexCoord = { 0.0, 0.5, 0.0, 1.0 };
	
	vec4 R0;                                                                                            //TEMP	R0;
	
	// world space light vector
	R0 = (u_lightOriginLocal) + (-attr_Position);                                                       //ADD		R0, program.env[4], -vertex.position;
	
	// put into texture space for TEX0
	var_tc0.x = dot(attr_Tangent.xyz, R0.xyz);                                                          //DP3		result.texcoord[0].x, vertex.attrib[9], R0;
	var_tc0.y = dot(attr_Bitangent.xyz, R0.xyz);                                                        //DP3		result.texcoord[0].y, vertex.attrib[10], R0;
	var_tc0.z = dot(vec4(attr_Normal, 1).xyz, R0.xyz);                                                  //DP3		result.texcoord[0].z, vertex.attrib[11], R0;
	
	// texture 1 takes the base coordinates by the texture matrix
	var_tc1 = defaultTexCoord;                                                                          //MOV		result.texcoord[1], defaultTexCoord;
	var_tc1.x = dot(attr_TexCoord, u_bumpMatrixS);                                                      //DP4 	result.texcoord[1].x, vertex.attrib[8], program.env[10];
	var_tc1.y = dot(attr_TexCoord, u_bumpMatrixT);                                                      //DP4 	result.texcoord[1].y, vertex.attrib[8], program.env[11];
	
	// texture 2 has three texgens (cubemap projection)
	var_tc2.x = dot(attr_Position, u_lightProjectS);                                                    //DP4		result.texcoord[2].x, vertex.position, program.env[6];
	var_tc2.y = dot(attr_Position, u_lightProjectT);                                                    //DP4		result.texcoord[2].y, vertex.position, program.env[7];
	var_tc2.z = dot(attr_Position, u_lightFalloffS);                                                    //DP4		result.texcoord[2].z, vertex.position, program.env[9];
	var_tc2.w = dot(attr_Position, u_lightProjectQ);                                                    //DP4		result.texcoord[2].w, vertex.position, program.env[8];
	
	// tangent space -> world space conversion matrix
	var_tc3.x = dot(attr_Tangent.xyz, u_lightProjectS.xyz);                                             //DP3		result.texcoord[3].x, vertex.attrib[9], program.env[6];
	var_tc3.y = dot(attr_Bitangent.xyz, u_lightProjectS.xyz);                                           //DP3		result.texcoord[3].y, vertex.attrib[10], program.env[6];
	var_tc3.z = dot(vec4(attr_Normal, 1).xyz, u_lightProjectS.xyz);                                     //DP3		result.texcoord[3].z, vertex.attrib[11], program.env[6];
	
	var_tc4.x = dot(attr_Tangent.xyz, u_lightProjectT.xyz);                                             //DP3		result.texcoord[4].x, vertex.attrib[9], program.env[7];
	var_tc4.y = dot(attr_Bitangent.xyz, u_lightProjectT.xyz);                                           //DP3		result.texcoord[4].y, vertex.attrib[10], program.env[7];
	var_tc4.z = dot(vec4(attr_Normal, 1).xyz, u_lightProjectT.xyz);                                     //DP3		result.texcoord[4].z, vertex.attrib[11], program.env[7];
	
	var_tc5.x = dot(attr_Tangent.xyz, u_lightFalloffS.xyz);                                             //DP3		result.texcoord[5].x, vertex.attrib[9], program.env[9];
	var_tc5.y = dot(attr_Bitangent.xyz, u_lightFalloffS.xyz);                                           //DP3		result.texcoord[5].y, vertex.attrib[10], program.env[9];
	var_tc5.z = dot(vec4(attr_Normal, 1).xyz, u_lightFalloffS.xyz);                                     //DP3		result.texcoord[5].z, vertex.attrib[11], program.env[9];
	
	// world space view vector
	R0 = (u_viewOriginLocal) + (-attr_Position);                                                        //ADD		R0, program.env[5], -vertex.position;
	
	// put into texture space for TEX6
	var_tc6.x = dot(attr_Tangent.xyz, R0.xyz);                                                          //DP3		result.texcoord[6].x, vertex.attrib[9], R0;
	var_tc6.y = dot(attr_Bitangent.xyz, R0.xyz);                                                        //DP3		result.texcoord[6].y, vertex.attrib[10], R0;
	var_tc6.z = dot(vec4(attr_Normal, 1).xyz, R0.xyz);                                                  //DP3		result.texcoord[6].z, vertex.attrib[11], R0;
	
	// move world space view vector to TEX7
	var_tc7.x = dot(attr_Tangent.xyz, u_worldUpLocal.xyz);                                              //DP3		result.texcoord[7].x, vertex.attrib[9], program.env[21];
	var_tc7.y = dot(attr_Bitangent.xyz, u_worldUpLocal.xyz);                                            //DP3		result.texcoord[7].y, vertex.attrib[10], program.env[21];
	var_tc7.z = dot(vec4(attr_Normal, 1).xyz, u_worldUpLocal.xyz);                                      //DP3		result.texcoord[7].z, vertex.attrib[11], program.env[21];
	var_tc7 = R0;                                                                                       //MOV		result.texcoord[7], R0;
	
	// generate the vertex color, which can be 1.0, color, or 1.0 - color
	// for 1.0 			: env[16] =  0.0, env[17] = 1.0
	// for color			: env[16] =  1.0, env[17] = 0.0
	// for 1.0 - color	: env[16] = -1.0, env[17] = 1.0
	var_color = (attr_Color) * (u_colorModulate) + (u_colorAdd);                                        //MAD		result.color, vertex.color, program.env[16], program.env[17];
	
	gl_Position = tdm_transform(attr_Position);
}
