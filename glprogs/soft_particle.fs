/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/
#version 140
// !!ARBfp1.0

in vec4 var_color;
in vec4 var_tc0;
out vec4 draw_Color;
uniform sampler2D u_texture0;
uniform sampler2D u_texture1;
uniform vec4 u_scaleDepthCoords;
uniform vec4 u_softParticleBlend;
uniform vec4 u_softParticleParams;

void main() {
	// == Fragment Program ==
	//
	// Input textures
	//   texture[0]   particle diffusemap
	//   texture[1]   _currentDepth
	// 	
	// Constants set by the engine:
	//   program.env[4] is reciprocal of _currentDepth size. Lets us convert a screen position to a texcoord in _currentDepth
	//   program.env[5] is the particle radius, given as { radius, 1/(fadeRange), 1/radius } 
	//		fadeRange is the particle diameter for alpha blends (like smoke), but the particle radius for additive 
	// 		blends (light glares), because additive effects work differently. Fog is half as apparent when a wall 
	// 		is in the middle of it. Light glares lose no visibility when they have something to reflect off.
	//   program.env[6] is the color channel mask. Particles with additive blend need their RGB channels modifying to blend them out. 
	//                                             Particles with an alpha blend need their alpha channel modifying.
	//
	// Hard-coded constants
	//    depth_consts allows us to recover the original depth in Doom units of anything in the depth 
	//    buffer. TDM's projection matrix differs slightly from the classic projection matrix as it 
	//    implements a "nearly-infinite" zFar. The matrix is hard-coded in the engine, so we use hard-coded 
	//    constants here for efficiency. depth_consts is derived from the numbers in that matrix.
	//
	
	vec4 depth_myconsts = vec4(0.33333333, -0.33316667, 0.0, 0.0);                                      //PARAM   depth_consts = { 0.33333333, -0.33316667, 0.0, 0.0 }; 
	vec4 particle_radius = u_softParticleParams;                                                        //PARAM	particle_radius  = program.env[5];
	vec4 tmp, scene_depth, particle_depth, near_fade, fade;                                             //TEMP    tmp, scene_depth, particle_depth, near_fade, fade;
	
	// Map the fragment to a texcoord on our depth image, and sample to find scene_depth
	tmp.xy = (gl_FragCoord.xy) * (u_scaleDepthCoords.xy);                                               //MUL   tmp.xy, fragment.position, program.env[4];
	scene_depth = texture(u_texture1, tmp.xy);                                                          //TEX   scene_depth, tmp, texture[1], 2D;
	scene_depth = min(scene_depth, vec4(0.9994));                                                       //MIN	  scene_depth, scene_depth, 0.9994;	
											
											
											
	
	// Recover original depth in doom units
	tmp = (scene_depth) * (depth_myconsts.xxxx) + (depth_myconsts.yyyy);                                //MAD   tmp, scene_depth, depth_consts.x, depth_consts.y;
	scene_depth = vec4(1.0 / tmp.x);                                                                    //RCP   scene_depth, tmp.x;
	
	// Convert particle depth to doom units too
	tmp = (gl_FragCoord.zzzz) * (depth_myconsts.xxxx) + (depth_myconsts.yyyy);                          //MAD   tmp, fragment.position.z, depth_consts.x, depth_consts.y;
	particle_depth = vec4(1.0 / tmp.x);                                                                 //RCP   particle_depth, tmp.x;
	
	// Scale the depth difference by the particle diameter to calc an alpha 
	// value based on how much of the 3d volume represented by the particle 
	// is in front of the solid scene
	tmp = (-scene_depth) + (particle_depth);                                                            //ADD		 tmp, -scene_depth, particle_depth;	 
	tmp = (tmp) + (particle_radius.xxxx);                                                               //ADD      tmp, tmp, particle_radius.x; 		 
	fade = clamp((tmp) * (particle_radius.yyyy), 0.0, 1.0);                                             //MUL_SAT  fade, tmp, particle_radius.y; 		 
	
	// Also fade if the particle is too close to our eye position, so it doesn't 'pop' in and out of view
	// Start a linear fade at particle_radius distance from the particle.
	near_fade = clamp((particle_depth) * (-particle_radius.zzzz), 0.0, 1.0);                            //MUL_SAT  near_fade, particle_depth, -particle_radius.z; 
	
	// Calculate final fade and apply the channel mask
	fade = (near_fade) * (fade);                                                                        //MUL      fade, near_fade, fade;
	fade = clamp((fade) + (u_softParticleBlend), 0.0, 1.0);                                             //ADD_SAT  fade, fade, program.env[6];  
	
	// Set the color. Multiply by vertex/fragment color as that's how the particle system fades particles in and out
	vec4 oColor;                                                                                        //TEMP  oColor;
	oColor = texture(u_texture0, var_tc0.xy);                                                           //TEX   oColor, fragment.texcoord, texture[0], 2D;
	oColor = (oColor) * (fade);                                                                         //MUL   oColor, oColor, fade;
	draw_Color = (oColor) * (var_color);                                                                //MUL   result.color, oColor, fragment.color; 
	
}
