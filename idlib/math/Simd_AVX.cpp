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
	extern idCVar r_ignore;
	if ( !r_ignore.GetBool() ) { // hidden under this cvar for now
		return idSIMD_Generic::CullByFrustum( verts, numVerts, frustum, pointCull, epsilon );
	}
	float frustumData[32]; // TODO replace with _mm256_set_ps
	for ( int i = 0; i < 6; i++ ) {
		auto f = frustum[i].ToFloatPtr();
		frustumData[i] = f[0];
		frustumData[i + 8] = f[1];
		frustumData[i + 16] = f[2];
		frustumData[i + 24] = f[3];
	}
	__m256 fA = _mm256_load_ps( frustum[0].ToFloatPtr() );
	__m256 fB = _mm256_load_ps( &frustumData[8] );
	__m256 fC = _mm256_load_ps( &frustumData[16] );
	__m256 fD = _mm256_load_ps( &frustumData[24] );
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
		//_mm256_storeu_ps( distances, d );
		const short mask6 = (1 << 6) - 1;
		__m256 eps = _mm256_set1_ps( epsilon );
		int mask_lo = _mm256_movemask_ps( _mm256_cmp_ps( d, eps, _CMP_LT_OQ ) );
		eps = _mm256_set1_ps( -epsilon );
		int mask_hi = _mm256_movemask_ps( _mm256_cmp_ps( d, eps, _CMP_GT_OQ ) );
		pointCull[j] = mask_lo & mask6 | (mask_hi & mask6) << 6;
	}
}


