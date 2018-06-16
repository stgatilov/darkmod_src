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

//===============================================================
//
//	SSE2 implementation of idSIMDProcessor
//
//===============================================================

/*
============
idSIMD_SSE2::GetName
============
*/
const char * idSIMD_SSE2::GetName( void ) const {
	return "MMX & SSE & SSE2";
}

#if defined(MACOS_X) && defined(__i386__)

#include <xmmintrin.h>

#define SHUFFLEPS( x, y, z, w )		(( (x) & 3 ) << 6 | ( (y) & 3 ) << 4 | ( (z) & 3 ) << 2 | ( (w) & 3 ))
#define R_SHUFFLEPS( x, y, z, w )	(( (w) & 3 ) << 6 | ( (z) & 3 ) << 4 | ( (y) & 3 ) << 2 | ( (x) & 3 ))

/*
============
idSIMD_SSE::CmpLT

  dst[i] |= ( src0[i] < constant ) << bitNum;
============
*/
void VPCALL idSIMD_SSE2::CmpLT( byte *dst, const byte bitNum, const float *src0, const float constant, const int count ) {
	int i, cnt, pre, post;
	float *aligned;
	__m128 xmm0, xmm1;
	__m128i xmm0i;
	int cnt_l;
	char *src0_p;
	char *constant_p;
	char *dst_p;
	int mask_l;
	int dst_l;
	
	/* if the float array is not aligned on a 4 byte boundary */
	if ( ptrdiff_t(src0) & 3 ) {
		/* unaligned memory access */
		pre = 0;
		cnt = count >> 2;
		post = count - (cnt<<2);

	/*
		__asm	mov			edx, cnt													
		__asm	test		edx, edx													
		__asm	je			doneCmp														
	*/
		cnt_l = cnt;
		if(cnt_l != 0) {
	/*
		__asm	push		ebx															
		__asm	neg			edx															
		__asm	mov			esi, src0													
		__asm	prefetchnta	[esi+64]													
		__asm	movss		xmm1, constant												
		__asm	shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 0, 0 )						
		__asm	mov			edi, dst													
		__asm	mov			cl, bitNum	
	*/
			cnt_l = -cnt_l;
			src0_p = (char *) src0;
			_mm_prefetch(src0_p+64, _MM_HINT_NTA);
			constant_p = (char *) &constant;
			xmm1 = _mm_load_ss((float *)constant_p);
			xmm1 = _mm_shuffle_ps(xmm1, xmm1, R_SHUFFLEPS( 0, 0, 0, 0 ));
			dst_p = (char *)dst;
	/*
			__asm loopNA:																	
	*/
			do {
	/*
		__asm	movups		xmm0, [esi]													
		__asm	prefetchnta	[esi+128]													
		__asm	cmpltps		xmm0, xmm1													
		__asm	movmskps	eax, xmm0																												\
		__asm	mov			ah, al														
		__asm	shr			ah, 1														
		__asm	mov			bx, ax														
		__asm	shl			ebx, 14														
		__asm	mov			bx, ax														
		__asm	and			ebx, 0x01010101												
		__asm	shl			ebx, cl														
		__asm	or			ebx, dword ptr [edi]										
		__asm	mov			dword ptr [edi], ebx										
		__asm	add			esi, 16														
		__asm	add			edi, 4														
		__asm	inc			edx															
		__asm	jl			loopNA														
		__asm	pop			ebx		
	*/
				xmm0 = _mm_loadu_ps((float *) src0_p);
				_mm_prefetch(src0_p+128, _MM_HINT_NTA);
				xmm0 = _mm_cmplt_ps(xmm0, xmm1);
				// Simplify using SSE2
				xmm0i = (__m128i) xmm0;
				xmm0i = _mm_packs_epi32(xmm0i, xmm0i);
				xmm0i = _mm_packs_epi16(xmm0i, xmm0i);
				mask_l = _mm_cvtsi128_si32(xmm0i);
				// End 
				mask_l = mask_l &  0x01010101;
				mask_l = mask_l << bitNum;
				dst_l  = *((int *) dst_p);
				mask_l = mask_l | dst_l;
				*((int *) dst_p) = mask_l;
				src0_p = src0_p + 16;
				dst_p = dst_p + 4;
				cnt_l = cnt_l + 1;
			} while (cnt_l < 0);
		}													
	}																					
	else {																				
		/* aligned memory access */														
		aligned = (float *) ((ptrdiff_t(src0) + 15) & ~15);
		if ( ptrdiff_t(aligned) > ptrdiff_t(src0) + count ) {
			pre = count;																
			post = 0;																	
		}																				
		else {																			
			pre = aligned - src0;														
			cnt = (count - pre) >> 2;													
			post = count - pre - (cnt<<2);
	/*
			__asm	mov			edx, cnt												
			__asm	test		edx, edx												
			__asm	je			doneCmp													
	*/
			cnt_l = cnt;
			if(cnt_l != 0) {
	/*
			__asm	push		ebx														
			__asm	neg			edx														
			__asm	mov			esi, aligned											
			__asm	prefetchnta	[esi+64]												
			__asm	movss		xmm1, constant											
			__asm	shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 0, 0 )					
			__asm	mov			edi, dst												
			__asm	add			edi, pre												
			__asm	mov			cl, bitNum
	*/
				cnt_l = -cnt_l;
				src0_p = (char *) src0;
				_mm_prefetch(src0_p+64, _MM_HINT_NTA);
				constant_p = (char *) &constant;
				xmm1 = _mm_load_ss((float *)constant_p);
				xmm1 = _mm_shuffle_ps(xmm1, xmm1, R_SHUFFLEPS( 0, 0, 0, 0 ));
				dst_p = (char *)dst;
				dst_p = dst_p + pre;
	/*
			__asm loopA:																
	*/
			do {
	/*
			__asm	movaps		xmm0, [esi]												
			__asm	prefetchnta	[esi+128]												
			__asm	cmpltps		xmm0, xmm1												
			__asm	movmskps	eax, xmm0																											\
			__asm	mov			ah, al													
			__asm	shr			ah, 1													
			__asm	mov			bx, ax													
			__asm	shl			ebx, 14													
			__asm	mov			bx, ax													
			__asm	and			ebx, 0x01010101											
			__asm	shl			ebx, cl													
			__asm	or			ebx, dword ptr [edi]									
			__asm	mov			dword ptr [edi], ebx									
			__asm	add			esi, 16													
			__asm	add			edi, 4													
			__asm	inc			edx														
			__asm	jl			loopA													
			__asm	pop			ebx
	*/
					xmm0 = _mm_load_ps((float *) src0_p);
					_mm_prefetch(src0_p+128, _MM_HINT_NTA);
					xmm0 = _mm_cmplt_ps(xmm0, xmm1);
					// Simplify using SSE2
					xmm0i = (__m128i) xmm0;
					xmm0i = _mm_packs_epi32(xmm0i, xmm0i);
					xmm0i = _mm_packs_epi16(xmm0i, xmm0i);
					mask_l = _mm_cvtsi128_si32(xmm0i);
					// End 
					mask_l = mask_l &  0x01010101;
					mask_l = mask_l << bitNum;
					dst_l  = *((int *) dst_p);
					mask_l = mask_l | dst_l;
					*((int *) dst_p) = mask_l;
					src0_p = src0_p + 16;
					dst_p = dst_p + 4;
					cnt_l = cnt_l + 1;
				} while (cnt_l < 0);
			}	
		}
	}	
	/*
	doneCmp:																			
	*/
	float c = constant;																	
	for ( i = 0; i < pre; i++ ) {														
		dst[i] |= ( src0[i] < c ) << bitNum;											
	}																					
 	for ( i = count - post; i < count; i++ ) {											
		dst[i] |= ( src0[i] < c ) << bitNum;											
	}
}

