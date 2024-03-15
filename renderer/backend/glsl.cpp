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
#include "renderer/backend/ImmediateRendering.h"


void Attributes::BindToProgram(GLSLProgram *program) {
	program->BindAttribLocation( Position, "attr_Position" );
	program->BindAttribLocation( Normal, "attr_Normal" );
	program->BindAttribLocation( Color, "attr_Color" );
	program->BindAttribLocation( TexCoord, "attr_TexCoord" );
	program->BindAttribLocation( Tangent, "attr_Tangent" );
	program->BindAttribLocation( Bitangent, "attr_Bitangent" );
	program->BindAttribLocation( DrawId, "attr_DrawId" );
}

void Attributes::EnableVertexRegular() {
	qglEnableVertexAttribArray( Position );
	qglEnableVertexAttribArray( Normal );
	qglEnableVertexAttribArray( Color );
	qglEnableVertexAttribArray( TexCoord );
	qglEnableVertexAttribArray( Tangent );
	qglEnableVertexAttribArray( Bitangent );
	auto *null = (idDrawVert*)(size_t)0;
	qglVertexAttribPointer( Position, 3, GL_FLOAT, false, sizeof( *null ), null->xyz.ToFloatPtr() );
	qglVertexAttribPointer( Normal, 3, GL_FLOAT, false, sizeof( *null ), null->normal.ToFloatPtr() );
	qglVertexAttribPointer( Color, 4, GL_UNSIGNED_BYTE, true, sizeof( *null ), &null->color[0] );
	qglVertexAttribPointer( TexCoord, 2, GL_FLOAT, false, sizeof( *null ), null->st.ToFloatPtr() );
	qglVertexAttribPointer( Tangent, 3, GL_FLOAT, false, sizeof( *null ), null->tangents[0].ToFloatPtr() );
	qglVertexAttribPointer( Bitangent, 3, GL_FLOAT, false, sizeof( *null ), null->tangents[1].ToFloatPtr() );
}

void Attributes::EnableVertexShadow() {
	qglEnableVertexAttribArray( Position );
	qglDisableVertexAttribArray( Normal );
	qglDisableVertexAttribArray( Color );
	qglDisableVertexAttribArray( TexCoord );
	qglDisableVertexAttribArray( Tangent );
	qglDisableVertexAttribArray( Bitangent );
	auto *null = (shadowCache_t*)(size_t)0;
	qglVertexAttribPointer( Position, 4, GL_FLOAT, false, sizeof( *null ), null->xyz.ToFloatPtr() );
}

void Attributes::EnableVertexImmediate() {
	qglEnableVertexAttribArray( Position );
	qglDisableVertexAttribArray( Normal );
	qglEnableVertexAttribArray( Color );
	qglEnableVertexAttribArray( TexCoord );
	qglDisableVertexAttribArray( Tangent );
	qglDisableVertexAttribArray( Bitangent );
	auto *null = (ImmediateRendering::VertexData*)(size_t)0;
	qglVertexAttribPointer( Position, 4, GL_FLOAT, GL_FALSE, sizeof( *null ), null->vertex.ToFloatPtr() );
	qglVertexAttribPointer( Color, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( *null ), &null->color[0] );
	qglVertexAttribPointer( TexCoord, 2, GL_FLOAT, GL_FALSE, sizeof( *null ), null->texCoord.ToFloatPtr() );
}

void Uniforms::Transform::Set( const viewEntity_t *space ) {
	modelMatrix.Set( space->modelMatrix );
	projectionMatrix.Set( backEnd.viewDef->projectionMatrix );
	modelViewMatrix.Set( space->modelViewMatrix );
}
