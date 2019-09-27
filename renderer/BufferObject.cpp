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

void BufferObject::Resize( int allocSize ) {
	common->Printf( "New buffer size: %d kb\n", allocSize / 1024 );

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
		qglBufferStorage( bufferType, numBytes, NULL, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT );
	else
		qglBufferData( bufferType, numBytes, NULL, GL_STATIC_DRAW );

	GLenum err = qglGetError();
	if ( err == GL_OUT_OF_MEMORY ) {
		common->FatalError( "BufferObject::AllocBufferObject: allocation failed - out of memory" );
	}

	if ( oldBuffObj ) {
		qglBindBuffer( GL_COPY_READ_BUFFER, oldBuffObj );
		qglBindBuffer( GL_COPY_WRITE_BUFFER, bufferObject );
		qglCopyBufferSubData( GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, oldSize );
		qglDeleteBuffers( 1, &oldBuffObj );
	}
	if ( persistentMap ) {
		mapBuff = qglMapBufferRange( bufferType, 0, numBytes, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT );
		if ( mapBuff == NULL ) {
			GL_CheckErrors();
			common->Error( "BufferObject::MapBuffer: failed" );
		}
	}
}

/*
========================
FreeBufferObject
========================
*/
void BufferObject::FreeBufferObject() {
	qglDeleteBuffers( 1, &tempBuff );
	if( bufferObject == 0 ) {
		return;
	}

	if( IsMapped() ) {
		UnmapBuffer( 0 );
	}
	qglDeleteBuffers( 1, &bufferObject );

	size = 0;
	bufferObject = 0;
}

/*
========================
MapBuffer
========================
*/
void * BufferObject::MapBuffer( int mapOffset, int size ) {
	assert( bufferObject != 0 );
	assert( IsMapped() == false );

	void *buffer = NULL;

	qglBindBuffer( bufferType, bufferObject );

	if ( persistentMap ) {
		//buffer = qglMapBufferRange( bufferType, mapOffset, mappedSize, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_FLUSH_EXPLICIT_BIT | GL_MAP_PERSISTENT_BIT );
		lastMapOffset = mapOffset;
		buffer = (byte*)mapBuff + mapOffset;
	} else {
		lastMapOffset = mapOffset;
		if ( !tempBuff )
			qglGenBuffers( 1, &tempBuff );
		qglBindBuffer( bufferType, tempBuff );
		qglBufferData( bufferType, mappedSize = size, NULL, GL_STATIC_DRAW );
		buffer = qglMapBufferRange( bufferType, 0, mappedSize, GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT );
	}

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
	assert( tempBuff != 0 );
	assert( IsMapped() );
	//if ( !length )
	//	return;
	if ( length > mappedSize )
		length = mappedSize;

	qglBindBuffer( bufferType, bufferObject );

	if ( persistentMap ) {
		//qglFlushMappedBufferRange( bufferType, offset, length );
		//qglFlushMappedBufferRange( bufferType, offset + lastMapOffset, length );
	} else {
		qglBindBuffer( bufferType, tempBuff );
		if ( length )
			qglFlushMappedBufferRange( bufferType, 0, length );
		qglUnmapBuffer( bufferType );
		qglBindBuffer( GL_COPY_READ_BUFFER, tempBuff );
		qglBindBuffer( GL_COPY_WRITE_BUFFER, bufferObject );
		qglCopyBufferSubData( GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, lastMapOffset, length );
	}

	SetUnmapped();
}
