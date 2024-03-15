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

#include "renderer/backend/qgl/qgl.h"
#include "renderer/tr_local.h"

#include "renderer/backend/GLSLUniforms.h"


// stgatilov: it would be great to DELETE THIS FILE
// after we drop old code behind r_useNewShaderPasses cvar


//pack of attributes used (almost) everywhere
namespace Attributes {
	namespace Default {
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
		//startOffset is byte offset of first idDrawVert in current VBO
		//arrayMask is a bitmask with attributes fetched from vertex array (arrays are disabled for unset attributes)
		//void SetDrawVert(size_t startOffset, int arrayMask);
	}
};

namespace Uniforms {
	//pack of uniforms defined in every shader program
	struct Global : public GLSLUniformGroup {
		UNIFORM_GROUP_DEF(Global)

		//DEFINE_UNIFORM( mat4, projectionMatrix )
		DEFINE_UNIFORM( mat4, modelMatrix )
		DEFINE_UNIFORM( mat4, modelViewMatrix )
		DEFINE_UNIFORM( vec4, viewOriginLocal )
		DEFINE_UNIFORM( mat4, textureMatrix )

		//TODO: is space necessary as argument, or we can take backEnd->currentSpace ?
		void Set( const viewEntity_t *space );
	};
};
