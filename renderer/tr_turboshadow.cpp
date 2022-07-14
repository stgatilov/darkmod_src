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



#include "tr_local.h"

int	c_turboUsedVerts;
int c_turboUnusedVerts;


/*
=====================
R_CreateVertexProgramTurboShadowVolume

are dangling edges that are outside the light frustum still making planes?
=====================
*/
srfTriangles_t *R_CreateVertexProgramTurboShadowVolume( const idRenderEntityLocal *ent, 
														const srfTriangles_t *tri, const idRenderLightLocal *light,
														srfCullInfo_t &cullInfo ) {
	int		i, j;
	srfTriangles_t	*newTri;
	silEdge_t	*sil;
	const glIndex_t *indexes;
	const byte *facing;

	R_CalcInteractionFacing( ent, tri, light, cullInfo );
	if ( r_useShadowProjectedCull.GetBool() ) {
		R_CalcInteractionCullBits( ent, tri, light, cullInfo );
	}

	int numFaces = tri->numIndexes / 3;
	int	numShadowingFaces = 0;
	facing = cullInfo.facing;

	// if all the triangles are inside the light frustum
	if ( cullInfo.cullBits == LIGHT_CULL_ALL_FRONT || !r_useShadowProjectedCull.GetBool() ) {

		// count the number of shadowing faces
		for ( i = 0; i < numFaces; i++ ) {
			numShadowingFaces += facing[i];
		}
		numShadowingFaces = numFaces - numShadowingFaces;

	} else {

		// make all triangles that are outside the light frustum "facing", so they won't cast shadows
		indexes = tri->indexes;
		byte *modifyFacing = cullInfo.facing;
		const byte *cullBits = cullInfo.cullBits;
		for ( j = i = 0; i < tri->numIndexes; i += 3, j++ ) {
			if ( !modifyFacing[j] ) {
				int	i1 = indexes[i+0];
				int	i2 = indexes[i+1];
				int	i3 = indexes[i+2];
				if ( cullBits[i1] & cullBits[i2] & cullBits[i3] ) {
					modifyFacing[j] = 1;
				} else {
					numShadowingFaces++;
				}
			}
		}
	}

	if ( !numShadowingFaces ) {
		// no faces are inside the light frustum and still facing the right way
		return NULL;
	}

	// shadowVerts will be NULL on these surfaces, so the shadowVerts will be taken from the ambient surface
	newTri = R_AllocStaticTriSurf();

	newTri->numVerts = tri->numVerts * 2;

	// alloc the max possible size
#ifdef USE_TRI_DATA_ALLOCATOR
	R_AllocStaticTriSurfIndexes( newTri, ( numShadowingFaces + tri->numSilEdges ) * 6 );
	glIndex_t *tempIndexes = newTri->indexes;
	glIndex_t *shadowIndexes = newTri->indexes;
#else
	glIndex_t *tempIndexes = (glIndex_t *)_alloca16( tri->numSilEdges * 6 * sizeof( tempIndexes[0] ) );
	glIndex_t *shadowIndexes = tempIndexes;
#endif

	// create new triangles along sil planes
	for ( sil = tri->silEdges, i = tri->numSilEdges; i > 0; i--, sil++ ) {

		int f1 = facing[sil->p1];
		int f2 = facing[sil->p2];

		if ( !( f1 ^ f2 ) ) {
			continue;
		}

		int v1 = sil->v1 << 1;
		int v2 = sil->v2 << 1;

		// set the two triangle winding orders based on facing
		// without using a poorly-predictable branch

		shadowIndexes[0] = v1;
		shadowIndexes[1] = v2 ^ f1;
		shadowIndexes[2] = v2 ^ f2;
		shadowIndexes[3] = v1 ^ f2;
		shadowIndexes[4] = v1 ^ f1;
		shadowIndexes[5] = v2 ^ 1;

		shadowIndexes += 6;
	}

	int	numShadowIndexes = shadowIndexes - tempIndexes;

	// we aren't bothering to separate front and back caps on these
	newTri->numIndexes = newTri->numShadowIndexesNoFrontCaps = numShadowIndexes + numShadowingFaces * 6;
	newTri->numShadowIndexesNoCaps = numShadowIndexes;
	newTri->shadowCapPlaneBits = SHADOW_CAP_INFINITE;

#ifdef USE_TRI_DATA_ALLOCATOR
	// decrease the size of the memory block to only store the used indexes
	R_ResizeStaticTriSurfIndexes( newTri, newTri->numIndexes );
#else
	// allocate memory for the indexes
	R_AllocStaticTriSurfIndexes( newTri, newTri->numIndexes );
	// copy the indexes we created for the sil planes
	SIMDProcessor->Memcpy( newTri->indexes, tempIndexes, numShadowIndexes * sizeof( tempIndexes[0] ) );
#endif

	// these have no effect, because they extend to infinity
	newTri->bounds.Clear();

	// put some faces on the model and some on the distant projection
	indexes = tri->indexes;
	shadowIndexes = newTri->indexes + numShadowIndexes;
	for ( i = 0, j = 0; i < tri->numIndexes; i += 3, j++ ) {
		if ( facing[j] ) {
			continue;
		}

		int i0 = indexes[i+0] << 1;
		shadowIndexes[2] = i0;
		shadowIndexes[3] = i0 ^ 1;
		int i1 = indexes[i+1] << 1;
		shadowIndexes[1] = i1;
		shadowIndexes[4] = i1 ^ 1;
		int i2 = indexes[i+2] << 1;
		shadowIndexes[0] = i2;
		shadowIndexes[5] = i2 ^ 1;

		shadowIndexes += 6;
	}

	return newTri;
}

