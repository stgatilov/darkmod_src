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
uniform vec4 u_lightProjectQ;
uniform vec4 u_lightProjectS;
uniform vec4 u_lightProjectT;
uniform vec4 u_specularMatrixS;
uniform vec4 u_specularMatrixT;
uniform vec4 u_viewOriginLocal;
uniform vec4 u_worldUpLocal;

void main() {
	
	// OPTION ARB_position_invariant;
	
	// VPROG_INTERACTION
	//
	// input:
	//
	// attrib[8]		TEX0	texture coordinates
	// attrib[9]		TEX1	normal
	// attrib[10]	TEX2	tangent[0]
	// attrib[11]	TEX3	tangent[1]
	// attrib[3] (former COL)	vertex color
	//
	// c[4]	localLightOrigin
	// c[5]	localViewOrigin
	// c[6]	lightProjection S
	// c[7]	lightProjection T
	// c[8]	lightProjection Q
	// c[9]	lightFalloff S
	// c[10]	bumpMatrix S
	// c[11]	bumpMatrix T
	// c[12]	diffuseMatrix S
	// c[13]	diffuseMatrix T
	// c[14]	specularMatrix S
	// c[15]	specularMatrix T
	// c[16]	vertex color modulate
	// c[17]	vertex color add
	// c[21]	world up direction vector in object space
	//
	// output:
	//
	// texture 0 is the cube map
	// texture 1 is the per-surface bump map
	// texture 2 is the light falloff texture
	// texture 3 is the light projection texture
	// texture 4 is the per-surface diffuse map
	// texture 5 is the per-surface specular map
	// texture 6 is the specular lookup table
	
	vec4 R0, R1, R2;                                                                                    //TEMP	R0, R1, R2;
	
	vec4 defaultTexCoord = vec4(0, 0.5, 0, 1);                                                          //PARAM	defaultTexCoord = { 0, 0.5, 0, 1 };
	vec4 dirFromSky = vec4(0.0, 0.0, 1.0, 1.0);                                                         //PARAM	dirFromSky		= { 0.0, 0.0, 1.0 };
	
	// textures 1 takes the base coordinates by the texture matrix
	var_tc1 = defaultTexCoord;                                                                          //MOV	result.texcoord[1], defaultTexCoord;
	var_tc1.x = dot(attr_TexCoord, u_bumpMatrixS);                                                      //DP4 	result.texcoord[1].x, vertex.attrib[8], program.env[10];
	var_tc1.y = dot(attr_TexCoord, u_bumpMatrixT);                                                      //DP4 	result.texcoord[1].y, vertex.attrib[8], program.env[11];
	
	// texture 2 has one texgen
	var_tc2 = defaultTexCoord;                                                                          //MOV	result.texcoord[2], defaultTexCoord;
	var_tc2.x = dot(attr_Position, u_lightFalloffS);                                                    //DP4	result.texcoord[2].x, vertex.position, program.env[9];
	
	// texture 3 has three texgens
	var_tc3.x = dot(attr_Position, u_lightProjectS);                                                    //DP4	result.texcoord[3].x, vertex.position, program.env[6];
	var_tc3.y = dot(attr_Position, u_lightProjectT);                                                    //DP4	result.texcoord[3].y, vertex.position, program.env[7];
	var_tc3.w = dot(attr_Position, u_lightProjectQ);                                                    //DP4	result.texcoord[3].w, vertex.position, program.env[8];
	
	// textures 4 takes the base coordinates by the texture matrix
	var_tc4 = defaultTexCoord;                                                                          //MOV	result.texcoord[4], defaultTexCoord;
	var_tc4.x = dot(attr_TexCoord, u_diffuseMatrixS);                                                   //DP4	result.texcoord[4].x, vertex.attrib[8], program.env[12];
	var_tc4.y = dot(attr_TexCoord, u_diffuseMatrixT);                                                   //DP4	result.texcoord[4].y, vertex.attrib[8], program.env[13];
	
	// textures 5 takes the base coordinates by the texture matrix
	var_tc5 = defaultTexCoord;                                                                          //MOV	result.texcoord[5], defaultTexCoord;
	var_tc5.x = dot(attr_TexCoord, u_specularMatrixS);                                                  //DP4	result.texcoord[5].x, vertex.attrib[8], program.env[14];
	var_tc5.y = dot(attr_TexCoord, u_specularMatrixT);                                                  //DP4	result.texcoord[5].y, vertex.attrib[8], program.env[15];
	
	// calculate vector to viewer in R0
	R0 = (u_viewOriginLocal) - (attr_Position);                                                         //SUB	R0, program.env[5], vertex.position;
	
	// put into texture space for TEX6
	var_tc6.x = dot(attr_Tangent.xyz, R0.xyz);                                                          //DP3	result.texcoord[6].x, vertex.attrib[9], R0;
	var_tc6.y = dot(attr_Bitangent.xyz, R0.xyz);                                                        //DP3	result.texcoord[6].y, vertex.attrib[10], R0;
	var_tc6.z = dot(vec4(attr_Normal, 1).xyz, R0.xyz);                                                  //DP3	result.texcoord[6].z, vertex.attrib[11], R0;
	
	// put Sky Dir into texture space for TEX7
	var_tc7.x = dot(attr_Tangent.xyz, u_worldUpLocal.xyz);                                              //DP3	result.texcoord[7].x, vertex.attrib[9], program.env[21];
	var_tc7.y = dot(attr_Bitangent.xyz, u_worldUpLocal.xyz);                                            //DP3	result.texcoord[7].y, vertex.attrib[10], program.env[21];
	var_tc7.z = dot(vec4(attr_Normal, 1).xyz, u_worldUpLocal.xyz);                                      //DP3	result.texcoord[7].z, vertex.attrib[11], program.env[21];
	
	// generate the vertex color, which can be 1.0, color, or 1.0 - color
	// for 1.0 : env[16] = 0, env[17] = 1
	// for color : env[16] = 1, env[17] = 0
	// for 1.0-color : env[16] = -1, env[17] = 1
	var_color = (attr_Color) * (u_colorModulate) + (u_colorAdd);                                        //MAD	result.color, vertex.attrib[3], program.env[16], program.env[17];
	
	gl_Position = tdm_transform(attr_Position);
}
