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

#ifndef __MATH_SIMD_3DNOW_H__
#define __MATH_SIMD_3DNOW_H__

/*
===============================================================================

	3DNow! implementation of idSIMDProcessor

===============================================================================
*/

class idSIMD_3DNow : public idSIMD_MMX {
#if SIMD_USE_ASM
public:
	virtual const char * VPCALL GetName( void ) const;

	virtual void VPCALL Memcpy( void *dst,			const void *src,		const int count );

#endif
};

#endif /* !__MATH_SIMD_3DNOW_H__ */
