#version 140
// !!ARBvp1.0 OPTION ARB_position_invariant ;

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
uniform vec4 u_diffuseMatrixS;
uniform vec4 u_diffuseMatrixT;
uniform vec4 u_lightFalloffS;
uniform vec4 u_lightOriginLocal;
uniform vec4 u_lightProjectQ;
uniform vec4 u_lightProjectS;
uniform vec4 u_lightProjectT;
uniform vec4 u_specularMatrixS;
uniform vec4 u_specularMatrixT;
uniform vec4 u_viewOriginLocal;

void main() {
	
	// DarkMod Enhanced Interaction
	// Version 280809
	// Author : rebb ( rebb [at] trisoup [dot] net )
	
	// Different changes, including rebb's Ambient-Light Fix.
	// Improves the looks of Doom3 Ambient Lights, removing the "Coal Mine"-Look.
	// The Projection-Texture ( not lightFalloffImage ) of the ambient-light's material must have an alpha-channel
	// with a Value of 1 ( on the 0-255 scale ), 0 doesn't work because Doom3 automatically "optimizes" black alpha-channels away completely.
	// If this is not the case, ambient-lights will not work correctly when this VFP is enabled !
	
	// Many thanks for helping in testing go to :
	//   New Horizon and the DarkMod Team, Neuling, chumbucket91, The Happy Friar
	//   the TTLG boards and Doom3World
	
	vec4 defaultTexCoord = vec4(0, 0.5, 0, 1);                                                          //PARAM defaultTexCoord = { 0, 0.5, 0, 1 };
	vec4 R0, R1, R2;                                                                                    //TEMP R0, R1, R2;
	R0 = (u_lightOriginLocal) - (attr_Position);                                                        //SUB R0, program.env[4], vertex.position;
	var_tc0.x = dot(attr_Tangent.xyz, R0.xyz);                                                          //DP3 result.texcoord[0].x, vertex.attrib[9], R0;
	var_tc0.y = dot(attr_Bitangent.xyz, R0.xyz);                                                        //DP3 result.texcoord[0].y, vertex.attrib[10], R0;
	var_tc0.z = dot(vec4(attr_Normal, 1).xyz, R0.xyz);                                                  //DP3 result.texcoord[0].z, vertex.attrib[11], R0;
	var_tc1 = defaultTexCoord;                                                                          //MOV result.texcoord[1], defaultTexCoord;
	var_tc1.x = dot(attr_TexCoord, u_bumpMatrixS);                                                      //DP4 result.texcoord[1].x, vertex.attrib[8], program.env[10];
	var_tc1.y = dot(attr_TexCoord, u_bumpMatrixT);                                                      //DP4 result.texcoord[1].y, vertex.attrib[8], program.env[11];
	var_tc2 = defaultTexCoord;                                                                          //MOV result.texcoord[2], defaultTexCoord;
	var_tc2.x = dot(attr_Position, u_lightFalloffS);                                                    //DP4 result.texcoord[2].x, vertex.position, program.env[9];
	var_tc3.x = dot(attr_Position, u_lightProjectS);                                                    //DP4 result.texcoord[3].x, vertex.position, program.env[6];
	var_tc3.y = dot(attr_Position, u_lightProjectT);                                                    //DP4 result.texcoord[3].y, vertex.position, program.env[7];
	var_tc3.w = dot(attr_Position, u_lightProjectQ);                                                    //DP4 result.texcoord[3].w, vertex.position, program.env[8];
	var_tc4 = defaultTexCoord;                                                                          //MOV result.texcoord[4], defaultTexCoord;
	var_tc4.x = dot(attr_TexCoord, u_diffuseMatrixS);                                                   //DP4 result.texcoord[4].x, vertex.attrib[8], program.env[12];
	var_tc4.y = dot(attr_TexCoord, u_diffuseMatrixT);                                                   //DP4 result.texcoord[4].y, vertex.attrib[8], program.env[13];
	var_tc5 = defaultTexCoord;                                                                          //MOV result.texcoord[5], defaultTexCoord;
	var_tc5.x = dot(attr_TexCoord, u_specularMatrixS);                                                  //DP4 result.texcoord[5].x, vertex.attrib[8], program.env[14];
	var_tc5.y = dot(attr_TexCoord, u_specularMatrixT);                                                  //DP4 result.texcoord[5].y, vertex.attrib[8], program.env[15];
	R1.x = dot(R0.xyz, R0.xyz);                                                                         //DP3 R1.x, R0, R0;
	R1.x = 1.0 / sqrt(R1.x);                                                                            //RSQ R1.x, R1.x;
	R0 = (R0) * (R1.xxxx);                                                                              //MUL R0, R0, R1.x;
	R1 = (u_viewOriginLocal) - (attr_Position);                                                         //SUB R1, program.env[5], vertex.position;
	R2.x = dot(R1.xyz, R1.xyz);                                                                         //DP3 R2.x, R1, R1;
	R2.x = 1.0 / sqrt(R2.x);                                                                            //RSQ R2.x, R2.x;
	R1 = (R1) * (R2.xxxx);                                                                              //MUL R1, R1, R2.x;
	R0 = (R0) + (R1);                                                                                   //ADD R0, R0, R1;
	var_tc6.x = dot(attr_Tangent.xyz, R0.xyz);                                                          //DP3 result.texcoord[6].x, vertex.attrib[9], R0;
	var_tc6.y = dot(attr_Bitangent.xyz, R0.xyz);                                                        //DP3 result.texcoord[6].y, vertex.attrib[10], R0;
	var_tc6.z = dot(vec4(attr_Normal, 1).xyz, R0.xyz);                                                  //DP3 result.texcoord[6].z, vertex.attrib[11], R0;
	R0 = (R1) + (vec4(attr_Normal, 1));                                                                 //ADD R0, R1, vertex.attrib[11];
	R1 = (R0) * (vec4(0.5));                                                                            //MUL R1, R0, 0.5;
	var_tc7.x = dot(attr_Tangent.xyz, R1.xyz);                                                          //DP3 result.texcoord[7].x, vertex.attrib[9], R1;
	var_tc7.y = dot(attr_Bitangent.xyz, R1.xyz);                                                        //DP3 result.texcoord[7].y, vertex.attrib[10], R1;
	var_tc7.z = dot(vec4(attr_Normal, 1).xyz, R1.xyz);                                                  //DP3 result.texcoord[7].z, vertex.attrib[11], R1;
	var_color = (attr_Color) * (u_colorModulate) + (u_colorAdd);                                        //MAD result.color, vertex.color, program.env[16], program.env[17];
	gl_Position = tdm_transform(attr_Position);
}
