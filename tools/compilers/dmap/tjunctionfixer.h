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

#include "containers/List.h"
#include "bv/Bounds.h"


/**
 * Processes a triangular mesh:
 *  1) merges close vertices toghether (transivitely)
 *  2) snaps merged vertices to Z/32 kind of coordinates.
 *  3) splits triangle edges by vertices lying on them
 */
class TJunctionFixer {
public:
	~TJunctionFixer();
	TJunctionFixer();
	void Reset();

	void AddTriList(struct mapTri_s **pTriList);

	void Run(float sameVertexTolerance, float vertexOnEdgeTolerance, int snapDiv);

	int GetInitialTriCount() const { return allTris.Num(); }
	int GetMergedVertsCount() const { return clusterPos.Num(); }
	int GetSplitTriCount() const { return splitAllTris.Num(); }

private:

	void SetupWorkArea();
	void MergeCloseVertices();
	void SplitEdgesByVertices();
	int64_t EstimateWork(int resol);
	void UpdateLists();

	int CellIndex(const int location[3]) const;
	void GetCellRange(const idBounds &bounds, int locations[2][3]) const;
	const idVec3 &GetCorner(int cornerIdx) const;

	// input data
	idList<struct mapTri_s **> linkedLists;		// singly-linked lists to be updated
	float vertexTol, edgeTol;					// tolerances
	int snapDivisor;							// vertex coordinates are snapped to Z/divisor

	// array of all triangles
	// allTris[starts[k] .. starts[k+1]) belong to *linkedLists[k]
	idList<struct mapTri_s *> allTris;
	idList<int> starts;
	idList<idVec3> clusterPos;

	// bounding box of all vertices
	idBounds bounds;
	float avgTriSize;

	// cubic workarea split into uniform grid of cells
	idBounds workArea;
	idVec3 workSizeInv;
	int resolution;

	// hash table
	int hashSize, hashShift;
	idList<int> numPointsInCell;		// for preliminary estimation only
	idList<int> clusterOfCorner;		// index of unique vertex in clusterPos for each 3 T "corners" of triangles
	idHashIndex hashIndex;

	// list of triangle pieces created by SplitEdgesByVertices
	idList<struct mapTri_s *> splitAllTris;
	idList<int> splitStarts;
};
