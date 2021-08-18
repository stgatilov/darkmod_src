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

#include "Simd_SSE2.h"


idSIMD_SSE2::idSIMD_SSE2() {
	name = "SSE2";
}

#ifdef ENABLE_SSE_PROCESSORS

//in "Debug with Inlines" config, optimize all the remaining functions of this file
DEBUG_OPTIMIZE_ON

#if defined(_MSVC) && defined(_DEBUG) && !defined(_INLINEDEBUG)
	//assert only checked in "Debug" build of MSVC
	#define DBG_ASSERT(cond) assert(cond)
#else
	#define DBG_ASSERT(cond) ((void)0)
#endif

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


//suitable for any compiler, OS and bitness  (intrinsics)
//generally used on Windows 64-bit and all Linuxes

//somewhat slower than ID's code (28.6K vs 21.4K)
void idSIMD_SSE2::NormalizeTangents( idDrawVert *verts, const int numVerts ) {
	//in all vector normalizations, W component is can be zero (division by zero)
	//we have to mask any exceptions here
	idIgnoreFpExceptions guardFpExceptions;

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
	//idMD5Mesh::CalcBounds calls this with uninitialized texcoords
	//we have to mask any exceptions here
	idIgnoreFpExceptions guardFpExceptions;

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

int idSIMD_SSE2::CreateVertexProgramShadowCache( idVec4 *shadowVerts, const idDrawVert *verts, const int numVerts ) {
	__m128 xyzMask = _mm_castsi128_ps(_mm_setr_epi32(-1, -1, -1, 0));
	__m128 oneW = _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0);
	for ( int i = 0; i < numVerts; i++ ) {
		const float *v = verts[i].xyz.ToFloatPtr();
		__m128 vec = _mm_loadu_ps(v);
		vec = _mm_and_ps(vec, xyzMask);
		_mm_storeu_ps(&shadowVerts[i*2+0].x, _mm_xor_ps(vec, oneW));
		_mm_storeu_ps(&shadowVerts[i*2+1].x, vec);
	}
	return numVerts * 2;
}

void idSIMD_SSE2::TracePointCull( byte *cullBits, byte &totalOr, const float radius, const idPlane *planes, const idDrawVert *verts, const int numVerts ) {
	__m128 pA = _mm_loadu_ps(planes[0].ToFloatPtr());
	__m128 pB = _mm_loadu_ps(planes[1].ToFloatPtr());
	__m128 pC = _mm_loadu_ps(planes[2].ToFloatPtr());
	__m128 pD = _mm_loadu_ps(planes[3].ToFloatPtr());
	_MM_TRANSPOSE4_PS(pA, pB, pC, pD);

	__m128 radP = _mm_set1_ps( radius);
	__m128 radM = _mm_set1_ps(-radius);

	size_t orAll = 0;
	for ( int i = 0; i < numVerts; i++ ) {
		__m128 xyzs = _mm_loadu_ps(&verts[i].xyz.x);
		__m128 vX = _mm_shuffle_ps(xyzs, xyzs, SHUF(0, 0, 0, 0));
		__m128 vY = _mm_shuffle_ps(xyzs, xyzs, SHUF(1, 1, 1, 1));
		__m128 vZ = _mm_shuffle_ps(xyzs, xyzs, SHUF(2, 2, 2, 2));
		__m128 dist = _mm_add_ps(
			_mm_add_ps(_mm_mul_ps(pA, vX), _mm_mul_ps(pB, vY)), 
			_mm_add_ps(_mm_mul_ps(pC, vZ), pD)
		);
		size_t lower = _mm_movemask_ps(_mm_cmpgt_ps(dist, radM));
		size_t upper = _mm_movemask_ps(_mm_cmplt_ps(dist, radP));
		size_t mask = lower + (upper << 4);
		cullBits[i] = mask;
		orAll |= mask;
	}

	totalOr = orAll;
}

