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
BufferObject::BufferObject( GLenum targetType, GLenum usage ) {
	size = 0;
	bufferObject = 0;
	bufferType = targetType;
	bufferUsage = usage;
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
	canMap = glConfig.bufferStorageAvailable && r_usePersistentMapping;
	if ( canMap )
		qglBufferStorage( bufferType, numBytes, NULL, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT );
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
		UnmapBuffer();
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
void * BufferObject::MapBuffer( int mapOffset ) {
	assert( bufferObject != 0 );
	assert( IsMapped() == false );

	void *buffer = NULL;

	qglBindBuffer( bufferType, bufferObject );

	if ( canMap )
		buffer = qglMapBufferRange( bufferType, mapOffset, mappedSize, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_FLUSH_EXPLICIT_BIT | GL_MAP_PERSISTENT_BIT );
	else {
		lastMapOffset = mapOffset;
		//mapBuff = buffer = Mem_Alloc16( mappedSize );
		if ( !tempBuff )
			qglGenBuffers( 1, &tempBuff );
		qglBindBuffer( bufferType, tempBuff );
		if ( lastTempSize != mappedSize ) {
			//qglBufferData( bufferType, lastTempSize = mappedSize, NULL, GL_STATIC_DRAW );
		}
		qglBufferData( bufferType, lastTempSize = mappedSize, NULL, GL_STATIC_DRAW );
		buffer = qglMapBufferRange( bufferType, 0, mappedSize, GL_MAP_WRITE_BIT | /*GL_MAP_UNSYNCHRONIZED_BIT |*/ GL_MAP_FLUSH_EXPLICIT_BIT );
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
FlushBuffer
========================
*/
void BufferObject::FlushBuffer( int offset, int length ) {
	assert( bufferObject != 0 );
	assert( IsMapped() );
	//if ( !length )
	//	return;
	if ( length > mappedSize )
		length = mappedSize;

	qglBindBuffer( bufferType, bufferObject );

	if ( canMap )
		qglFlushMappedBufferRange( bufferType, offset, length );
	else {
#if 0
		qglBufferSubData( bufferType, lastMapOffset, length, mapBuff );
#else
		qglBindBuffer( bufferType, tempBuff );
		if ( length )
			qglFlushMappedBufferRange( bufferType, 0, length );
		qglUnmapBuffer( bufferType );
		qglBindBuffer( GL_COPY_READ_BUFFER, tempBuff );
		qglBindBuffer( GL_COPY_WRITE_BUFFER, bufferObject );
		qglCopyBufferSubData( GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, lastMapOffset, length );
#endif
	}
}

/*
========================
UnmapBuffer
========================
*/
void BufferObject::UnmapBuffer() {
	assert( bufferObject != 0 );
	assert( IsMapped() );

	qglBindBuffer( bufferType, bufferObject );

	if ( canMap ) {
		if ( !qglUnmapBuffer( bufferType ) ) {
			common->Warning( "BufferObject::UnmapBuffer failed\n" );
		}
	} else {
		//Mem_Free16( mapBuff );
	}
	SetUnmapped();
}
