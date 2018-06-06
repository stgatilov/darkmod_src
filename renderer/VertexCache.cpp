/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

$Revision$ (Revision of last commit)
$Date$ (Date of last commit)
$Author$ (Author of last commit)

******************************************************************************/

#include "precompiled.h"
#include <mutex>
#pragma hdrstop

#include "tr_local.h"

idCVar idVertexCache::r_showVertexCache( "r_showVertexCache", "0", CVAR_INTEGER | CVAR_RENDERER, "Show VertexCache usage statistics" );
idCVar idVertexCache::r_staticVertexMemory( "r_staticVertexMemory", "65536", CVAR_INTEGER | CVAR_RENDERER, "Amount of static vertex memory, in kB (max 65535)" );
idCVar idVertexCache::r_staticIndexMemory( "r_staticIndexMemory", "16384", CVAR_INTEGER | CVAR_RENDERER, "Amount of static index memory, in kB (max 65535)" );
idCVar idVertexCache::r_frameVertexMemory( "r_frameVertexMemory", "12288", CVAR_INTEGER | CVAR_RENDERER, "Amount of per-frame temporary vertex memory, in kB (max 65535)" );
idCVar idVertexCache::r_frameIndexMemory( "r_frameIndexMemory", "20480", CVAR_INTEGER | CVAR_RENDERER, "Amount of per-frame temporary index memory, in kB (max 65535)" );

idVertexCache		vertexCache;

void CopyBuffer( byte * dst, const byte * src, int numBytes );

/*
==============
ClearGeoBufferSet
==============
*/
static void ClearGeoBufferSet( geoBufferSet_t &gbs ) {
	gbs.indexMemUsed = 0;
	gbs.vertexMemUsed = 0;
	gbs.allocations = 0;
	gbs.vertexMapOffset = 0;
	gbs.indexMapOffset = 0;
}

/*
==============
MapGeoBufferSet
==============
*/
static void MapGeoBufferSet( geoBufferSet_t &gbs ) {
	if( gbs.mappedVertexBase == NULL ) {
		gbs.mappedVertexBase = ( byte * )gbs.vertexBuffer.MapBuffer( gbs.vertexMapOffset );
	}
	if( gbs.mappedIndexBase == NULL ) {
		gbs.mappedIndexBase = ( byte * )gbs.indexBuffer.MapBuffer( gbs.indexMapOffset );
	}
}

/*
==============
UnmapGeoBufferSet
==============
*/