void idSIMD_SSE2::CalcTriFacing( const idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes, const idVec3 &lightOrigin, byte *facing ) {
	int numTris = numIndexes / 3;
	__m128 orig = _mm_setr_ps(lightOrigin.x, lightOrigin.y, lightOrigin.z, 0.0f);

	for (int i = 0; i < numTris; i++) {
		int idxA = indexes[3 * i + 0];
		int idxB = indexes[3 * i + 1];
		int idxC = indexes[3 * i + 2];
		const idDrawVert &vA = verts[idxA];
		const idDrawVert &vB = verts[idxB];
		const idDrawVert &vC = verts[idxC];

		__m128 posA = _mm_loadu_ps( &vA.xyz.x );		//xyzs A
		__m128 posB = _mm_loadu_ps( &vB.xyz.x );		//xyzs B
		__m128 posC = _mm_loadu_ps( &vC.xyz.x );		//xyzs C
		//compute AB/AC differences
		__m128 dpAB = _mm_sub_ps( posB, posA );			//xyzs AB
		__m128 dpAC = _mm_sub_ps( posC, posA );			//xyzs AC
		//compute normal vector (length can be arbitrary)
		CROSS_PRODUCT( normal, dpAC, dpAB );

		//get orientation
		__m128 vertToOrig = _mm_sub_ps(orig, posA);
		DOT_PRODUCT( signedVolume, vertToOrig, normal );
		__m128 oriPositive = _mm_cmple_ss(_mm_setzero_ps(), signedVolume);
		int num = _mm_cvtsi128_si32(_mm_castps_si128(oriPositive));

		facing[i] = -num;		//-1 when true, 0 when false
	}
}

void CopyBufferSSE2( byte* dst, const byte* src, int numBytes ) {
	typedef unsigned int uint32;
	int i = 0;
	for ( ; i < numBytes && (size_t(src + i) & 15); i++ )
		dst[i] = src[i];
	for ( ; i + 128 <= numBytes; i += 128 ) {
		__m128i d0 = _mm_load_si128( ( __m128i* ) & src[i + 0 * 16] );
		__m128i d1 = _mm_load_si128( ( __m128i* ) & src[i + 1 * 16] );
		__m128i d2 = _mm_load_si128( ( __m128i* ) & src[i + 2 * 16] );
		__m128i d3 = _mm_load_si128( ( __m128i* ) & src[i + 3 * 16] );
		__m128i d4 = _mm_load_si128( ( __m128i* ) & src[i + 4 * 16] );
		__m128i d5 = _mm_load_si128( ( __m128i* ) & src[i + 5 * 16] );
		__m128i d6 = _mm_load_si128( ( __m128i* ) & src[i + 6 * 16] );
		__m128i d7 = _mm_load_si128( ( __m128i* ) & src[i + 7 * 16] );
		_mm_stream_si128( ( __m128i* ) & dst[i + 0 * 16], d0 );
		_mm_stream_si128( ( __m128i* ) & dst[i + 1 * 16], d1 );
		_mm_stream_si128( ( __m128i* ) & dst[i + 2 * 16], d2 );
		_mm_stream_si128( ( __m128i* ) & dst[i + 3 * 16], d3 );
		_mm_stream_si128( ( __m128i* ) & dst[i + 4 * 16], d4 );
		_mm_stream_si128( ( __m128i* ) & dst[i + 5 * 16], d5 );
		_mm_stream_si128( ( __m128i* ) & dst[i + 6 * 16], d6 );
		_mm_stream_si128( ( __m128i* ) & dst[i + 7 * 16], d7 );
	}
	for ( ; i + 16 <= numBytes; i += 16 ) {
		__m128i d = _mm_load_si128( ( __m128i* ) & src[i] );
		_mm_stream_si128( ( __m128i* ) & dst[i], d );
	}
	for ( ; i + 4 <= numBytes; i += 4 ) {
		*(uint32*)& dst[i] = *(const uint32*)& src[i];
	}
	for ( ; i < numBytes; i++ ) {
		dst[i] = src[i];
	}
	_mm_sfence();
	assert(i == numBytes);
}

void idSIMD_SSE2::Memcpy( void* dst, const void* src, const int count ) {
	if ( ( (size_t)src ^ (size_t)dst ) & 15 ) // FIXME allow SSE2 on differently aligned addresses
		idSIMD_Generic::Memcpy( dst, src, count );
	else
		CopyBufferSSE2( (byte*)dst, (byte*)src, count );
}

