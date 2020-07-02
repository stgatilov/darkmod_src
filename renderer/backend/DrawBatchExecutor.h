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
#include "UniversalGpuBuffer.h"
#include "../tr_local.h"

struct DrawElementsIndirectCommand {
    uint count;
    uint instanceCount;
    uint firstIndex;
    uint baseVertex;
    uint baseInstance;
};

class DrawBatchExecutor {
public:
	void Init();
	void Destroy();

	void BeginBatch(int maxDrawCalls);
	void AddDrawVertSurf(const drawSurf_t *surf);
	void DrawBatch();
	void Lock();

	static const int MAX_DRAW_COMMANDS = 4096;
private:
	UniversalGpuBuffer drawCommandBuffer;
	GLuint drawIdBuffer = 0;
	bool drawIdVertexEnabled = false;
	std::vector<DrawElementsIndirectCommand> fallbackBuffer;
	
	DrawElementsIndirectCommand *currentCommands = nullptr;
	uint maxDrawCommands = 0;
	uint currentIndex = 0;

	uint numVerts = 0;
	uint numIndexes = 0;

	bool ShouldUseMultiDraw() const;
	void InitDrawIdBuffer();
};
