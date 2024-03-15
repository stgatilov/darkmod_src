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
#include "precompiled.h"
#pragma hdrstop

#include "renderer/tr_local.h"
#include "renderer/backend/glsl.h"


void Attributes::Regular::Bind(GLSLProgram *program) {
	using namespace Attributes::Regular;
	program->BindAttribLocation(Position, "attr_Position");
	program->BindAttribLocation(Normal, "attr_Normal");
	program->BindAttribLocation(Color, "attr_Color");
	program->BindAttribLocation(TexCoord, "attr_TexCoord");
	program->BindAttribLocation(Tangent, "attr_Tangent");
	program->BindAttribLocation(Bitangent, "attr_Bitangent");
	program->BindAttribLocation(DrawId, "attr_DrawId");
}

void Attributes::Shadow::Bind(GLSLProgram *program) {
	using namespace Attributes::Shadow;
	program->BindAttribLocation(Position, "attr_Position");
}

void Uniforms::Transform::Set( const viewEntity_t *space ) {
	modelMatrix.Set( space->modelMatrix );
	projectionMatrix.Set( backEnd.viewDef->projectionMatrix );
	modelViewMatrix.Set( space->modelViewMatrix );
}
