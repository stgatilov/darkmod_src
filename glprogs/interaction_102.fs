#version 140
// !!ARBfp1.0

in vec4 var_color;
in vec4 var_tc0;
in vec4 var_tc1;
in vec4 var_tc2;
in vec4 var_tc3;
in vec4 var_tc4;
in vec4 var_tc5;
in vec4 var_tc6;
in vec4 var_tc7;
out vec4 draw_Color;
uniform sampler2D u_texture1;
uniform sampler2D u_texture2;
uniform sampler2D u_texture3;
uniform sampler2D u_texture4;
uniform sampler2D u_texture5;
uniform vec4 u_diffuseColor;
uniform vec4 u_specularColor;

void main() {
	// OPTION ARB_precision_hint_fastest;
	vec4 x0 = vec4(8, 1, 1, 1);                                                                         //PARAM x0 = { 8,1,1,1 };
	vec4 x1 = vec4(-0.57735, -0.57735, 0.57735, 1.0);                                                   //PARAM x1 = { -0.57735, -0.57735, 0.57735 };
	vec4 light, color, R0, R1, R2, R3, R4, localNormal, specular;                                       //TEMP light, color, R0, R1, R2, R3, R4, localNormal, specular;
	vec4 outCol;                                                                                        //OUTPUT outCol = result.color;
	localNormal = texture(u_texture1, var_tc1.xy);                                                      //TEX localNormal, fragment.texcoord[1], texture[1], 2D;
	localNormal.x = localNormal.a;                                                                      //MOV localNormal.x, localNormal.a;
	localNormal.xyz = (localNormal.xyz) * (vec3(2.0)) + (vec3(-1.0));                                   //MAD localNormal.xyz, localNormal, 2.0, -1.0;
	light.w = dot(var_tc0.xyz, var_tc0.xyz);                                                            //DP3 light.w, fragment.texcoord[0], fragment.texcoord[0];
	specular.w = dot(var_tc6.xyz, var_tc6.xyz);                                                         //DP3 specular.w, fragment.texcoord[6], fragment.texcoord[6];
	localNormal.w = dot(localNormal.xyz, localNormal.xyz);                                              //DP3 localNormal.w, localNormal, localNormal;
	R0.w = (light.w) * (localNormal.w);                                                                 //MUL R0.w, light.w, localNormal.w;
	R0.w = 1.0 / sqrt(R0.w);                                                                            //RSQ R0.w, R0.w;
	light.w = dot(var_tc0.xyz, localNormal.xyz);                                                        //DP3 light.w, fragment.texcoord[0], localNormal;
	light.w = (light.w) * (R0.w);                                                                       //MUL light.w, light.w, R0.w;
	R1 = texture(u_texture3, var_tc3.xy / var_tc3.w);                                                   //TXP R1, fragment.texcoord[3], texture[3], 2D;
	R4.w = (R1.w) - (0.01);                                                                             //SUB R4.w, R1.w, 0.01;
	R2 = texture(u_texture2, var_tc2.xy / var_tc2.w);                                                   //TXP R2, fragment.texcoord[2], texture[2], 2D;
	R0.xyz = (R1.xyz) * (R2.xyz);                                                                       //MUL R0.xyz, R1, R2;
	light.xyz = (light.www) * (R0.xyz);                                                                 //MUL light.xyz, light.w, R0;
	R1 = texture(u_texture4, var_tc4.xy);                                                               //TEX R1, fragment.texcoord[4], texture[4], 2D;
	color.xyz = (R1.xyz) * (u_diffuseColor.xyz);                                                        //MUL color.xyz, R1, program.env[0];
	R0.w = (specular.w) * (localNormal.w);                                                              //MUL R0.w, specular.w, localNormal.w;
	R0.w = 1.0 / sqrt(R0.w);                                                                            //RSQ R0.w, R0.w;
	specular.w = dot(var_tc6.xyz, localNormal.xyz);                                                     //DP3 specular.w, fragment.texcoord[6], localNormal;
	specular.w = (specular.w) * (R0.w);                                                                 //MUL specular.w, specular.w, R0.w;
	R1.w = clamp((specular.w) * (4.0) + (-3.0), 0.0, 1.0);                                              //MAD_SAT R1.w, specular.w, 4.0, -3.0;
	R1.w = (R1.w) * (R1.w);                                                                             //MUL R1.w, R1.w, R1.w;
	R1.xyz = (R1.www) * (u_specularColor.xyz);                                                          //MUL R1.xyz, R1.w, program.env[1];
	R4.x = dot(x1.xyz, localNormal.xyz);                                                                //DP3 R4.x, x1, localNormal;
	R4.x = (R4.x) * (0.6) + (0.25);                                                                     //MAD R4.x, R4.x, 0.6, 0.25;
	R4.y = clamp(dot(localNormal.xyz, var_tc7.xyz), 0.0, 1.0);                                          //DP3_SAT R4.y, localNormal, fragment.texcoord[7];
	R4.x = (R4.x) * (R4.y);                                                                             //MUL R4.x, R4.x, R4.y;
	R4.x = (R4.x) * (0.8) + (0.4);                                                                      //MAD R4.x, R4.x, 0.8, 0.4;
	R4.x = (R4.x) * (localNormal.z);                                                                    //MUL R4.x, R4.x, localNormal.z;
	R0.xyz = (R0.xyz) * (R4.xxx);                                                                       //MUL R0.xyz, R0, R4.x;
	light.xyz = mix(R0.xyz, light.xyz, step(vec3(0.0), R4.www));                                        //CMP light.xyz, R4.w, R0, light;
	R2 = texture(u_texture5, var_tc5.xy);                                                               //TEX R2, fragment.texcoord[5], texture[5], 2D;
	R2.xyz = (R2.xyz) + (R2.xyz);                                                                       //ADD R2.xyz, R2, R2;
	color.xyz = (R1.xyz) * (R2.xyz) + (color.xyz);                                                      //MAD color.xyz, R1, R2, color;
	color.xyz = clamp((light.xyz) * (color.xyz), 0.0, 1.0);                                             //MUL_SAT color.xyz, light, color;
	outCol.xyz = (color.xyz) * (var_color.xyz);                                                         //MUL outCol.xyz, color, fragment.color;
	draw_Color = outCol;
}
