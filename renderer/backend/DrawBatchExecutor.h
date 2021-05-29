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
#include "GpuBuffer.h"

template< typename ShaderParams >
struct DrawBatch {
    ShaderParams *shaderParams;
    const drawSurf_t **surfs;
    uint maxBatchSize;
};

/**
 * Use this class to batch draw calls sharing the same GL state together.
 * Depending on hardware capabilities and active cvars, the batch will be
 * submitted in a single MultiDrawIndirect call or in separate
 * DrawElementsBaseVertex calls. To pass draw call parameters to the shader,
 * the class also manages and provides a UBO that can be filled with the
 * relevant data.
 *
 * To start a batch, call `BeginBatch` with the ShaderParams struct that
 * represents your UBO params structure. Note that this struct must adhere
 * to the std140 layout rules specified in the OpenGL specification. You
 * will receive a DrawBatch struct that contains two arrays - one for your
 * ShaderParams and one for the drawSurfs to render. For each surf you want
 * to render, store the drawSurf_t* in batch.surfs[i] and the corresponding
 * shader parameters in batch.shaderParams[i] (with i < `maxBatchSize`).
 * Then call `ExecuteDrawVertBatch` or `ExecuteShadowVertBatch`, depending
 * on what you intend to draw, with the actual number of surfs to draw.
 */
class DrawBatchExecutor {
public:
	static const GLuint DEFAULT_UBO_INDEX = 1;
	
	void Init();
	void Destroy();

	template< typename ShaderParams >
	DrawBatch<ShaderParams> BeginBatch();

	void ExecuteDrawVertBatch( int numDrawSurfs, int numInstances = 1, GLuint uboIndex = DEFAULT_UBO_INDEX );
	void ExecuteShadowVertBatch( int numDrawSurfs, GLuint uboIndex = DEFAULT_UBO_INDEX );

	/**
	 * Use this to upload and bind an additional UBO besides the default per draw call buffer.
	 * You need to call this before starting draw batches, as otherwise you'll corrupt the underlying memory.
	 */
	void UploadExtraUboData( void *data, size_t size, GLuint uboIndex );

	void EndFrame();

	template< typename ShaderParams >
	uint MaxShaderParamsArraySize() const {
		return MaxShaderParamsArraySize( sizeof(ShaderParams) );
	}
	uint MaxShaderParamsArraySize( uint paramsSize ) const {
	    // limit max supported value, since AMD cards support insanely large UBOs (in theory)
		return Min( (uint)(maxUniformBlockSize / paramsSize), MAX_PARAMS_ARRAY_SIZE );
	}

private:
	static const uint MAX_SHADER_PARAMS_SIZE = 512;
	static const uint MAX_PARAMS_ARRAY_SIZE = 256;
	
	GpuBuffer shaderParamsBuffer;
	GpuBuffer drawCommandBuffer;
	GLuint drawIdBuffer = 0;
	bool drawIdVertexEnabled = false;
	int drawIdVertexDivisor = 1;

	int maxUniformBlockSize = 0;

	int maxBatchSize = 0;
	uint shaderParamsSize = 0;

	idList<const drawSurf_t *> drawSurfs;

	bool ShouldUseMultiDraw() const;
	void InitDrawIdBuffer();

	uint EnsureAvailableStorageInBuffers( uint shaderParamsSize );

	typedef uint (*BaseVertexFn)(const drawSurf_t *);
	void ExecuteBatch( int numDrawSurfs, int numInstances, GLuint uboIndex, attribBind_t attribBind, BaseVertexFn baseVertexFn );
	void BatchMultiDraw( int numDrawSurfs, int numInstances, BaseVertexFn baseVertexFn );
	void BatchSingleDraws( int numDrawSurfs, int numInstances, BaseVertexFn baseVertexFn );
};

template<typename ShaderParams>
DrawBatch<ShaderParams> DrawBatchExecutor::BeginBatch() {
	static_assert( sizeof(ShaderParams) % 16 == 0,
		"UBO structs must be 16-byte aligned, use padding if necessary. Be sure to obey the std140 layout rules." );
	static_assert( sizeof(ShaderParams) <= MAX_SHADER_PARAMS_SIZE,
		"Struct surpasses assumed max shader params size. Make struct smaller or increase MAX_SHADER_PARAMS_SIZE if necessary");

	shaderParamsSize = sizeof(ShaderParams);

    ::DrawBatch<ShaderParams> drawBatch;
    drawBatch.maxBatchSize = maxBatchSize = EnsureAvailableStorageInBuffers( sizeof(ShaderParams) );
    drawBatch.shaderParams = reinterpret_cast<ShaderParams *>( shaderParamsBuffer.CurrentWriteLocation() );
	drawBatch.surfs = drawSurfs.Ptr();
    return drawBatch;
}
