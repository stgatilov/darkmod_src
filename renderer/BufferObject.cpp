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

idCVar r_showBuffers( "r_showBuffers", "0", CVAR_INTEGER, "" );


static const GLenum bufferUsage = GL_DYNAMIC_DRAW_ARB;


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

#ifdef ID_WIN_X86_SSE2_INTRIN
typedef unsigned int uint32;
void CopyBuffer( byte * dst, const byte * src, int numBytes ) {
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
		*( uint32 * )&dst[i] = *( const uint32 * )&src[i];
	}
	for( ; i < numBytes; i++ ) {
		dst[i] = src[i];
	}
	_mm_sfence();
}

#else

void CopyBuffer( byte * dst, const byte * src, int numBytes ) {
	assert_16_byte_aligned( dst );
	assert_16_byte_aligned( src );
	//memcpy( dst, src, numBytes );
	SIMDProcessor->Memcpy( dst, src, numBytes );
}

#endif

/*
================================================================================================

idVertexBuffer

================================================================================================
*/

/*
========================
idVertexBuffer::idVertexBuffer
========================
*/
idVertexBuffer::idVertexBuffer() {
	size = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = NULL;
	SetUnmapped();
}

/*
========================
idVertexBuffer::~idVertexBuffer
========================
*/
idVertexBuffer::~idVertexBuffer() {
	FreeBufferObject();
}

/*
========================
idVertexBuffer::AllocBufferObject
========================
*/
bool idVertexBuffer::AllocBufferObject( const void * data, int allocSize ) {
	assert( apiObject == NULL );
	assert_16_byte_aligned( data );

	if( allocSize <= 0 ) {
		idLib::Error( "idVertexBuffer::AllocBufferObject: allocSize = %i", allocSize );
	}

	size = allocSize;

	bool allocationFailed = false;

	int numBytes = GetAllocedSize();


	// clear out any previous error
	qglGetError();

	GLuint bufferObject = 0xFFFF;
	qglGenBuffersARB( 1, &bufferObject );
	if( bufferObject == 0xFFFF ) {
		common->FatalError( "idVertexBuffer::AllocBufferObject: failed" );
	}
	qglBindBufferARB( GL_ARRAY_BUFFER_ARB, bufferObject );

	// these are rewritten every frame
	qglBufferDataARB( GL_ARRAY_BUFFER_ARB, numBytes, NULL, bufferUsage );
	apiObject = reinterpret_cast< void * >( bufferObject );

	GLenum err = qglGetError();
	if( err == GL_OUT_OF_MEMORY ) {
		common->Warning( "idVertexBuffer::AllocBufferObject: allocation failed" );
		allocationFailed = true;
	}


	if( r_showBuffers.GetBool() ) {
		common->Printf( "vertex buffer alloc %p, api %p (%i bytes)\n", this, GetAPIObject(), GetSize() );
	}

	// copy the data
	if( data != NULL ) {
		Update( data, allocSize );
	}

	return !allocationFailed;
}

/*
========================
idVertexBuffer::FreeBufferObject
========================
*/
void idVertexBuffer::FreeBufferObject() {
	if( IsMapped() ) {
		UnmapBuffer();
	}

	// if this is a sub-allocation inside a larger buffer, don't actually free anything.
	if( OwnsBuffer() == false ) {
		ClearWithoutFreeing();
		return;
	}

	if( apiObject == NULL ) {
		return;
	}

	if( r_showBuffers.GetBool() ) {
		common->Printf( "vertex buffer free %p, api %p (%i bytes)\n", this, GetAPIObject(), GetSize() );
	}

	GLuint bufferObject = reinterpret_cast< GLuint >( apiObject );
	qglDeleteBuffersARB( 1, &bufferObject );

	ClearWithoutFreeing();
}

