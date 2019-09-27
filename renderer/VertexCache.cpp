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
#include <mutex>
#pragma hdrstop

#include "tr_local.h"

const int MAX_VERTCACHE_SIZE = VERTCACHE_OFFSET_MASK+1;

idCVar idVertexCache::r_showVertexCache( "r_showVertexCache", "0", CVAR_INTEGER | CVAR_RENDERER, "Show VertexCache usage statistics" );
idCVar idVertexCache::r_frameVertexMemory( "r_frameVertexMemory", "4096", CVAR_INTEGER | CVAR_RENDERER | CVAR_ARCHIVE, "Initial amount of per-frame temporary vertex memory, in kB (max 131071)" );
idCVar idVertexCache::r_frameIndexMemory( "r_frameIndexMemory", "4096", CVAR_INTEGER | CVAR_RENDERER | CVAR_ARCHIVE, "Initial amount of per-frame temporary index memory, in kB (max 131071)" );
idCVar r_useFenceSync( "r_useFenceSync", "1", CVAR_BOOL | CVAR_RENDERER | CVAR_ARCHIVE, "Use GPU sync" );

idVertexCache		vertexCache;
GLsync				bufferLock[3] = { 0,0,0 };
uint32_t			staticVertexSize, staticIndexSize;

/*
==============
ClearGeoBufferSet
==============
*/
static void ClearGeoBufferSet( geoBufferSet_t &gbs ) {
	gbs.indexMemUsed = 0;
	gbs.vertexMemUsed = 0;
	gbs.allocations = 0;
}

/*
==============
MapGeoBufferSet
==============
*/
static void MapGeoBufferSet( geoBufferSet_t &gbs, int frame ) {
	if ( gbs.mappedVertexBase == NULL ) {
		int dynamicSize = gbs.vertexBuffer.size - staticVertexSize;
		int frameSize = dynamicSize / VERTCACHE_NUM_FRAMES;
		gbs.vertexMapOffset = staticVertexSize + frameSize * frame;
		gbs.mappedVertexBase = ( byte * )gbs.vertexBuffer.MapBuffer( gbs.vertexMapOffset, frameSize );
	}
	if ( gbs.mappedIndexBase == NULL ) {
		int dynamicSize = gbs.indexBuffer.size - staticIndexSize;
		int frameSize = dynamicSize / VERTCACHE_NUM_FRAMES;
		gbs.indexMapOffset = staticIndexSize + frameSize * frame;
		gbs.mappedIndexBase = ( byte * )gbs.indexBuffer.MapBuffer( gbs.indexMapOffset, frameSize );
	}
}

/*
==============
UnmapGeoBufferSet
==============
*/

static void UnmapGeoBufferSet( geoBufferSet_t &gbs, int frame ) {
	if ( gbs.mappedVertexBase != NULL ) {
		gbs.vertexBuffer.UnmapBuffer( gbs.vertexMemUsed );
		gbs.mappedVertexBase = NULL;
	}
	if ( gbs.mappedIndexBase != NULL ) {
		gbs.indexBuffer.UnmapBuffer( gbs.indexMemUsed );
		gbs.mappedIndexBase = NULL;
	}
}

/*
==============
AllocGeoBufferSet
==============
*/
static void AllocGeoBufferSet( geoBufferSet_t &gbs, const int vertexBytes, const int indexBytes ) {
	gbs.vertexBuffer.AllocBufferObject( vertexBytes );
	gbs.indexBuffer.AllocBufferObject( indexBytes );
	ClearGeoBufferSet( gbs );
}

static void LockGeoBufferSet( int frame ) {
	GL_CheckErrors();
	bufferLock[frame] = qglFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
}

static void WaitForGeoBufferSet( int frame ) {
	if ( bufferLock[frame] == 0 ) { 
		return;
	}
	GLenum result = qglClientWaitSync( bufferLock[frame], 0, 1 );

	while ( result != GL_ALREADY_SIGNALED && result != GL_CONDITION_SATISFIED ) {
		result = qglClientWaitSync( bufferLock[frame], GL_SYNC_FLUSH_COMMANDS_BIT, 1000000 );
		if ( result == GL_CONDITION_SATISFIED )	{ 
			backEnd.pc.waitedFor = 'S'; 
		}
		if ( result == GL_WAIT_FAILED ) {
			common->Warning( "glClientWaitSync failed.\n" );
			break;
		}
	}
	qglDeleteSync( bufferLock[frame] );
	bufferLock[frame] = 0;
}