static void UnmapGeoBufferSet( geoBufferSet_t &gbs ) {
	if( gbs.mappedVertexBase != NULL ) {
		gbs.vertexBuffer.FlushBuffer( 0, gbs.vertexMemUsed - gbs.vertexMapOffset );
		gbs.vertexBuffer.UnmapBuffer();
		gbs.mappedVertexBase = NULL;
	}
	if( gbs.mappedIndexBase != NULL ) {
		gbs.indexBuffer.FlushBuffer( 0, gbs.indexMemUsed - gbs.indexMapOffset );
		gbs.indexBuffer.UnmapBuffer();
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
	gbs.bufferLock = 0;
	ClearGeoBufferSet( gbs );
}

static void LockGeoBufferSet(geoBufferSet_t &gbs) {
	gbs.bufferLock = qglFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

static void WaitForGeoBufferSet(geoBufferSet_t &gbs) {
	if (gbs.bufferLock == 0)
		return;

	GLenum result = qglClientWaitSync(gbs.bufferLock, 0, 1);
	while (result != GL_ALREADY_SIGNALED && result != GL_CONDITION_SATISFIED) {
		result = qglClientWaitSync(gbs.bufferLock, GL_SYNC_FLUSH_COMMANDS_BIT, 1000000);
		if (result == GL_WAIT_FAILED) {
			common->Warning("glClientWaitSync failed.\n");
			break;
		}
	}
	qglDeleteSync(gbs.bufferLock);
	gbs.bufferLock = 0;
}

/*
==============
idVertexCache::VertexPosition
==============
*/
void *idVertexCache::VertexPosition( vertCacheHandle_t handle ) {
	GLuint vbo;
	if( handle == 0 ) {
		vbo = 0;
	} else if( CacheIsStatic( handle ) ) {
		++staticBufferUsed;
		vbo = staticData.vertexBuffer.GetAPIObject();
	} else {
		++tempBufferUsed;
		vbo = frameData[backendListNum].vertexBuffer.GetAPIObject();
	}
	if( vbo != currentVertexBuffer ) {
		qglBindBufferARB( GL_ARRAY_BUFFER_ARB, vbo );
		currentVertexBuffer = vbo;
	}
	return ( void * )( ( handle >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK );
}

/*
==============
idVertexCache::IndexPosition
==============
*/
void *idVertexCache::IndexPosition( vertCacheHandle_t handle ) {
	GLuint vbo;
	if( handle == 0 ) {
		vbo = 0;
	} else if( CacheIsStatic( handle ) ) {
		++staticBufferUsed;
		vbo = staticData.indexBuffer.GetAPIObject();
	}
	else {
		++tempBufferUsed;
		vbo = frameData[backendListNum].indexBuffer.GetAPIObject();
	}
	if( vbo != currentIndexBuffer ) {
		qglBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, vbo );
		currentIndexBuffer = vbo;
	}
	return ( void * )( ( handle >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK );
}

void idVertexCache::UnbindIndex() {
	if( currentIndexBuffer != 0 ) {
		qglBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, 0 );
		currentIndexBuffer = 0;
	}
}


//================================================================================

geoBufferSet_t::geoBufferSet_t( GLenum usage ) : indexBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, usage), vertexBuffer(GL_ARRAY_BUFFER_ARB, usage) {}

idVertexCache::idVertexCache() : staticData(GL_STATIC_DRAW_ARB) {}

/*
===========
idVertexCache::Init
===========
*/
void idVertexCache::Init() {
	listNum = 0;
	backendListNum = 0;
	currentFrame = 0;

	// set up the dynamic frame memory
	for( int i = 0; i < VERTCACHE_NUM_FRAMES; ++i ) {
		AllocGeoBufferSet( frameData[i], r_frameVertexMemory.GetInteger() * 1024, r_frameIndexMemory.GetInteger() * 1024 );
	}
	AllocGeoBufferSet( staticData, r_staticVertexMemory.GetInteger() * 1024, r_staticIndexMemory.GetInteger() * 1024 );

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
	//Shutdown();
	//Init();
}

/*
===========
idVertexCache::Shutdown
===========
*/
void idVertexCache::Shutdown() {
	for( int i = 0; i < VERTCACHE_NUM_FRAMES; ++i ) {
		UnmapGeoBufferSet( frameData[i] );
		ClearGeoBufferSet( frameData[i] );
		frameData[i].vertexBuffer.FreeBufferObject();
		frameData[i].indexBuffer.FreeBufferObject();
	}

	UnmapGeoBufferSet( staticData );
	ClearGeoBufferSet( staticData );
	staticData.vertexBuffer.FreeBufferObject();
	staticData.indexBuffer.FreeBufferObject();
}

/*
===========
idVertexCache::EndFrame
===========
*/
void idVertexCache::EndFrame() {
	// display debug information
	if ( r_showVertexCache.GetBool() ) {
		common->Printf( "(backend) used temp buffers: %d - used static buffers: %d\n", tempBufferUsed, staticBufferUsed );
		common->Printf( "(frontend) temp vertex alloc: %d kB - temp index alloc: %d kB - static vertex alloc: %d kB - static index alloc: %d kB\n", frameData[listNum].vertexMemUsed / 1024, frameData[listNum].indexMemUsed / 1024, staticData.vertexMemUsed / 1024, staticData.indexMemUsed / 1024 );
	}

	// unmap the current frame so the GPU can read it
	UnmapGeoBufferSet( frameData[listNum] );
	UnmapGeoBufferSet( staticData );

	// ensure no GL draws are still active on the next buffer to write to
	int nextListNum = (listNum + 1) % VERTCACHE_NUM_FRAMES;
	if (glConfig.fenceSyncAvailable) {
		LockGeoBufferSet(frameData[backendListNum]);
		WaitForGeoBufferSet(frameData[nextListNum]);
	} else {
		// this is going to stall, but it probably doesn't matter on such old GPUs...
		qglFinish();
	}
	currentFrame++;
	backendListNum = listNum;
	listNum = nextListNum;
	tempBufferUsed = 0;
	staticBufferUsed = 0;

	// prepare the next frame for writing to by the CPU
	MapGeoBufferSet( frameData[listNum] );
	ClearGeoBufferSet( frameData[listNum] );
	staticData.indexMapOffset = staticData.indexMemUsed;
	staticData.vertexMapOffset = staticData.vertexMemUsed;

	qglBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
	qglBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, 0 );
	currentVertexBuffer = 0;
	currentIndexBuffer = 0;
}

/*
==============
idVertexCache::ActuallyAlloc
==============
*/
vertCacheHandle_t idVertexCache::ActuallyAlloc( geoBufferSet_t & vcs, const void * data, int bytes, cacheType_t type ) {
	if( bytes == 0 ) {
		return 0;
	}

	assert( ( ( ( UINT_PTR )( data ) ) & 15 ) == 0 );
	assert( ( bytes & 15 ) == 0 );

	// thread safe interlocked adds
	byte ** base = NULL;
	int	endPos = 0;
	int mapOffset = 0;
	if( type == CACHE_INDEX ) {
		base = &vcs.mappedIndexBase;
		endPos = vcs.indexMemUsed += bytes;
		mapOffset = vcs.indexMapOffset;
		if( endPos > vcs.indexBuffer.GetAllocedSize() ) {
			common->Warning( "Out of index cache" );
			return 0;
		}
	}
	else if( type == CACHE_VERTEX ) {
		base = &vcs.mappedVertexBase;
		endPos = vcs.vertexMemUsed += bytes;
		mapOffset = vcs.vertexMapOffset;
		if( endPos > vcs.vertexBuffer.GetAllocedSize() ) {
			common->Warning( "Out of vertex cache" );
			return 0;
		}
	}
	else {
		assert( false );
		return 0;
	}

	vcs.allocations++;

	int offset = endPos - bytes;

	// Actually perform the data transfer
	if( data != NULL ) {
		MapGeoBufferSet( vcs );
		CopyBuffer( *base + offset - mapOffset, (byte*)data, bytes );
	}

	vertCacheHandle_t handle = ( ( uint64 )( currentFrame & VERTCACHE_FRAME_MASK ) << VERTCACHE_FRAME_SHIFT ) |
		( ( uint64 )( offset & VERTCACHE_OFFSET_MASK ) << VERTCACHE_OFFSET_SHIFT ) |
		( ( uint64 )( bytes & VERTCACHE_SIZE_MASK ) << VERTCACHE_SIZE_SHIFT );
	if( &vcs == &staticData ) {
		handle |= VERTCACHE_STATIC;
	}
	return handle;
}