/*
========================
idVertexBuffer::Reference
========================
*/
void idVertexBuffer::Reference( const idVertexBuffer & other ) {
	assert( IsMapped() == false );
	assert( other.GetAPIObject() != NULL );
	assert( other.GetSize() > 0 );

	FreeBufferObject();
	size = other.GetSize();						// this strips the MAPPED_FLAG
	offsetInOtherBuffer = other.GetOffset();	// this strips the OWNS_BUFFER_FLAG
	apiObject = other.apiObject;
	assert( OwnsBuffer() == false );
}

/*
========================
idVertexBuffer::Reference
========================
*/
void idVertexBuffer::Reference( const idVertexBuffer & other, int refOffset, int refSize ) {
	assert( IsMapped() == false );
	assert( other.GetAPIObject() != NULL );
	assert( refOffset >= 0 );
	assert( refSize >= 0 );
	assert( refOffset + refSize <= other.GetSize() );

	FreeBufferObject();
	size = refSize;
	offsetInOtherBuffer = other.GetOffset() + refOffset;
	apiObject = other.apiObject;
	assert( OwnsBuffer() == false );
}

/*
========================
idVertexBuffer::Update
========================
*/
void idVertexBuffer::Update( const void * data, int updateSize ) const {
	assert( apiObject != NULL );
	assert( IsMapped() == false );
	assert_16_byte_aligned( data );
	assert( ( GetOffset() & 15 ) == 0 );

	if( updateSize > size ) {
		common->FatalError( "idVertexBuffer::Update: size overrun, %i > %i\n", updateSize, GetSize() );
	}

	int numBytes = ( updateSize + 15 ) & ~15;

	GLuint bufferObject = reinterpret_cast< GLuint >( apiObject );
	qglBindBufferARB( GL_ARRAY_BUFFER_ARB, bufferObject );
	qglBufferSubDataARB( GL_ARRAY_BUFFER_ARB, GetOffset(), ( GLsizeiptrARB )numBytes, data );
}

/*
========================
idVertexBuffer::MapBuffer
========================
*/
void * idVertexBuffer::MapBuffer( bufferMapType_t mapType, int mapOffset ) const {
	assert( apiObject != NULL );
	assert( IsMapped() == false );

	void * buffer = NULL;

	GLuint bufferObject = reinterpret_cast< GLuint >( apiObject );
	qglBindBufferARB( GL_ARRAY_BUFFER_ARB, bufferObject );
	if( mapType == BM_READ ) {
		buffer = glMapBufferRange( GL_ARRAY_BUFFER_ARB, mapOffset, GetAllocedSize() - mapOffset, GL_MAP_READ_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT );
		if( buffer != NULL ) {
			buffer = ( byte * )buffer + GetOffset();
		}
	}
	else if( mapType == BM_WRITE ) {
		buffer = glMapBufferRange( GL_ARRAY_BUFFER_ARB, mapOffset, GetAllocedSize() - mapOffset, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_FLUSH_EXPLICIT_BIT );
		if( buffer != NULL ) {
			buffer = ( byte * )buffer + GetOffset();
		}
	}
	else {
		assert( false );
	}

	SetMapped();

	if( buffer == NULL ) {
		common->FatalError( "idVertexBuffer::MapBuffer: failed" );
	}
	return buffer;
}

/*
========================
idVertexBuffer::UnmapBuffer
========================
*/
void idVertexBuffer::FlushBuffer( int offset, int length ) {
	assert( apiObject != NULL );
	assert( IsMapped() );

	GLuint bufferObject = reinterpret_cast< GLuint >( apiObject );
	qglBindBufferARB( GL_ARRAY_BUFFER_ARB, bufferObject );
	glFlushMappedBufferRange( GL_ARRAY_BUFFER_ARB, offset, length );
}

