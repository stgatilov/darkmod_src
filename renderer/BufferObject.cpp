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
#pragma hdrstop
#include "tr_local.h"
#include "BufferObject.h"

idCVarBool r_usePersistentMapping( "r_usePersistentMapping", "1", CVAR_RENDERER | CVAR_ARCHIVE, "Use persistent buffer mapping" );

/*
================================================================================================

Buffer Objects

================================================================================================
*/

/*
========================
UnbindBufferObjects
========================
*/
void UnbindBufferObjects() {
	qglBindBuffer( GL_ARRAY_BUFFER, 0 );
	qglBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
}

/*
========================
BufferObject
========================
*/
BufferObject::BufferObject( GLenum targetType ) {
	mapBuff = nullptr;
	size = 0;
	bufferObject = 0;
	bufferType = targetType;
	SetUnmapped();
}

/*
========================
~BufferObject
========================
*/
BufferObject::~BufferObject() {
	FreeBufferObject();
}

/*
========================
AllocBufferObject
========================
*/
void BufferObject::AllocBufferObject( int allocSize, const void *initialData ) {
	assert( bufferObject == 0 );

	if( allocSize <= 0 ) {
		idLib::Error( "BufferObject::AllocBufferObject: allocSize = %i", allocSize );
	}
	Resize( allocSize );
}

void BufferObject::Resize( int allocSize, void* data ) {
	common->Printf( "New buffer size: %d kb\n", allocSize / 1024 );

	if ( !persistentMap && mapBuff ) {
		Mem_Free16( mapBuff );
		mapBuff = nullptr;
	}

	int oldSize = GetAllocedSize();
	size = allocSize;

	int numBytes = GetAllocedSize();

	// clear out any previous error
	qglGetError();

	GLuint oldBuffObj = bufferObject;
	qglGenBuffers( 1, &bufferObject );
	if ( bufferObject == 0 ) {
		common->FatalError( "BufferObject::AllocBufferObject: failed" );
	}
	qglBindBuffer( bufferType, bufferObject );
	persistentMap = glConfig.bufferStorageAvailable && r_usePersistentMapping;
	if ( persistentMap )
		qglBufferStorage( bufferType, numBytes, data, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT );
	else
		qglBufferData( bufferType, numBytes, data, GL_DYNAMIC_DRAW );

	GLenum err = qglGetError();
	if ( err == GL_OUT_OF_MEMORY ) {
		common->FatalError( "BufferObject::AllocBufferObject: allocation failed - out of memory" );
	}

	if ( oldBuffObj ) {
		if ( !data ) {
			qglBindBuffer( GL_COPY_READ_BUFFER, oldBuffObj );
			qglBindBuffer( GL_COPY_WRITE_BUFFER, bufferObject );
			qglCopyBufferSubData( GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, Min( oldSize, numBytes ) );
		}
		qglDeleteBuffers( 1, &oldBuffObj );
	}
	if ( persistentMap ) {
		mapBuff = qglMapBufferRange( bufferType, 0, numBytes, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT | GL_MAP_UNSYNCHRONIZED_BIT );
		if ( mapBuff == NULL ) {
			GL_CheckErrors();
			common->Error( "BufferObject::MapBuffer: failed" );
		}
	} else {
		mapBuff = Mem_Alloc16( numBytes );
	}
}

/*
========================
FreeBufferObject
========================
*/
void BufferObject::FreeBufferObject() {
	if( bufferObject == 0 ) {
		return;
	}

	if( IsMapped() ) {
		UnmapBuffer( 0 );
	}
	qglDeleteBuffers( 1, &bufferObject );

	size = 0;
	bufferObject = 0;
	if ( !persistentMap && mapBuff ) {
		Mem_Free16( mapBuff );
		mapBuff = nullptr;
	}
}

/*
========================
MapBuffer
========================
*/
void * BufferObject::MapBuffer( int mapOffset, int size ) {
	assert( bufferObject != 0 );
	assert( IsMapped() == false );

	mappedSize = size;
	void *buffer = NULL;

	lastMapOffset = mapOffset;
	buffer = (byte*)mapBuff + mapOffset;

	if( buffer == NULL ) {
		GL_CheckErrors();
		common->Error( "BufferObject::MapBuffer: failed" );
	}
	SetMapped();

	return buffer;
}

/*
========================
UnmapBuffer
========================
*/
void BufferObject::UnmapBuffer( int length ) {
	assert( bufferObject != 0 );
	assert( IsMapped() );
	if ( length > mappedSize )
		length = mappedSize;

	qglBindBuffer( bufferType, bufferObject );

	if ( persistentMap ) {
		if ( length )
			qglFlushMappedBufferRange( bufferType, lastMapOffset, length );
		GL_CheckErrors();
	} else {
		if ( length ) {
			qglBufferSubData( bufferType, lastMapOffset, length, (byte*)mapBuff + lastMapOffset );
		}
	}

	SetUnmapped();
}
