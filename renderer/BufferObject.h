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

// Returns all targets to virtual memory use instead of buffer object use.
// Call this before doing any conventional buffer reads, like screenshots.
void UnbindBufferObjects();

/*
================================================
BufferObject
================================================
*/
class BufferObject {
public:
	BufferObject(GLenum targetType, GLenum usage = GL_DYNAMIC_DRAW_ARB);
	~BufferObject();

	// Allocate or free the buffer.
	void				AllocBufferObject(int allocSize, const void *initialData = nullptr);
	void				FreeBufferObject();

	// Map / flush / unmap buffer
	void *				MapBuffer(int mapOffset = 0);
	void				FlushBuffer(int offset, int length);
	void				UnmapBuffer();

	bool				IsMapped() const { return ( size & MAPPED_FLAG ) != 0; }
	int					GetSize() const { return ( size & ~MAPPED_FLAG ); }
	int					GetAllocedSize() const { return ( ( size & ~MAPPED_FLAG ) + 15 ) & ~15;	}

	GLuint				GetAPIObject() const { return bufferObject; }

	int					size;					// size in bytes
private:
	GLuint				bufferObject;
	GLenum              bufferType;
	GLenum              bufferUsage;

	// sizeof() confuses typeinfo...
	static const int	MAPPED_FLAG = 1 << ( 4 /* sizeof( int ) */ * 8 - 1 );

	void				SetMapped() { size |= MAPPED_FLAG; }
	void				SetUnmapped() { size &= ~MAPPED_FLAG; }

	BufferObject( const BufferObject& ) = delete;
	void operator=( const BufferObject& ) = delete;
};

#endif // !__BUFFEROBJECT_H__
