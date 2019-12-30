#version 140
// !!ARBfp1.0 

in vec4 var_tc0;
in vec4 var_tc1;
out vec4 draw_Color;
uniform sampler2D u_texture0;
uniform vec4 u_scalePotToWindow;
uniform vec4 u_scaleWindowToUnit;

void main() {
	// OPTION ARB_precision_hint_fastest;
	
	vec4 R0, SRC;                                                                                       //TEMP	R0, SRC;
	
	// calculate the screen texcoord in the 0.0 to 1.0 range
	R0 = (gl_FragCoord) * (u_scaleWindowToUnit);                                                        //MUL		R0, fragment.position, program.env[1];
	
	// scale by the screen non-power-of-two-adjust
	R0 = (R0) * (u_scalePotToWindow);                                                                   //MUL		R0, R0, program.env[0];
	
	// load the screen render
	SRC = texture(u_texture0, R0.xy);                                                                   //TEX		SRC, R0, texture[0], 2D;
	
	// calculate the grey scale version of the color
	R0.x = (SRC.x) + (SRC.y);                                                                           //ADD		R0.x, SRC.x, SRC.y;
	R0.x = (R0.x) + (SRC.z);                                                                            //ADD		R0.x, R0.x, SRC.z;
	R0 = (R0.xxxx) * (vec4(0.33));                                                                      //MUL		R0, R0.x, 0.33;
	
	// scale by the target color
	R0 = (R0) * (var_tc1);                                                                              //MUL		R0, R0, fragment.texcoord[1];
	
	// lerp between the source color and the target color
	draw_Color.xyz = (SRC.xyz) * (var_tc0.xyz) + (R0.xyz);                                              //MAD		result.color.xyz, SRC, fragment.texcoord[0], R0;
	
	
}