/*
============
idSIMD_SSE2::GenerateMipMap2x2
============
*/
void idSIMD_SSE2::GenerateMipMap2x2( const byte *srcPtr, int srcStride, int halfWidth, int halfHeight, byte *dstPtr, int dstStride ) {
	for (int i = 0; i < halfHeight; i++) {
		const byte *inRow0 = &srcPtr[(2*i+0) * srcStride];
		const byte *inRow1 = &srcPtr[(2*i+1) * srcStride];
		byte *outRow = &dstPtr[i * dstStride];

		int j;
		for (j = 0; j + 4 <= halfWidth; j += 4) {
			__m128i A0 = _mm_loadu_si128((__m128i*)(inRow0 + 8*j + 0));
			__m128i A1 = _mm_loadu_si128((__m128i*)(inRow0 + 8*j + 16));
			__m128i B0 = _mm_loadu_si128((__m128i*)(inRow1 + 8*j + 0));
			__m128i B1 = _mm_loadu_si128((__m128i*)(inRow1 + 8*j + 16));

			__m128i A0shuf = _mm_shuffle_epi32(A0, SHUF(0, 2, 1, 3));
			__m128i A1shuf = _mm_shuffle_epi32(A1, SHUF(0, 2, 1, 3));
			__m128i B0shuf = _mm_shuffle_epi32(B0, SHUF(0, 2, 1, 3));
			__m128i B1shuf = _mm_shuffle_epi32(B1, SHUF(0, 2, 1, 3));
			__m128i A0l = _mm_unpacklo_epi8(A0shuf, _mm_setzero_si128());
			__m128i A0r = _mm_unpackhi_epi8(A0shuf, _mm_setzero_si128());
			__m128i A1l = _mm_unpacklo_epi8(A1shuf, _mm_setzero_si128());
			__m128i A1r = _mm_unpackhi_epi8(A1shuf, _mm_setzero_si128());
			__m128i B0l = _mm_unpacklo_epi8(B0shuf, _mm_setzero_si128());
			__m128i B0r = _mm_unpackhi_epi8(B0shuf, _mm_setzero_si128());
			__m128i B1l = _mm_unpacklo_epi8(B1shuf, _mm_setzero_si128());
			__m128i B1r = _mm_unpackhi_epi8(B1shuf, _mm_setzero_si128());

			__m128i sum0 = _mm_add_epi16(_mm_add_epi16(A0l, A0r), _mm_add_epi16(B0l, B0r));
			__m128i sum1 = _mm_add_epi16(_mm_add_epi16(A1l, A1r), _mm_add_epi16(B1l, B1r));
			__m128i avg0 = _mm_srli_epi16(_mm_add_epi16(sum0, _mm_set1_epi16(2)), 2);
			__m128i avg1 = _mm_srli_epi16(_mm_add_epi16(sum1, _mm_set1_epi16(2)), 2);

			__m128i res = _mm_packus_epi16(avg0, avg1);
			_mm_storeu_si128((__m128i*)(outRow + 4*j), res);
		}

		for (; j < halfWidth; j++) {
			unsigned sum0 = (unsigned)inRow0[8*j+0] + inRow0[8*j+4+0] + inRow1[8*j+0] + inRow1[8*j+4+0];
			unsigned sum1 = (unsigned)inRow0[8*j+1] + inRow0[8*j+4+1] + inRow1[8*j+1] + inRow1[8*j+4+1];
			unsigned sum2 = (unsigned)inRow0[8*j+2] + inRow0[8*j+4+2] + inRow1[8*j+2] + inRow1[8*j+4+2];
			unsigned sum3 = (unsigned)inRow0[8*j+3] + inRow0[8*j+4+3] + inRow1[8*j+3] + inRow1[8*j+4+3];
			outRow[4*j+0] = (sum0 + 2) >> 2;
			outRow[4*j+1] = (sum1 + 2) >> 2;
			outRow[4*j+2] = (sum2 + 2) >> 2;
			outRow[4*j+3] = (sum3 + 2) >> 2;
		}
	}
}