#elif SIMD_USE_ASM

#include <xmmintrin.h>

#define SHUFFLEPS( x, y, z, w )		(( (x) & 3 ) << 6 | ( (y) & 3 ) << 4 | ( (z) & 3 ) << 2 | ( (w) & 3 ))
#define R_SHUFFLEPS( x, y, z, w )	(( (w) & 3 ) << 6 | ( (z) & 3 ) << 4 | ( (y) & 3 ) << 2 | ( (x) & 3 ))
#define SHUFFLEPD( x, y )			(( (x) & 1 ) << 1 | ( (y) & 1 ))
#define R_SHUFFLEPD( x, y )			(( (y) & 1 ) << 1 | ( (x) & 1 ))


#define ALIGN4_INIT1( X, INIT )				ALIGN16( static X[4] ) = { INIT, INIT, INIT, INIT }
#define ALIGN4_INIT4( X, I0, I1, I2, I3 )	ALIGN16( static X[4] ) = { I0, I1, I2, I3 }
#define ALIGN8_INIT1( X, INIT )				ALIGN16( static X[8] ) = { INIT, INIT, INIT, INIT, INIT, INIT, INIT, INIT }

ALIGN8_INIT1( unsigned short SIMD_W_zero, 0 );
ALIGN8_INIT1( unsigned short SIMD_W_maxShort, 1<<15 );

ALIGN4_INIT4( unsigned int SIMD_SP_singleSignBitMask, (unsigned int) ( 1 << 31 ), 0, 0, 0 );
ALIGN4_INIT1( unsigned int SIMD_SP_signBitMask, (unsigned int) ( 1 << 31 ) );
ALIGN4_INIT1( unsigned int SIMD_SP_absMask, (unsigned int) ~( 1 << 31 ) );
ALIGN4_INIT1( unsigned int SIMD_SP_infinityMask, (unsigned int) ~( 1 << 23 ) );

ALIGN4_INIT1( float SIMD_SP_zero, 0.0f );
ALIGN4_INIT1( float SIMD_SP_one, 1.0f );
ALIGN4_INIT1( float SIMD_SP_two, 2.0f );
ALIGN4_INIT1( float SIMD_SP_three, 3.0f );
ALIGN4_INIT1( float SIMD_SP_four, 4.0f );
ALIGN4_INIT1( float SIMD_SP_maxShort, (1<<15) );
ALIGN4_INIT1( float SIMD_SP_tiny, 1e-10f );
ALIGN4_INIT1( float SIMD_SP_PI, idMath::PI );
ALIGN4_INIT1( float SIMD_SP_halfPI, idMath::HALF_PI );
ALIGN4_INIT1( float SIMD_SP_twoPI, idMath::TWO_PI );
ALIGN4_INIT1( float SIMD_SP_oneOverTwoPI, 1.0f / idMath::TWO_PI );
ALIGN4_INIT1( float SIMD_SP_infinity, idMath::INFINITY );


#if 0		// the SSE2 code is ungodly slow