/*
========================
idVertexBuffer::UnmapBuffer
========================
*/
void idVertexBuffer::UnmapBuffer() const {
	assert( apiObject != NULL );
	assert( IsMapped() );

	GLuint bufferObject = reinterpret_cast< GLuint >( apiObject );
	qglBindBufferARB( GL_ARRAY_BUFFER_ARB, bufferObject );
	if( !qglUnmapBufferARB( GL_ARRAY_BUFFER_ARB ) ) {
		common->Printf( "idVertexBuffer::UnmapBuffer failed\n" );
	}

	SetUnmapped();
}

/*
========================
idVertexBuffer::ClearWithoutFreeing
========================
*/
void idVertexBuffer::ClearWithoutFreeing() {
	size = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = NULL;
}

/*
================================================================================================

idIndexBuffer

================================================================================================
*/

/*
========================
idIndexBuffer::idIndexBuffer
========================
*/
idIndexBuffer::idIndexBuffer() {
	size = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = NULL;
	SetUnmapped();
}

/*
========================
idIndexBuffer::~idIndexBuffer
========================
*/
idIndexBuffer::~idIndexBuffer() {
	FreeBufferObject();
}

/*
========================
idIndexBuffer::AllocBufferObject
========================
*/
bool idIndexBuffer::AllocBufferObject( const void * data, int allocSize ) {
	assert( apiObject == NULL );
	assert_16_byte_aligned( data );

	if( allocSize <= 0 ) {
		idLib::Error( "idIndexBuffer::AllocBufferObject: allocSize = %i", allocSize );
	}

	size = allocSize;

	bool allocationFailed = false;

	int numBytes = GetAllocedSize();


	// clear out any previous error
	qglGetError();

	GLuint bufferObject = 0xFFFF;
	qglGenBuffersARB( 1, &bufferObject );
	if( bufferObject == 0xFFFF ) {
		GLenum error = qglGetError();
		common->FatalError( "idIndexBuffer::AllocBufferObject: failed - GL_Error %d", error );
	}
	qglBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, bufferObject );

	// these are rewritten every frame
	qglBufferDataARB( GL_ELEMENT_ARRAY_BUFFER_ARB, numBytes, NULL, bufferUsage );
	apiObject = reinterpret_cast< void * >( bufferObject );

	GLenum err = qglGetError();
	if( err == GL_OUT_OF_MEMORY ) {
		common->Warning( "idIndexBuffer:AllocBufferObject: allocation failed" );
		allocationFailed = true;
	}


	if( r_showBuffers.GetBool() ) {
		common->Printf( "index buffer alloc %p, api %p (%i bytes)\n", this, GetAPIObject(), GetSize() );
	}

	// copy the data
	if( data != NULL ) {
		Update( data, allocSize );
	}

	return !allocationFailed;
}

/*
========================
idIndexBuffer::FreeBufferObject
========================
*/
void idIndexBuffer::FreeBufferObject() {
	if( IsMapped() ) {
		UnmapBuffer();
	}

	// if this is a sub-allocation inside a larger buffer, don't actually free anything.
	if( OwnsBuffer() == false ) {
		ClearWithoutFreeing();
		return;
	}

	if( apiObject == NULL ) {
		return;
	}

	if( r_showBuffers.GetBool() ) {
		common->Printf( "index buffer free %p, api %p (%i bytes)\n", this, GetAPIObject(), GetSize() );
	}

	GLuint bufferObject = reinterpret_cast< GLuint >( apiObject );
	qglDeleteBuffersARB( 1, &bufferObject );

	ClearWithoutFreeing();
}

/*
========================
idIndexBuffer::Reference
========================
*/
void idIndexBuffer::Reference( const idIndexBuffer & other ) {
	assert( IsMapped() == false );
	//assert( other.IsMapped() == false );	// this happens when building idTriangles while at the same time setting up triIndex_t
	assert( other.GetAPIObject() != NULL );
	assert( other.GetSize() > 0 );

	FreeBufferObject();
	size = other.GetSize();						// this strips the MAPPED_FLAG
	offsetInOtherBuffer = other.GetOffset();	// this strips the OWNS_BUFFER_FLAG
	apiObject = other.apiObject;
	assert( OwnsBuffer() == false );
}

