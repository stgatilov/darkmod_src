#version 140
// !!ARBfp1.0

in vec4 var_tc0;
in vec4 var_tc1;
in vec4 var_tc2;
in vec4 var_tc3;
in vec4 var_tc4;
in vec4 var_tc5;
out vec4 draw_Color;
uniform sampler2D u_texture0;
uniform sampler2D u_texture1;
uniform sampler2D u_texture2;

void main() {
	// OPTION ARB_precision_hint_fastest;
	
	vec4 parms = vec4(1, 1, 1, 12);                                                                     //PARAM parms = { 1, 1, 1, 12 };
	vec4 pulseCol = vec4(1, 1, 1, 1);                                                                   //PARAM pulseCol = { 1, 1, 1, 1 };
	
	vec4 localNormal, viewVec, facing, diff, spec, globalNormal;                                        //TEMP localNormal, viewVec, facing, diff, spec, globalNormal;
	
	viewVec.w = dot(var_tc1.xyz, var_tc1.xyz);                                                          //DP3 viewVec.w, fragment.texcoord[1], fragment.texcoord[1];
	viewVec.w = 1.0 / sqrt(viewVec.w);                                                                  //RSQ viewVec.w, viewVec.w;
	viewVec.xyz = (var_tc1.xyz) * (viewVec.www);                                                        //MUL viewVec.xyz, fragment.texcoord[1], viewVec.w;
	
	localNormal = texture(u_texture0, var_tc0.xy);                                                      //TEX	localNormal, fragment.texcoord[0], texture[0], 2D;
	localNormal.x = localNormal.a;                                                                      //MOV localNormal.x, localNormal.a;
	localNormal = (localNormal) * (vec4(2.0)) + (vec4(-1.0));                                           //MAD	localNormal, localNormal, 2.0, -1.0;
	
	localNormal.w = dot(localNormal.xyz, localNormal.xyz);                                              //DP3 localNormal.w, localNormal, localNormal;
	localNormal.w = 1.0 / sqrt(localNormal.w);                                                          //RSQ localNormal.w, localNormal.w;
	localNormal.xyz = (localNormal.xyz) * (localNormal.www);                                            //MUL localNormal.xyz, localNormal, localNormal.w;
	
	globalNormal.x = dot(localNormal.xyz, var_tc2.xyz);                                                 //DP3	globalNormal.x, localNormal, fragment.texcoord[2];
	globalNormal.y = dot(localNormal.xyz, var_tc3.xyz);                                                 //DP3	globalNormal.y, localNormal, fragment.texcoord[3];
	globalNormal.z = dot(localNormal.xyz, var_tc4.xyz);                                                 //DP3	globalNormal.z, localNormal, fragment.texcoord[4];
	
	diff = texture(u_texture1, var_tc0.xy);                                                             //TEX diff, fragment.texcoord[0], texture[1], 2D;
	spec = texture(u_texture2, var_tc0.xy);                                                             //TEX spec, fragment.texcoord[0], texture[2], 2D;
	spec = (spec) + (spec);                                                                             //ADD spec, spec, spec;
	
	facing.x = clamp(dot(viewVec.xyz, globalNormal.xyz), 0.0, 1.0);                                     //DP3_SAT facing.x, viewVec, globalNormal;
	
	facing.y = pow(facing.x, parms.w);                                                                  //POW facing.y, facing.x, parms.w;
	facing.z = (1) - (facing.x);                                                                        //SUB facing.z, 1, facing.x;
	facing.z = (facing.z) * (facing.z);                                                                 //MUL facing.z, facing.z, facing.z;
	facing.w = (facing.z) * (facing.z);                                                                 //MUL facing.w, facing.z, facing.z;
	facing.x = (facing.w) * (2) + (facing.x);                                                           //MAD facing.x, facing.w, 2, facing.x;
	facing.x = min(facing.x, 1.25);                                                                     //MIN facing.x, facing.x, 1.25;
	
	diff = (spec) * (facing.yyyy) + (diff);                                                             //MAD diff, spec, facing.y, diff;
	
	//MUL diff, diff, fragment.texcoord[5].x;
	// add overlay effect
	diff = (pulseCol) * (var_tc5.xxxx) + (diff);                                                        //MAD diff, pulseCol, fragment.texcoord[5].x, diff;
	
	draw_Color.xyz = (diff.xyz) * (facing.xxx);                                                         //MUL result.color.xyz, diff, facing.x;
	draw_Color.w = 1;                                                                                   //MOV result.color.w, 1;
	
}