static void FreeGeoBufferSet( geoBufferSet_t &gbs ) {
	gbs.vertexBuffer.FreeBufferObject();
	gbs.indexBuffer.FreeBufferObject();
}

/*
==============
idVertexCache::VertexPosition
==============
*/
void *idVertexCache::VertexPosition( vertCacheHandle_t handle ) {
	GLuint vbo;
	if ( !handle.IsValid() ) {
		vbo = 0;
	} else {
		++vertexUseCount;
		vbo = dynamicData.vertexBuffer.GetAPIObject();
	}
	if ( vbo != currentVertexBuffer ) {
		qglBindBuffer( GL_ARRAY_BUFFER, vbo );
		currentVertexBuffer = vbo;
	}
	return ( void * )( size_t )( handle.offset );
}

/*
==============
idVertexCache::IndexPosition
==============
*/
void *idVertexCache::IndexPosition( vertCacheHandle_t handle ) {
	GLuint vbo;
	if ( !handle.IsValid() ) {
		vbo = 0;
	} else {
		++indexUseCount;
		vbo = dynamicData.indexBuffer.GetAPIObject();
	}
	if ( vbo != currentIndexBuffer ) {
		qglBindBuffer( GL_ELEMENT_ARRAY_BUFFER, vbo );
		currentIndexBuffer = vbo;
	}
	return ( void * )( size_t )( handle.offset );
}

/*
==============
idVertexCache::UnbindIndex
==============
*/
void idVertexCache::UnbindIndex() {
	if ( currentIndexBuffer != 0 ) {
		qglBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
		currentIndexBuffer = 0;
	}
}

//================================================================================

geoBufferSet_t::geoBufferSet_t() : indexBuffer( GL_ELEMENT_ARRAY_BUFFER ), vertexBuffer( GL_ARRAY_BUFFER ) {}

/*
===========
idVertexCache::Init
===========
*/
void idVertexCache::Init() {
	listNum = 0;
	backendListNum = 0;
	currentFrame = 0;
	currentIndexCacheSize = idMath::Imin( MAX_VERTCACHE_SIZE, r_frameIndexMemory.GetInteger() * 1024 );
	currentVertexCacheSize = idMath::Imin( MAX_VERTCACHE_SIZE, r_frameVertexMemory.GetInteger() * 1024 );
	if ( currentIndexCacheSize <= 0 || currentVertexCacheSize <= 0 ) {
		common->FatalError( "Dynamic vertex cache size is invalid. Please adjust r_frameIndexMemory and r_frameVertexMemory." );
	}

	// 2.08 core context https://stackoverflow.com/questions/13403807/glvertexattribpointer-raising-gl-invalid-operation
	GLuint vao;
	qglGenVertexArrays( 1, &vao );
	qglBindVertexArray( vao ); 
	
	AllocGeoBufferSet( dynamicData, currentVertexCacheSize * VERTCACHE_NUM_FRAMES, currentIndexCacheSize * VERTCACHE_NUM_FRAMES );
	EndFrame();
}

/*
===========
idVertexCache::PurgeAll

Used when toggling vertex programs on or off, because
the cached data isn't valid
===========
*/
void idVertexCache::PurgeAll() {
	Shutdown();
	Init();
}

/*
===========
idVertexCache::Shutdown
===========
*/
void idVertexCache::Shutdown() {
	UnmapGeoBufferSet( dynamicData, 0 );
	ClearGeoBufferSet( dynamicData );
	dynamicData.vertexBuffer.FreeBufferObject();
	dynamicData.indexBuffer.FreeBufferObject();
}

