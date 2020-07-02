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
#include "DrawBatchExecutor.h"

#include "../glsl.h"
#include "../qgl.h"

const int DrawBatchExecutor::MAX_DRAW_COMMANDS;

idCVarBool r_useMultiDrawIndirect("r_useMultiDrawIndirect", "1", CVAR_RENDERER|CVAR_BOOL, "Batch draw calls in multidraw commands if available");

void DrawBatchExecutor::Init() {
	if (GLAD_GL_ARB_multi_draw_indirect) {
		InitDrawIdBuffer();	
		drawCommandBuffer.Init( GL_DRAW_INDIRECT_BUFFER, MAX_DRAW_COMMANDS * sizeof(DrawElementsIndirectCommand) * 3, 16 );
		drawCommandBuffer.Bind();
	}
}

void DrawBatchExecutor::Destroy() {
	qglDeleteBuffers(1, &drawIdBuffer);
	drawIdBuffer = 0;
	drawCommandBuffer.Destroy();
}

void DrawBatchExecutor::BeginBatch( int maxDrawCalls ) {
	maxDrawCommands = maxDrawCalls;
	currentIndex = 0;
	if (ShouldUseMultiDraw()) {
		currentCommands = reinterpret_cast< DrawElementsIndirectCommand* >( drawCommandBuffer.Reserve(
			maxDrawCalls * sizeof( DrawElementsIndirectCommand ) ) );
	} else {
		fallbackBuffer.resize( maxDrawCalls );
		currentCommands = fallbackBuffer.data();
	}

	numVerts = 0;
	numIndexes = 0;
}

void DrawBatchExecutor::AddDrawVertSurf( const drawSurf_t *surf ) {
	if (currentIndex >= maxDrawCommands) {
		common->Warning( "Add surf to batch: exceeded allocated draw commands" );
		return;
	}

	int idx = currentIndex++;
	DrawElementsIndirectCommand &cmd = currentCommands[idx];
	cmd.count = surf->numIndexes;
	cmd.instanceCount = 1;
	cmd.firstIndex = surf->indexCache.offset / sizeof(glIndex_t);
	cmd.baseVertex = surf->ambientCache.offset / sizeof(idDrawVert);
	cmd.baseInstance = idx;

	if (surf->frontendGeo)
		numVerts += surf->frontendGeo->numVerts;
	numIndexes += surf->numIndexes;
}

void DrawBatchExecutor::DrawBatch() {
	int numDrawCalls = currentIndex;

	if (numDrawCalls == 0) {
		currentIndex = 0;
		currentCommands = nullptr;
		maxDrawCommands = 0;
		return;		
	}

	if (ShouldUseMultiDraw()) {
		if (!drawIdVertexEnabled) {
			drawIdVertexEnabled = true;
			qglEnableVertexAttribArray( Attributes::Default::DrawId );
		}
		drawCommandBuffer.Commit( reinterpret_cast< byte* >( currentCommands ), numDrawCalls * sizeof(DrawElementsIndirectCommand) );
		vertexCache.BindIndex();
		qglMultiDrawElementsIndirect(GL_TRIANGLES, GL_INDEX_TYPE, drawCommandBuffer.BufferOffset(currentCommands), numDrawCalls, 0);
	} else {
		if (drawIdVertexEnabled) {
			drawIdVertexEnabled = false;
			qglDisableVertexAttribArray( Attributes::Default::DrawId );
		}
		for (int i = 0; i < numDrawCalls; ++i) {
			DrawElementsIndirectCommand &cmd = currentCommands[i];
			const void *indexOffset = reinterpret_cast< const void* >( cmd.firstIndex * sizeof(glIndex_t) );
			qglVertexAttribI1i(Attributes::Default::DrawId, i);
			qglDrawElementsBaseVertex(GL_TRIANGLES, cmd.count, GL_INDEX_TYPE, indexOffset, cmd.baseVertex);
		}
	}

	currentIndex = 0;
	currentCommands = nullptr;
	maxDrawCommands = 0;

	if ( r_showPrimitives.GetBool() && !backEnd.viewDef->IsLightGem() && backEnd.viewDef->viewEntitys ) {
		backEnd.pc.c_drawElements += numDrawCalls;
		backEnd.pc.c_drawIndexes += numIndexes;
		backEnd.pc.c_drawVertexes += numVerts;
		backEnd.pc.c_vboIndexes += numIndexes;
	}
}

void DrawBatchExecutor::Lock() {
	drawCommandBuffer.Lock();
}

bool DrawBatchExecutor::ShouldUseMultiDraw() const {
	return GLAD_GL_ARB_multi_draw_indirect && r_useMultiDrawIndirect;
}

void DrawBatchExecutor::InitDrawIdBuffer() {
	qglGenBuffers(1, &drawIdBuffer);
	qglBindBuffer(GL_ARRAY_BUFFER, drawIdBuffer);
	std::vector<uint32_t> drawIds (MAX_DRAW_COMMANDS);
	for (uint32_t i = 0; i < MAX_DRAW_COMMANDS; ++i) {
		drawIds[i] = i;
	}
	qglBufferData(GL_ARRAY_BUFFER, drawIds.size(), drawIds.data(), GL_STATIC_DRAW);
	qglBindBuffer(GL_ARRAY_BUFFER, 0);

	qglVertexAttribIFormat( Attributes::Default::DrawId, 1, GL_UNSIGNED_INT, 0 );
	qglBindVertexBuffer( Attributes::Default::DrawId, drawIdBuffer, 0, sizeof(uint32_t) );
	qglVertexAttribBinding( Attributes::Default::DrawId, Attributes::Default::DrawId );
	qglVertexBindingDivisor( Attributes::Default::DrawId, 1 );
	qglEnableVertexAttribArray( Attributes::Default::DrawId );
	drawIdVertexEnabled = true;
}
