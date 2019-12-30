#version 140
// !!ARBfp1.0

out vec4 draw_Color;
uniform sampler2D u_texture0;
uniform vec4 u_scalePotToWindow;
uniform vec4 u_scaleWindowToUnit;

void main() {
	// OPTION ARB_precision_hint_fastest;
	
	vec4 pos0, pos1, pos2, pos3, pos4;                                                                  //TEMP pos0, pos1, pos2, pos3, pos4;
	vec4 coord0, coord1, coord2, coord3, coord4;                                                        //TEMP coord0, coord1, coord2, coord3, coord4;
	vec4 input0, input1, input2, input3, input4;                                                        //TEMP input0, input1, input2, input3, input4;
	vec4 avg;                                                                                           //TEMP avg;
	
	// greebo: Initialise the positions
	pos0 = gl_FragCoord;                                                                                //MOV pos0, fragment.position;
	pos1 = pos0;                                                                                        //MOV pos1, pos0;
	pos2 = pos0;                                                                                        //MOV pos2, pos0;
	pos3 = pos0;                                                                                        //MOV pos3, pos0;
	pos4 = pos0;                                                                                        //MOV pos4, pos0;
	
	// greebo: Offset the positions by a certain amount to the left/right/top/bottom
	pos1.y = (pos1.y) + (-3);                                                                           //ADD pos1.y, pos1.y, -3;
	pos2.x = (pos2.x) + (3);                                                                            //ADD pos2.x, pos2.x, 3;
	pos3.y = (pos3.y) + (4);                                                                            //ADD pos3.y, pos3.y, 4;
	pos4.x = (pos4.x) + (-5);                                                                           //ADD pos4.x, pos4.x, -5;
	
	// convert pixel's screen position to a fraction of the screen width & height
	// fraction will be between 0.0 and 1.0.
	// result is stored in temp1.
	coord0 = (pos0) * (u_scaleWindowToUnit);                                                            //MUL  coord0, pos0, program.env[1];
	coord1 = (pos1) * (u_scaleWindowToUnit);                                                            //MUL  coord1, pos1, program.env[1];
	coord2 = (pos2) * (u_scaleWindowToUnit);                                                            //MUL  coord2, pos2, program.env[1];
	coord3 = (pos3) * (u_scaleWindowToUnit);                                                            //MUL  coord3, pos3, program.env[1];
	coord4 = (pos4) * (u_scaleWindowToUnit);                                                            //MUL  coord4, pos4, program.env[1];
	
	coord0 = (coord0) * (u_scalePotToWindow);                                                           //MUL coord0, coord0, program.env[0];
	coord1 = (coord1) * (u_scalePotToWindow);                                                           //MUL coord1, coord1, program.env[0];
	coord2 = (coord2) * (u_scalePotToWindow);                                                           //MUL coord2, coord2, program.env[0];
	coord3 = (coord3) * (u_scalePotToWindow);                                                           //MUL coord3, coord3, program.env[0];
	coord4 = (coord4) * (u_scalePotToWindow);                                                           //MUL coord4, coord4, program.env[0];
	
	// pull the color value from _currentRender at texture coordinate input1.
	// store the r,g,b,a values in input1.
	input0 = texture(u_texture0, coord0.xy);                                                            //TEX  input0, coord0, texture[0], 2D;
	input1 = texture(u_texture0, coord1.xy);                                                            //TEX  input1, coord1, texture[0], 2D;
	input2 = texture(u_texture0, coord2.xy);                                                            //TEX  input2, coord2, texture[0], 2D;
	input3 = texture(u_texture0, coord3.xy);                                                            //TEX  input3, coord3, texture[0], 2D;
	input4 = texture(u_texture0, coord4.xy);                                                            //TEX  input4, coord4, texture[0], 2D;
	
	// greebo: Average the values and pump it into the fragment's color
	avg = (input0) + (input1);                                                                          //ADD avg, input0, input1;
	avg = (avg) + (input2);                                                                             //ADD avg, avg, input2;
	avg = (avg) + (input3);                                                                             //ADD avg, avg, input3;
	avg = (avg) + (input4);                                                                             //ADD avg, avg, input4;
	avg = (avg) * (vec4(0.2));                                                                          //MUL avg, avg, 0.2;
	
	draw_Color = avg;                                                                                   //MOV result.color, avg;
	
}
