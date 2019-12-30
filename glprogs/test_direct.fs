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
	
	// Scale the normal map scale up by a factor of 1.5.
	vec4 subOne = vec4(-1.4, -1.4, -1.0, -1.0);                                                         //PARAM	subOne		= { -1.4, -1.4, -1.0, -1.0 };
	vec4 scaleTwo = vec4(2.8, 2.8, 2.0, 2.0);                                                           //PARAM	scaleTwo	= {  2.8,  2.8,  2.0,  2.0 };
	//PARAM	subOneN		= { -1.0, -1.0, -1.0, -1.0 };
	//PARAM	scaleTwoN	= {  2.0,  2.0,  2.0,  2.0 };
	vec4 myhalf = vec4(0.5, 0.5, 0.5, 0.5);                                                             //PARAM	half		= {  0.5,  0.5,  0.5,  0.5 };
	
	//---------------------------------------------------------
	// Regardless of the name, the fresnel parameters have diverse usage.
	// Note that, some multipliers are really insane values, 
	// this enable us to utilize High Dynamic Range
	//---------------------------------------------------------
	// Pramaeters are, respectively:		unsused, (1 - RI) specular correction, rimcontrib, unused
	//---------------------------------------------------------
	vec4 fresnelParms = vec4(1.0, .23, .5, 1.0);                                                        //PARAM	fresnelParms		= { 1.0, .23, .5, 1.0  }; 	
	//---------------------------------------------------------
	
	//---------------------------------------------------------
	// Pramaeters are, respectively:		Unsused, RI + spec correction, specular multiplier, rim power;
	//---------------------------------------------------------
	vec4 fresnelParms2 = vec4(.2, .023, 120.0, 4.0);                                                    //PARAM   fresnelParms2		= { .2, .023, 120.0, 4.0 }; 
	//---------------------------------------------------------
	// Pramaeters are, respectively:		ambient rim scale, diffuse rim scale, min. spec exp, max spec exp;
	//---------------------------------------------------------
	vec4 lightParms = vec4(.7, 1.8, 10.0, 30.0);                                                        //PARAM   lightParms			= { .7, 1.8, 10.0, 30.0 }; 
	
	vec4 cubemapLookupVec = vec4(0.33333, 0.33333, 0.33333, 1.0);                                       //PARAM	cubemapLookupVec		= { 0.33333, 0.33333, 0.33333 };
	
	vec4 colGround = vec4(.35, .32, .32, 1.0);                                                          //PARAM	colGround				= { .35, .32, .32, 1.0 };
	vec4 colSky = vec4(.88, .88, .88, 1.0);                                                             //PARAM	colSky					= { .88, .88, .88, 1.0 };
	//---------------------------------------------------------
	
	//---------------------------------------------------------
	vec4 lightProjFallOff, fresnelTerm, ambientContrib, rimLight;                                       //TEMP	lightProjFallOff, fresnelTerm, ambientContrib, rimLight;
	//---------------------------------------------------------
	
	vec4 toLight, light, toViewer, dirSky, myhalfAngle, color, R1, R2, R3, localNormal, ambientLightParams;//TEMP	toLight, light, toViewer, dirSky, halfAngle, color, R1, R2, R3, localNormal, ambientLightParams;
	
	//
	// the amount of light contacting the fragment is the
	// product of the two light projections and the surface
	// bump mapping
	//
	
	// normalize the direction to the light
	toLight = vec4(dot(var_tc0.xyz, var_tc0.xyz));                                                      //DP3		toLight, fragment.texcoord[0],fragment.texcoord[0];
	toLight = vec4(1.0 / sqrt(toLight.x));                                                              //RSQ		toLight, toLight.x;
	toLight = (toLight.xxxx) * (var_tc0);                                                               //MUL		toLight, toLight.x, fragment.texcoord[0];
	
	// normalize the direction to the viewer
	toViewer = vec4(dot(var_tc6.xyz, var_tc6.xyz));                                                     //DP3		toViewer, fragment.texcoord[6],fragment.texcoord[6];
	toViewer = vec4(1.0 / sqrt(toViewer.x));                                                            //RSQ		toViewer, toViewer.x;
	toViewer = (toViewer.xxxx) * (var_tc6);                                                             //MUL		toViewer, toViewer.x, fragment.texcoord[6];
	
	// load the filtered normal map
	localNormal = texture(u_texture1, var_tc1.xy);                                                      //TEX		localNormal, fragment.texcoord[1], texture[1], 2D;
	localNormal.x = localNormal.a;                                                                      //MOV		localNormal.x, localNormal.a;
	localNormal.xyz = (localNormal.xyz) * (scaleTwo.xyz) + (subOne.xyz);                                //MAD		localNormal.xyz, localNormal, scaleTwo, subOne;
	
	//---------------------------------------------------------
	// localNormal (normalize)
	//---------------------------------------------------------
	R1.w = dot(localNormal.xyz, localNormal.xyz);                                                       //DP3		R1.w, localNormal, localNormal;
	R1.w = 1.0 / sqrt(R1.w);                                                                            //RSQ		R1.w, R1.w;
	localNormal.xyz = (R1.www) * (localNormal.xyz);                                                     //MUL		localNormal.xyz, R1.w, localNormal;
	//---------------------------------------------------------
	// diffuse dot product
	light = clamp(vec4(dot(toLight.xyz, localNormal.xyz)), 0.0, 1.0);                                   //DP3_SAT	light, toLight, localNormal;
	
	// modulate by the light projection
	R1 = texture(u_texture3, var_tc3.xy / var_tc3.w);                                                   //TXP 	R1, fragment.texcoord[3], texture[3], 2D;
	
	// modulate by the light falloff
	R2 = texture(u_texture2, var_tc2.xy / var_tc2.w);                                                   //TXP 	R2, fragment.texcoord[2], texture[2], 2D;
	
	//---------------------------------------------------------
	// Store light Projection and Fall off to a variable for later use.
	//---------------------------------------------------------
	lightProjFallOff = (R1) * (R2);                                                                     //MUL 	lightProjFallOff, R1, R2;
	
	// modulate by the diffuse map
	color = texture(u_texture4, var_tc4.xy);                                                            //TEX		color, fragment.texcoord[4], texture[4], 2D;
	
	//---------------------------------------------------------
	// Calculate Fresnel reflectance approximation for Diffuse lighting
	//---------------------------------------------------------
	fresnelTerm.x = clamp(dot(toViewer.xyz, localNormal.xyz), 0.0, 1.0);                                //DP3_SAT fresnelTerm.x, toViewer, localNormal;
	fresnelTerm = (vec4(1)) - (fresnelTerm);                                                            //SUB		fresnelTerm, 1, fresnelTerm;
	fresnelTerm = vec4(pow(fresnelTerm.x, fresnelParms2.w));                                            //POW		fresnelTerm, fresnelTerm.x, fresnelParms2.w;
	R1 = clamp((light) - (vec4(.3)), 0.0, 1.0);                                                         //SUB_SAT	R1, light, .3;
	R1 = min(R1, fresnelParms.zzzz);                                                                    //MIN		R1, R1, fresnelParms.zzzz;
	
	rimLight = (R1) * (fresnelTerm);                                                                    //MUL		rimLight, R1, fresnelTerm;
	
	// read specular map
	R2 = texture(u_texture5, var_tc5.xy);                                                               //TEX		R2, fragment.texcoord[5], texture[5], 2D;
	
	R1 = (u_specularColor) - (vec4(0.001));                                                             //SUB		R1, program.env[1], 0.001;  
	R2 = mix(vec4(.026), R2, step(vec4(0.0), R1));                                                      //CMP		R2, R1, .026, R2;		
	//MAD		R3, R2, .5, .5;
	
	rimLight = (rimLight) * (lightParms.yyyy);                                                          //MUL		rimLight, rimLight, lightParms.y;
	
	
	// calculate the half angle vector and normalize
	myhalfAngle = (toLight) + (toViewer);                                                               //ADD		halfAngle, toLight, toViewer;
	R1 = vec4(dot(myhalfAngle.xyz, myhalfAngle.xyz));                                                   //DP3		R1, halfAngle, halfAngle;
	R1 = vec4(1.0 / sqrt(R1.x));                                                                        //RSQ		R1, R1.x;
	myhalfAngle.xyz = (myhalfAngle.xyz) * (R1.xyz);                                                     //MUL		halfAngle.xyz, halfAngle, R1;
	
	//---------------------------------------------------------
	// calculate specular
	//---------------------------------------------------------
	R1 = clamp(vec4(dot(myhalfAngle.xyz, localNormal.xyz)), 0.0, 1.0);                                  //DP3_SAT		R1, halfAngle, localNormal;
	//---------------------------------------------------------
	
	//---------------------------------
	// Convert spec. exponent to ralnge lightParms.w - lightParms.z 
	//---------------------------------
	R2.a = mix(lightParms.z, lightParms.w, R2.z);                                                       //LRP		R2.a, R2.z, lightParms.w, lightParms.z;
	//---------------------------------
	
	// Don't use specular lookup texture. Use power instead.
	R1 = vec4(pow(R1.x, R2.a));                                                                         //POW		R1, R1.x, R2.a;
	//POW		R1, R1.x, 12.0.r;
	//MOV		R1.y, 0.047058823529411764705882352941176;
	//TEX		R1, R1, texture[6], 2D;
	
	//---------------------------------------------------------
	R1 = (R1) * (fresnelParms2.zzzz);                                                                   //MUL		R1, R1, fresnelParms2.z;
	//---------------------------------------------------------
	
	//---------------------------------------------------------
	// Calculate & add fresnel reflectance approximation for specular
	//---------------------------------------------------------
	R3 = (fresnelTerm.xxxx) * (fresnelParms.yyyy) + (fresnelParms2.yyyy);                               //MAD		R3, fresnelTerm.x, fresnelParms.y, fresnelParms2.y;		
	R1 = (R1) * (R3);                                                                                   //MUL		R1, R1, R3;
	//---------------------------------------------------------
	
	// modulate by the specular map
	R3 = (color) * (vec4(.25));                                                                         //MUL		R3, color, .25;
	R3 = (R2) * (R3);                                                                                   //MUL		R3, R2, R3;
	R3 = (R2) * (vec4(0.75)) + (R3);                                                                    //MAD		R3, R2, 0.75, R3;
	R1 = (R1) * (R3);                                                                                   //MUL		R1, R1, R3;
	
	// light color
	//MUL 	color, color, program.env[0];
	
	//---------------------------------------------------------
	// Calculate Self Shadow term and modulate with result.
	//---------------------------------------------------------
	R2 = clamp((toLight.zzzz) * (vec4(4.0)), 0.0, 1.0);                                                 //MUL_SAT		R2, toLight.z, 4.0;
	
	light = (rimLight) * (R2) + (light);                                                                //MAD			light, rimLight, R2, light;
	
	color = (R1) * (R2) + (color);                                                                      //MAD			color, R1, R2, color;
	color = (light) * (color);                                                                          //MUL			color, light, color;
	
	//Dont use specular color
	//MUL 	R1, R1, program.env[1];
	color = (color) * (u_diffuseColor);                                                                 //MUL 	color, color, program.env[0];
	
	
	//---------------------------------------------------------
	
	color = (color) * (lightProjFallOff);                                                               //MUL			color, color, lightProjFallOff;
	//---------------------------------------------------------
	
	// modify by the vertex color
	draw_Color.xyz = (color.xyz) * (var_color.xyz);                                                     //MUL		result.color.xyz, color, fragment.color;
	
}
