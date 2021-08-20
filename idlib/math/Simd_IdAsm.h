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

#include "Simd_SSE2.h"

/*
===============================================================================

	Original ID's inline assembly (up to SSE3) for idSIMDProcessor

===============================================================================
*/

class idSIMD_IdAsm : public idSIMD_SSE2 {
public:
	idSIMD_IdAsm();

//stgatilov: only compile these functions on MSVC 32-bit
#if defined(_MSC_VER) && defined(_M_IX86)

	//========================= Uses SSE =============================

	virtual void Add( float *dst,			const float constant,	const float *src,		const int count );
	virtual void Add( float *dst,			const float *src0,		const float *src1,		const int count );
	virtual void Sub( float *dst,			const float constant,	const float *src,		const int count );
	virtual void Sub( float *dst,			const float *src0,		const float *src1,		const int count );
	virtual void Mul( float *dst,			const float constant,	const float *src,		const int count );
	virtual void Mul( float *dst,			const float *src0,		const float *src1,		const int count );
	virtual void Div( float *dst,			const float constant,	const float *src,		const int count );
	virtual void Div( float *dst,			const float *src0,		const float *src1,		const int count );
	virtual void MulAdd( float *dst,			const float constant,	const float *src,		const int count );
	virtual void MulAdd( float *dst,			const float *src0,		const float *src1,		const int count );
	virtual void MulSub( float *dst,			const float constant,	const float *src,		const int count );
	virtual void MulSub( float *dst,			const float *src0,		const float *src1,		const int count );

	virtual void Dot( float *dst,			const idVec3 &constant,	const idVec3 *src,		const int count );
	virtual void Dot( float *dst,			const idVec3 &constant,	const idPlane *src,		const int count );
	virtual void Dot( float *dst,			const idVec3 &constant,	const idDrawVert *src,	const int count );
	virtual void Dot( float *dst,			const idPlane &constant,const idVec3 *src,		const int count );
	virtual void Dot( float *dst,			const idPlane &constant,const idPlane *src,		const int count );
	virtual void Dot( float *dst,			const idPlane &constant,const idDrawVert *src,	const int count );
	virtual void Dot( float *dst,			const idVec3 *src0,		const idVec3 *src1,		const int count );
	virtual void Dot( float &dot,			const float *src1,		const float *src2,		const int count );

	virtual void CmpGT( byte *dst,			const float *src0,		const float constant,	const int count );
	virtual void CmpGT( byte *dst,			const byte bitNum,		const float *src0,		const float constant,	const int count );
	virtual void CmpGE( byte *dst,			const float *src0,		const float constant,	const int count );
	virtual void CmpGE( byte *dst,			const byte bitNum,		const float *src0,		const float constant,	const int count );
	virtual void CmpLT( byte *dst,			const float *src0,		const float constant,	const int count );
	virtual void CmpLT( byte *dst,			const byte bitNum,		const float *src0,		const float constant,	const int count );
	virtual void CmpLE( byte *dst,			const float *src0,		const float constant,	const int count );
	virtual void CmpLE( byte *dst,			const byte bitNum,		const float *src0,		const float constant,	const int count );

	virtual void MinMax( float &min,			float &max,				const float *src,		const int count );
	virtual	void MinMax( idVec2 &min,		idVec2 &max,			const idVec2 *src,		const int count );
	virtual void MinMax( idVec3 &min,		idVec3 &max,			const idVec3 *src,		const int count );
	virtual	void MinMax( idVec3 &min,		idVec3 &max,			const idDrawVert *src,	const int count );
	virtual	void MinMax( idVec3 &min,		idVec3 &max,			const idDrawVert *src,	const int *indexes,		const int count );

	virtual void Clamp( float *dst,			const float *src,		const float min,		const float max,		const int count );
	virtual void ClampMin( float *dst,		const float *src,		const float min,		const int count );
	virtual void ClampMax( float *dst,		const float *src,		const float max,		const int count );

	virtual void Zero16( float *dst,			const int count );
	virtual void Negate16( float *dst,		const int count );
	virtual void Copy16( float *dst,			const float *src,		const int count );
	virtual void Add16( float *dst,			const float *src1,		const float *src2,		const int count );
	virtual void Sub16( float *dst,			const float *src1,		const float *src2,		const int count );
	virtual void Mul16( float *dst,			const float *src1,		const float constant,	const int count );
	virtual void AddAssign16( float *dst,	const float *src,		const int count );
	virtual void SubAssign16( float *dst,	const float *src,		const int count );
	virtual void MulAssign16( float *dst,	const float constant,	const int count );