/*
============
idSIMD_SSE2::MatX_LowerTriangularSolve

  solves x in Lx = b for the n * n sub-matrix of L
  if skip > 0 the first skip elements of x are assumed to be valid already
  L has to be a lower triangular matrix with (implicit) ones on the diagonal
  x == b is allowed
============
*/
void VPCALL idSIMD_SSE2::MatX_LowerTriangularSolve( const idMatX &L, float *x, const float *b, const int n, int skip ) {
	int nc;
	const float *lptr;

	if ( skip >= n ) {
		return;
	}

	lptr = L[skip];
	nc = L.GetNumColumns();

	// unrolled cases for n < 8
	if ( n < 8 ) {
		#define NSKIP( n, s )	((n<<3)|(s&7))
		switch( NSKIP( n, skip ) ) {
			case NSKIP( 1, 0 ): x[0] = b[0];
				return;
			case NSKIP( 2, 0 ): x[0] = b[0];
			case NSKIP( 2, 1 ): x[1] = b[1] - lptr[1*nc+0] * x[0];
				return;
			case NSKIP( 3, 0 ): x[0] = b[0];
			case NSKIP( 3, 1 ): x[1] = b[1] - lptr[1*nc+0] * x[0];
			case NSKIP( 3, 2 ): x[2] = b[2] - lptr[2*nc+0] * x[0] - lptr[2*nc+1] * x[1];
				return;
			case NSKIP( 4, 0 ): x[0] = b[0];
			case NSKIP( 4, 1 ): x[1] = b[1] - lptr[1*nc+0] * x[0];
			case NSKIP( 4, 2 ): x[2] = b[2] - lptr[2*nc+0] * x[0] - lptr[2*nc+1] * x[1];
			case NSKIP( 4, 3 ): x[3] = b[3] - lptr[3*nc+0] * x[0] - lptr[3*nc+1] * x[1] - lptr[3*nc+2] * x[2];
				return;
			case NSKIP( 5, 0 ): x[0] = b[0];
			case NSKIP( 5, 1 ): x[1] = b[1] - lptr[1*nc+0] * x[0];
			case NSKIP( 5, 2 ): x[2] = b[2] - lptr[2*nc+0] * x[0] - lptr[2*nc+1] * x[1];
			case NSKIP( 5, 3 ): x[3] = b[3] - lptr[3*nc+0] * x[0] - lptr[3*nc+1] * x[1] - lptr[3*nc+2] * x[2];
			case NSKIP( 5, 4 ): x[4] = b[4] - lptr[4*nc+0] * x[0] - lptr[4*nc+1] * x[1] - lptr[4*nc+2] * x[2] - lptr[4*nc+3] * x[3];
				return;
			case NSKIP( 6, 0 ): x[0] = b[0];
			case NSKIP( 6, 1 ): x[1] = b[1] - lptr[1*nc+0] * x[0];
			case NSKIP( 6, 2 ): x[2] = b[2] - lptr[2*nc+0] * x[0] - lptr[2*nc+1] * x[1];
			case NSKIP( 6, 3 ): x[3] = b[3] - lptr[3*nc+0] * x[0] - lptr[3*nc+1] * x[1] - lptr[3*nc+2] * x[2];
			case NSKIP( 6, 4 ): x[4] = b[4] - lptr[4*nc+0] * x[0] - lptr[4*nc+1] * x[1] - lptr[4*nc+2] * x[2] - lptr[4*nc+3] * x[3];
			case NSKIP( 6, 5 ): x[5] = b[5] - lptr[5*nc+0] * x[0] - lptr[5*nc+1] * x[1] - lptr[5*nc+2] * x[2] - lptr[5*nc+3] * x[3] - lptr[5*nc+4] * x[4];
				return;
			case NSKIP( 7, 0 ): x[0] = b[0];
			case NSKIP( 7, 1 ): x[1] = b[1] - lptr[1*nc+0] * x[0];
			case NSKIP( 7, 2 ): x[2] = b[2] - lptr[2*nc+0] * x[0] - lptr[2*nc+1] * x[1];
			case NSKIP( 7, 3 ): x[3] = b[3] - lptr[3*nc+0] * x[0] - lptr[3*nc+1] * x[1] - lptr[3*nc+2] * x[2];
			case NSKIP( 7, 4 ): x[4] = b[4] - lptr[4*nc+0] * x[0] - lptr[4*nc+1] * x[1] - lptr[4*nc+2] * x[2] - lptr[4*nc+3] * x[3];
			case NSKIP( 7, 5 ): x[5] = b[5] - lptr[5*nc+0] * x[0] - lptr[5*nc+1] * x[1] - lptr[5*nc+2] * x[2] - lptr[5*nc+3] * x[3] - lptr[5*nc+4] * x[4];
			case NSKIP( 7, 6 ): x[6] = b[6] - lptr[6*nc+0] * x[0] - lptr[6*nc+1] * x[1] - lptr[6*nc+2] * x[2] - lptr[6*nc+3] * x[3] - lptr[6*nc+4] * x[4] - lptr[6*nc+5] * x[5];
				return;
		}
		return;
	}

	// process first 4 rows
	switch( skip ) {
		case 0: x[0] = b[0];
		case 1: x[1] = b[1] - lptr[1*nc+0] * x[0];
		case 2: x[2] = b[2] - lptr[2*nc+0] * x[0] - lptr[2*nc+1] * x[1];
		case 3: x[3] = b[3] - lptr[3*nc+0] * x[0] - lptr[3*nc+1] * x[1] - lptr[3*nc+2] * x[2];
				skip = 4;
	}

	lptr = L[skip];

	__asm {
		push		ebx
		mov			eax, skip				// eax = i
		shl			eax, 2					// eax = i*4
		mov			edx, n					// edx = n
		shl			edx, 2					// edx = n*4
		mov			esi, x					// esi = x
		mov			edi, lptr				// edi = lptr
		add			esi, eax
		add			edi, eax
		mov			ebx, b					// ebx = b
		// aligned
	looprow:
		mov			ecx, eax
		neg			ecx
		cvtps2pd	xmm0, [esi+ecx]
		cvtps2pd	xmm2, [edi+ecx]
		mulpd		xmm0, xmm2
		cvtps2pd	xmm1, [esi+ecx+8]
		cvtps2pd	xmm3, [edi+ecx+8]
		mulpd		xmm1, xmm3
		add			ecx, 20*4
		jg			donedot16
	dot16:
		cvtps2pd	xmm2, [esi+ecx-(16*4)]
		cvtps2pd	xmm3, [edi+ecx-(16*4)]
		cvtps2pd	xmm4, [esi+ecx-(14*4)]
		mulpd		xmm2, xmm3
		cvtps2pd	xmm5, [edi+ecx-(14*4)]
		addpd		xmm0, xmm2
		cvtps2pd	xmm2, [esi+ecx-(12*4)]
		mulpd		xmm4, xmm5
		cvtps2pd	xmm3, [edi+ecx-(12*4)]
		addpd		xmm1, xmm4
		cvtps2pd	xmm4, [esi+ecx-(10*4)]
		mulpd		xmm2, xmm3
		cvtps2pd	xmm5, [edi+ecx-(10*4)]
		addpd		xmm0, xmm2
		cvtps2pd	xmm2, [esi+ecx-(8*4)]
		mulpd		xmm4, xmm5
		cvtps2pd	xmm3, [edi+ecx-(8*4)]
		addpd		xmm1, xmm4
		cvtps2pd	xmm4, [esi+ecx-(6*4)]
		mulpd		xmm2, xmm3
		cvtps2pd	xmm5, [edi+ecx-(6*4)]
		addpd		xmm0, xmm2
		cvtps2pd	xmm2, [esi+ecx-(4*4)]
		mulpd		xmm4, xmm5
		cvtps2pd	xmm3, [edi+ecx-(4*4)]
		addpd		xmm1, xmm4
		cvtps2pd	xmm4, [esi+ecx-(2*4)]
		mulpd		xmm2, xmm3
		cvtps2pd	xmm5, [edi+ecx-(2*4)]
		addpd		xmm0, xmm2
		add			ecx, 16*4
		mulpd		xmm4, xmm5
		addpd		xmm1, xmm4
		jle			dot16
	donedot16:
		sub			ecx, 8*4
		jg			donedot8
	dot8:
		cvtps2pd	xmm2, [esi+ecx-(8*4)]
		cvtps2pd	xmm3, [edi+ecx-(8*4)]
		cvtps2pd	xmm7, [esi+ecx-(6*4)]
		mulpd		xmm2, xmm3
		cvtps2pd	xmm5, [edi+ecx-(6*4)]
		addpd		xmm0, xmm2
		cvtps2pd	xmm6, [esi+ecx-(4*4)]
		mulpd		xmm7, xmm5
		cvtps2pd	xmm3, [edi+ecx-(4*4)]
		addpd		xmm1, xmm7
		cvtps2pd	xmm4, [esi+ecx-(2*4)]
		mulpd		xmm6, xmm3
		cvtps2pd	xmm7, [edi+ecx-(2*4)]
		addpd		xmm0, xmm6
		add			ecx, 8*4
		mulpd		xmm4, xmm7
		addpd		xmm1, xmm4
	donedot8:
		sub			ecx, 4*4
		jg			donedot4
	dot4:
		cvtps2pd	xmm2, [esi+ecx-(4*4)]
		cvtps2pd	xmm3, [edi+ecx-(4*4)]
		cvtps2pd	xmm4, [esi+ecx-(2*4)]
		mulpd		xmm2, xmm3
		cvtps2pd	xmm5, [edi+ecx-(2*4)]
		addpd		xmm0, xmm2
		add			ecx, 4*4
		mulpd		xmm4, xmm5
		addpd		xmm1, xmm4
	donedot4:
		addpd		xmm0, xmm1
		movaps		xmm1, xmm0
		shufpd		xmm1, xmm1, R_SHUFFLEPD( 1, 0 )
		addsd		xmm0, xmm1
		sub			ecx, 4*4
		jz			dot0
		add			ecx, 4
		jz			dot1
		add			ecx, 4
		jz			dot2
	//dot3:
		cvtss2sd	xmm1, [esi-(3*4)]
		cvtss2sd	xmm2, [edi-(3*4)]
		mulsd		xmm1, xmm2
		addsd		xmm0, xmm1
	dot2:
		cvtss2sd	xmm3, [esi-(2*4)]
		cvtss2sd	xmm4, [edi-(2*4)]
		mulsd		xmm3, xmm4
		addsd		xmm0, xmm3
	dot1:
		cvtss2sd	xmm5, [esi-(1*4)]
		cvtss2sd	xmm6, [edi-(1*4)]
		mulsd		xmm5, xmm6
		addsd		xmm0, xmm5
	dot0:
		cvtss2sd	xmm1, [ebx+eax]
		subsd		xmm1, xmm0
		cvtsd2ss	xmm0, xmm1
		movss		[esi], xmm0
		add			eax, 4
		cmp			eax, edx
		jge			done
		add			esi, 4
		mov			ecx, nc
		shl			ecx, 2
		add			edi, ecx
		add			edi, 4
		jmp			looprow
		// done
	done:
		pop			ebx
	}
}

