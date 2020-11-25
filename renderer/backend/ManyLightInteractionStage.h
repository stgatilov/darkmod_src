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

class GLSLProgram;

class ManyLightInteractionStage {
public:
	ManyLightInteractionStage( DrawBatchExecutor *drawBatchExecutor );

	void Init();
	void Shutdown();

	void DrawInteractions( const viewDef_t *viewDef );

private:
	static const int MAX_LIGHTS = 21; // limited by available texture units
	
	struct ShaderParams;
	struct LightParams;
	struct DrawInteraction;

	DrawBatchExecutor *drawBatchExecutor;
	GLSLProgram *shadowMapInteractionShader;
	GLSLProgram *bindlessShadowMapInteractionShader;
	GLSLProgram *interactionShader;
	uint maxShaderParamsArraySize;

	int curLight = 0;
	LightParams *lightParams = nullptr;
	int falloffTextureUnits[MAX_LIGHTS] = {0};
	int falloffCubeTextureUnits[MAX_LIGHTS] = {0};
	int projectionTextureUnits[MAX_LIGHTS] = {0};
	int projectionCubeTextureUnits[MAX_LIGHTS] = {0};

	DrawBatch<ShaderParams> drawBatch;
	int currentIndex;

	GLuint poissonSamplesUbo = 0;
	idList<idVec2> poissonSamples;

	int currentDepthFunc;
	void SetGlState(int depthFunc);

	void DrawAllSurfaces( idList<const drawSurf_t *> &drawSurfs );
	void LoadInteractionShader(GLSLProgram *shader, bool bindless);
	void BindShadowTexture();
	void PrepareInteractionProgram();
	void ProcessSingleSurface( const drawSurf_t *surf );
	void PrepareDrawCommand( DrawInteraction * inter );
	void BeginDrawBatch();
	void ExecuteDrawCalls();

	void PreparePoissonSamples();
	void ResetLightParams(const viewDef_t *viewDef);
};
