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

class ShaderParamsBuffer;
class DrawBatchExecutor;

class DepthStage
{
public:
	DepthStage( ShaderParamsBuffer *shaderParamsBuffer, DrawBatchExecutor *drawBatchExecutor );

	void Init();
	void Shutdown();

	void DrawDepth( const viewDef_t *viewDef, drawSurf_t **drawSurfs, int numDrawSurfs );

private:
	struct ShaderParams;

	ShaderParamsBuffer *shaderParamsBuffer;
	DrawBatchExecutor *drawBatchExecutor;
	GLSLProgram *depthShader = nullptr;
	GLSLProgram *depthShaderBindless = nullptr;

	int maxSupportedDrawsPerBatch = 0;

	int currentIndex = 0;
	ShaderParams *shaderParams;

	bool ShouldDrawSurf( const drawSurf_t *surf ) const;
	void DrawSurf( const drawSurf_t * drawSurf );
	void CreateDrawCommands( const drawSurf_t *surf );
	void IssueDrawCommand( const drawSurf_t *surf, const shaderStage_t *stage );

	void ResetShaderParams();
	void ExecuteDrawCalls();
};
