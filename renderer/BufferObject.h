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
	BufferObject(GLenum targetType);
	~BufferObject();

	// Allocate or free the buffer.
	void				AllocBufferObject(int allocSize, const void *initialData = nullptr);
	void				FreeBufferObject();
	void				Resize( int allocSize, void* data=NULL );

	// Map / flush / unmap buffer
	void *				MapBuffer( int mapOffset, int size );
	void				UnmapBuffer( int length );

	bool				IsMapped() const { return mapped; }
	int					GetSize() const { return size; }
	int					GetAllocedSize() const { return ( size + 15 ) & ~15;	}

	GLuint				GetAPIObject() const { return bufferObject; }
	GLenum				GetBufferType() const { return bufferType; }

	int					size;					// size in bytes

private:
	GLuint				bufferObject;
	GLenum              bufferType;

	bool				persistentMap;
	void *				mapBuff;
	int					mappedSize;
	int					lastMapOffset;
	bool				mapped;

	void				SetMapped() { mapped = true; }
	void				SetUnmapped() { mapped = false; }

	BufferObject( const BufferObject& ) = delete;
	void operator=( const BufferObject& ) = delete;
};

#endif // !__BUFFEROBJECT_H__