/*
============
idSIMD_SSE2::MatX_LowerTriangularSolveTranspose

  solves x in L'x = b for the n * n sub-matrix of L
  L has to be a lower triangular matrix with (implicit) ones on the diagonal
  x == b is allowed
============
*/
void VPCALL idSIMD_SSE2::MatX_LowerTriangularSolveTranspose( const idMatX &L, float *x, const float *b, const int n ) {
	int nc;
	const float *lptr;

	lptr = L.ToFloatPtr();
	nc = L.GetNumColumns();

	// unrolled cases for n < 8
	if ( n < 8 ) {
		switch( n ) {
			case 0:
				return;
			case 1:
				x[0] = b[0];
				return;
			case 2:
				x[1] = b[1];
				x[0] = b[0] - lptr[1*nc+0] * x[1];
				return;
			case 3:
				x[2] = b[2];
				x[1] = b[1] - lptr[2*nc+1] * x[2];
				x[0] = b[0] - lptr[2*nc+0] * x[2] - lptr[1*nc+0] * x[1];
				return;
			case 4:
				x[3] = b[3];
				x[2] = b[2] - lptr[3*nc+2] * x[3];
				x[1] = b[1] - lptr[3*nc+1] * x[3] - lptr[2*nc+1] * x[2];
				x[0] = b[0] - lptr[3*nc+0] * x[3] - lptr[2*nc+0] * x[2] - lptr[1*nc+0] * x[1];
				return;
			case 5:
				x[4] = b[4];
				x[3] = b[3] - lptr[4*nc+3] * x[4];
				x[2] = b[2] - lptr[4*nc+2] * x[4] - lptr[3*nc+2] * x[3];
				x[1] = b[1] - lptr[4*nc+1] * x[4] - lptr[3*nc+1] * x[3] - lptr[2*nc+1] * x[2];
				x[0] = b[0] - lptr[4*nc+0] * x[4] - lptr[3*nc+0] * x[3] - lptr[2*nc+0] * x[2] - lptr[1*nc+0] * x[1];
				return;
			case 6:
				x[5] = b[5];
				x[4] = b[4] - lptr[5*nc+4] * x[5];
				x[3] = b[3] - lptr[5*nc+3] * x[5] - lptr[4*nc+3] * x[4];
				x[2] = b[2] - lptr[5*nc+2] * x[5] - lptr[4*nc+2] * x[4] - lptr[3*nc+2] * x[3];
				x[1] = b[1] - lptr[5*nc+1] * x[5] - lptr[4*nc+1] * x[4] - lptr[3*nc+1] * x[3] - lptr[2*nc+1] * x[2];
				x[0] = b[0] - lptr[5*nc+0] * x[5] - lptr[4*nc+0] * x[4] - lptr[3*nc+0] * x[3] - lptr[2*nc+0] * x[2] - lptr[1*nc+0] * x[1];
				return;
			case 7:
				x[6] = b[6];
				x[5] = b[5] - lptr[6*nc+5] * x[6];
				x[4] = b[4] - lptr[6*nc+4] * x[6] - lptr[5*nc+4] * x[5];
				x[3] = b[3] - lptr[6*nc+3] * x[6] - lptr[5*nc+3] * x[5] - lptr[4*nc+3] * x[4];
				x[2] = b[2] - lptr[6*nc+2] * x[6] - lptr[5*nc+2] * x[5] - lptr[4*nc+2] * x[4] - lptr[3*nc+2] * x[3];
				x[1] = b[1] - lptr[6*nc+1] * x[6] - lptr[5*nc+1] * x[5] - lptr[4*nc+1] * x[4] - lptr[3*nc+1] * x[3] - lptr[2*nc+1] * x[2];
				x[0] = b[0] - lptr[6*nc+0] * x[6] - lptr[5*nc+0] * x[5] - lptr[4*nc+0] * x[4] - lptr[3*nc+0] * x[3] - lptr[2*nc+0] * x[2] - lptr[1*nc+0] * x[1];
				return;
		}
		return;
	}

	int i, j, m;
	float *xptr;
	double s0;

	// if the number of columns is not a multiple of 2 we're screwed for alignment.
	// however, if the number of columns is a multiple of 2 but the number of to be
	// processed rows is not a multiple of 2 we can still run 8 byte aligned
	m = n;
	if ( m & 1 ) {
		m--;
		x[m] = b[m];

		lptr = L[m] + m - 4;
		xptr = x + m;
		__asm {
			push		ebx
			mov			eax, m					// eax = i
			mov			esi, xptr				// esi = xptr
			mov			edi, lptr				// edi = lptr
			mov			ebx, b					// ebx = b
			mov			edx, nc					// edx = nc*sizeof(float)
			shl			edx, 2
		process4rows_1:
			cvtps2pd	xmm0, [ebx+eax*4-16]	// load b[i-2], b[i-1]
			cvtps2pd	xmm2, [ebx+eax*4-8]		// load b[i-4], b[i-3]
			xor			ecx, ecx
			sub			eax, m
			neg			eax
			jz			done4x4_1
		process4x4_1:	// process 4x4 blocks
			cvtps2pd	xmm3, [edi]
			cvtps2pd	xmm4, [edi+8]
			add			edi, edx
			cvtss2sd	xmm5, [esi+4*ecx+0]
			shufpd		xmm5, xmm5, R_SHUFFLEPD( 0, 0 )
			mulpd		xmm3, xmm5
			cvtps2pd	xmm1, [edi]
			mulpd		xmm4, xmm5
			cvtps2pd	xmm6, [edi+8]
			subpd		xmm0, xmm3
			subpd		xmm2, xmm4
			add			edi, edx
			cvtss2sd	xmm7, [esi+4*ecx+4]
			shufpd		xmm7, xmm7, R_SHUFFLEPD( 0, 0 )
			mulpd		xmm1, xmm7
			cvtps2pd	xmm3, [edi]
			mulpd		xmm6, xmm7
			cvtps2pd	xmm4, [edi+8]
			subpd		xmm0, xmm1
			subpd		xmm2, xmm6
			add			edi, edx
			cvtss2sd	xmm5, [esi+4*ecx+8]
			shufpd		xmm5, xmm5, R_SHUFFLEPD( 0, 0 )
			mulpd		xmm3, xmm5
			cvtps2pd	xmm1, [edi]
			mulpd		xmm4, xmm5
			cvtps2pd	xmm6, [edi+8]
			subpd		xmm0, xmm3
			subpd		xmm2, xmm4
			add			edi, edx
			cvtss2sd	xmm7, [esi+4*ecx+12]
			shufpd		xmm7, xmm7, R_SHUFFLEPD( 0, 0 )
			mulpd		xmm1, xmm7
			add			ecx, 4
			mulpd		xmm6, xmm7
			cmp			ecx, eax
			subpd		xmm0, xmm1
			subpd		xmm2, xmm6
			jl			process4x4_1
		done4x4_1:		// process left over of the 4 rows
			cvtps2pd	xmm3, [edi]
			cvtps2pd	xmm4, [edi+8]
			cvtss2sd	xmm5, [esi+4*ecx]
			shufpd		xmm5, xmm5, R_SHUFFLEPD( 0, 0 )
			mulpd		xmm3, xmm5
			mulpd		xmm4, xmm5
			subpd		xmm0, xmm3
			subpd		xmm2, xmm4
			imul		ecx, edx
			sub			edi, ecx
			neg			eax

			add			eax, m
			sub			eax, 4
			movapd		xmm1, xmm0
			shufpd		xmm1, xmm1, R_SHUFFLEPD( 1, 1 )
			movapd		xmm3, xmm2
			shufpd		xmm3, xmm3, R_SHUFFLEPD( 1, 1 )
			sub			edi, edx
			cvtsd2ss	xmm7, xmm3
			movss		[esi-4], xmm7			// xptr[-1] = s3
			movsd		xmm4, xmm3
			movsd		xmm5, xmm3
			cvtss2sd	xmm7, [edi+8]
			mulsd		xmm3, xmm7				// lptr[-1*nc+2] * s3
			cvtss2sd	xmm7, [edi+4]
			mulsd		xmm4, xmm7				// lptr[-1*nc+1] * s3
			cvtss2sd	xmm7, [edi]
			mulsd		xmm5, xmm7				// lptr[-1*nc+0] * s3
			subsd		xmm2, xmm3
			cvtsd2ss	xmm7, xmm2
			movss		[esi-8], xmm7			// xptr[-2] = s2
			movsd		xmm6, xmm2
			sub			edi, edx
			subsd		xmm0, xmm5
			subsd		xmm1, xmm4
			cvtss2sd	xmm7, [edi+4]
			mulsd		xmm2, xmm7				// lptr[-2*nc+1] * s2
			cvtss2sd	xmm7, [edi]
			mulsd		xmm6, xmm7				// lptr[-2*nc+0] * s2
			subsd		xmm1, xmm2
			cvtsd2ss	xmm7, xmm1
			movss		[esi-12], xmm7			// xptr[-3] = s1
			subsd		xmm0, xmm6
			sub			edi, edx
			cmp			eax, 4
			cvtss2sd	xmm7, [edi]
			mulsd		xmm1, xmm7				// lptr[-3*nc+0] * s1
			subsd		xmm0, xmm1
			cvtsd2ss	xmm7, xmm0
			movss		[esi-16], xmm7			// xptr[-4] = s0
			jl			done4rows_1
			sub			edi, edx
			sub			edi, 16
			sub			esi, 16
			jmp			process4rows_1
		done4rows_1:
			pop			ebx
		}
	}
	else {
		lptr = L.ToFloatPtr() + m * L.GetNumColumns() + m - 4;
		xptr = x + m;
		__asm {
			push		ebx
			mov			eax, m					// eax = i
			mov			esi, xptr				// esi = xptr
			mov			edi, lptr				// edi = lptr
			mov			ebx, b					// ebx = b
			mov			edx, nc					// edx = nc*sizeof(float)
			shl			edx, 2
		process4rows:
			cvtps2pd	xmm0, [ebx+eax*4-16]	// load b[i-2], b[i-1]
			cvtps2pd	xmm2, [ebx+eax*4-8]		// load b[i-4], b[i-3]
			sub			eax, m
			jz			done4x4
			neg			eax
			xor			ecx, ecx
		process4x4:		// process 4x4 blocks
			cvtps2pd	xmm3, [edi]
			cvtps2pd	xmm4, [edi+8]
			add			edi, edx
			cvtss2sd	xmm5, [esi+4*ecx+0]
			shufpd		xmm5, xmm5, R_SHUFFLEPD( 0, 0 )
			mulpd		xmm3, xmm5
			cvtps2pd	xmm1, [edi]
			mulpd		xmm4, xmm5
			cvtps2pd	xmm6, [edi+8]
			subpd		xmm0, xmm3
			subpd		xmm2, xmm4
			add			edi, edx
			cvtss2sd	xmm7, [esi+4*ecx+4]
			shufpd		xmm7, xmm7, R_SHUFFLEPD( 0, 0 )
			mulpd		xmm1, xmm7
			cvtps2pd	xmm3, [edi]
			mulpd		xmm6, xmm7
			cvtps2pd	xmm4, [edi+8]
			subpd		xmm0, xmm1
			subpd		xmm2, xmm6
			add			edi, edx
			cvtss2sd	xmm5, [esi+4*ecx+8]
			shufpd		xmm5, xmm5, R_SHUFFLEPD( 0, 0 )
			mulpd		xmm3, xmm5
			cvtps2pd	xmm1, [edi]
			mulpd		xmm4, xmm5
			cvtps2pd	xmm6, [edi+8]
			subpd		xmm0, xmm3
			subpd		xmm2, xmm4
			add			edi, edx
			cvtss2sd	xmm7, [esi+4*ecx+12]
			shufpd		xmm7, xmm7, R_SHUFFLEPD( 0, 0 )
			mulpd		xmm1, xmm7
			add			ecx, 4
			mulpd		xmm6, xmm7
			cmp			ecx, eax
			subpd		xmm0, xmm1
			subpd		xmm2, xmm6
			jl			process4x4
			imul		ecx, edx
			sub			edi, ecx
			neg			eax
		done4x4:		// process left over of the 4 rows
			add			eax, m
			sub			eax, 4
			movapd		xmm1, xmm0
			shufpd		xmm1, xmm1, R_SHUFFLEPD( 1, 1 )
			movapd		xmm3, xmm2
			shufpd		xmm3, xmm3, R_SHUFFLEPD( 1, 1 )
			sub			edi, edx
			cvtsd2ss	xmm7, xmm3
			movss		[esi-4], xmm7			// xptr[-1] = s3
			movsd		xmm4, xmm3
			movsd		xmm5, xmm3
			cvtss2sd	xmm7, [edi+8]
			mulsd		xmm3, xmm7				// lptr[-1*nc+2] * s3
			cvtss2sd	xmm7, [edi+4]
			mulsd		xmm4, xmm7				// lptr[-1*nc+1] * s3
			cvtss2sd	xmm7, [edi]
			mulsd		xmm5, xmm7				// lptr[-1*nc+0] * s3
			subsd		xmm2, xmm3
			cvtsd2ss	xmm7, xmm2
			movss		[esi-8], xmm7			// xptr[-2] = s2
			movsd		xmm6, xmm2
			sub			edi, edx
			subsd		xmm0, xmm5
			subsd		xmm1, xmm4
			cvtss2sd	xmm7, [edi+4]
			mulsd		xmm2, xmm7				// lptr[-2*nc+1] * s2
			cvtss2sd	xmm7, [edi]
			mulsd		xmm6, xmm7				// lptr[-2*nc+0] * s2
			subsd		xmm1, xmm2
			cvtsd2ss	xmm7, xmm1
			movss		[esi-12], xmm7			// xptr[-3] = s1
			subsd		xmm0, xmm6
			sub			edi, edx
			cmp			eax, 4
			cvtss2sd	xmm7, [edi]
			mulsd		xmm1, xmm7				// lptr[-3*nc+0] * s1
			subsd		xmm0, xmm1
			cvtsd2ss	xmm7, xmm0
			movss		[esi-16], xmm7			// xptr[-4] = s0
			jl			done4rows
			sub			edi, edx
			sub			edi, 16
			sub			esi, 16
			jmp			process4rows
		done4rows:
			pop			ebx
		}
	}

	// process left over rows
	for ( i = (m&3)-1; i >= 0; i-- ) {
		s0 = b[i];
		lptr = L[i+1] + i;
		for ( j = i + 1; j < m; j++ ) {
			s0 -= lptr[0] * x[j];
			lptr += nc;
		}
		x[i] = s0;
	}
}

