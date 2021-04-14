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


idCVarBool r_legacyTangents( "r_legacyTangents", "1", CVAR_RENDERER | CVAR_ARCHIVE, "1 = legacy CPU tangent calculation" );

//in "Debug with Inlines" config, optimize all the remaining functions of this file
DEBUG_OPTIMIZE_ON

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


idSIMD_SSE2::idSIMD_SSE2() {
	name = "SSE2";
}

//suitable for any compiler, OS and bitness  (intrinsics)
//generally used on Windows 64-bit and all Linuxes

void NormalizeTangentsLess( idDrawVert* verts, const int numVerts ) {
	for ( int i = 0; i < numVerts; i++ ) {
		idDrawVert& vertex = verts[i];
		__m128 normal = _mm_loadu_ps( &vertex.normal.x );

		DOT_PRODUCT( dotNormal, normal, normal )
			normal = _mm_mul_ps( normal, _mm_rsqrt_ps( dotNormal ) );

		// FIXME somehow check that there's unused space after normal
		_mm_storeu_ps( &verts[i].normal.x, normal );
	}
}

//somewhat slower than ID's code (28.6K vs 21.4K)
void idSIMD_SSE2::NormalizeTangents( idDrawVert *verts, const int numVerts ) {
	if ( !r_legacyTangents ) {
		NormalizeTangentsLess( verts, numVerts );
		return;
	}
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

#define NORMAL_EPS 1e-10f

void DeriveTangentsLess( idPlane* planes, idDrawVert* verts, const int numVerts, const int* indexes, const int numIndexes ) {
	int numTris = numIndexes / 3;
	for ( int i = 0; i < numTris; i++ ) {
		int idxA = indexes[3 * i + 0];
		int idxB = indexes[3 * i + 1];
		int idxC = indexes[3 * i + 2];
		idDrawVert& vA = verts[idxA];
		idDrawVert& vB = verts[idxB];
		idDrawVert& vC = verts[idxC];

		__m128 posA = _mm_loadu_ps( &vA.xyz.x );		//xyzs A
		__m128 posB = _mm_loadu_ps( &vB.xyz.x );		//xyzs B
		__m128 posC = _mm_loadu_ps( &vC.xyz.x );		//xyzs C
		__m128 tA = _mm_load_ss( &vA.st.y );				//t    A
		__m128 tB = _mm_load_ss( &vB.st.y );				//t    B
		__m128 tC = _mm_load_ss( &vC.st.y );				//t    C

		//compute AB/AC differences
		__m128 dpAB = _mm_sub_ps( posB, posA );			//xyzs AB
		__m128 dpAC = _mm_sub_ps( posC, posA );			//xyzs AC
		__m128 dtAB = _mm_sub_ps( tB, tA );					//tttt AB
		__m128 dtAC = _mm_sub_ps( tC, tA );					//tttt AC
		dtAB = _mm_shuffle_ps( dtAB, dtAB, SHUF( 0, 0, 0, 0 ) );
		dtAC = _mm_shuffle_ps( dtAC, dtAC, SHUF( 0, 0, 0, 0 ) );

		//compute normal unit vector
		CROSS_PRODUCT( normal, dpAC, dpAB )
			DOT_PRODUCT( normalSqr, normal, normal );
		normalSqr = _mm_max_ps( normalSqr, _mm_set1_ps( NORMAL_EPS ) );
		normalSqr = _mm_rsqrt_ps( normalSqr );
		normal = _mm_mul_ps( normal, normalSqr );

		//fit plane though point A
		DOT_PRODUCT( planeShift, posA, normal );
		planeShift = _mm_xor_ps( planeShift, _mm_castsi128_ps( _mm_set1_epi32( 0x80000000 ) ) );
		//save plane equation
		static_assert( sizeof( idPlane ) == 16, "Wrong idPlane size" );
		_mm_storeu_ps( planes[i].ToFloatPtr(), normal );
		_mm_store_ss( planes[i].ToFloatPtr() + 3, planeShift );

		//check area sign
		__m128 area = _mm_sub_ps( _mm_mul_ps( dpAB, dtAC ), _mm_mul_ps( dpAC, dtAB ) );
		area = _mm_shuffle_ps( area, area, SHUF( 3, 3, 3, 3 ) );
		__m128 sign = _mm_and_ps( area, _mm_castsi128_ps( _mm_set1_epi32( 0x80000000 ) ) );

		//add computed normal and tangents to endpoints' data (for averaging)
#define ADDV(dst, src) _mm_storeu_ps(dst, _mm_add_ps(_mm_loadu_ps(dst), src));
		ADDV( &vA.normal.x, normal );
		ADDV( &vB.normal.x, normal );
		ADDV( &vC.normal.x, normal );
#undef ADDV
	}
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

	if ( !r_legacyTangents ) {
		DeriveTangentsLess( planes, verts, numVerts, indexes, numIndexes );
		return;
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
	for ( ; (size_t)( src + i ) & 15; i++ )
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
}

void idSIMD_SSE2::Memcpy( void* dst, const void* src, const int count ) {
	if ( ( (size_t)src ^ (size_t)dst ) & 15 ) // FIXME allow SSE2 on differently aligned addresses
		idSIMD_Generic::Memcpy( dst, src, count );
	else
		CopyBufferSSE2( (byte*)dst, (byte*)src, count );
}