/*
========================
idIndexBuffer::Reference
========================
*/
void idIndexBuffer::Reference( const idIndexBuffer & other, int refOffset, int refSize ) {
	assert( IsMapped() == false );
	assert( other.GetAPIObject() != NULL );
	assert( refOffset >= 0 );
	assert( refSize >= 0 );
	assert( refOffset + refSize <= other.GetSize() );

	FreeBufferObject();
	size = refSize;
	offsetInOtherBuffer = other.GetOffset() + refOffset;
	apiObject = other.apiObject;
	assert( OwnsBuffer() == false );
}

/*
========================
idIndexBuffer::Update
========================
*/
void idIndexBuffer::Update( const void * data, int updateSize ) const {

	assert( apiObject != NULL );
	assert( IsMapped() == false );
	assert_16_byte_aligned( data );
	assert( ( GetOffset() & 15 ) == 0 );

	if( updateSize > size ) {
		common->FatalError( "idIndexBuffer::Update: size overrun, %i > %i\n", updateSize, GetSize() );
	}

	int numBytes = ( updateSize + 15 ) & ~15;

	GLuint bufferObject = reinterpret_cast< GLuint >( apiObject );
	qglBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, bufferObject );
	qglBufferSubDataARB( GL_ELEMENT_ARRAY_BUFFER_ARB, GetOffset(), ( GLsizeiptrARB )numBytes, data );
}

/*
========================
idIndexBuffer::MapBuffer
========================
*/
void * idIndexBuffer::MapBuffer( bufferMapType_t mapType, int mapOffset ) const {

	assert( apiObject != NULL );
	assert( IsMapped() == false );

	void * buffer = NULL;

	GLuint bufferObject = reinterpret_cast< GLuint >( apiObject );
	qglBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, bufferObject );
	if( mapType == BM_READ ) {
		buffer = glMapBufferRange( GL_ELEMENT_ARRAY_BUFFER_ARB, 0, GetAllocedSize(), GL_MAP_READ_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT );
		if( buffer != NULL ) {
			buffer = ( byte * )buffer + GetOffset();
		}
	}
	else if( mapType == BM_WRITE ) {
		buffer = glMapBufferRange( GL_ELEMENT_ARRAY_BUFFER_ARB, 0, GetAllocedSize(), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_FLUSH_EXPLICIT_BIT );
		if( buffer != NULL ) {
			buffer = ( byte * )buffer + GetOffset();
		}
	}
	else {
		assert( false );
	}

	SetMapped();

	if( buffer == NULL ) {
		common->FatalError( "idIndexBuffer::MapBuffer: failed" );
	}
	return buffer;
}

/*
========================
idIndexBuffer::FlushBuffer
========================
*/
void idIndexBuffer::FlushBuffer( int offset, int length ) {
	assert( apiObject != NULL );
	assert( IsMapped() );

	GLuint bufferObject = reinterpret_cast< GLuint >( apiObject );
	qglBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, bufferObject );
	glFlushMappedBufferRange( GL_ELEMENT_ARRAY_BUFFER_ARB, offset, length );
}

/*
========================
idIndexBuffer::UnmapBuffer
========================
*/
void idIndexBuffer::UnmapBuffer() const {
	assert( apiObject != NULL );
	assert( IsMapped() );

	GLuint bufferObject = reinterpret_cast< GLuint >( apiObject );
	qglBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, bufferObject );
	if( !qglUnmapBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB ) ) {
		common->Printf( "idIndexBuffer::UnmapBuffer failed\n" );
	}

	SetUnmapped();
}

/*
========================
idIndexBuffer::ClearWithoutFreeing
========================
*/
void idIndexBuffer::ClearWithoutFreeing() {
	size = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = NULL;
}
