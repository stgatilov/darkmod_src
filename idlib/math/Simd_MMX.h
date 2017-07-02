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

#ifndef __MATH_SIMD_MMX_H__
#define __MATH_SIMD_MMX_H__

/*
===============================================================================

	MMX implementation of idSIMDProcessor

===============================================================================
*/

class idSIMD_MMX : public idSIMD_Generic {
public:
	virtual const char * VPCALL GetName( void ) const;
#if defined(MACOS_X) && defined(__i386__)
#elif SIMD_USE_ASM
	virtual void VPCALL Memcpy( void *dst,			const void *src,		const int count );
	virtual void VPCALL Memset( void *dst,			const int val,			const int count );
#endif
};

#endif /* !__MATH_SIMD_MMX_H__ */
