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
#version 430

layout(location = 0) in vec4 attr_Vertex;

layout (location = 0) uniform mat4[2] u_MVP;

out vec4 csThis;
out vec4 lightProject;
out vec4 worldPosition;

void main() {
	gl_Position = u_MVP[1] * u_MVP[0] * attr_Vertex;
	worldPosition = attr_Vertex;
	// fragment position in clip space
	csThis = u_MVP[1] * u_MVP[0] * attr_Vertex;
}