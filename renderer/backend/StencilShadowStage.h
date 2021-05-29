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
#include "../tr_local.h"
#include "DrawBatchExecutor.h"

class StencilShadowStage {
public:
	StencilShadowStage( DrawBatchExecutor *drawBatchExecutor );

	void Init();
	void Shutdown();

	void DrawStencilShadows( viewLight_t *vLight, const drawSurf_t *shadowSurfs );

private:
	struct ShaderParams;

	DrawBatchExecutor *drawBatchExecutor = nullptr;
	GLSLProgram *stencilShadowShader = nullptr;

	void DrawSurfs(const drawSurf_t **surfs, size_t count);
};
