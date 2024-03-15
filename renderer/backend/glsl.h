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

#pragma once

#include "renderer/tr_local.h"

#include "renderer/backend/GLSLUniforms.h"


namespace Attributes {
	// attribute indexes and GLSL names are the same for all shaders
	enum Names {
		Position  = 0,
		Normal	  = 2,
		Color	  = 3,
		TexCoord  = 8,
		Tangent	  = 9,
		Bitangent = 10,
		DrawId    = 15,
	};
	// connect GLSL program to attribute indexes
	void BindToProgram(GLSLProgram *program);

	// enable exactly the specified set of vertex attribute arrays
	// and configure them to take data from corresponding vertex structure

	// idDrawVert / ATTRIB_REGULAR
	void EnableVertexRegular();
	// shadowCache_t / ATTRIB_SHADOW
	void EnableVertexShadow();
	// ImmediateRendering::VertexData
	void EnableVertexImmediate();
};

namespace Uniforms {
	// pack of uniforms defined in most shader programs
	struct Transform : public GLSLUniformGroup {
		UNIFORM_GROUP_DEF(Transform)

		DEFINE_UNIFORM( mat4, projectionMatrix )
		DEFINE_UNIFORM( mat4, modelMatrix )
		DEFINE_UNIFORM( mat4, modelViewMatrix )

		void Set( const viewEntity_t *space );
	};
};
