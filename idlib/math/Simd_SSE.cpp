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
#pragma hdrstop
#pragma warning(disable: 4740)

#include "Simd_SSE.h"


idSIMD_SSE::idSIMD_SSE() {
	name = "SSE";
}

#ifdef ENABLE_SSE_PROCESSORS

#include <xmmintrin.h>

/*
============
idSIMD_SSE::CullByFrustum
============
*/
void idSIMD_SSE::CullByFrustum( idDrawVert *verts, const int numVerts, const idPlane frustum[6], byte *pointCull, float epsilon ) {
	__m128 fA14 = _mm_set_ps( frustum[3][0], frustum[2][0], frustum[1][0], frustum[0][0] );
	__m128 fA56 = _mm_set_ps( 0, 0, frustum[5][0], frustum[4][0] );
	__m128 fB14 = _mm_set_ps( frustum[3][1], frustum[2][1], frustum[1][1], frustum[0][1] );
	__m128 fB56 = _mm_set_ps( 0, 0, frustum[5][1], frustum[4][1] );
	__m128 fC14 = _mm_set_ps( frustum[3][2], frustum[2][2], frustum[1][2], frustum[0][2] );
	__m128 fC56 = _mm_set_ps( 0, 0, frustum[5][2], frustum[4][2] );
	__m128 fD14 = _mm_set_ps( frustum[3][3], frustum[2][3], frustum[1][3], frustum[0][3] );
	__m128 fD56 = _mm_set_ps( 0, 0, frustum[5][3], frustum[4][3] );
	for ( int j = 0; j < numVerts; j++ ) {
		auto &vec = verts[j].xyz;
		__m128 vX = _mm_set1_ps( vec.x );
		__m128 vY = _mm_set1_ps( vec.y );
		__m128 vZ = _mm_set1_ps( vec.z );
		__m128 d14 = _mm_add_ps(
			_mm_add_ps(
				_mm_mul_ps( fA14, vX ),
				_mm_mul_ps( fB14, vY )
			),
			_mm_add_ps(
				_mm_mul_ps( fC14, vZ ),
				fD14
			)
		);
		__m128 d56 = _mm_add_ps(
			_mm_add_ps(
				_mm_mul_ps( fA56, vX ),
				_mm_mul_ps( fB56, vY )
			),
			_mm_add_ps(
				_mm_mul_ps( fC56, vZ ),
				fD56
			)
		);
		const short mask6 = (1 << 6) - 1;
		__m128 eps = _mm_set1_ps( epsilon );
		int mask_lo14 = _mm_movemask_ps( _mm_cmplt_ps( d14, eps ) );
		int mask_lo56 = _mm_movemask_ps( _mm_cmplt_ps( d56, eps ) );
		int mask_lo = mask_lo14 | mask_lo56 << 4;
		pointCull[j] = mask_lo & mask6;
	}
}

/*
============
idSIMD_SSE::CullByFrustum2
============
*/
void idSIMD_SSE::CullByFrustum2( idDrawVert *verts, const int numVerts, const idPlane frustum[6], unsigned short *pointCull, float epsilon ) {
	__m128 fA14 = _mm_set_ps( frustum[3][0], frustum[2][0], frustum[1][0], frustum[0][0] );
	__m128 fA56 = _mm_set_ps( 0, 0, frustum[5][0], frustum[4][0] );
	__m128 fB14 = _mm_set_ps( frustum[3][1], frustum[2][1], frustum[1][1], frustum[0][1] );
	__m128 fB56 = _mm_set_ps( 0, 0, frustum[5][1], frustum[4][1] );
	__m128 fC14 = _mm_set_ps( frustum[3][2], frustum[2][2], frustum[1][2], frustum[0][2] );
	__m128 fC56 = _mm_set_ps( 0, 0, frustum[5][2], frustum[4][2] );
	__m128 fD14 = _mm_set_ps( frustum[3][3], frustum[2][3], frustum[1][3], frustum[0][3] );
	__m128 fD56 = _mm_set_ps( 0, 0, frustum[5][3], frustum[4][3] );
	const __m128 eps  = _mm_set1_ps(  epsilon );
	const __m128 epsM = _mm_set1_ps( -epsilon );
	for ( int j = 0; j < numVerts; j++ ) {
		auto &vec = verts[j].xyz;
		__m128 vX = _mm_set1_ps( vec.x );
		__m128 vY = _mm_set1_ps( vec.y );
		__m128 vZ = _mm_set1_ps( vec.z );
		__m128 d14 = _mm_add_ps(
			_mm_add_ps(
				_mm_mul_ps( fA14, vX ),
				_mm_mul_ps( fB14, vY )
			),
			_mm_add_ps(
				_mm_mul_ps( fC14, vZ ),
				fD14
			)
		);
		__m128 d56 = _mm_add_ps(
			_mm_add_ps(
				_mm_mul_ps( fA56, vX ),
				_mm_mul_ps( fB56, vY )
			),
			_mm_add_ps(
				_mm_mul_ps( fC56, vZ ),
				fD56
			)
		);
		const short mask6 = (1 << 6) - 1;
		int mask_lo14 = _mm_movemask_ps( _mm_cmplt_ps( d14, eps  ) );
		int mask_lo56 = _mm_movemask_ps( _mm_cmplt_ps( d56, eps  ) );
		int mask_hi14 = _mm_movemask_ps( _mm_cmpgt_ps( d14, epsM ) );
		int mask_hi56 = _mm_movemask_ps( _mm_cmpgt_ps( d56, epsM ) );
		int mask_lo = mask_lo14 | mask_lo56 << 4;
		int mask_hi = mask_hi14 | mask_hi56 << 4;
		pointCull[j] = mask_lo & mask6 | (mask_hi & mask6) << 6;
	}
}

#endif
