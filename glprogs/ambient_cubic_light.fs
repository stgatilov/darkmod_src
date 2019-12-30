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
	
	vec4 subOne = vec4(-1.4, -1.4, -1.0, -1.0);                                                         //PARAM	subOne		= { -1.4, -1.4, -1.0, -1.0 };
	vec4 scaleTwo = vec4(2.8, 2.8, 2.0, 2.0);                                                           //PARAM	scaleTwo	= {  2.8,  2.8,  2.0,  2.0 };
	
	vec4 myconst = vec4(1.0, 2.0, 4.0, 5.0);                                                            //PARAM	const		= { 1.0,  2.0,  4.0,  5.0  };
	
	vec4 lightParms = vec4(.7, 1.8, 4.0, 20.0);                                                         //PARAM   lightParms	= { .7, 1.8, 4.0, 20.0 };
	vec4 colSky = vec4(.88, .88, .88, 1.0);                                                             //PARAM	colSky		= { .88, .88, .88, 1.0 };
	vec4 colGround = vec4(.35, .32, .32, 1.0);                                                          //PARAM	colGround	= { .35, .32, .32, 1.0 };
	
	vec4 M_inv8PI = vec4(0.039788735772973833942220940843129);                                          //PARAM   M_inv8PI = 0.039788735772973833942220940843129;
	
	vec4 lightVec, viewVec, normalVec, myhalfVec, reflectVec, wViewVec, wNormalVec;                     //TEMP	lightVec, viewVec, normalVec, halfVec, reflectVec, wViewVec, wNormalVec; 
	vec4 diffuse, specular, gloss, color, ambient, rough, fresnel;                                      //TEMP	diffuse, specular, gloss, color, ambient, rough, fresnel; 
	vec4 light, atten, pos, lightCube, lightProj;                                                       //TEMP	light, atten, pos, lightCube, lightProj;
	vec4 NdotL, NdotV, NdotH, VdotL;                                                                    //TEMP	NdotL, NdotV, NdotH, VdotL;
	vec4 R1, R2;                                                                                        //TEMP	R1, R2;
	vec4 C1, C2, C3;                                                                                    //TEMP 	C1, C2, C3;
	
	// calculate point light tc
	R1.xyz = (lightProjTC.xyz) * (vec3(2.0)) + (vec3(-1.0));                                            //MAD 	R1.xyz, lightProjTC, 2.0, -1.0;
	
	// calculate projected light tc
	R2 = lightProjTC;                                                                                   //MOV 	R2, lightProjTC;
	R2.z = -1.0;                                                                                        //MOV 	R2.z, -1.0;
	R2.w = 1.0 / lightProjTC.w;                                                                         //RCP 	R2.w, lightProjTC.w;
	R2.xy = (R2.xy) * (R2.ww);                                                                          //MUL 	R2.xy, R2, R2.w;
	R2.xy = (R2.xy) * (vec2(2.0)) + (vec2(-1.0));                                                       //MAD 	R2.xy, R2, 2.0, -1.0;
	
	// sample projection cube map
	lightCube = texture(u_texture3, R1.xyz);                                                            //TEX 	lightCube, R1, texture[3], CUBE;
	lightProj = texture(u_texture3, R2.xyz);                                                            //TEX 	lightProj, R2, texture[3], CUBE;
	
	// calculate attenuation (x = point, y = projected)
	R1.x = dot(R1.xyz, R1.xyz);                                                                         //DP3 	R1.x, R1, R1;
	R1.x = pow(R1.x, 0.5);                                                                              //POW 	R1.x, R1.x, 0.5.x;
	R1.y = lightProjTC.z;                                                                               //MOV		R1.y, lightProjTC.z;
	R1.xy = clamp((vec2(1.0)) + (-R1.xy), 0.0, 1.0);                                                    //ADD_SAT R1.xy, 1.0, -R1;
	R1.xy = (R1.xy) * (R1.xy);                                                                          //MUL 	R1.xy, R1, R1;
	lightCube = (lightCube) * (R1.xxxx);                                                                //MUL 	lightCube, lightCube, R1.x;
	lightProj = (lightProj) * (R1.yyyy);                                                                //MUL 	lightProj, lightProj, R1.y;
	
	// if w > 1.0, light = projected, else light = point
	R1.w = float((1.0) < (lightProjTC.w));                                                              //SLT 	R1.w, 1.0, lightProjTC.w;
	atten = mix(lightProj, lightCube, step(vec4(0.0), -R1.wwww));                                       //CMP 	atten, -R1.w, lightProj, lightCube;
	
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
	normalVec.xyz = (normalVec.wyz) * (scaleTwo.xyz) + (subOne.xyz);                                    //MAD		normalVec.xyz, normalVec.wyzx, scaleTwo, subOne;
	
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
	
	// calculate ambient diffuse term
	R1 = (wNormalVec) * (wNormalVec);                                                                   //MUL 	R1, wNormalVec, wNormalVec;
	C1 = mix(vec4(-1.0, 0.0, 0.0, 1.0), vec4(1.0, 0.0, 0.0, 1.0), step(vec4(0.0), wNormalVec.xxxx));    //CMP 	C1, wNormalVec.x, { -1.0,  0.0,  0.0 }, { 1.0, 0.0, 0.0 };
	C2 = mix(vec4(0.0, -1.0, 0.0, 1.0), vec4(0.0, 1.0, 0.0, 1.0), step(vec4(0.0), wNormalVec.yyyy));    //CMP 	C2, wNormalVec.y, {  0.0, -1.0,  0.0 }, { 0.0, 1.0, 0.0 };
	C3 = mix(vec4(0.0, 0.0, -1.0, 1.0), vec4(0.0, 0.0, 1.0, 1.0), step(vec4(0.0), wNormalVec.zzzz));    //CMP 	C3, wNormalVec.z, {  0.0,  0.0, -1.0 }, { 0.0, 0.0, 1.0 };
	C1 = texture(u_texture3, C1.xyz);                                                                   //TEX 	C1, C1, texture[3], CUBE;
	C2 = texture(u_texture3, C2.xyz);                                                                   //TEX 	C2, C2, texture[3], CUBE;
	C3 = texture(u_texture3, C3.xyz);                                                                   //TEX 	C3, C3, texture[3], CUBE;
	
	ambient.xyz = (C1.xyz) * (R1.xxx);                                                                  //MUL 	ambient.xyz, C1, R1.x;
	ambient.xyz = (C2.xyz) * (R1.yyy) + (ambient.xyz);                                                  //MAD 	ambient.xyz, C2, R1.y, ambient;
	ambient.xyz = (C3.xyz) * (R1.zzz) + (ambient.xyz);                                                  //MAD 	ambient.xyz, C3, R1.z, ambient;
	// TEX 	ambient.xyz, wNormalVec, texture[3], CUBE;
	
	diffuse.xyz = (diffuse.xyz) * (lightColor.xyz);                                                     //MUL 	diffuse.xyz, diffuse, lightColor;
	diffuse = (diffuse) * (vec4(0.15));                                                                 //MUL		diffuse, diffuse, 0.15;
	
	// calculate ambient specular term
	reflectVec = (NdotV.yyyy) * (wNormalVec);                                                           //MUL 	reflectVec, NdotV.y, wNormalVec;
	reflectVec.xyz = (reflectVec.xyz) * (myconst.yyy) + (-wViewVec.xyz);                                //MAD 	reflectVec.xyz, reflectVec, const.y, -wViewVec;
	reflectVec.w = (1.75) + (-NdotV.y);                                                                 //ADD 	reflectVec.w, 1.75, -NdotV.y;
	specular = texture(u_texture3, reflectVec.xyz);                                                     //TEX 	specular, reflectVec, texture[3], CUBE;
	
	specular.x = pow(specular.x, gloss.a);                                                              //POW 	specular.x, specular.x, gloss.a;
	specular.y = pow(specular.y, gloss.a);                                                              //POW 	specular.y, specular.y, gloss.a;
	specular.z = pow(specular.z, gloss.a);                                                              //POW 	specular.z, specular.z, gloss.a;
	specular = (specular) * (reflectVec.wwww);                                                          //MUL 	specular, specular, reflectVec.w;
	specular = (specular) * (vec4(0.75));                                                               //MUL		specular, specular, 0.75;
	
	
	// calculate directional specular term
	specular.w = clamp(pow(NdotH.x, 64.0), 0.0, 1.0);                                                   //POW_SAT specular.w, NdotH.x, 64.0.x;
	
	ambient.w = float((ambient.w) < (myconst.x));                                                       //SLT 	ambient.w, ambient.w, const.x;
	light = clamp(mix(NdotL.xxxx, ambient, step(vec4(0.0), -ambient.wwww)), 0.0, 1.0);                  //CMP_SAT light, -ambient.w, NdotL.x, ambient;
	specular = mix(specular.wwww, specular, step(vec4(0.0), -ambient.wwww));                            //CMP 	specular, -ambient.w, specular.w, specular;
	atten = clamp(mix(atten, atten.wwww, step(vec4(0.0), -ambient.wwww)), 0.0, 1.0);                    //CMP_SAT atten, -ambient.w, atten, atten.w;
	
	// combine diffuse and specular terms
	gloss = (gloss) + (gloss);                                                                          //ADD 	gloss, gloss, gloss;
	diffuse.x = pow(diffuse.x, 2.2);                                                                    //POW 	diffuse.x, diffuse.x, 2.2.x;
	diffuse.y = pow(diffuse.y, 2.2);                                                                    //POW 	diffuse.y, diffuse.y, 2.2.x;
	diffuse.z = pow(diffuse.z, 2.2);                                                                    //POW 	diffuse.z, diffuse.z, 2.2.x;
	diffuse = (diffuse) * (lightColor);                                                                 //MUL 	diffuse, diffuse, lightColor;
	specular = (specular) * (specColor);                                                                //MUL 	specular, specular, specColor;
	specular = (specular) * (vec4(0.7));                                                                //MUL		specular, specular, 0.7;
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