/*
=====================
R_CreateVertexProgramBvhShadowVolume

stgatilov #5886: Similar to "turbo shadow volume", but uses BVH tree for acceleration
=====================
*/
srfTriangles_t *R_CreateVertexProgramBvhShadowVolume( const idRenderEntityLocal *ent, const srfTriangles_t *tri, const idRenderLightLocal *light ) {
	// transform light geometry into model space
	idVec3 localLightOrigin;
	idPlane localLightFrustum[6];
	R_GlobalPointToLocal( ent->modelMatrix, light->globalLightOrigin, localLightOrigin );
	for ( int i = 0; i < 6; i++ )
		R_GlobalPlaneToLocal( ent->modelMatrix, -light->frustum[i], localLightFrustum[i] );

	// filter triangles:
	//   1) inside light frustum
	//   2) backfacing
	// such triangles are called "shadowing" below
	idFlexListHuge<bvhTriRange_t> intervals;
	R_CullBvhByFrustumAndOrigin(
		tri->bounds, tri->bvhNodes,
		localLightFrustum, -1, localLightOrigin,
		0, intervals
	);

	// stores whether each triangle in the found intervals is shadowing or not
	// more precisely, flags for triangles in intervals[i] are laid at range [ shadowingStarts[i] .. shadowingStarts[i+1] )
	// note: if interval surely matches (all triangles surely shadowing), then flags are not stored (i.e. shadowingStarts[i] == shadowingStarts[i+1])
	idFlexListHuge<bool> shadowingData;
	idFlexList<int, 1024> shadowingStarts;
	shadowingStarts.AddGrow(0);

	// same as IsShadowing, but caller guarantees that triangle belongs to specified interval (faster)
	auto IsShadowingInRange = [&]( int t, int rngIdx ) -> bool {
		if ( intervals[rngIdx].info == BVH_TRI_SURELY_MATCH ) {
			// intervals surely matches: all triangles are implicitly shadowing
			return true;
		}
		// interval is uncertain: read flag from appropriate location
		assert( t >= intervals[rngIdx].beg && t < intervals[rngIdx].end );
		int offset = t - intervals[rngIdx].beg;
		int pos = shadowingStarts[rngIdx] + offset;
		return shadowingData[pos];
	};

	// determines if triangle with specified index is shadowing
	auto IsShadowing = [&]( int t ) -> bool {
		// find interval which contains t using binary search
		int q = std::lower_bound( intervals.Ptr(), intervals.Ptr() + intervals.Num(), t, []( const bvhTriRange_t &rng, int value ) {
			return rng.end <= value;
		}) - intervals.Ptr();
		// we have found min q : intervals[q].end > value
		// check if we are indeed within q-th interval, and if yes, check flag in this interval
		if ( q < intervals.Num() && t >= intervals[q].beg && IsShadowingInRange( t, q ) )
			return true;
		return false;
	};

	int totalShadowingTris = 0;

	// uncertain intervals should usually be short
	idFlexList<byte, 1024> triCull, triFacing;
	// go through all intervals and fill shadowing data
	for ( int i = 0; i < intervals.Num(); i++ ) {
		int beg = intervals[i].beg;
		int len = intervals[i].end - intervals[i].beg;
		int info = intervals[i].info;

		if ( info == BVH_TRI_SURELY_MATCH ) {
			// all triangles surely match, so all will be considered shadowing implicitly
			totalShadowingTris += len;
			shadowingStarts.AddGrow( shadowingData.Num() );
			continue;
		}

		// see which triangles are within light frustum
		triCull.SetNum( len );
		if ( info & BVH_TRI_SURELY_WITHIN_LIGHT )
			memset( triCull.Ptr(), 0, triCull.Num() * sizeof(triCull[0]) );
		else {
			SIMDProcessor->CullTrisByFrustum(
				tri->verts, tri->numVerts,
				tri->indexes + 3 * beg, 3 * len,
				localLightFrustum, triCull.Ptr(), LIGHT_CLIP_EPSILON
			);
		}

		// see which triangles are backfacing
		triFacing.SetNum( len );
		if ( info & BVH_TRI_SURELY_GOOD_ORI )
			memset( triFacing.Ptr(), true, triFacing.Num() * sizeof(triFacing[0]) );
		else {
			SIMDProcessor->CalcTriFacing(
				tri->verts, tri->numVerts,
				tri->indexes + 3 * beg, 3 * len,
				localLightOrigin, triFacing.Ptr()
			);
		}

		for ( int t = 0; t < len; t++ ) {
			// detect if triangle is shadowing, and save this data
			bool shadow = ( triCull[t] == 0 && triFacing[t] == false );
			shadowingData.AddGrow(shadow);
			totalShadowingTris += int(shadow);
		}
		shadowingStarts.AddGrow( shadowingData.Num() );
	}

	if ( totalShadowingTris == 0 ) {
		// no shadowing triangles -> no shadow geometry
		return nullptr;
	}

	srfTriangles_t *newTri = R_AllocStaticTriSurf();
	// shadowVerts will be NULL on these surfaces, so the shadowVerts will be taken from the ambient surface
	newTri->numVerts = tri->numVerts * 2;
	// these have no effect, because they extend to infinity
	newTri->bounds.Clear();

	// for each shadowing triangles, check if its edges are silhouette (separate shadowing from non-shadowing)
	// all silhouette edges with proper orientation are collected in this list
	idFlexListHuge<glIndex_t> silEdges;

	for ( int i = 0; i < intervals.Num(); i++ ) {
		bvhTriRange_t r = intervals[i];

		for ( int t = r.beg; t < r.end; t++ ) {
			// only consider shadowing triangles
			if ( !IsShadowingInRange( t, i ) )
				continue;

			// iterate over edges of this tri
			for ( int s = 0; s < 3; s++ ) {
				// index of adjacent triangle (along edge opposite to s-th vertex)
				int adj = tri->adjTris[3 * t + s];

				// see if adjacent tri is also shadowing
				bool adjIsShadowing = false;
				if ( adj >= r.beg && adj < r.end )
					adjIsShadowing = IsShadowingInRange( adj, i );	// fast case: same range
				else
					adjIsShadowing = IsShadowing( adj );			// general case: search for range

				if ( adjIsShadowing )
					continue;

				// this is edge from shadowing tri to non-shadowing tri = silhouette edge
				// store the edge oriented in such way that shadowing triangle is to the left of it
				silEdges.AddGrow( tri->indexes[3 * t + (s+1) % 3] );
				silEdges.AddGrow( tri->indexes[3 * t + (s+2) % 3] );
			}
		}
	}

	// we know silhouette edges and number of shadowing tris -> can set all numbers
	newTri->numIndexes = newTri->numShadowIndexesNoFrontCaps = 3 * silEdges.Num() + totalShadowingTris * 6;
	newTri->numShadowIndexesNoCaps = 3 * silEdges.Num();
	newTri->shadowCapPlaneBits = SHADOW_CAP_INFINITE;

	// allocate memory for new shadow index buffer
	R_AllocStaticTriSurfIndexes( newTri, newTri->numIndexes );
	int pos = 0;

	for ( int i = 0; i < silEdges.Num(); i += 2 ) {
		// generate a quad from each silhouette edge
		int v1 = silEdges[i + 0];
		int v2 = silEdges[i + 1];
		newTri->indexes[pos++] = 2 * v2 + 0;
		newTri->indexes[pos++] = 2 * v1 + 1;
		newTri->indexes[pos++] = 2 * v1 + 0;
		newTri->indexes[pos++] = 2 * v2 + 0;
		newTri->indexes[pos++] = 2 * v2 + 1;
		newTri->indexes[pos++] = 2 * v1 + 1;
	}
	assert( pos == newTri->numShadowIndexesNoCaps );

	for ( int i = 0; i < intervals.Num(); i++ ) {
		bvhTriRange_t r = intervals[i];

		for ( int t = r.beg; t < r.end; t++ ) {
			if ( !IsShadowingInRange( t, i ) )
				continue;

			// this tri is shadowing
			// add it to front and back caps
			int a = tri->indexes[3 * t + 0];
			int b = tri->indexes[3 * t + 1];
			int c = tri->indexes[3 * t + 2];
			newTri->indexes[pos++] = 2 * c + 0;
			newTri->indexes[pos++] = 2 * b + 0;
			newTri->indexes[pos++] = 2 * a + 0;
			newTri->indexes[pos++] = 2 * a + 1;
			newTri->indexes[pos++] = 2 * b + 1;
			newTri->indexes[pos++] = 2 * c + 1;
		}
	}
	assert( pos == newTri->numIndexes );

	return newTri;
}