	virtual void MatX_MultiplyVecX( idVecX &dst, const idMatX &mat, const idVecX &vec );
	virtual void MatX_MultiplyAddVecX( idVecX &dst, const idMatX &mat, const idVecX &vec );
	virtual void MatX_MultiplySubVecX( idVecX &dst, const idMatX &mat, const idVecX &vec );
	virtual void MatX_TransposeMultiplyVecX( idVecX &dst, const idMatX &mat, const idVecX &vec );
	virtual void MatX_TransposeMultiplyAddVecX( idVecX &dst, const idMatX &mat, const idVecX &vec );
	virtual void MatX_TransposeMultiplySubVecX( idVecX &dst, const idMatX &mat, const idVecX &vec );
	virtual void MatX_MultiplyMatX( idMatX &dst, const idMatX &m1, const idMatX &m2 );
	virtual void MatX_TransposeMultiplyMatX( idMatX &dst, const idMatX &m1, const idMatX &m2 );
	virtual void MatX_LowerTriangularSolve( const idMatX &L, float *x, const float *b, const int n, int skip = 0 );
	virtual void MatX_LowerTriangularSolveTranspose( const idMatX &L, float *x, const float *b, const int n );
	virtual bool MatX_LDLTFactor( idMatX &mat, idVecX &invDiag, const int n );

	virtual void BlendJoints( idJointQuat *joints, const idJointQuat *blendJoints, const float lerp, const int *index, const int numJoints );
	virtual void ConvertJointQuatsToJointMats( idJointMat *jointMats, const idJointQuat *jointQuats, const int numJoints );
	virtual void ConvertJointMatsToJointQuats( idJointQuat *jointQuats, const idJointMat *jointMats, const int numJoints );
	virtual void TransformJoints( idJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint );
	virtual void UntransformJoints( idJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint );
	virtual void TransformVerts_SSE_( idDrawVert *verts, const int numVerts, const idJointMat *joints, const idVec4 *weights, const int *index, const int numWeights );
	virtual void TracePointCull( byte *cullBits, byte &totalOr, const float radius, const idPlane *planes, const idDrawVert *verts, const int numVerts );
	virtual void DecalPointCull( byte *cullBits, const idPlane *planes, const idDrawVert *verts, const int numVerts );
	virtual void OverlayPointCull( byte *cullBits, idVec2 *texCoords, const idPlane *planes, const idDrawVert *verts, const int numVerts );
	virtual void DeriveTriPlanes( idPlane *planes, const idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes );
	virtual void DeriveTangents( idPlane *planes, idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes );
	virtual void DeriveUnsmoothedTangents( idDrawVert *verts, const dominantTri_s *dominantTris, const int numVerts );
	virtual void NormalizeTangents( idDrawVert *verts, const int numVerts );
	virtual int  CreateShadowCache( idVec4 *vertexCache, int *vertRemap, const idVec3 &lightOrigin, const idDrawVert *verts, const int numVerts );
	virtual int  CreateVertexProgramShadowCache( idVec4 *vertexCache, const idDrawVert *verts, const int numVerts );

	virtual void MixSoundTwoSpeakerMono( float *mixBuffer, const float *samples, const int numSamples, const float lastV[2], const float currentV[2] );
	virtual void MixSoundTwoSpeakerStereo( float *mixBuffer, const float *samples, const int numSamples, const float lastV[2], const float currentV[2] );
	virtual void MixSoundSixSpeakerMono( float *mixBuffer, const float *samples, const int numSamples, const float lastV[6], const float currentV[6] );
	virtual void MixSoundSixSpeakerStereo( float *mixBuffer, const float *samples, const int numSamples, const float lastV[6], const float currentV[6] );
	virtual void MixedSoundToSamples_SSE_( short *samples, const float *mixBuffer, const int numSamples );

	//========================= Uses SSE2 =============================

	//virtual void MatX_LowerTriangularSolve( const idMatX &L, float *x, const float *b, const int n, int skip = 0 );
	//virtual void MatX_LowerTriangularSolveTranspose( const idMatX &L, float *x, const float *b, const int n );
	virtual void MixedSoundToSamples( short *samples, const float *mixBuffer, const int numSamples );

	//========================= Uses SSE3 =============================

	virtual void TransformVerts( idDrawVert *verts, const int numVerts, const idJointMat *joints, const idVec4 *weights, const int *index, const int numWeights );

#endif
};