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

// vertex cache calls should only be made by the front end

const int NUM_VERTEX_FRAMES = 2;

typedef enum {
	TAG_FREE,
	TAG_USED,
	TAG_FIXED,		// for the temp buffers
	TAG_TEMP		// in frame temp area, not static area
} vertBlockTag_t;

typedef struct vertCache_s {
	GLuint			vbo;
	void			*virtMem;			// only one of vbo / virtMem will be set
	bool			indexBuffer;		// holds indexes instead of vertexes

	int				offset;
	int				size;				// may be larger than the amount asked for, due
										// to round up and minimum fragment sizes
	int				tag;				// a tag of 0 is a free block
	struct vertCache_s	**	user;				// will be set to zero when purged
	struct vertCache_s *next, *prev;	// may be on the static list or one of the frame lists
	int				frameUsed;			// it can't be purged if near the current frame
} vertCache_t;


class idVertexCache {
public:
	void			Init();
	void			Shutdown();

	// just for gfxinfo printing
	bool			IsFast();

	// called when vertex programs are enabled or disabled, because
	// the cached data is no longer valid
	void			PurgeAll();

	// Tries to allocate space for the given data in fast vertex
	// memory, and copies it over.
	// Alloc does NOT do a touch, which allows purging of things
	// created at level load time even if a frame hasn't passed yet.
	// These allocations can be purged, which will zero the pointer.
	void			Alloc( void *data, int bytes, vertCache_t **buffer, bool indexBuffer = false );

	// This will be a real pointer with virtual memory,
	// but it will be an int offset cast to a pointer of ARB_vertex_buffer_object
	void *			Position( vertCache_t *buffer );

	// if r_useIndexBuffers is enabled, but you need to draw something without
	// an indexCache, this must be called to reset GL_ELEMENT_ARRAY_BUFFER_ARB
	void			UnbindIndex();

	// automatically freed at the end of the next frame
	// used for specular texture coordinates and gui drawing, which
	// will change every frame.
	// will return NULL if the vertex cache is completely full
	// As with Position(), this may not actually be a pointer you can access.
	vertCache_t	*	AllocFrameTemp( void *data, int bytes );

	// notes that a buffer is used this frame, so it can't be purged
	// out from under the GPU
	void			Touch( vertCache_t *buffer );

	// this block won't have to zero a buffer pointer when it is purged,
	// but it must still wait for the frames to pass, in case the GPU
	// is still referencing it
	void			Free( vertCache_t *buffer );	

	// updates the counter for determining which temp space to use
	// and which blocks can be purged
	// Also prints debugging info when enabled
	void			EndFrame();

	// listVertexCache calls this
	void			List();

private:
	void			InitMemoryBlocks( int size );
	void			ActuallyFree( vertCache_t *block );

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

	bool			virtualMemory;			// not fast stuff

	bool			allocatingTempBuffer;	// force GL_STREAM_DRAW_ARB

	vertCache_t		*tempBuffers[NUM_VERTEX_FRAMES];		// allocated at startup
	bool			tempOverflow;			// had to alloc a temp in static memory

	idBlockAlloc<vertCache_t,1024>	headerAllocator;

	vertCache_t		freeStaticHeaders;		// head of doubly linked list
	vertCache_t		freeDynamicHeaders;		// head of doubly linked list
	vertCache_t		dynamicHeaders;			// head of doubly linked list
	vertCache_t		deferredFreeList;		// head of doubly linked list
	vertCache_t		staticHeaders;			// head of doubly linked list in MRU order,
											// staticHeaders.next is most recently used

	int				frameBytes;				// for each of NUM_VERTEX_FRAMES frames
};

extern	idVertexCache	vertexCache;
