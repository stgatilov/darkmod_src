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

#include "UniversalGpuBuffer.h"

class ShaderParamsBuffer {
public:
	void Init();
	void Destroy();

	void Lock() { uniformBuffer.Lock(); }

	template<typename T>
	T *Request( uint32_t count, bool precommit = false ) {
		static_assert( sizeof(T) % 16 == 0,
			"UBO structs must be 16-byte aligned, use padding if necessary. Be sure to obey the std140 layout rules." );
		assert(count <= MaxSupportedParamBufferSize<T>());
		
		T *array = reinterpret_cast<T*>( uniformBuffer.Reserve( sizeof(T) * count, precommit ) );
		return array;
	}

	template<typename T>
	void Commit(T *array, uint32_t count) {
		uniformBuffer.Commit( (byte*)array, sizeof(T) * count );		
	}

	template<typename T>
	void BindRange( GLuint index, T *array, uint32_t count ) {
		uniformBuffer.BindRange( index, (byte*)array, sizeof(T) * count );		
	}

	template<typename T>
	int MaxSupportedParamBufferSize() const {
		// some cards (AMD) don't have any relevant limit on the UBO block size, so limit this to a sensible number as necessary
		return Min(256, maxUniformBlockSize / (int)sizeof(T));
	}

private:
	UniversalGpuBuffer uniformBuffer;
	int maxUniformBlockSize = 0;
};
