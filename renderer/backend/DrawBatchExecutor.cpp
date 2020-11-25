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

const uint DrawBatchExecutor::DEFAULT_UBO_INDEX;
const uint DrawBatchExecutor::MAX_SHADER_PARAMS_SIZE;

namespace {
	const uint MAX_DRAWS_PER_FRAME = 8192;

	struct DrawElementsIndirectCommand {
		uint count;
		uint instanceCount;
		uint firstIndex;
		uint baseVertex;
		uint baseInstance;
	};

	uint BaseVertexDrawVert( const drawSurf_t *surf ) {
		return surf->ambientCache.offset / sizeof( idDrawVert );
	}

	uint BaseVertexShadowVert( const drawSurf_t *surf ) {
		return surf->shadowCache.offset / sizeof( shadowCache_t );
	}

}

idCVarBool r_useMultiDrawIndirect("r_useMultiDrawIndirect", "1", CVAR_RENDERER|CVAR_BOOL, "Batch draw calls in multidraw commands if available");

void DrawBatchExecutor::Init() {
	GLint uboAlignment;
	qglGetIntegerv( GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uboAlignment );
	qglGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUniformBlockSize);

	uint shaderParamsBufferSize = MAX_DRAWS_PER_FRAME * MAX_SHADER_PARAMS_SIZE;
	shaderParamsBuffer.Init( GL_UNIFORM_BUFFER, shaderParamsBufferSize, uboAlignment );

	if (GLAD_GL_ARB_multi_draw_indirect) {
		InitDrawIdBuffer();	
		uint drawCommandBufferSize = MAX_DRAWS_PER_FRAME * sizeof(DrawElementsIndirectCommand);
		drawCommandBuffer.Init( GL_DRAW_INDIRECT_BUFFER, drawCommandBufferSize, 16 );
		drawCommandBuffer.Bind();
	}
}

void DrawBatchExecutor::Destroy() {
	shaderParamsBuffer.Destroy();
	qglDeleteBuffers(1, &drawIdBuffer);
	drawIdBuffer = 0;
	drawCommandBuffer.Destroy();
}

void DrawBatchExecutor::ExecuteDrawVertBatch( int numDrawSurfs, int numInstances, GLuint uboIndex ) {
	ExecuteBatch( numDrawSurfs, numInstances, uboIndex, ATTRIB_REGULAR, &BaseVertexDrawVert );

	if ( r_showPrimitives.GetBool() && !backEnd.viewDef->IsLightGem() && backEnd.viewDef->viewEntitys ) {
		for ( int i = 0; i < numDrawSurfs; ++i ) {
			const drawSurf_t *surf = drawSurfs[i];
			backEnd.pc.c_drawIndexes += surf->numIndexes;
			if ( surf->frontendGeo ) {
				backEnd.pc.c_drawVertexes += surf->frontendGeo->numVerts;
			}
			backEnd.pc.c_vboIndexes += surf->numIndexes;
		}
		backEnd.pc.c_drawElements += numDrawSurfs;
	}
}

void DrawBatchExecutor::ExecuteShadowVertBatch( int numDrawSurfs, GLuint uboIndex ) {
	ExecuteBatch( numDrawSurfs, 1, uboIndex, ATTRIB_SHADOW, &BaseVertexShadowVert );

	if ( r_showPrimitives.GetBool() && !backEnd.viewDef->IsLightGem() && backEnd.viewDef->viewEntitys ) {
		for ( int i = 0; i < numDrawSurfs; ++i ) {
			const drawSurf_t *surf = drawSurfs[i];
			backEnd.pc.c_shadowIndexes += surf->numIndexes;
			if ( surf->frontendGeo ) {
				backEnd.pc.c_shadowVertexes += surf->frontendGeo->numVerts;
			}
		}
		backEnd.pc.c_shadowElements += numDrawSurfs;
	}
}

void DrawBatchExecutor::EndFrame() {
	shaderParamsBuffer.SwitchFrame();
	if ( ShouldUseMultiDraw() ) {
		drawCommandBuffer.SwitchFrame();
	}
}

bool DrawBatchExecutor::ShouldUseMultiDraw() const {
	return GLAD_GL_ARB_multi_draw_indirect && r_useMultiDrawIndirect;
}

void DrawBatchExecutor::InitDrawIdBuffer() {
	qglGenBuffers(1, &drawIdBuffer);
	qglBindBuffer(GL_ARRAY_BUFFER, drawIdBuffer);
	std::vector<uint32_t> drawIds (MAX_DRAWS_PER_FRAME);
	for (uint32_t i = 0; i < MAX_DRAWS_PER_FRAME; ++i) {
		drawIds[i] = i;
	}
	qglBufferData(GL_ARRAY_BUFFER, drawIds.size() * sizeof(uint32_t), drawIds.data(), GL_STATIC_DRAW);
	qglBindBuffer(GL_ARRAY_BUFFER, 0);

	qglVertexAttribIFormat( Attributes::Default::DrawId, 1, GL_UNSIGNED_INT, 0 );
	qglBindVertexBuffer( Attributes::Default::DrawId, drawIdBuffer, 0, sizeof(uint32_t) );
	qglVertexAttribBinding( Attributes::Default::DrawId, Attributes::Default::DrawId );
	qglVertexBindingDivisor( Attributes::Default::DrawId, drawIdVertexDivisor );
	qglEnableVertexAttribArray( Attributes::Default::DrawId );
	drawIdVertexEnabled = true;
}