static void CompressRGTCFromRGBA8_Kernel8x4( const byte *srcPtr, int stride, byte *dstPtr ) {
	// Load rows and remove blue/alpha channels
	__m128i rgrg0, rgrg1, rgrg2, rgrg3;
	#define LOADROW(r) {\
		__m128i rgbaBlock0 = _mm_loadu_si128((__m128i*)(srcPtr + r * stride + 0)); \
		__m128i rgbaBlock1 = _mm_loadu_si128((__m128i*)(srcPtr + r * stride + 16)); \
		rgrg##r = _mm_xor_si128( \
			_mm_slli_epi32(rgbaBlock1, 16), \
			_mm_and_si128(rgbaBlock0, _mm_set1_epi32(0xFFFF)) \
		);\
	}
	LOADROW(0)
	LOADROW(1)
	LOADROW(2)
	LOADROW(3)
	#undef LOADROW

	// Compute min/max values
	__m128i minBytes = _mm_min_epu8(_mm_min_epu8(rgrg0, rgrg1), _mm_min_epu8(rgrg2, rgrg3));
	__m128i maxBytes = _mm_max_epu8(_mm_max_epu8(rgrg0, rgrg1), _mm_max_epu8(rgrg2, rgrg3));
	minBytes = _mm_min_epu8(minBytes, _mm_shuffle_epi32(minBytes, SHUF(2, 3, 0, 1)));
	maxBytes = _mm_max_epu8(maxBytes, _mm_shuffle_epi32(maxBytes, SHUF(2, 3, 0, 1)));
	minBytes = _mm_min_epu8(minBytes, _mm_shuffle_epi32(minBytes, SHUF(1, 0, 1, 0)));
	maxBytes = _mm_max_epu8(maxBytes, _mm_shuffle_epi32(maxBytes, SHUF(1, 0, 1, 0)));
	// (each 32-bit element contains min/max in RGRG format)
	for (int i = 0; i < 4; i++)
		DBG_ASSERT(maxBytes.m128i_u8[i] >= minBytes.m128i_u8[i]);

	// Make sure min != max
	__m128i deltaBytes = _mm_sub_epi8(maxBytes, minBytes);
	__m128i maskDeltaZero = _mm_cmpeq_epi8(deltaBytes, _mm_setzero_si128());
	maxBytes = _mm_sub_epi8(maxBytes, maskDeltaZero);
	deltaBytes = _mm_sub_epi8(deltaBytes, maskDeltaZero);
	__m128i maskMaxOverflown = _mm_and_si128(maskDeltaZero, _mm_cmpeq_epi8(maxBytes, _mm_setzero_si128()));
	minBytes = _mm_add_epi8(minBytes, maskMaxOverflown);
	maxBytes = _mm_add_epi8(maxBytes, maskMaxOverflown);
	for (int i = 0; i < 4; i++) {
		DBG_ASSERT(maxBytes.m128i_u8[i] > minBytes.m128i_u8[i]);
		DBG_ASSERT(maxBytes.m128i_u8[i] - minBytes.m128i_u8[i] == deltaBytes.m128i_u8[i]);
	}

	// Prepare multiplier
	__m128i deltaDwords = _mm_unpacklo_epi8(deltaBytes, _mm_setzero_si128());
	deltaDwords = _mm_unpacklo_epi16(deltaDwords, _mm_setzero_si128());
	__m128 deltaFloat = _mm_cvtepi32_ps(deltaDwords);
	__m128 multFloat = _mm_div_ps(_mm_set1_ps(7 << 12), deltaFloat);
	__m128i multWords = _mm_cvttps_epi32(_mm_add_ps(multFloat, _mm_set1_ps(0.999f)));
	multWords = _mm_packs_epi32(multWords, multWords);

	__m128i chunksRow0, chunksRow1, chunksRow2, chunksRow3;
	#define PROCESS_ROW(r) { \
		/* Compute ratio, find closest ramp point */ \
		__m128i numerBytes = _mm_sub_epi8(rgrg##r, minBytes); \
		__m128i numerWords0 = _mm_unpacklo_epi8(numerBytes, _mm_setzero_si128()); \
		__m128i numerWords1 = _mm_unpackhi_epi8(numerBytes, _mm_setzero_si128()); \
		__m128i fixedQuot0 = _mm_mullo_epi16(numerWords0, multWords); \
		__m128i fixedQuot1 = _mm_mullo_epi16(numerWords1, multWords); \
		__m128i idsWords0 = _mm_srli_epi16(_mm_add_epi16(fixedQuot0, _mm_set1_epi16(1 << 11)), 12); \
		__m128i idsWords1 = _mm_srli_epi16(_mm_add_epi16(fixedQuot1, _mm_set1_epi16(1 << 11)), 12); \
		for (int i = 0; i < 8; i++) \
			DBG_ASSERT(idsWords0.m128i_u16[i] <= 7 && idsWords1.m128i_u16[i] <= 7); \
		__m128i idsBytes = _mm_packs_epi16(idsWords0, idsWords1); \
		/* Convert ramp point index to DXT index */ \
		__m128i dxtIdsBytes = _mm_sub_epi8(_mm_set1_epi8(8), idsBytes); \
		dxtIdsBytes = _mm_add_epi8(dxtIdsBytes, _mm_cmpeq_epi8(idsBytes, _mm_set1_epi8(7))); \
		dxtIdsBytes = _mm_sub_epi8(dxtIdsBytes, _mm_and_si128(_mm_cmpeq_epi8(idsBytes, _mm_setzero_si128()), _mm_set1_epi8(7))); \
		for (int i = 0; i < 8; i++) \
			DBG_ASSERT(dxtIdsBytes.m128i_u8[i] <= 7); \
		/* Compress 3-bit indices into row 12-bit chunks */ \
		__m128i temp = dxtIdsBytes; \
		temp = _mm_xor_si128(temp, _mm_srli_epi64(temp, 32 - 3)); \
		__m128i tempLo = _mm_unpacklo_epi8(temp, _mm_setzero_si128()); \
		__m128i tempHi = _mm_unpackhi_epi8(temp, _mm_setzero_si128()); \
		temp = _mm_xor_si128(tempLo, _mm_slli_epi16(tempHi, 6)); \
		temp = _mm_unpacklo_epi16(temp, _mm_setzero_si128()); \
		temp = _mm_and_si128(temp, _mm_set1_epi32((1 << 12) - 1)); \
		for (int i = 0; i < 8; i++) \
			DBG_ASSERT(temp.m128i_u32[i] < (1 << 12)); \
		chunksRow##r = temp; \
	}
	PROCESS_ROW(0)
	PROCESS_ROW(1)
	PROCESS_ROW(2)
	PROCESS_ROW(3)
	#undef PROCESS_ROW

	// Compress 12-bit row chunks into (16+32)-bit block chunks
	__m128i blockLow32 = _mm_xor_si128(_mm_slli_epi32(chunksRow0, 16), _mm_slli_epi32(chunksRow1, 28));
	__m128i blockHigh32 = _mm_xor_si128(_mm_slli_epi32(chunksRow2, 8), _mm_slli_epi32(chunksRow3, 20));
	blockHigh32 = _mm_xor_si128(blockHigh32, _mm_srli_epi32(chunksRow1, 4));
	for (int i = 0; i < 4; i++)
		DBG_ASSERT(blockLow32.m128i_u16[2*i] == 0);
	// Add max/min bytes to first 16 bits
	__m128i maxMinDwords = _mm_unpacklo_epi8(maxBytes, minBytes);
	maxMinDwords = _mm_unpacklo_epi16(maxMinDwords, _mm_setzero_si128());
	blockLow32 = _mm_xor_si128(blockLow32, maxMinDwords);

	// Write out two blocks
	__m128i block0 = _mm_unpacklo_epi32(blockLow32, blockHigh32);
	__m128i block1 = _mm_unpackhi_epi32(blockLow32, blockHigh32);
	_mm_storeu_si128((__m128i*)(dstPtr + 0), block0);
	_mm_storeu_si128((__m128i*)(dstPtr + 16), block1);
}

