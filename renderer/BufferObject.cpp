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
	qglBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
	qglBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, 0 );
}

/*
========================
CopyBuffer
========================
*/
#ifdef __SSE2__
typedef unsigned int uint32;
void CopyBuffer( byte * dst, const byte * src, int numBytes ) {
	if (((size_t)dst | (size_t)src) & 15) {
		//report error, but do not spam
		static int reportedCount = 0;
		if (reportedCount < 5) {
			reportedCount++;
			common->Warning("CopyBuffer: pointer not aligned, falling back to memcpy (slow)");
		}
		//use memcpy instead
		memcpy(dst, src, numBytes);
		return;
	}
	assert_16_byte_aligned( dst );
	assert_16_byte_aligned( src );

	int i = 0;
	for( ; i + 128 <= numBytes; i += 128 ) {
		__m128i d0 = _mm_load_si128( ( __m128i * )&src[i + 0 * 16] );
		__m128i d1 = _mm_load_si128( ( __m128i * )&src[i + 1 * 16] );
		__m128i d2 = _mm_load_si128( ( __m128i * )&src[i + 2 * 16] );
		__m128i d3 = _mm_load_si128( ( __m128i * )&src[i + 3 * 16] );
		__m128i d4 = _mm_load_si128( ( __m128i * )&src[i + 4 * 16] );
		__m128i d5 = _mm_load_si128( ( __m128i * )&src[i + 5 * 16] );
		__m128i d6 = _mm_load_si128( ( __m128i * )&src[i + 6 * 16] );
		__m128i d7 = _mm_load_si128( ( __m128i * )&src[i + 7 * 16] );
		_mm_stream_si128( ( __m128i * )&dst[i + 0 * 16], d0 );
		_mm_stream_si128( ( __m128i * )&dst[i + 1 * 16], d1 );
		_mm_stream_si128( ( __m128i * )&dst[i + 2 * 16], d2 );
		_mm_stream_si128( ( __m128i * )&dst[i + 3 * 16], d3 );
		_mm_stream_si128( ( __m128i * )&dst[i + 4 * 16], d4 );
		_mm_stream_si128( ( __m128i * )&dst[i + 5 * 16], d5 );
		_mm_stream_si128( ( __m128i * )&dst[i + 6 * 16], d6 );
		_mm_stream_si128( ( __m128i * )&dst[i + 7 * 16], d7 );
	}
	for( ; i + 16 <= numBytes; i += 16 ) {
		__m128i d = _mm_load_si128( ( __m128i * )&src[i] );
		_mm_stream_si128( ( __m128i * )&dst[i], d );
	}
	for( ; i + 4 <= numBytes; i += 4 ) {
		*(uint32 *)&dst[i] = *(const uint32 *)&src[i];
	}
	for( ; i < numBytes; i++ ) {
		dst[i] = src[i];
	}
	_mm_sfence();
}

#else

void CopyBuffer( byte *dst, const byte *src, int numBytes ) {
	assert_16_byte_aligned( dst );
	assert_16_byte_aligned( src );
	SIMDProcessor->Memcpy( dst, src, numBytes );
}

#endif

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

	qglGenBuffersARB( 1, &bufferObject );
	if( bufferObject == 0 ) {
		common->FatalError( "BufferObject::AllocBufferObject: failed" );
	}
	qglBindBufferARB( bufferType, bufferObject );
	qglBufferDataARB( bufferType, numBytes, initialData, bufferUsage );

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
	qglDeleteBuffersARB( 1, &bufferObject );

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

	qglBindBufferARB( bufferType, bufferObject );

	if (qglMapBufferRange) {
		buffer = qglMapBufferRange(bufferType, mapOffset, GetAllocedSize() - mapOffset, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);
	} else {
		if (mapOffset != 0) {
			common->FatalError("Cannot map range of buffer starting from %d without glMapBufferRange", mapOffset);
		}
		buffer = qglMapBufferARB(bufferType, GL_WRITE_ONLY);
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

	qglBindBufferARB( bufferType, bufferObject );

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

	qglBindBufferARB( bufferType, bufferObject );

	if( !qglUnmapBufferARB( bufferType ) ) {
		common->Warning( "BufferObject::UnmapBuffer failed\n" );
	}
	SetUnmapped();
}
