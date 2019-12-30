#version 140
// !!ARBfp1.0 

out vec4 draw_Color;
uniform sampler2D u_texture0;
uniform vec4 u_scalePotToWindow;
uniform vec4 u_scaleWindowToUnit;

void main() {
	// OPTION ARB_precision_hint_fastest;
	
	// texture 0 is _currentRender
	//
	// env[0] is the 1.0 to _currentRender conversion
	// env[1] is the fragment.position to 0.0 - 1.0 conversion
	
	vec4 R0;                                                                                            //TEMP	R0;
	
	// calculate the screen texcoord in the 0.0 to 1.0 range
	R0 = (gl_FragCoord) * (u_scaleWindowToUnit);                                                        //MUL		R0, fragment.position, program.env[1];
	
	// scale by the screen non-power-of-two-adjust
	R0 = (R0) * (u_scalePotToWindow);                                                                   //MUL		R0, R0, program.env[0];
	
	// load the screen render
	draw_Color.xyz = texture(u_texture0, R0.xy).xyz;                                                    //TEX		result.color.xyz, R0, texture[0], 2D;
	
}