#endif

/*
============
idSIMD_SSE2::MixedSoundToSamples
============
*/
void VPCALL idSIMD_SSE2::MixedSoundToSamples( short *samples, const float *mixBuffer, const int numSamples ) {

	assert( ( numSamples % MIXBUFFER_SAMPLES ) == 0 );

	__asm {

		mov			eax, numSamples
		mov			edi, mixBuffer
		mov			esi, samples
		shl			eax, 2
		add			edi, eax
		neg			eax

	loop16:

		movaps		xmm0, [edi+eax+0*16]
		movaps		xmm1, [edi+eax+1*16]
		movaps		xmm2, [edi+eax+2*16]
		movaps		xmm3, [edi+eax+3*16]

		add			esi, 4*4*2

		cvtps2dq	xmm4, xmm0
		cvtps2dq	xmm5, xmm1
		cvtps2dq	xmm6, xmm2
		cvtps2dq	xmm7, xmm3

		prefetchnta	[edi+eax+128]

		packssdw	xmm4, xmm5
		packssdw	xmm6, xmm7

		add			eax, 4*16

		movlps		[esi-4*4*2], xmm4		// FIXME: should not use movlps/movhps to move integer data
		movhps		[esi-3*4*2], xmm4
		movlps		[esi-2*4*2], xmm6
		movhps		[esi-1*4*2], xmm6

		jl			loop16
	}
}

