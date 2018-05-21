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

#ifndef __BUFFEROBJECT_H__
#define __BUFFEROBJECT_H__

/*
================================================================================================

Buffer Objects

================================================================================================
*/

class idIndexBuffer;
class idDrawVert;

enum bufferMapType_t {
	BM_READ,			// map for reading
	BM_WRITE			// map for writing
};

// Returns all targets to virtual memory use instead of buffer object use.
// Call this before doing any conventional buffer reads, like screenshots.
void UnbindBufferObjects();

/*
================================================
idVertexBuffer
================================================
*/
class idVertexBuffer {
public:
	idVertexBuffer();
	~idVertexBuffer();

	// Allocate or free the buffer.
	bool				AllocBufferObject( const void * data, int allocSize );
	void				FreeBufferObject();

	// Make this buffer a reference to another buffer.
	void				Reference( const idVertexBuffer & other );
	void				Reference( const idVertexBuffer & other, int refOffset, int refSize );

	// Copies data to the buffer. 'size' may be less than the originally allocated size.
	void				Update( const void * data, int updateSize ) const;

	void *				MapBuffer( bufferMapType_t mapType, int mapOffset = 0 ) const;
	idDrawVert *		MapVertexBuffer( bufferMapType_t mapType ) const { return static_cast< idDrawVert * >( MapBuffer( mapType ) ); }
	void				FlushBuffer( int offset, int length );
	void				UnmapBuffer() const;
	bool				IsMapped() const { return ( size & MAPPED_FLAG ) != 0; }

	int					GetSize() const { return ( size & ~MAPPED_FLAG ); }
	int					GetAllocedSize() const { return ( ( size & ~MAPPED_FLAG ) + 15 ) & ~15; }
	void *				GetAPIObject() const { return apiObject; }
	int					GetOffset() const { return ( offsetInOtherBuffer & ~OWNS_BUFFER_FLAG ); }

private:
	int					size;					// size in bytes
	int					offsetInOtherBuffer;	// offset in bytes
	void *				apiObject;

	// sizeof() confuses typeinfo...
	static const int	MAPPED_FLAG = 1 << ( 4 /* sizeof( int ) */ * 8 - 1 );
	static const int	OWNS_BUFFER_FLAG = 1 << ( 4 /* sizeof( int ) */ * 8 - 1 );

private:
	void				ClearWithoutFreeing();
	void				SetMapped() const { const_cast< int & >( size ) |= MAPPED_FLAG; }
	void				SetUnmapped() const { const_cast< int & >( size ) &= ~MAPPED_FLAG; }
	bool				OwnsBuffer() const { return ( ( offsetInOtherBuffer & OWNS_BUFFER_FLAG ) != 0 ); }

	idVertexBuffer( const idVertexBuffer& );
	void operator=( const idVertexBuffer& );
};

/*
================================================
idIndexBuffer
================================================
*/
typedef unsigned short triIndex_t;
class idIndexBuffer {
public:
	idIndexBuffer();
	~idIndexBuffer();

	// Allocate or free the buffer.
	bool				AllocBufferObject( const void * data, int allocSize );
	void				FreeBufferObject();

	// Make this buffer a reference to another buffer.
	void				Reference( const idIndexBuffer & other );
	void				Reference( const idIndexBuffer & other, int refOffset, int refSize );

	// Copies data to the buffer. 'size' may be less than the originally allocated size.
	void				Update( const void * data, int updateSize ) const;

	void *				MapBuffer( bufferMapType_t mapType, int mapOffset = 0 ) const;
	triIndex_t *		MapIndexBuffer( bufferMapType_t mapType ) const { return static_cast< triIndex_t * >( MapBuffer( mapType ) ); }
	void				FlushBuffer( int offset, int length );
	void				UnmapBuffer() const;
	bool				IsMapped() const { return ( size & MAPPED_FLAG ) != 0; }

	int					GetSize() const { return ( size & ~MAPPED_FLAG ); }
	int					GetAllocedSize() const { return ( ( size & ~MAPPED_FLAG ) + 15 ) & ~15; }
	void *				GetAPIObject() const { return apiObject; }
	int					GetOffset() const { return ( offsetInOtherBuffer & ~OWNS_BUFFER_FLAG ); }

private:
	int					size;					// size in bytes
	int					offsetInOtherBuffer;	// offset in bytes
	void *				apiObject;

	// sizeof() confuses typeinfo...
	static const int	MAPPED_FLAG = 1 << ( 4 /* sizeof( int ) */ * 8 - 1 );
	static const int	OWNS_BUFFER_FLAG = 1 << ( 4 /* sizeof( int ) */ * 8 - 1 );

private:
	void				ClearWithoutFreeing();
	void				SetMapped() const { const_cast< int & >( size ) |= MAPPED_FLAG; }
	void				SetUnmapped() const { const_cast< int & >( size ) &= ~MAPPED_FLAG; }
	bool				OwnsBuffer() const { return ( ( offsetInOtherBuffer & OWNS_BUFFER_FLAG ) != 0 ); }

	idIndexBuffer( const idIndexBuffer& );
	void operator=( const idIndexBuffer& );
};

#endif // !__BUFFEROBJECT_H__