/*
===========
idVertexCache::EndFrame
===========
*/
void idVertexCache::EndFrame() {
	// display debug information
	if ( r_showVertexCache.GetBool() ) {
		common->Printf( "(FRONT) vertex: %d times totaling %d kB, index: %d times totaling %d kB\n", vertexAllocCount, dynamicData.vertexMemUsed / 1024, indexAllocCount, dynamicData.indexMemUsed / 1024 );
		common->Printf( "(BACK) vertex: %d = %d (x%d)/%d kB, index: %d = %d (x%d)/%d kB\n", vertexUseCount, currentVertexCacheSize>>10, VERTCACHE_NUM_FRAMES, dynamicData.vertexBuffer.GetAllocedSize()>>10, indexUseCount, currentIndexCacheSize>>10, VERTCACHE_NUM_FRAMES, dynamicData.indexBuffer.GetAllocedSize()>>10 );
	}

	// unmap the current frame so the GPU can read it
	UnmapGeoBufferSet( dynamicData, listNum );

	// check if we need to increase the buffer size
	if ( dynamicData.indexMemUsed > currentIndexCacheSize ) {
		common->Printf( "Exceeded index frame limit (%d kb), resizing...\n", currentIndexCacheSize / 1024 );
		while ( currentIndexCacheSize <= MAX_VERTCACHE_SIZE / VERTCACHE_NUM_FRAMES / 2 && currentIndexCacheSize < dynamicData.indexMemUsed ) {
			currentIndexCacheSize *= 2;
		}
	}
	if ( dynamicData.vertexMemUsed > currentVertexCacheSize ) {
		common->Printf( "Exceeded vertex frame limit (%d kb), resizing...\n", currentVertexCacheSize / 1024 );
		while ( currentVertexCacheSize < MAX_VERTCACHE_SIZE / VERTCACHE_NUM_FRAMES / 2 && currentVertexCacheSize < dynamicData.vertexMemUsed ) {
			currentVertexCacheSize *= 2;
		}
	}

	// ensure no GL draws are still active on the next buffer to write to
	int nextListNum = ( listNum + 1 ) % VERTCACHE_NUM_FRAMES;
	if ( glConfig.fenceSyncAvailable && r_useFenceSync.GetBool() ) {
		LockGeoBufferSet( backendListNum );
		WaitForGeoBufferSet( nextListNum );
	} else {
		// this is going to stall, but it probably doesn't matter on such old GPUs...
		qglFinish();
	}
	currentFrame++;
	backendListNum = listNum;
	listNum = nextListNum;
	indexAllocCount = indexUseCount = vertexAllocCount = vertexUseCount = 0;

	// check if we need to resize current buffer set
	if ( (uint32_t)dynamicData.vertexBuffer.GetSize() < staticVertexSize + currentVertexCacheSize * VERTCACHE_NUM_FRAMES ) {
		dynamicData.vertexBuffer.Resize( staticVertexSize + currentVertexCacheSize * VERTCACHE_NUM_FRAMES );
	}
	if ( ( uint32_t) dynamicData.indexBuffer.GetSize() < staticIndexSize + currentIndexCacheSize * VERTCACHE_NUM_FRAMES ) {
		dynamicData.indexBuffer.Resize( staticIndexSize + currentIndexCacheSize * VERTCACHE_NUM_FRAMES );
	}

	// prepare the next frame for writing to by the CPU
	MapGeoBufferSet( dynamicData, listNum );
	ClearGeoBufferSet( dynamicData );

	qglBindBuffer( GL_ARRAY_BUFFER, currentVertexBuffer = 0 );
	qglBindBuffer( GL_ELEMENT_ARRAY_BUFFER, currentIndexBuffer = 0 );
}