#else /* SIMD_USE_ASM */

//suitable for any compiler, OS and bitness  (intrinsics)
//generally used on Windows 64-bit and all Linuxes

#define OFFSETOF(s, m) offsetof(s, m)
#define SHUF(i0, i1, i2, i3) _MM_SHUFFLE(i3, i2, i1, i0)

#define DOT_PRODUCT(xyz, a, b) \
	__m128 xyz = _mm_mul_ps(a, b); \
	{ \
		__m128 yzx = _mm_shuffle_ps(xyz, xyz, SHUF(1, 2, 0, 3)); \
		__m128 zxy = _mm_shuffle_ps(xyz, xyz, SHUF(2, 0, 1, 3)); \
		xyz = _mm_add_ps(_mm_add_ps(xyz, yzx), zxy); \
	}
#define CROSS_PRODUCT(dst, a, b) \
	__m128 dst = _mm_mul_ps(a, _mm_shuffle_ps(b, b, SHUF(1, 2, 0, 3))); \
	dst = _mm_sub_ps(dst, _mm_mul_ps(b, _mm_shuffle_ps(a, a, SHUF(1, 2, 0, 3)))); \
	dst = _mm_shuffle_ps(dst, dst, SHUF(1, 2, 0, 3));


//somewhat slower than ID's code (28.6K vs 21.4K)
void idSIMD_SSE2::NormalizeTangents( idDrawVert *verts, const int numVerts ) {
	for (int i = 0; i < numVerts; i++) {
		idDrawVert &vertex = verts[i];
		__m128 normal = _mm_loadu_ps(&vertex.normal.x);
		__m128 tangU  = _mm_loadu_ps(&vertex.tangents[0].x);
		__m128 tangV  = _mm_loadu_ps(&vertex.tangents[1].x);

		DOT_PRODUCT(dotNormal, normal, normal)
		normal = _mm_mul_ps(normal, _mm_rsqrt_ps(dotNormal));

		DOT_PRODUCT(dotTangU, tangU, normal)
		tangU = _mm_sub_ps(tangU, _mm_mul_ps(normal, dotTangU));
		DOT_PRODUCT(dotTangV, tangV, normal)
		tangV = _mm_sub_ps(tangV, _mm_mul_ps(normal, dotTangV));

		DOT_PRODUCT(sqlenTangU, tangU, tangU);
		tangU = _mm_mul_ps(tangU, _mm_rsqrt_ps(sqlenTangU));
		DOT_PRODUCT(sqlenTangV, tangV, tangV);
		tangV = _mm_mul_ps(tangV, _mm_rsqrt_ps(sqlenTangV));

		static_assert(OFFSETOF(idDrawVert, normal) + sizeof(vertex.normal) == OFFSETOF(idDrawVert, tangents), "Bad members offsets");
		static_assert(sizeof(vertex.tangents) == 24, "Bad members offsets");
		//note: we do overlapping stores here (protected by static asserts)
		_mm_storeu_ps(&verts[i].normal.x, normal);
		_mm_storeu_ps(&verts[i].tangents[0].x, tangU);
		//last store is tricky (must not overwrite)
		_mm_store_sd((double*)&verts[i].tangents[1].x, _mm_castps_pd(tangV));
		_mm_store_ss(&verts[i].tangents[1].z, _mm_movehl_ps(tangV, tangV));
	}
}


