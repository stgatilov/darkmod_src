/*****************************************************************************
                    The Dark Mod GPL Source Code

 This file is part of the The Dark Mod Source Code, originally based
 on the Doom 3 GPL Source Code as published in 2011.

 The Dark Mod Source Code is free software: you can redistribute it
 and/or modify it under the terms of the GNU General Public License as
 published by the Free Software Foundation, either version 3 of the License,
 or (at your option) any later version. For details, see LICENSE.TXT.

 Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/
#pragma once

class UniversalGpuBuffer {
public:
	void Init( GLenum type, GLuint size, GLuint alignment );
	void Destroy();

	void Lock();

	byte *Reserve( GLuint size, bool precommit = false );
	void Commit( byte *offset, GLuint size );
	void BindRange( GLuint index, byte *offset, GLuint size );
	void Bind();
	const void * BufferOffset( const void *offset );

private:
	struct LockedRange {
		GLuint offset;
		GLuint count;
		GLsync fenceSync;

		bool Overlaps(const LockedRange& other) const {
			return offset < other.offset + other.count && other.offset < offset + count;
		}
	};

	bool mUsePersistentMapping;

	GLenum mType;
	GLuint mBufferObject = 0;
	GLuint mSize = 0;
	GLuint mAlign = 0;
	byte * mMapBase = nullptr;
	GLuint mCurrentOffset = 0;
	GLuint mLastLocked = 0;
	std::vector<LockedRange> mRangeLocks;

	void LockRange( GLuint offset, GLuint count );
	void WaitForLockedRange( GLuint offset, GLuint count );
	void Wait( LockedRange &range );	
};