/*
==============
idVertexCache::ActuallyAlloc
==============
*/
vertCacheHandle_t idVertexCache::ActuallyAlloc( geoBufferSet_t &vcs, const void *data, int bytes, cacheType_t type ) {
	if ( bytes == 0 ) {
		return NO_CACHE;
	}
	assert( ( ( ( uintptr_t )( data ) ) & 15 ) == 0 );
	assert( ( bytes & 15 ) == 0 );

	// thread safe interlocked adds
	byte **base = NULL;
	int	endPos = 0;
	int mapOffset = 0;

	if ( type == CACHE_INDEX ) {
		base = &vcs.mappedIndexBase;
		endPos = vcs.indexMemUsed += bytes;
		mapOffset = vcs.indexMapOffset;
		if ( endPos > currentIndexCacheSize ) {
			// out of index cache, will be resized next frame
			return NO_CACHE;
		}
		indexAllocCount++;
	} else if ( type == CACHE_VERTEX ) {
		base = &vcs.mappedVertexBase;
		endPos = vcs.vertexMemUsed += bytes;
		mapOffset = vcs.vertexMapOffset;
		if ( endPos > currentVertexCacheSize ) {
			// out of vertex cache, will be resized next frame
			return NO_CACHE;
		}
		vertexAllocCount++;
	} else {
		assert( false );
		return NO_CACHE;
	}
	vcs.allocations++;

	int offset = endPos - bytes;

	// Actually perform the data transfer
	if ( data != NULL ) {
		void* dst = *base + offset;
		const byte* src = (byte*)data;
		assert_16_byte_aligned( dst );
		assert_16_byte_aligned( src );
		SIMDProcessor->Memcpy( dst, src, bytes );
	}

	return {
		static_cast<uint32_t>( bytes & VERTCACHE_SIZE_MASK ),
		static_cast<uint32_t>( (mapOffset + offset) & VERTCACHE_OFFSET_MASK ),
		static_cast<uint16_t>( currentFrame & VERTCACHE_FRAME_MASK ),
		false //&vcs == &staticData
	};
}

typedef idList <std::pair<const void*, int>> StaticList;
StaticList staticVertexList, staticIndexList;

/*
==============
idVertexCache::PrepareStaticCacheForUpload
==============
*/
void idVertexCache::PrepareStaticCacheForUpload() {
	auto upload = [](char *msg, BufferObject &buffer, int size, StaticList &staticList ) {
		common->Printf( msg );
		int offset = 0;
#if 1	// AMD
#if 1	// init storage
		byte* ptr = (byte*)Mem_Alloc16( size );
		for ( auto& pair : staticList ) {
			memcpy( ptr + offset, pair.first, pair.second );
			offset += pair.second;
		}
		buffer.Resize( size, ptr );
		Mem_Free16( ptr );
#else	// SubData
		buffer.Resize( size );
		for ( auto& pair : staticList ) {
			qglBufferSubData( buffer.GetBufferType(), offset, pair.second, pair.first );
			offset += pair.second;
		}
#endif
#else	// all
		buffer.Resize( size );
		byte* ptr = (byte*)buffer.MapBuffer( offset, size );
		for ( auto& pair : staticList ) {
			memcpy( ptr + offset, pair.first, pair.second );
			offset += pair.second;
		}
		buffer.UnmapBuffer( offset );
#endif
		staticList.Clear();
	};
	for ( int i = 0; i < VERTCACHE_NUM_FRAMES; i++ )
		EndFrame();
	UnmapGeoBufferSet( dynamicData, listNum );
	upload( "Static vertex data ready\n", dynamicData.vertexBuffer, staticVertexSize + currentVertexCacheSize * VERTCACHE_NUM_FRAMES, staticVertexList );
	upload( "Static index data ready\n", dynamicData.indexBuffer, staticIndexSize + currentIndexCacheSize * VERTCACHE_NUM_FRAMES, staticIndexList );
	MapGeoBufferSet( dynamicData, listNum );
	EndFrame();
}

vertCacheHandle_t idVertexCache::AllocStaticVertex( const void* data, int bytes ) {
	if ( !staticVertexList.Num() )
		staticVertexSize = 0;
	staticVertexList.Append( std::make_pair( data, bytes ) );
	staticVertexSize += bytes;
	return {
		static_cast<uint32_t>( bytes & VERTCACHE_SIZE_MASK ),
		static_cast<uint32_t>( (staticVertexSize-bytes) & VERTCACHE_OFFSET_MASK ),
		0,
		true,
	};
}

vertCacheHandle_t idVertexCache::AllocStaticIndex( const void* data, int bytes ) {
	if ( !staticIndexList.Num() )
		staticIndexSize = 0;
	staticIndexList.Append( std::make_pair( data, bytes ) );
	staticIndexSize += bytes;
	return {
		static_cast<uint32_t>( bytes & VERTCACHE_SIZE_MASK ),
		static_cast<uint32_t>( (staticIndexSize-bytes) & VERTCACHE_OFFSET_MASK ),
		0,
		true,
	};
}
