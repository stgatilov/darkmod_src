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

#include "BufferObject.h"

// vertex cache calls should only be made by the front end

#define NUM_VERTEX_FRAMES			2
#define FRAME_MEMORY_BYTES			0x200000 // frame size
#define EXPAND_HEADERS				1024

const int VERTCACHE_INDEX_MEMORY_PER_FRAME = 16 * 1024 * 1024;
const int VERTCACHE_VERTEX_MEMORY_PER_FRAME = 16 * 1024 * 1024;

// there are a lot more static indexes than vertexes, because interactions are just new
// index lists that reference existing vertexes
const int STATIC_INDEX_MEMORY = 31 * 1024 * 1024;
const int STATIC_VERTEX_MEMORY = 31 * 1024 * 1024;	// make sure it fits in VERTCACHE_OFFSET_MASK!

const int VERTCACHE_NUM_FRAMES = 2;

const int VERTCACHE_STATIC = 1;					// in the static set, not the per-frame set
const int VERTCACHE_SIZE_SHIFT = 1;
const int VERTCACHE_SIZE_MASK = 0x7fffff;		// 8 megs 
const int VERTCACHE_OFFSET_SHIFT = 24;
const int VERTCACHE_OFFSET_MASK = 0x1ffffff;	// 32 megs 
const int VERTCACHE_FRAME_SHIFT = 49;
const int VERTCACHE_FRAME_MASK = 0x7fff;		// 15 bits = 32k frames to wrap around

const int VERTEX_CACHE_ALIGN = 32;
const int INDEX_CACHE_ALIGN = 16;
#define ALIGN( x, a ) ( ( ( x ) + ((a)-1) ) & ~((a)-1) )

enum cacheType_t {
	CACHE_VERTEX,
	CACHE_INDEX,
	CACHE_JOINT
};

struct geoBufferSet_t {
	idIndexBuffer		indexBuffer;
	idVertexBuffer		vertexBuffer;
	byte *				mappedVertexBase;
	byte *				mappedIndexBase;
	std::atomic<int>	indexMemUsed;
	std::atomic<int>	vertexMemUsed;
	int					allocations;	// number of index and vertex allocations combined
	int					vertexMapOffset;
	int					indexMapOffset;
};

class idVertexCache {
public:
	void			Init();
	void			Shutdown();

	// called when vertex programs are enabled or disabled, because
	// the cached data is no longer valid
	void			PurgeAll();

	// will be an int offset cast to a pointer of ARB_vertex_buffer_object
	void *			VertexPosition( vertCacheHandle_t handle );
	void *			IndexPosition( vertCacheHandle_t handle );

	// if r_useIndexBuffers is enabled, but you need to draw something without
	// an indexCache, this must be called to reset GL_ELEMENT_ARRAY_BUFFER_ARB
	void			UnbindIndex();

	// updates the counter for determining which temp space to use
	// and which blocks can be purged
	// Also prints debugging info when enabled
	void			EndFrame();

	// listVertexCache calls this
	void			List();

	// this data is only valid for one frame of rendering
	vertCacheHandle_t AllocVertex( const void * data, int bytes ) {
		return ActuallyAlloc( frameData[listNum], data, bytes, CACHE_VERTEX );
	}
	vertCacheHandle_t AllocIndex( const void * data, int bytes ) {
		return ActuallyAlloc( frameData[listNum], data, bytes, CACHE_INDEX );
	}

	// this data is valid until the next map load
	vertCacheHandle_t AllocStaticVertex( const void * data, int bytes ) {
		if( staticData.vertexMemUsed + bytes > STATIC_VERTEX_MEMORY ) {
			common->FatalError( "AllocStaticVertex failed, increase STATIC_VERTEX_MEMORY" );
		}
		return ActuallyAlloc( staticData, data, bytes, CACHE_VERTEX );
	}
	vertCacheHandle_t AllocStaticIndex( const void * data, int bytes ) {
		if( staticData.indexMemUsed + bytes > STATIC_INDEX_MEMORY ) {
			common->FatalError( "AllocStaticIndex failed, increase STATIC_INDEX_MEMORY" );
		}
		return ActuallyAlloc( staticData, data, bytes, CACHE_INDEX );
	}

	byte *			  MappedVertexBuffer( vertCacheHandle_t handle ) {
		assert( !CacheIsStatic( handle ) );
		const uint64 offset = ( int )( handle >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK;
		const uint64 frameNum = ( int )( handle >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
		assert( frameNum == ( currentFrame & VERTCACHE_FRAME_MASK ) );
		return frameData[listNum].mappedVertexBase + offset;
	}

	byte *			  MappedIndexBuffer( vertCacheHandle_t handle ) {
		assert( !CacheIsStatic( handle ) );
		const uint64 offset = ( int )( handle >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK;
		const uint64 frameNum = ( int )( handle >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
		assert( frameNum == ( currentFrame & VERTCACHE_FRAME_MASK ) );
		return frameData[listNum].mappedIndexBase + offset;
	}

	// Returns false if it's been purged
	// This can only be called by the front end, the back end should only be looking at
	// vertCacheHandle_t that are already validated.
	bool			CacheIsCurrent( const vertCacheHandle_t handle ) const {
		if( CacheIsStatic( handle ) ) {
			return true;
		}
		const uint64 frameNum = ( int )( handle >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
		if( frameNum != ( currentFrame & VERTCACHE_FRAME_MASK ) ) {
			return false;
		}
		return true;
	}

	static bool		CacheIsStatic( const vertCacheHandle_t handle ) {
		return ( handle & VERTCACHE_STATIC ) != 0;
	}


private:
	static idCVar	r_showVertexCache;
	static idCVar	r_vertexBufferMegs;

	int				staticCountTotal;
	int				staticAllocTotal;		// for end of frame purging

	int				staticAllocThisFrame;	// debug counter
	int				staticCountThisFrame;
	int				dynamicAllocThisFrame;
	int				dynamicCountThisFrame;

	int				currentFrame;			// for purgable block tracking
	int				listNum;				// currentFrame % NUM_VERTEX_FRAMES, determines which tempBuffers to use
	int				backendListNum;

	geoBufferSet_t	frameData[VERTCACHE_NUM_FRAMES];
	geoBufferSet_t  staticData;

	GLuint			currentVertexBuffer;
	GLuint			currentIndexBuffer;

	// High water marks for the per-frame buffers
	int				mostUsedVertex;
	int				mostUsedIndex;

	int				staticBufferUsed;
	int				tempBufferUsed;

	// Try to make room for <bytes> bytes
	vertCacheHandle_t ActuallyAlloc( geoBufferSet_t & vcs, const void * data, int bytes, cacheType_t type );
};

extern	idVertexCache	vertexCache;