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

//returns eye Z coordinate with reversed sign (monotonically increasing with depth)
//in other words, it is eye-fragment distance along view direction
float depthToZ(mat4 projectionMatrix, float depth) {
	float clipZ = 2.0 * depth - 1.0;
	float A = projectionMatrix[2].z;
	float B = projectionMatrix[3].z;
	return B / (A + clipZ);
}
