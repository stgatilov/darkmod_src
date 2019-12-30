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
out vec4 draw_Color;
uniform sampler2D u_texture1;
uniform sampler2D u_texture2;
uniform sampler2D u_texture3;
uniform sampler2D u_texture4;
uniform sampler2D u_texture5;
uniform sampler2D u_texture6;
uniform samplerCube u_texture0;
uniform vec4 u_diffuseColor;
uniform vec4 u_specularColor;

void main() {
	// OPTION ARB_precision_hint_fastest;
	
	// texture 0 is the cube map
	// texture 1 is the per-surface bump map
	// texture 2 is the light falloff texture
	// texture 3 is the light projection texture
	// texture 4 is the per-surface diffuse map
	// texture 5 is the per-surface specular map
	// texture 6 is the specular lookup table
	
	// env[0] is the diffuse modifier
	// env[1] is the specular modifier
	
	vec4 light, color, R1, R2, localNormal, specular;                                                   //TEMP	light, color, R1, R2, localNormal, specular;
	
	vec4 subOne = vec4(-1, -1, -1, -1);                                                                 //PARAM	subOne = { -1, -1, -1, -1 };
	vec4 scaleTwo = vec4(2, 2, 2, 2);                                                                   //PARAM	scaleTwo = { 2, 2, 2, 2 };
	
	// load the specular half angle first, because
	// the ATI shader gives a "too many indirections" error
	// if this is done right before the texture indirection
	
	//-----------------
	//TEX	specular, fragment.texcoord[6], texture[0], CUBE;
	//MAD	specular, specular, scaleTwo, subOne;
	
	
	// instead of using the normalization cube map, normalize with math
	specular = vec4(dot(var_tc6.xyz, var_tc6.xyz));                                                     //DP3		specular, fragment.texcoord[6],fragment.texcoord[6];
	specular = vec4(1.0 / sqrt(specular.x));                                                            //RSQ		specular, specular.x;
	specular = (specular.xxxx) * (var_tc6);                                                             //MUL		specular, specular.x, fragment.texcoord[6];
	//-----------------
	
	
	//
	// the amount of light contacting the fragment is the
	// product of the two light projections and the surface
	// bump mapping
	//
	
	// perform the diffuse bump mapping
	
	//-----------------
	light = texture(u_texture0, var_tc0.xyz);                                                           //TEX	light, fragment.texcoord[0], texture[0], CUBE;
	light = (light) * (scaleTwo) + (subOne);                                                            //MAD	light, light, scaleTwo, subOne;
	
	// instead of using the normalization cube map, normalize with math
	//DP3		light, fragment.texcoord[0],fragment.texcoord[0];
	//RSQ		light, light.x;
	//MUL		light, light.x, fragment.texcoord[0];
	//-----------------
	
	localNormal = texture(u_texture1, var_tc1.xy);                                                      //TEX	localNormal, fragment.texcoord[1], texture[1], 2D;
	localNormal.x = localNormal.a;                                                                      //MOV localNormal.x, localNormal.a;
	localNormal = (localNormal) * (scaleTwo) + (subOne);                                                //MAD	localNormal, localNormal, scaleTwo, subOne;
	light = vec4(dot(light.xyz, localNormal.xyz));                                                      //DP3	light, light, localNormal;
	
	// modulate by the light projection
	R1 = texture(u_texture3, var_tc3.xy / var_tc3.w);                                                   //TXP	R1, fragment.texcoord[3], texture[3], 2D;
	light = (light) * (R1);                                                                             //MUL	light, light, R1;
	
	// modulate by the light falloff
	R1 = texture(u_texture2, var_tc2.xy / var_tc2.w);                                                   //TXP	R1, fragment.texcoord[2], texture[2], 2D;
	light = (light) * (R1);                                                                             //MUL	light, light, R1;
	
	//
	// the light will be modulated by the diffuse and
	// specular surface characteristics
	//
	
	// modulate by the diffuse map and constant diffuse factor
	R1 = texture(u_texture4, var_tc4.xy);                                                               //TEX	R1, fragment.texcoord[4], texture[4], 2D;
	color = (R1) * (u_diffuseColor);                                                                    //MUL	color, R1, program.env[0];
	
	// perform the specular bump mapping
	specular = vec4(dot(specular.xyz, localNormal.xyz));                                                //DP3	specular, specular, localNormal;
	
	// perform a dependent table read for the specular falloff
	R1 = texture(u_texture6, specular.xy);                                                              //TEX	R1, specular, texture[6], 2D;
	
	// modulate by the constant specular factor
	R1 = (R1) * (u_specularColor);                                                                      //MUL	R1, R1, program.env[1];
	
	// modulate by the specular map * 2
	R2 = texture(u_texture5, var_tc5.xy);                                                               //TEX	R2, fragment.texcoord[5], texture[5], 2D;
	R2 = (R2) + (R2);                                                                                   //ADD	R2, R2, R2;
	color = (R1) * (R2) + (color);                                                                      //MAD	color, R1, R2, color;
	
	
	color = (light) * (color);                                                                          //MUL	color, light, color;
	
	// modify by the vertex color
	draw_Color = (color) * (var_color);                                                                 //MUL result.color, color, fragment.color;
	
	// this should be better on future hardware, but current drivers make it slower
	//MUL result.color.xyz, color, fragment.color;
	
	
}
