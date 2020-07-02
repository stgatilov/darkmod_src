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

class GLSLProgram;
class ShaderParamsBuffer;
class DrawBatchExecutor;

class InteractionStage {
public:
	InteractionStage( ShaderParamsBuffer *shaderParamsBuffer, DrawBatchExecutor *drawBatchExecutor );

	void Init();
	void Shutdown();

	void DrawInteractions( viewLight_t *vLight, const drawSurf_t *interactionSurfs );

private:
	struct ShaderParams;

	ShaderParamsBuffer *shaderParamsBuffer;
	DrawBatchExecutor *drawBatches;
	GLSLProgram *stencilInteractionShader;
	GLSLProgram *ambientInteractionShader;
	GLSLProgram *bindlessStencilInteractionShader;
	GLSLProgram *bindlessAmbientInteractionShader;
	GLSLProgram *interactionShader;
	ShaderParams *shaderParams;
	int currentIndex;

	int maxSupportedDrawsPerBatch;

	void LoadInteractionShader(GLSLProgram *shader, const idStr &baseName, bool bindless);
	void BindShadowTexture();
	void ChooseInteractionProgram( viewLight_t *vLight );
	void ProcessSingleSurface( viewLight_t *vLight, const shaderStage_t *lightStage, const drawSurf_t *surf );
	void PrepareDrawCommand( drawInteraction_t * inter );
	void ResetShaderParams();
	void ExecuteDrawCalls();
};