//note: this version is slower than ID's original code in testSIMD benchmark (10K vs 5K)
//however, the ID's code contains branches, which are highly predictable in the benchmark (and not so predictable in real workload)
//this version is branchless: it does not suffer from branch mispredictions, so it won't slow down even on long random inputs
void idSIMD_SSE2::TransformVerts( idDrawVert *verts, const int numVerts, const idJointMat *joints, const idVec4 *weights, const int *index, int numWeights ) {
	const byte *jointsPtr = (byte *)joints;

	int i = 0;
	__m128 sum = _mm_setzero_ps();

	for (int j = 0; j < numWeights; j++) {
		int offset = index[j*2], isLast = index[j*2+1];
		const idJointMat &matrix = *(idJointMat *) (jointsPtr + offset);
		const idVec4 &weight = weights[j];

		__m128 wgt = _mm_load_ps(&weight.x);
		__m128 mulX = _mm_mul_ps(_mm_load_ps(matrix.ToFloatPtr() + 0), wgt);		//x0as
		__m128 mulY = _mm_mul_ps(_mm_load_ps(matrix.ToFloatPtr() + 4), wgt);		//y1bt
		__m128 mulZ = _mm_mul_ps(_mm_load_ps(matrix.ToFloatPtr() + 8), wgt);		//z2cr

		//transpose 3 x 4 matrix
		__m128 xy01 = _mm_unpacklo_ps(mulX, mulY);
		__m128 abst = _mm_unpackhi_ps(mulX, mulY);
		__m128 Vxyz = _mm_shuffle_ps(xy01, mulZ, _MM_SHUFFLE(0, 0, 1, 0));
		__m128 V012 = _mm_shuffle_ps(xy01, mulZ, _MM_SHUFFLE(1, 1, 3, 2));
		__m128 Vabc = _mm_shuffle_ps(abst, mulZ, _MM_SHUFFLE(2, 2, 1, 0));
		__m128 Vstr = _mm_shuffle_ps(abst, mulZ, _MM_SHUFFLE(3, 3, 3, 2));

		__m128 res = _mm_add_ps(_mm_add_ps(Vxyz, V012), _mm_add_ps(Vabc, Vstr));
		sum = _mm_add_ps(sum, res);

		//note: branchless version here
		//current sum is always stored to memory, but pointer does not always move
		_mm_store_sd((double*)&verts[i].xyz.x, _mm_castps_pd(sum));
		_mm_store_ss(&verts[i].xyz.z, _mm_movehl_ps(sum, sum));
		i += isLast;
		//zero current sum if last
		__m128 mask = _mm_castsi128_ps(_mm_cvtsi32_si128(isLast - 1));	//empty mask <=> last
		mask = _mm_shuffle_ps(mask, mask, _MM_SHUFFLE(0, 0, 0, 0));
		sum = _mm_and_ps(sum, mask);
	}
}


template<class Lambda> static ID_INLINE void VertexMinMax( idVec3 &min, idVec3 &max, const idDrawVert *src, const int count, Lambda Index ) {
	__m128 rmin = _mm_set1_ps( 1e30f);
	__m128 rmax = _mm_set1_ps(-1e30f);
	int i = 0;
	for (; i < (count & (~3)); i += 4) {
		__m128 pos0 = _mm_loadu_ps(&src[Index(i + 0)].xyz.x);
		__m128 pos1 = _mm_loadu_ps(&src[Index(i + 1)].xyz.x);
		__m128 pos2 = _mm_loadu_ps(&src[Index(i + 2)].xyz.x);
		__m128 pos3 = _mm_loadu_ps(&src[Index(i + 3)].xyz.x);
		__m128 min01 = _mm_min_ps(pos0, pos1);
		__m128 max01 = _mm_max_ps(pos0, pos1);
		__m128 min23 = _mm_min_ps(pos2, pos3);
		__m128 max23 = _mm_max_ps(pos2, pos3);
		__m128 minA = _mm_min_ps(min01, min23);
		__m128 maxA = _mm_max_ps(max01, max23);
		rmin = _mm_min_ps(rmin, minA);
		rmax = _mm_max_ps(rmax, maxA);
	}
	if (i + 0 < count) {
		__m128 pos = _mm_loadu_ps(&src[Index(i + 0)].xyz.x);
		rmin = _mm_min_ps(rmin, pos);
		rmax = _mm_max_ps(rmax, pos);
		if (i + 1 < count) {
			pos = _mm_loadu_ps(&src[Index(i + 1)].xyz.x);
			rmin = _mm_min_ps(rmin, pos);
			rmax = _mm_max_ps(rmax, pos);
			if (i + 2 < count) {
				pos = _mm_loadu_ps(&src[Index(i + 2)].xyz.x);
				rmin = _mm_min_ps(rmin, pos);
				rmax = _mm_max_ps(rmax, pos);
			}
		}
	}
	_mm_store_sd((double*)&min.x, _mm_castps_pd(rmin));
	_mm_store_ss(&min.z, _mm_movehl_ps(rmin, rmin));
	_mm_store_sd((double*)&max.x, _mm_castps_pd(rmax));
	_mm_store_ss(&max.z, _mm_movehl_ps(rmax, rmax));
}
void idSIMD_SSE2::MinMax( idVec3 &min, idVec3 &max, const idDrawVert *src, const int count ) {
	VertexMinMax(min, max, src, count, [](int i) { return i; });
}
void idSIMD_SSE2::MinMax( idVec3 &min, idVec3 &max, const idDrawVert *src, const int *indexes, const int count ) {
	VertexMinMax(min, max, src, count, [indexes](int i) { return indexes[i]; });
}


