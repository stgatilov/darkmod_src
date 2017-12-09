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

#define MAX_POLYTOPE_PLANES		6

/*
=====================
R_PolytopeSurface

Generate vertexes and indexes for a polytope, and optionally returns the polygon windings.
The positive sides of the planes will be visible.
=====================
*/
srfTriangles_t *R_PolytopeSurface( int numPlanes, const idPlane *planes, idWinding *windings ) {
	int i, j;
	srfTriangles_t *tri;
	idFixedWinding planeWindings[MAX_POLYTOPE_PLANES];
	int numVerts, numIndexes;

	if ( numPlanes > MAX_POLYTOPE_PLANES ) {
		common->Error( "R_PolytopeSurface: more than %d planes", MAX_POLYTOPE_PLANES );
	}

	numVerts = 0;
	numIndexes = 0;
	for ( i = 0; i < numPlanes; i++ ) {
		const idPlane &plane = planes[i];
		idFixedWinding &w = planeWindings[i];

		w.BaseForPlane( plane );
		for ( j = 0; j < numPlanes; j++ ) {
			const idPlane &plane2 = planes[j];
			if ( j == i ) {
				continue;
			} else if ( !w.ClipInPlace( -plane2, ON_EPSILON ) ) {
				break;
			}
		}
		if ( w.GetNumPoints() <= 2 ) {
			continue;
		}
		numVerts += w.GetNumPoints();
		numIndexes += ( w.GetNumPoints() - 2 ) * 3;
	}

	// allocate the surface
	tri = R_AllocStaticTriSurf();
	R_AllocStaticTriSurfVerts( tri, numVerts );
	R_AllocStaticTriSurfIndexes( tri, numIndexes );

	// copy the data from the windings
	for ( i = 0; i < numPlanes; i++ ) {
		idFixedWinding &w = planeWindings[i];
		if ( !w.GetNumPoints() ) {
			continue;
		}
		for ( j = 0 ; j < w.GetNumPoints() ; j++ ) {
			tri->verts[tri->numVerts + j ].Clear();
			tri->verts[tri->numVerts + j ].xyz = w[j].ToVec3();
		}

		for ( j = 1 ; j < w.GetNumPoints() - 1 ; j++ ) {
			tri->indexes[ tri->numIndexes + 0 ] = tri->numVerts;
			tri->indexes[ tri->numIndexes + 1 ] = tri->numVerts + j;
			tri->indexes[ tri->numIndexes + 2 ] = tri->numVerts + j + 1;
			tri->numIndexes += 3;
		}
		tri->numVerts += w.GetNumPoints();

		// optionally save the winding
		if ( windings ) {
			//windings[i] = new idWinding( w.GetNumPoints() );
			windings[i] = w;
		}
	}

	R_BoundTriSurf( tri );

	return tri;
}