void idSIMD_SSE2::CompressRGTCFromRGBA8( const byte *srcPtr, int width, int height, int stride, byte *dstPtr ) {
	ALIGNTYPE16 dword inputBlocks[4][8];
	__m128i outputBlocks[2];

	for (int sr = 0; sr < height; sr += 4) {
		int fitsNum = (sr + 4 <= height ? width >> 3 : 0);

		int iters;
		for (iters = 0; iters < fitsNum; iters++) {
			// Load blocks directly from image memory
			CompressRGTCFromRGBA8_Kernel8x4(&srcPtr[sr * stride + 32 * iters], stride, dstPtr);
			dstPtr += 32;
		}

		for (int sc = 8 * iters; sc < width; sc += 8) {
			// Copy blocks with clamp-style padding
			for (int i = 0; i < 4; i++)
				for (int j = 0; j < 8; j++) {
					int a = sr + i;
					int b = sc + j;
					if (a > height - 1)
						a = height - 1;
					if (b > width - 1)
						b = width - 1;
					inputBlocks[i][j] = *(dword*)&srcPtr[a * stride + 4 * b];
				}
			CompressRGTCFromRGBA8_Kernel8x4((byte*)&inputBlocks[0][0], sizeof(inputBlocks[0]), (byte*)outputBlocks);
			// Copy one or two blocks to output
			int numBlocks = idMath::Imin((width - sc + 3) >> 2, 2);
			for (int b = 0; b < numBlocks; b++) {
				_mm_storeu_si128((__m128i*)dstPtr, outputBlocks[b]);
				dstPtr += 16;
			}
		}
	}
}

#endif