//this thing is significantly faster that ID's original code (50K vs 87K)
//one major difference is: this version zeroes all resulting vectors in preprocessing step
//this allows to write completely branchless code
//ID' original version has many flaws, namely:
//  1. branches for singular cases
//  2. storing data is done with scalar C++ code
//  3. branches for detecting: store or add
void idSIMD_SSE2::DeriveTangents( idPlane *planes, idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes ) {
	int numTris = numIndexes / 3;
#define NORMAL_EPS 1e-10f

	//note that idDrawVerts must have normal & tangents tightly packed, going on after the other
	static_assert(OFFSETOF(idDrawVert, normal) + sizeof(verts->normal) == OFFSETOF(idDrawVert, tangents), "Bad members offsets");
	static_assert(sizeof(verts->tangents) == 24, "Bad members offsets");
	for (int i = 0; i < numVerts; i++) {
		float *ptr = &verts[i].normal.x;
		_mm_storeu_ps(ptr, _mm_setzero_ps());
		_mm_storeu_ps(ptr + 4, _mm_setzero_ps());
		_mm_store_ss(ptr + 8, _mm_setzero_ps());
	}

	for (int i = 0; i < numTris; i++) {
		int idxA = indexes[3 * i + 0];
		int idxB = indexes[3 * i + 1];
		int idxC = indexes[3 * i + 2];
		idDrawVert &vA = verts[idxA];
		idDrawVert &vB = verts[idxB];
		idDrawVert &vC = verts[idxC];

		__m128 posA = _mm_loadu_ps(&vA.xyz.x);		//xyzs A
		__m128 posB = _mm_loadu_ps(&vB.xyz.x);		//xyzs B
		__m128 posC = _mm_loadu_ps(&vC.xyz.x);		//xyzs C
		__m128 tA = _mm_load_ss(&vA.st.y);				//t    A
		__m128 tB = _mm_load_ss(&vB.st.y);				//t    B
		__m128 tC = _mm_load_ss(&vC.st.y);				//t    C

		//compute AB/AC differences
		__m128 dpAB = _mm_sub_ps(posB, posA);			//xyzs AB
		__m128 dpAC = _mm_sub_ps(posC, posA);			//xyzs AC
		__m128 dtAB = _mm_sub_ps(tB, tA);					//tttt AB
		__m128 dtAC = _mm_sub_ps(tC, tA);					//tttt AC
		dtAB = _mm_shuffle_ps(dtAB, dtAB, SHUF(0, 0, 0, 0));
		dtAC = _mm_shuffle_ps(dtAC, dtAC, SHUF(0, 0, 0, 0));

		//compute normal unit vector
		CROSS_PRODUCT(normal, dpAC, dpAB)
		DOT_PRODUCT(normalSqr, normal, normal);
		normalSqr = _mm_max_ps(normalSqr, _mm_set1_ps(NORMAL_EPS));
		normalSqr = _mm_rsqrt_ps(normalSqr);
		normal = _mm_mul_ps(normal, normalSqr);

		//fit plane though point A
		DOT_PRODUCT(planeShift, posA, normal);
		planeShift = _mm_xor_ps(planeShift, _mm_castsi128_ps(_mm_set1_epi32(0x80000000)));
		//save plane equation
		static_assert(sizeof(idPlane) == 16, "Wrong idPlane size");
		_mm_storeu_ps(planes[i].ToFloatPtr(), normal);
		_mm_store_ss(planes[i].ToFloatPtr() + 3, planeShift);

		//check area sign
		__m128 area = _mm_sub_ps(_mm_mul_ps(dpAB, dtAC), _mm_mul_ps(dpAC, dtAB));
		area = _mm_shuffle_ps(area, area, SHUF(3, 3, 3, 3));
		__m128 sign = _mm_and_ps(area, _mm_castsi128_ps(_mm_set1_epi32(0x80000000)));

		//compute first tangent
		__m128 tangentU = _mm_sub_ps(
			_mm_mul_ps(dpAB, dtAC),
			_mm_mul_ps(dpAC, dtAB)
		);
		DOT_PRODUCT(tangUlen, tangentU, tangentU)
		tangUlen = _mm_max_ps(tangUlen, _mm_set1_ps(NORMAL_EPS));
		tangUlen = _mm_rsqrt_ps(tangUlen);
		tangUlen = _mm_xor_ps(tangUlen, sign);
		tangentU = _mm_mul_ps(tangentU, tangUlen);

		//compute second tangent
		__m128 tangentV = _mm_sub_ps(
			_mm_mul_ps(dpAC, _mm_shuffle_ps(dpAB, dpAB, SHUF(3, 3, 3, 3))),
			_mm_mul_ps(dpAB, _mm_shuffle_ps(dpAC, dpAC, SHUF(3, 3, 3, 3)))
		);
		DOT_PRODUCT(tangVlen, tangentV, tangentV)
		tangVlen = _mm_max_ps(tangVlen, _mm_set1_ps(NORMAL_EPS));
		tangVlen = _mm_rsqrt_ps(tangVlen);
		tangVlen = _mm_xor_ps(tangVlen, sign);
		tangentV = _mm_mul_ps(tangentV, tangVlen);

		//pack normal and tangents into 9 values
		__m128 pack0 = _mm_xor_ps(normal, _mm_castsi128_ps(_mm_slli_si128(_mm_castps_si128(tangentU), 12)));
		__m128 pack1 = _mm_shuffle_ps(tangentU, tangentV, SHUF(1, 2, 0, 1));
		__m128 pack2 = _mm_movehl_ps(tangentV, tangentV);

		//add computed normal and tangents to endpoints' data (for averaging)
		#define ADDV(dst, src) _mm_storeu_ps(dst, _mm_add_ps(_mm_loadu_ps(dst), src));
		#define ADDS(dst, src) _mm_store_ss (dst, _mm_add_ss(_mm_load_ss (dst), src));
		ADDV(&vA.normal.x, pack0);
		ADDV(&vA.tangents[0].y, pack1);
		ADDS(&vA.tangents[1].z, pack2);
		ADDV(&vB.normal.x, pack0);
		ADDV(&vB.tangents[0].y, pack1);
		ADDS(&vB.tangents[1].z, pack2);
		ADDV(&vC.normal.x, pack0);
		ADDV(&vC.tangents[0].y, pack1);
		ADDS(&vC.tangents[1].z, pack2);
		#undef ADDV
		#undef ADDS
	}
}

#endif /* SIMD_USE_ASM */
