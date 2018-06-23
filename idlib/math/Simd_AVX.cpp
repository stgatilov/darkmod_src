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

#include "Simd_Generic.h"
#include "Simd_MMX.h"
#include "Simd_SSE.h"
#include "Simd_SSE2.h"
#include "Simd_SSE3.h"
#include "Simd_AVX.h"
#include <immintrin.h>

//===============================================================
//
//	AVX implementation of idSIMDProcessor
//
//===============================================================

/*
============
idSIMD_AVX::GetName
============
*/
const char * idSIMD_AVX::GetName( void ) const {
	return "MMX & SSE & SSE2 & SSE3 & AVX";
}

/*
============
idSIMD_AVX::GetName
============
*/
void VPCALL idSIMD_AVX::CullByFrustum( idDrawVert *verts, const int numVerts, const idPlane frustum[6], unsigned short int *pointCull, float epsilon ) {
	idCVar com_tempAllowAVX( "com_tempAllowAVX", "0", CVAR_SYSTEM, "to be removed before release" );
	if ( !com_tempAllowAVX.GetBool() ) { // hidden under this cvar for now
		return idSIMD_Generic::CullByFrustum( verts, numVerts, frustum, pointCull, epsilon );
	}
	const idVec4 &f0 = frustum[0].ToVec4(), &f1 = frustum[1].ToVec4(), &f2 = frustum[2].ToVec4(), 
		&f3 = frustum[3].ToVec4(), &f4 = frustum[4].ToVec4(), &f5 = frustum[5].ToVec4();
	__m256 fA = _mm256_set_ps( 0, 0, f5.x, f4.x, f3.x, f2.x, f1.x, f0.x );
	__m256 fB = _mm256_set_ps( 0, 0, f5.y, f4.y, f3.y, f2.y, f1.y, f0.y );
	__m256 fC = _mm256_set_ps( 0, 0, f5.z, f4.z, f3.z, f2.z, f1.z, f0.z );
	__m256 fD = _mm256_set_ps( 0, 0, f5.w, f4.w, f3.w, f2.w, f1.w, f0.w);
	for ( int j = 0; j < numVerts; j++ ) {
		auto &vec = verts[j].xyz;
		__m256 vX = _mm256_set1_ps( vec.x );
		__m256 vY = _mm256_set1_ps( vec.y );
		__m256 vZ = _mm256_set1_ps( vec.z );
		__m256 d = _mm256_add_ps(
			_mm256_add_ps(
				_mm256_mul_ps( fA, vX ),
				_mm256_mul_ps( fB, vY )
			),
			_mm256_add_ps(
				_mm256_mul_ps( fC, vZ ),
				fD
			)
		);
		const short mask6 = (1 << 6) - 1;
		__m256 eps = _mm256_set1_ps( epsilon );
		int mask_lo = _mm256_movemask_ps( _mm256_cmp_ps( d, eps, _CMP_LT_OQ ) );
		eps = _mm256_set1_ps( -epsilon );
		int mask_hi = _mm256_movemask_ps( _mm256_cmp_ps( d, eps, _CMP_GT_OQ ) );
		pointCull[j] = mask_lo & mask6 | (mask_hi & mask6) << 6;
	}
}
