#version 140
// !!ARBfp1.0

in vec4 var_color;
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
	
	// texture 0 is the cube map
	// texture 1 is the per-surface bump map
	// texture 2 is the light falloff texture
	// texture 3 is the light projection texture
	// texture 4 is the per-surface diffuse map
	// texture 5 is the per-surface specular map
	// texture 6 is the specular lookup table
	
	// env[0] is the diffuse modifier
	// env[1] is the specular modifier
	
	// parameters : ambient rim scale, diffuse rim scale, min spec exp, max spec exp
	vec4 lightParms = vec4(.7, 1.8, 4.0, 20.0);                                                         //PARAM   lightParms	= { .7, 1.8, 4.0, 20.0 };
	vec4 colSky = vec4(.88, .88, .88, 1.0);                                                             //PARAM	colSky		= { .88, .88, .88, 1.0 };
	vec4 colGround = vec4(.35, .32, .32, 1.0);                                                          //PARAM	colGround	= { .35, .32, .32, 1.0 };
	
	vec4 localNormal, toViewer, light, color, R1, R2, lightProj;                                        //TEMP	localNormal, toViewer, light, color, R1, R2, lightProj;
	
	// normalized view direction
	R1.w = dot(var_tc6.xyz, var_tc6.xyz);                                                               //DP3		R1.w, fragment.texcoord[6],fragment.texcoord[6];
	R1.w = 1.0 / sqrt(R1.w);                                                                            //RSQ		R1.w, R1.w;
	toViewer.xyz = (R1.www) * (var_tc6.xyz);                                                            //MUL		toViewer.xyz, R1.w, fragment.texcoord[6];
	
	// load normalmap and normalize
	localNormal = texture(u_texture1, var_tc1.xy);                                                      //TEX		localNormal, fragment.texcoord[1], texture[1], 2D;
	localNormal.x = localNormal.a;                                                                      //MOV		localNormal.x, localNormal.a;
	localNormal.xyz = (localNormal.xyz) * (vec3(2.0)) + (vec3(-1.0));                                   //MAD		localNormal.xyz, localNormal, 2.0, -1.0;
	
	R1.w = dot(localNormal.xyz, localNormal.xyz);                                                       //DP3		R1.w, localNormal, localNormal;
	R1.w = 1.0 / sqrt(R1.w);                                                                            //RSQ		R1.w, R1.w;
	localNormal.xyz = (R1.www) * (localNormal.xyz);                                                     //MUL		localNormal.xyz, R1.w, localNormal;
	
	// main diffuse shading
	light.w = dot(localNormal.xyz, var_tc7.xyz);                                                        //DP3		light.w, localNormal, fragment.texcoord[7];
	light.w = (light.w) * (0.5) + (0.5);                                                                //MAD		light.w, light.w, 0.5, 0.5;
	light.xyz = mix(colGround.xyz, colSky.xyz, light.www);                                              //LRP		light.xyz, light.w, colSky, colGround;
	
	// light projection and falloff
	R1.xyz = texture(u_texture3, var_tc3.xy / var_tc3.w).xyz;                                           //TXP 		R1.xyz, fragment.texcoord[3], texture[3], 2D;
	R2.xyz = texture(u_texture2, var_tc2.xy / var_tc2.w).xyz;                                           //TXP 		R2.xyz, fragment.texcoord[2], texture[2], 2D;
	lightProj.xyz = (R1.xyz) * (R2.xyz);                                                                //MUL 		lightProj.xyz, R1, R2;
	
	// load diffuse and specular maps
	color = texture(u_texture4, var_tc4.xy);                                                            //TEX		color, fragment.texcoord[4], texture[4], 2D;
	R2 = texture(u_texture5, var_tc5.xy);                                                               //TEX		R2, fragment.texcoord[5], texture[5], 2D;
	
	// calculate specularity
	R1.y = clamp(dot(toViewer.xyz, localNormal.xyz), 0.0, 1.0);                                         //DP3_SAT	R1.y, toViewer, localNormal;
	R2.w = mix(lightParms.z, lightParms.w, R2.g);                                                       //LRP		R2.w, R2.g, lightParms.w, lightParms.z;
	R1.x = pow(R1.y, R2.w);                                                                             //POW		R1.x, R1.y, R2.w;
	R1.x = (R1.x) * (0.65);                                                                             //MUL		R1.x, R1.x, 0.65;
	
	// "diffuse" fresnel effect
	vec4 fresnelTerm;                                                                                   //TEMP		fresnelTerm;
	fresnelTerm.x = (1.0) - (R1.y);                                                                     //SUB		fresnelTerm.x, 1.0, R1.y;
	fresnelTerm.x = pow(fresnelTerm.x, R2.w);                                                           //POW		fresnelTerm.x, fresnelTerm.x, R2.w;
	R1.w = clamp((toViewer.z) + (toViewer.z), 0.0, 1.0);                                                //ADD_SAT	R1.w, toViewer.z, toViewer.z;
	fresnelTerm.x = (fresnelTerm.x) * (R1.w);                                                           //MUL 		fresnelTerm.x, fresnelTerm.x, R1.w;
	light.xyz = (fresnelTerm.xxx) * (vec3(0.4)) + (light.xyz);                                          //MAD		light.xyz, fresnelTerm.x, 0.4, light;
	
	// modulate diffuse and specular maps
	color.xyz = (color.xyz) * (u_diffuseColor.xyz);                                                     //MUL 		color.xyz, color, program.env[0];
	R2.xyz = (R2.xyz) * (u_specularColor.xyz);                                                          //MUL 		R2.xyz, R2, program.env[1];
	
	// combine terms
	color.xyz = (R1.xxx) * (R2.xyz) + (color.xyz);                                                      //MAD		color.xyz, R1.x, R2, color;
	color.xyz = (light.xyz) * (color.xyz);                                                              //MUL		color.xyz, light, color;
	color.xyz = (color.xyz) * (lightProj.xyz);                                                          //MUL		color.xyz, color, lightProj;
	
	// modify by the vertex color
	draw_Color.xyz = (color.xyz) * (var_color.xyz);                                                     //MUL		result.color.xyz, color, fragment.color;
	
}
