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
#include "precompiled.h"
#include "UniversalGpuBuffer.h"
#include "../tr_local.h"

extern idCVarBool r_usePersistentMapping;

void UniversalGpuBuffer::Init( GLenum type, GLuint size, GLuint alignment ) {
	if( mMapBase ) {
		Destroy();
	}

	mSize = ALIGN( size, alignment );
	mAlign = alignment;
	mType = type;

	qglGenBuffers( 1, &mBufferObject );
	qglBindBuffer( type, mBufferObject );

	mUsePersistentMapping = r_usePersistentMapping && glConfig.bufferStorageAvailable;

	if (mUsePersistentMapping) {
		qglBufferStorage( type, mSize, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT );
		mMapBase = ( byte* )qglMapBufferRange( type, 0, mSize, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT );
	} else {
		qglBufferData( type, mSize, nullptr, GL_DYNAMIC_DRAW );
		mMapBase = ( byte* )Mem_Alloc16( mSize );
	}
	mCurrentOffset = 0;
	mLastLocked = 0;
}

void UniversalGpuBuffer::Destroy() {
	if( !mBufferObject ) {
		return;
	}

	for( auto it : mRangeLocks ) {
		qglDeleteSync( it.fenceSync );
	}
	mRangeLocks.clear();

	if (mUsePersistentMapping) {
		qglBindBuffer( mType, mBufferObject );
		qglUnmapBuffer( mType );
	} else {
		Mem_Free16( mMapBase );
	}
	qglDeleteBuffers( 1, &mBufferObject );
	mBufferObject = 0;
	mMapBase = nullptr;
}

void UniversalGpuBuffer::Lock() {
	if( mCurrentOffset != mLastLocked ) {
		LockRange( mLastLocked, mCurrentOffset - mLastLocked );
		mLastLocked = mCurrentOffset;
	}
}

byte * UniversalGpuBuffer::Reserve( GLuint size, bool precommit ) {
	GLuint requestedSize = ALIGN( size, mAlign );
	if( requestedSize > mSize ) {
		common->Error( "Requested shader param size exceeds buffer size" );
	}

	if( mCurrentOffset + requestedSize > mSize ) {
		if( mCurrentOffset != mLastLocked ) {
			LockRange( mLastLocked, mCurrentOffset - mLastLocked );
		}
		mCurrentOffset = 0;
		mLastLocked = 0;
	}

	WaitForLockedRange( mCurrentOffset, requestedSize );
	byte *reserved = mMapBase + mCurrentOffset;

	if (precommit) {
		// already mark the reserved storage as used, so that additional reservations
		// get different memories
		mCurrentOffset += requestedSize;
	}
	return reserved;
}

void UniversalGpuBuffer::Commit( byte *offset, GLuint size ) {
	GLuint requestedSize = ALIGN( size, mAlign );
	GLintptr mapOffset = offset - mMapBase;
	assert( mapOffset >= 0 && mapOffset < mSize );

	// for persistent mapping, nothing to do. Otherwise, we need to upload the committed data
	if( !mUsePersistentMapping ) {
		qglBindBuffer( mType, mBufferObject );
		qglBufferSubData( mType, mapOffset, requestedSize, offset );
	}

	// mark as used
	mCurrentOffset = std::max( static_cast< GLuint >( mapOffset + requestedSize ), mCurrentOffset );
}

void UniversalGpuBuffer::BindRange( GLuint index, byte *offset, GLuint size ) {
	GLintptr mapOffset = offset - mMapBase;
	assert(mapOffset >= 0 && mapOffset < mSize);
	qglBindBufferRange( mType, index, mBufferObject, mapOffset, size );
}

void UniversalGpuBuffer::Bind() {
	qglBindBuffer(mType, mBufferObject);
}

const void * UniversalGpuBuffer::BufferOffset( const void *offset ) {
	GLintptr mapOffset = static_cast< const byte* >( offset ) - mMapBase;
	assert(mapOffset >= 0 && mapOffset < mSize);
	return reinterpret_cast< const void* >( mapOffset );
}

void UniversalGpuBuffer::LockRange( GLuint offset, GLuint count ) {
	GLsync fence = qglFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
	LockedRange range = { offset, count, fence };
	mRangeLocks.push_back( range );
}

void UniversalGpuBuffer::WaitForLockedRange( GLuint offset, GLuint count ) {
	LockedRange waitRange = { offset, count, 0 };
	for( auto it = mRangeLocks.begin(); it != mRangeLocks.end(); ) {
		if( waitRange.Overlaps( *it ) ) {
			Wait( *it );
			it = mRangeLocks.erase( it );
		} else {
			++it;
		}
	}
}

void UniversalGpuBuffer::Wait( LockedRange &range ) {
	GLenum result = qglClientWaitSync( range.fenceSync, 0, 0 );
	while( result != GL_ALREADY_SIGNALED && result != GL_CONDITION_SATISFIED ) {
		result = qglClientWaitSync( range.fenceSync, GL_SYNC_FLUSH_COMMANDS_BIT, 1000000 );
		if( result == GL_WAIT_FAILED ) {
			assert( !"glClientWaitSync failed" );
			break;
		}
	}
	qglDeleteSync( range.fenceSync );
}
