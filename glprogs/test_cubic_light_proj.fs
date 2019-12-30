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
uniform sampler2D u_texture4;
uniform sampler2D u_texture5;
uniform samplerCube u_texture3;
uniform vec4 u_diffuseColor;
uniform vec4 u_specularColor;

void main() {
	// OPTION ARB_precision_hint_fastest;
	
	// Instruction Count: ALU: 56 TEX: 6 Total: 88
	
	vec4 oColor;                                                                                        //OUTPUT 	oColor 		= result.color;
	vec4 vColor = var_color;                                                                            //ATTRIB 	vColor 		= fragment.color;
	
	vec4 lightVecTS = var_tc0;                                                                          //ATTRIB 	lightVecTS 	= fragment.texcoord[0];
	vec4 defaultTC = var_tc1;                                                                           //ATTRIB 	defaultTC	= fragment.texcoord[1];
	vec4 lightProjTC = var_tc2;                                                                         //ATTRIB 	lightProjTC = fragment.texcoord[2];
	vec4 tangent = var_tc3;                                                                             //ATTRIB 	tangent 	= fragment.texcoord[3];
	vec4 bitangent = var_tc4;                                                                           //ATTRIB 	bitangent 	= fragment.texcoord[4];
	vec4 normal = var_tc5;                                                                              //ATTRIB 	normal 		= fragment.texcoord[5];
	vec4 viewVecTS = var_tc6;                                                                           //ATTRIB 	viewVecTS 	= fragment.texcoord[6];
	vec4 viewVecWS = var_tc7;                                                                           //ATTRIB 	viewVecWS	= fragment.texcoord[7];
	
	vec4 lightColor = u_diffuseColor;                                                                   //PARAM 	lightColor 	= program.env[0];
	vec4 specColor = u_specularColor;                                                                   //PARAM 	specColor 	= program.env[1];
	
	vec4 myconst = vec4(1.0, 2.0, 4.0, 5.0);                                                            //PARAM	const		= { 1.0,  2.0,  4.0,  5.0  };
	
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
	vec4 fresnelTerm, rimLight;                                                                         //TEMP	fresnelTerm, rimLight;
	//---------------------------------------------------------
	
	vec4 lightVec, viewVec, normalVec, myhalfVec, reflectVec, wViewVec, wNormalVec;                     //TEMP	lightVec, viewVec, normalVec, halfVec, reflectVec, wViewVec, wNormalVec; 
	vec4 diffuse, specular, gloss, color, ambient;                                                      //TEMP	diffuse, specular, gloss, color, ambient; 
	vec4 light, atten, pos, lightCube, lightProj;                                                       //TEMP	light, atten, pos, lightCube, lightProj;
	vec4 NdotL, NdotV, NdotH;                                                                           //TEMP	NdotL, NdotV, NdotH;
	vec4 R1, R2, R3;                                                                                    //TEMP	R1, R2, R3;
	vec4 C1, C2, C3;                                                                                    //TEMP 	C1, C2, C3;
	
	
	// calculate projected light tc
	R1 = lightProjTC;                                                                                   //MOV 	R1, lightProjTC;
	R1.z = -1.0;                                                                                        //MOV 	R1.z, -1.0;
	R1.w = 1.0 / lightProjTC.w;                                                                         //RCP 	R1.w, lightProjTC.w;
	R1.xy = (R1.xy) * (R1.ww);                                                                          //MUL 	R1.xy, R1, R1.w;
	R1.xy = (R1.xy) * (vec2(2.0)) + (vec2(-1.0));                                                       //MAD 	R1.xy, R1, 2.0, -1.0;
	
	// sample projection cube map
	lightProj = texture(u_texture3, R1.xyz);                                                            //TEX 	lightProj, R1, texture[3], CUBE;
	
	// calculate attenuation
	R1.y = dot(R1.xyz, R1.xyz);                                                                         //DP3 	R1.y, R1, R1;
	R1.y = lightProjTC.z;                                                                               //MOV		R1.y, lightProjTC.z;
	atten = (lightProj) * (R1.yyyy);                                                                    //MUL 	atten, lightProj, R1.y;
	
	// early out
	// DP3 	atten.w, atten, atten;
	// SLT 	atten.w, atten.w, 0.00001;
	atten.w = R1.x;                                                                                     //MOV 	atten.w, R1.x;
	
	// load texture maps
	ambient = texture(u_texture3, lightProjTC.xyz);                                                     //TEX 	ambient, lightProjTC, texture[3], CUBE;
	normalVec = texture(u_texture1, defaultTC.xy);                                                      //TEX 	normalVec, defaultTC, texture[1], 2D;
	diffuse = texture(u_texture4, defaultTC.xy);                                                        //TEX		diffuse, defaultTC, texture[4], 2D;
	gloss = texture(u_texture5, defaultTC.xy);                                                          //TEX		gloss, defaultTC, texture[5], 2D;
	specular = (specColor) - (vec4(0.001));                                                             //SUB		specular, specColor, 0.001;  
	gloss = mix(vec4(.026), gloss, step(vec4(0.0), specular));                                          //CMP		gloss, specular, .026, gloss;		
	
	// normalize world space light vector
	lightVec.w = dot(lightVecTS.xyz, lightVecTS.xyz);                                                   //DP3 	lightVec.w, lightVecTS, lightVecTS;
	lightVec.w = 1.0 / sqrt(lightVec.w);                                                                //RSQ 	lightVec.w, lightVec.w;
	lightVec.xyz = (lightVecTS.xyz) * (lightVec.www);                                                   //MUL 	lightVec.xyz, lightVecTS, lightVec.w;
	
	// normalize tangent space view vector
	viewVec.w = dot(viewVecTS.xyz, viewVecTS.xyz);                                                      //DP3 	viewVec.w, viewVecTS, viewVecTS;
	viewVec.w = 1.0 / sqrt(viewVec.w);                                                                  //RSQ 	viewVec.w, viewVec.w;
	viewVec.xyz = (viewVecTS.xyz) * (viewVec.www);                                                      //MUL 	viewVec.xyz, viewVecTS, viewVec.w;
	
	// normalize world space view vector
	wViewVec.w = dot(viewVecWS.xyz, viewVecWS.xyz);                                                     //DP3 	wViewVec.w, viewVecWS, viewVecWS;
	wViewVec.w = 1.0 / sqrt(wViewVec.w);                                                                //RSQ 	wViewVec.w, wViewVec.w;
	wViewVec.xyz = (viewVecWS.xyz) * (wViewVec.www);                                                    //MUL 	wViewVec.xyz, viewVecWS, wViewVec.w;
	
	// calculate the half angle vector and normalize
	myhalfVec = (lightVec) + (viewVec);                                                                 //ADD 	halfVec, lightVec, viewVec;
	myhalfVec.w = dot(myhalfVec.xyz, myhalfVec.xyz);                                                    //DP3 	halfVec.w, halfVec, halfVec;
	myhalfVec.w = 1.0 / sqrt(myhalfVec.w);                                                              //RSQ 	halfVec.w, halfVec.w;
	myhalfVec.xyz = (myhalfVec.xyz) * (myhalfVec.www);                                                  //MUL 	halfVec.xyz, halfVec, halfVec.w;
	
	// scale tangent space normal vector to -1.0<->1.0 range
	// MAD		normalVec.xyz, normalVec.wyzx, const.y, -const.x;
	normalVec.x = normalVec.a;                                                                          //MOV		normalVec.x, normalVec.a;
	normalVec.xyz = (normalVec.xyz) * (scaleTwo.xyz) + (subOne.xyz);                                    //MAD		normalVec.xyz, normalVec, scaleTwo, subOne;
	
	// transform tangent space normal vector into world space
	wNormalVec.x = dot(normalVec.xyz, tangent.xyz);                                                     //DP3		wNormalVec.x, normalVec, tangent;
	wNormalVec.y = dot(normalVec.xyz, bitangent.xyz);                                                   //DP3		wNormalVec.y, normalVec, bitangent;
	wNormalVec.z = dot(normalVec.xyz, normal.xyz);                                                      //DP3		wNormalVec.z, normalVec, normal;
	
	// normalize tangent space normal vector
	normalVec.w = dot(normalVec.xyz, normalVec.xyz);                                                    //DP3 	normalVec.w, normalVec, normalVec;
	normalVec.w = 1.0 / sqrt(normalVec.w);                                                              //RSQ 	normalVec.w, normalVec.w;
	normalVec.xyz = (normalVec.xyz) * (normalVec.www);                                                  //MUL 	normalVec.xyz, normalVec, normalVec.w;
	
	// normalize world space normal vector
	wNormalVec.w = dot(wNormalVec.xyz, wNormalVec.xyz);                                                 //DP3 	wNormalVec.w, wNormalVec, wNormalVec;
	wNormalVec.w = 1.0 / sqrt(wNormalVec.w);                                                            //RSQ 	wNormalVec.w, wNormalVec.w;
	wNormalVec.xyz = (wNormalVec.xyz) * (wNormalVec.www);                                               //MUL 	wNormalVec.xyz, wNormalVec, wNormalVec.w;
	
	// calculate vector dot products
	NdotL.x = clamp(dot(normalVec.xyz, lightVec.xyz), 0.0, 1.0);                                        //DP3_SAT NdotL.x, normalVec, lightVec;
	NdotV.x = clamp(dot(normalVec.xyz, viewVec.xyz), 0.0, 1.0);                                         //DP3_SAT NdotV.x, normalVec, viewVec;
	NdotV.y = dot(wNormalVec.xyz, wViewVec.xyz);                                                        //DP3 	NdotV.y, wNormalVec, wViewVec;
	NdotH.x = clamp(dot(normalVec.xyz, myhalfVec.xyz), 0.0, 1.0);                                       //DP3_SAT NdotH.x, normalVec, halfVec;
	
	// diffuse dot product
	light = clamp(vec4(dot(lightVec.xyz, normalVec.xyz)), 0.0, 1.0);                                    //DP3_SAT	light, lightVec, normalVec;
	
	//---------------------------------------------------------
	// Calculate Fresnel reflectance approximation for Diffuse lighting
	//---------------------------------------------------------
	fresnelTerm.x = clamp(dot(viewVec.xyz, normalVec.xyz), 0.0, 1.0);                                   //DP3_SAT fresnelTerm.x, viewVec, normalVec;
	fresnelTerm = (vec4(1)) - (fresnelTerm);                                                            //SUB		fresnelTerm, 1, fresnelTerm;
	fresnelTerm = vec4(pow(fresnelTerm.x, fresnelParms2.w));                                            //POW		fresnelTerm, fresnelTerm.x, fresnelParms2.w;
	R1 = clamp((light) - (vec4(.3)), 0.0, 1.0);                                                         //SUB_SAT	R1, light, .3;
	R1 = min(R1, fresnelParms.zzzz);                                                                    //MIN		R1, R1, fresnelParms.zzzz;
	rimLight = (R1) * (fresnelTerm);                                                                    //MUL		rimLight, R1, fresnelTerm;
	
	// calculate directional specular term
	specular.w = clamp(pow(NdotH.x, 64.0), 0.0, 1.0);                                                   //POW_SAT specular.w, NdotH.x, 64.0.x;
	
	//---------------------------------
	// Convert spec. exponent to ralnge lightParms.w - lightParms.z 
	//---------------------------------
	gloss.a = mix(lightParms.z, lightParms.w, gloss.z);                                                 //LRP		gloss.a, gloss.z, lightParms.w, lightParms.z;
	//---------------------------------
	
	// Don't use specular lookup texture. Use power instead.
	specular.x = pow(specular.x, gloss.a);                                                              //POW		specular.x, specular.x, gloss.a;
	specular.y = pow(specular.y, gloss.a);                                                              //POW		specular.y, specular.y, gloss.a;
	specular.z = pow(specular.z, gloss.a);                                                              //POW		specular.z, specular.z, gloss.a;
	//---------------------------------------------------------
	specular = (specular) * (fresnelParms2.zzzz);                                                       //MUL		specular, specular, fresnelParms2.z;
	//---------------------------------------------------------
	
	//---------------------------------------------------------
	// Calculate & add fresnel reflectance approximation for specular
	//---------------------------------------------------------
	R3 = (fresnelTerm.xxxx) * (fresnelParms.yyyy) + (fresnelParms2.yyyy);                               //MAD		R3, fresnelTerm.x, fresnelParms.y, fresnelParms2.y;		
	specular = (specular) * (R3);                                                                       //MUL		specular, specular, R3;
	//---------------------------------------------------------
	
	ambient.w = float((ambient.w) < (myconst.x));                                                       //SLT 	ambient.w, ambient.w, const.x;
	light = clamp(mix(NdotL.xxxx, ambient, step(vec4(0.0), -ambient.wwww)), 0.0, 1.0);                  //CMP_SAT light, -ambient.w, NdotL.x, ambient;
	specular = mix(specular.wwww, specular, step(vec4(0.0), -ambient.wwww));                            //CMP 	specular, -ambient.w, specular.w, specular;
	atten = clamp(mix(atten, atten.wwww, step(vec4(0.0), -ambient.wwww)), 0.0, 1.0);                    //CMP_SAT atten, -ambient.w, atten, atten.w;
	
	
	// combine diffuse and specular terms
	// ADD 	gloss, gloss, gloss;
	// modulate by the specular map
	R3 = (color) * (vec4(.25));                                                                         //MUL		R3, color, .25;
	R3 = (gloss) * (R3);                                                                                //MUL		R3, gloss, R3;
	R3 = (gloss) * (vec4(0.75)) + (R3);                                                                 //MAD		R3, gloss, 0.75, R3;
	specular = (specular) * (R3);                                                                       //MUL		specular, specular, R3;
	
	//---------------------------------------------------------
	// Calculate Self Shadow term and modulate with result.
	//---------------------------------------------------------
	gloss = clamp((lightVec.zzzz) * (vec4(4.0)), 0.0, 1.0);                                             //MUL_SAT		gloss, lightVec.z, 4.0;
	diffuse = (rimLight) * (gloss) + (diffuse);                                                         //MAD			diffuse, rimLight, gloss, diffuse;
	
	diffuse = (specular) * (gloss) + (diffuse);                                                         //MAD		diffuse, specular, gloss, diffuse;
	diffuse = (light) * (diffuse);                                                                      //MUL		diffuse, light, diffuse;
	diffuse.x = pow(diffuse.x, 2.2);                                                                    //POW 	diffuse.x, diffuse.x, 2.2.x;
	diffuse.y = pow(diffuse.y, 2.2);                                                                    //POW 	diffuse.y, diffuse.y, 2.2.x;
	diffuse.z = pow(diffuse.z, 2.2);                                                                    //POW 	diffuse.z, diffuse.z, 2.2.x;
	diffuse = (diffuse) * (lightColor);                                                                 //MUL 	diffuse, diffuse, lightColor;
	specular = (specular) * (specColor);                                                                //MUL 	specular, specular, specColor;
	color = (specular) * (gloss) + (diffuse);                                                           //MAD 	color, specular, gloss, diffuse;
	
	// modulate by light & attenuation
	color = (color) * (light);                                                                          //MUL		color, color, light;
	color = (color) * (atten);                                                                          //MUL 	color, color, atten;
	color.x = pow(color.x, 0.45454545454545454545454545454545);                                         //POW 	color.x, color.x, 0.45454545454545454545454545454545.x;
	color.y = pow(color.y, 0.45454545454545454545454545454545);                                         //POW 	color.y, color.y, 0.45454545454545454545454545454545.x;
	color.z = pow(color.z, 0.45454545454545454545454545454545);                                         //POW 	color.z, color.z, 0.45454545454545454545454545454545.x;
	
	// modify by the vertex color
	oColor.xyz = (color.xyz) * (vColor.xyz);                                                            //MUL 	oColor.xyz, color, vColor;
	
	draw_Color = oColor;
}
