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

#ifndef __MATH_SIMD_SSE2_H__
#define __MATH_SIMD_SSE2_H__

/*
===============================================================================

	SSE2 implementation of idSIMDProcessor

===============================================================================
*/

class idSIMD_SSE2 : public idSIMD_SSE {
public:
#if defined(MACOS_X) && defined(__i386__)
	virtual const char * VPCALL GetName( void ) const;
	virtual void VPCALL CmpLT( byte *dst,			const byte bitNum,		const float *src0,		const float constant,	const int count );		

#elif defined(_WIN32)
	virtual const char * VPCALL GetName( void ) const;

	//virtual void VPCALL MatX_LowerTriangularSolve( const idMatX &L, float *x, const float *b, const int n, int skip = 0 );
	//virtual void VPCALL MatX_LowerTriangularSolveTranspose( const idMatX &L, float *x, const float *b, const int n );

	virtual void VPCALL MixedSoundToSamples( short *samples, const float *mixBuffer, const int numSamples );

#endif
};

#endif /* !__MATH_SIMD_SSE2_H__ */
