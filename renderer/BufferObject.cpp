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
	qglBindBuffer( GL_ARRAY_BUFFER_ARB, 0 );
	qglBindBuffer( GL_ELEMENT_ARRAY_BUFFER_ARB, 0 );
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
	size = allocSize;

	int numBytes = GetAllocedSize();

	// clear out any previous error
	qglGetError();

	qglGenBuffers( 1, &bufferObject );
	if( bufferObject == 0 ) {
		common->FatalError( "BufferObject::AllocBufferObject: failed" );
	}
	qglBindBuffer( bufferType, bufferObject );
	qglBufferData( bufferType, numBytes, initialData, bufferUsage );

	GLenum err = qglGetError();
	if( err == GL_OUT_OF_MEMORY ) {
		common->FatalError( "BufferObject::AllocBufferObject: allocation failed - out of memory" );
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

	if (qglMapBufferRange) {
		buffer = qglMapBufferRange(bufferType, mapOffset, GetAllocedSize() - mapOffset, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);
	} else {
		if (mapOffset != 0) {
			common->FatalError("Cannot map range of buffer starting from %d without glMapBufferRange", mapOffset);
		}
		buffer = qglMapBuffer(bufferType, GL_WRITE_ONLY);
	}

	if( buffer == NULL ) {
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

	qglBindBuffer( bufferType, bufferObject );

	if (qglFlushMappedBufferRange) {
		qglFlushMappedBufferRange( bufferType, offset, length );
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

	if( !qglUnmapBuffer( bufferType ) ) {
		common->Warning( "BufferObject::UnmapBuffer failed\n" );
	}
	SetUnmapped();
}