uint DrawBatchExecutor::EnsureAvailableStorageInBuffers(uint shaderParamsSize) {
    // check if our GPU buffers have enough storage left for at least 1 draw command
    // if not, prematurely switch to the next frame of storage, possibly incurring a wait on a GPU fence
    if ( shaderParamsBuffer.BytesRemaining() < shaderParamsSize ) {
        shaderParamsBuffer.SwitchFrame();
    }
    if ( ShouldUseMultiDraw() && drawCommandBuffer.BytesRemaining() < sizeof(DrawElementsIndirectCommand) ) {
        drawCommandBuffer.SwitchFrame();
    }

	uint maxBatchSize = Min( shaderParamsBuffer.BytesRemaining(), (uint)maxUniformBlockSize ) / shaderParamsSize;
	if ( ShouldUseMultiDraw() ) {
		uint maxDrawCommandsCount = drawCommandBuffer.BytesRemaining() / sizeof(DrawElementsIndirectCommand);
		maxBatchSize = Min( maxBatchSize, maxDrawCommandsCount );
	}

	drawSurfs.AssureSize( maxBatchSize );

	return maxBatchSize;
}

void DrawBatchExecutor::ExecuteBatch( int numDrawSurfs, int numInstances, GLuint uboIndex, attribBind_t attribBind, BaseVertexFn baseVertexFn ) {
	assert( numDrawSurfs <= maxBatchSize );
	maxBatchSize = 0;

	if ( numDrawSurfs == 0 ) {
		return;
	}

	byte *shaderParamsContents = shaderParamsBuffer.CurrentWriteLocation();
	uint shaderParamsCommitSize = numDrawSurfs * shaderParamsSize;
	shaderParamsBuffer.Commit( shaderParamsCommitSize );
	shaderParamsBuffer.BindRangeToIndexTarget( uboIndex, shaderParamsContents, shaderParamsCommitSize );

	vertexCache.BindVertex( attribBind );
	vertexCache.BindIndex();
	if ( ShouldUseMultiDraw() ) {
		BatchMultiDraw( numDrawSurfs, numInstances, baseVertexFn );
	} else {
		BatchSingleDraws( numDrawSurfs, numInstances, baseVertexFn );
	}
}

void DrawBatchExecutor::BatchMultiDraw( int numDrawSurfs, int numInstances, BaseVertexFn baseVertexFn ) {
	DrawElementsIndirectCommand * drawCommands = reinterpret_cast<DrawElementsIndirectCommand *>( drawCommandBuffer.CurrentWriteLocation() );	
	for ( int i = 0; i < numDrawSurfs; ++i ) {
		DrawElementsIndirectCommand &cmd = drawCommands[i];
		const drawSurf_t *surf = drawSurfs[i];
		cmd.count = surf->numIndexes;
		cmd.instanceCount = numInstances;
		cmd.firstIndex = surf->indexCache.offset / sizeof(glIndex_t);
		cmd.baseVertex = baseVertexFn( surf );
		cmd.baseInstance = i;
	}
	drawCommandBuffer.Commit( numDrawSurfs * sizeof(DrawElementsIndirectCommand) );

	if (!drawIdVertexEnabled) {
		drawIdVertexEnabled = true;
		qglEnableVertexAttribArray( Attributes::Default::DrawId );
	}
	if (drawIdVertexDivisor != numInstances) {
	    drawIdVertexDivisor = numInstances;
        qglVertexBindingDivisor( Attributes::Default::DrawId, drawIdVertexDivisor );
	}
	qglMultiDrawElementsIndirect(GL_TRIANGLES, GL_INDEX_TYPE, drawCommandBuffer.BufferOffset(drawCommands), numDrawSurfs, 0);
}

void DrawBatchExecutor::BatchSingleDraws( int numDrawSurfs, int numInstances, BaseVertexFn baseVertexFn ) {
	if (drawIdVertexEnabled) {
		drawIdVertexEnabled = false;
		qglDisableVertexAttribArray( Attributes::Default::DrawId );
	}
	for (int i = 0; i < numDrawSurfs; ++i) {
		const drawSurf_t *surf = drawSurfs[i];
		qglVertexAttribI1i(Attributes::Default::DrawId, i);
		const void *indexOffset = (void*)(uintptr_t)surf->indexCache.offset;
		uint baseVertex = baseVertexFn( surf );
		if ( numInstances == 1 ) {
			qglDrawElementsBaseVertex(GL_TRIANGLES, surf->numIndexes, GL_INDEX_TYPE, indexOffset, baseVertex);
		} else {
			qglDrawElementsInstancedBaseVertex( GL_TRIANGLES, surf->numIndexes, GL_INDEX_TYPE, indexOffset, numInstances, baseVertex );
		}
	}
}
