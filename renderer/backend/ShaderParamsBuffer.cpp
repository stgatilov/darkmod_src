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

#include "ShaderParamsBuffer.h"

const uint32_t SHADER_BUFFER_SIZE = 8192 * 1024 * 3;

void ShaderParamsBuffer::Init() {
	GLint uboAlignment;
	qglGetIntegerv( GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uboAlignment );
	uniformBuffer.Init( GL_UNIFORM_BUFFER, SHADER_BUFFER_SIZE, uboAlignment );

	qglGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUniformBlockSize);
}

void ShaderParamsBuffer::Destroy() {
	uniformBuffer.Destroy();
}
