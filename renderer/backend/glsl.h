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
	// attributes used (almost) everywhere
	namespace Regular {
		enum Names {
			Position  = 0,
			Normal	  = 2,
			Color	  = 3,
			TexCoord  = 8,
			Tangent	  = 9,
			Bitangent = 10,
			DrawId    = 15,
		};
		void Bind(GLSLProgram *program);
	}
	// attributes for stencil shadows
	namespace Shadow {
		enum Names {
			Position  = 0,
		};
		void Bind(GLSLProgram *program);
	}
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
