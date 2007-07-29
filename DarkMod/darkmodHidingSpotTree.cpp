/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/
/*!
* Implementation of the darkmod hiding spot tree.
*
* Identified hiding spots are tracked by AAS area.
* Written for the darkmod.
*
*/

// Includes
#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include "darkmodHidingSpotTree.h"

//--------------------------------------------------------------------------

/*!
* Standard constructor, creates an empty tree.
*/
CDarkmodHidingSpotTree::CDarkmodHidingSpotTree()
{
	numAreas = 0;
	numSpots = 0;
	p_firstArea = NULL;
	p_lastArea = NULL;

	lastIndex_indexRetrieval = 0;
	lastAreaHandle_indexRetrieval = NULL;
	lastSpotHandle_indexRetrieval = NULL;

}

//--------------------------------------------------------------------------

/*!
* Destructor
*/
CDarkmodHidingSpotTree::~CDarkmodHidingSpotTree()
{
	clear();
}

//--------------------------------------------------------------------------

void CDarkmodHidingSpotTree::clear()
{
	TDarkmodHidingSpotAreaNode* p_node = p_firstArea;
	while (p_node != NULL)
	{
		darkModHidingSpotNode* p_spot = p_node->p_firstSpot;
		while (p_spot != NULL)
		{
			darkModHidingSpotNode* p_temp = p_spot->p_next;
			delete p_spot;
			p_spot = p_temp;
		}

		TDarkmodHidingSpotAreaNode* p_temp2 = p_node->p_nextSibling;
		delete p_node;
		p_node = p_temp2;
	}

	// Now empty
	numAreas = 0;
	numSpots = 0;
	p_firstArea = NULL;
	p_lastArea = NULL;

}

void CDarkmodHidingSpotTree::Save( idSaveGame *savefile ) const
{
	// TODO
}

void CDarkmodHidingSpotTree::Restore( idRestoreGame *savefile )
{
	// TODO
}

//-------------------------------------------------------------------------

TDarkmodHidingSpotAreaNode* CDarkmodHidingSpotTree::getArea
(
	unsigned int areaIndex
)
{
	TDarkmodHidingSpotAreaNode* p_node = p_firstArea;
	while (p_node != NULL)
	{
		if (p_node->aasAreaIndex == areaIndex)
		{
			return p_node;
		}

		p_node = p_node->p_nextSibling;
	}

	return NULL;
}

//-------------------------------------------------------------------------

void CDarkmodHidingSpotTree::clearIndexRetrievalTracking()
{
	lastIndex_indexRetrieval = 0;
	lastAreaHandle_indexRetrieval = NULL;
	lastSpotHandle_indexRetrieval = NULL;

}

//-------------------------------------------------------------------------

TDarkmodHidingSpotAreaNode* CDarkmodHidingSpotTree::insertArea
(
	unsigned int areaIndex
)
{
	TDarkmodHidingSpotAreaNode* p_node = new TDarkmodHidingSpotAreaNode;
	if (p_node == NULL)
	{
		return NULL;
	}


	p_node->aasAreaIndex = areaIndex;
	p_node->count = 0;
	p_node->bestSpotQuality = 0.0;
	p_node->p_firstSpot = NULL;
	p_node->p_lastSpot = NULL;

	// Put at end (worst areas) of list for now
	p_node->p_nextSibling = NULL;
	if (p_lastArea != NULL)
	{
		p_lastArea->p_nextSibling = p_node;
	}
	p_node->p_prevSibling = p_lastArea;
	p_lastArea = p_node;

	// Special case, was empty list
	if (p_firstArea == NULL)
	{
		p_firstArea = p_node;
	}

	numAreas ++;

	clearIndexRetrievalTracking();

	return p_node;

}

//-------------------------------------------------------------------------

bool CDarkmodHidingSpotTree::determineSpotRedundancy
(
	TDarkmodHidingSpotAreaNode* p_areaNode,
	aasGoal_t goal,
	int hidingSpotTypes,
	float quality,
	float redundancyDistance
)
{
	// Test parameters
	if (p_areaNode == NULL)
	{
		return false;
	}

	// Compare distance with other points in the area
	darkModHidingSpotNode* p_cursor = p_areaNode->p_firstSpot;
	while (p_cursor != NULL)
	{
		// Compute distance
		idVec3 distanceVec = goal.origin - p_cursor->spot.goal.origin;
		if (distanceVec.LengthFast() <= redundancyDistance)
		{
			// This point is redundant, should combine.
			p_cursor->spot.hidingSpotTypes |= hidingSpotTypes;
			if (p_cursor->spot.quality < quality)
			{
				// Use higher quality location
				p_cursor->spot.quality = quality;
				p_cursor->spot.goal = goal;
			}

			// Combined
			return true;
		}

		// Next spot
		p_cursor = p_cursor->p_next;
	}

	// Not redundant
	return false;
}

//-------------------------------------------------------------------------

bool CDarkmodHidingSpotTree::insertHidingSpot
(
	TDarkmodHidingSpotAreaNode* p_areaNode,
	aasGoal_t goal,
	int hidingSpotTypes,
	float lightQuotient,
	float qualityWithoutDistanceFactor,
	float quality,
	float redundancyDistance
)
{
	// Test parameters
	if (p_areaNode == NULL)
	{
		return false;
	}


	// Update best spot quality in area
	if (quality > p_areaNode->bestSpotQuality)
	{
		p_areaNode->bestSpotQuality = quality;

		// Slide area toward front of list until it is in
		// best spot quality order
		while (p_areaNode->p_prevSibling != NULL)
		{
			if (p_areaNode->p_prevSibling->bestSpotQuality < p_areaNode->bestSpotQuality)
			{
				TDarkmodHidingSpotAreaNode* p_bumped = p_areaNode->p_prevSibling;
				TDarkmodHidingSpotAreaNode* p_bumpedPrev = p_bumped->p_prevSibling;
				if (p_bumpedPrev != NULL)
				{
					p_bumpedPrev->p_nextSibling = p_areaNode;
				}
				else
				{
					p_firstArea = p_areaNode;
				}
				p_areaNode->p_prevSibling = p_bumpedPrev;

				p_bumped->p_nextSibling = p_areaNode->p_nextSibling;
				if (p_bumped->p_nextSibling != NULL)
				{
					p_bumped->p_nextSibling->p_prevSibling = p_bumped;
				}
				else
				{
					p_lastArea = p_bumped;
				}
				p_bumped->p_prevSibling = p_areaNode;
				p_areaNode->p_nextSibling = p_bumped;
				
			}
			else
			{
				break;
			}
		}


	}

	// Test if it is redundant
	if (redundancyDistance >= 0.0)
	{
		if (determineSpotRedundancy (p_areaNode, goal, hidingSpotTypes, quality, redundancyDistance))
		{
			// Spot was redundant with other points. The other points may have
			// been modified, but we do not add the new point
			return true;
		}
	}

	// Not redundant, so adding new spot
	darkModHidingSpotNode* p_spot = new darkModHidingSpotNode;
	if (p_spot == NULL)
	{
		return false;
	}

	p_spot->spot.goal = goal;
	p_spot->spot.hidingSpotTypes = hidingSpotTypes;
	p_spot->spot.lightQuotient = lightQuotient;
	p_spot->spot.qualityWithoutDistanceFactor = qualityWithoutDistanceFactor;
	p_spot->spot.quality = quality;


	// Add some randomness to the order of points in the areas.
	// Randomly add to either front or back of the area list
	if (gameLocal.random.RandomFloat() < 0.5)
	{
		// Add to front of list
		p_spot->p_next = p_areaNode->p_firstSpot;
		p_areaNode->p_firstSpot = p_spot;
		if (p_areaNode->p_lastSpot == NULL)
		{
			p_areaNode->p_lastSpot = p_spot;
		}
		p_areaNode->count ++;
	}
	else
	{
		// Add to end of list
		p_spot->p_next = NULL;
		
		if (p_areaNode->p_lastSpot == NULL)
		{
			p_areaNode->p_lastSpot = p_spot;
			p_areaNode->p_firstSpot = p_spot;
		}
		else
		{
			p_areaNode->p_lastSpot->p_next = p_spot;
			p_areaNode->p_lastSpot = p_spot;
		}
		p_areaNode->count ++;
	}

	// Change bounds of area
	if (p_areaNode->count == 1)
	{
		p_areaNode->bounds.Clear();
		p_areaNode->bounds.AddPoint (p_spot->spot.goal.origin);
	}
	else 
	{
		p_areaNode->bounds.AddPoint (p_spot->spot.goal.origin);
	}

	// One more point 
	numSpots ++;

	clearIndexRetrievalTracking();

	// Done
	return true;
}

//-------------------------------------------------------------------------
#define NUM_SECTORS_IN_SUBDIVIDE 8

bool CDarkmodHidingSpotTree::subDivideArea
(
	TDarkmodHidingSpotAreaNode* in_p_areaNode,
	unsigned int& out_numSubAreasWithPoints,
	TDarkmodHidingSpotAreaNode* out_p_subAreas[8]
)
{

	// test params
	if ((out_p_subAreas == NULL) || (in_p_areaNode == NULL))
	{
		return false;
	}

	// No sub areas yet
	out_numSubAreasWithPoints = 0;
	for (int subAreaIndex = 0; subAreaIndex < NUM_SECTORS_IN_SUBDIVIDE; subAreaIndex ++)
	{
		out_p_subAreas[subAreaIndex] = NULL;
	}

	// Divide bounds into subAreas
	idBounds subAreaBounds[NUM_SECTORS_IN_SUBDIVIDE];

	idVec3 midPoint = in_p_areaNode->bounds.GetCenter();
	
	// < x, < y, < z
	subAreaBounds[0][0].x = in_p_areaNode->bounds[0].x;
	subAreaBounds[0][1].x = midPoint.x;
	subAreaBounds[0][0].y = in_p_areaNode->bounds[0].y;
	subAreaBounds[0][1].y = midPoint.y;
	subAreaBounds[0][0].z = in_p_areaNode->bounds[0].z;
	subAreaBounds[0][1].z = midPoint.z;

	// > x, < y, < z
	subAreaBounds[1][0].x = midPoint.x;
	subAreaBounds[1][1].x = in_p_areaNode->bounds[1].x;
	subAreaBounds[1][0].y = in_p_areaNode->bounds[0].y;
	subAreaBounds[1][1].y = midPoint.y;
	subAreaBounds[1][0].z = in_p_areaNode->bounds[0].z;
	subAreaBounds[1][1].z = midPoint.z;

	// < x, > y, < z
	subAreaBounds[2][0].x = in_p_areaNode->bounds[0].x;
	subAreaBounds[2][1].x = midPoint.x;
	subAreaBounds[2][0].y = midPoint.y;
	subAreaBounds[2][1].y = in_p_areaNode->bounds[1].y;
	subAreaBounds[2][0].z = in_p_areaNode->bounds[0].z;
	subAreaBounds[2][1].z = midPoint.z;

	// > x, > y, < z
	subAreaBounds[3][0].x = midPoint.x;
	subAreaBounds[3][1].x = in_p_areaNode->bounds[1].x;
	subAreaBounds[3][0].y = midPoint.y;
	subAreaBounds[3][1].y = in_p_areaNode->bounds[1].y;
	subAreaBounds[3][0].z = in_p_areaNode->bounds[0].z;
	subAreaBounds[3][1].z = midPoint.z;

	// < x, < y, > z
	subAreaBounds[4][0].x = in_p_areaNode->bounds[0].x;
	subAreaBounds[4][1].x = midPoint.x;
	subAreaBounds[4][0].y = in_p_areaNode->bounds[0].y;
	subAreaBounds[4][1].y = midPoint.y;
	subAreaBounds[4][0].z = midPoint.z;
	subAreaBounds[4][1].z = in_p_areaNode->bounds[1].z;

	// > x, < y, > z
	subAreaBounds[5][0].x = midPoint.x;
	subAreaBounds[5][1].x = in_p_areaNode->bounds[1].x;
	subAreaBounds[5][0].y = in_p_areaNode->bounds[0].y;
	subAreaBounds[5][1].y = midPoint.y;
	subAreaBounds[5][0].z = midPoint.z;
	subAreaBounds[5][1].z = in_p_areaNode->bounds[1].z;

	// < x, > y, > z
	subAreaBounds[6][0].x = in_p_areaNode->bounds[0].x;
	subAreaBounds[6][1].x = midPoint.x;
	subAreaBounds[6][0].y = midPoint.y;
	subAreaBounds[6][1].y = in_p_areaNode->bounds[1].y;
	subAreaBounds[6][0].z = midPoint.z;
	subAreaBounds[6][1].z = in_p_areaNode->bounds[1].z;

	// > x, > y, > z
	subAreaBounds[7][0].x = midPoint.x;
	subAreaBounds[7][1].x = in_p_areaNode->bounds[1].x;
	subAreaBounds[7][0].y = midPoint.y;
	subAreaBounds[7][1].y = in_p_areaNode->bounds[1].y;
	subAreaBounds[7][0].z = midPoint.z;
	subAreaBounds[7][1].z = in_p_areaNode->bounds[1].z;


	// Create sub lists for points
	darkModHidingSpotNode* p_subAreaPointHeaders[NUM_SECTORS_IN_SUBDIVIDE] = 
	{
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
	};
	darkModHidingSpotNode* p_subAreaPointTails[NUM_SECTORS_IN_SUBDIVIDE] = 
	{
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
	};
	unsigned int subAreaPointCounts[NUM_SECTORS_IN_SUBDIVIDE] = {0,0,0,0,0,0,0,0};
	float subAreaPointBestQualities[NUM_SECTORS_IN_SUBDIVIDE] = {0,0,0,0,0,0,0,0};

	// Ride list of points, and move points into sub lists
	darkModHidingSpotNode* p_originalCursor = in_p_areaNode->p_firstSpot;
	while (p_originalCursor != NULL)
	{
		// Remember what the next point is
		darkModHidingSpotNode* p_tempNextCursor = p_originalCursor->p_next;

		// What subArea does the point fall within?
		for (int subAreaIndex = 0; subAreaIndex < NUM_SECTORS_IN_SUBDIVIDE; subAreaIndex ++)
		{
			// If point falls in here, or it is last subArea and no earlier one took it
			if ( (subAreaBounds[subAreaIndex].ContainsPoint (p_originalCursor->spot.goal.origin)) || (subAreaIndex >= (NUM_SECTORS_IN_SUBDIVIDE-1) ))
			{
				// Point falls in this subArea
				subAreaPointCounts[subAreaIndex] ++;
				if (p_originalCursor->spot.quality > subAreaPointBestQualities[subAreaIndex])
				{
					subAreaPointBestQualities[subAreaIndex] = p_originalCursor->spot.quality;
				}

				// Put it at the end of its point list
				if (p_subAreaPointHeaders[subAreaIndex] == NULL)
				{
					p_subAreaPointHeaders[subAreaIndex] = p_originalCursor;

				}
				else
				{
					p_subAreaPointTails[subAreaIndex]->p_next = p_originalCursor;
				}

				p_originalCursor->p_next = NULL;
				p_subAreaPointTails[subAreaIndex] = p_originalCursor;

				break;
			}
		
		} // Test next subArea

		// next point
		p_originalCursor = p_tempNextCursor;
		
	} // Continue until there are no more points to consider

	// Make nodes for any subArea that isn't empty
	bool b_originalUsed = false;
	TDarkmodHidingSpotAreaNode* p_nodeAfterOriginal = in_p_areaNode->p_nextSibling;
	TDarkmodHidingSpotAreaNode* p_nodeBeforeOriginal = in_p_areaNode->p_prevSibling;
	TDarkmodHidingSpotAreaNode* p_newNodePreviousSibling = in_p_areaNode;
	TDarkmodHidingSpotAreaNode* p_newNodeNextSibling = in_p_areaNode->p_nextSibling;



	for (int subAreaIndex = 0; subAreaIndex < NUM_SECTORS_IN_SUBDIVIDE; subAreaIndex ++)
	{
		TDarkmodHidingSpotAreaNode* p_areaNode = NULL;
		
		// Any points in this subArea?
		if (subAreaPointCounts[subAreaIndex] > 0)
		{
			// One more sub area with points
			out_numSubAreasWithPoints ++;

			// Need node, is original already used?
			if (!b_originalUsed)
			{
				p_areaNode = in_p_areaNode;
				b_originalUsed = true;
			}
			else
			{
				// Make new node
				p_areaNode = new TDarkmodHidingSpotAreaNode;
				if (p_areaNode == NULL)
				{
					// Doh, structure corrupt, out of memory
					return false;
				}

				// Fill out node properties that don't change from original
				p_areaNode->aasAreaIndex = in_p_areaNode->aasAreaIndex;
	
				// One more area in the tree
				numAreas ++;

			}

			// Make pointer to the area node in out array for caller
			out_p_subAreas[out_numSubAreasWithPoints-1] = p_areaNode;

			// Fill out node properties that are different than original
			p_areaNode->bestSpotQuality = subAreaPointBestQualities[subAreaIndex];
			p_areaNode->bounds = subAreaBounds[subAreaIndex];
			p_areaNode->count = subAreaPointCounts[subAreaIndex];
			p_areaNode->p_firstSpot = p_subAreaPointHeaders[subAreaIndex];
			p_areaNode->p_lastSpot = p_subAreaPointTails[subAreaIndex];
			
			// Link into area list just after original (if not original already)
			if (p_areaNode != in_p_areaNode)
			{
				// Link with previous sibling
				p_areaNode->p_prevSibling =	p_newNodePreviousSibling;
				if (p_newNodePreviousSibling != NULL)
				{
					p_newNodePreviousSibling->p_nextSibling = p_areaNode;
				}
				else
				{
					// This area is now at start of list
					p_firstArea = p_areaNode;
				}

				// Link to next sibling
				p_areaNode->p_nextSibling = p_newNodeNextSibling;
				if (p_newNodeNextSibling != NULL)
				{
					p_newNodeNextSibling->p_prevSibling = p_areaNode;
				}
				else
				{
					// This area is now at end of list
					p_lastArea = p_areaNode;
				}

				// Next new node comes after this new node but before next original node
				p_newNodePreviousSibling = p_areaNode;

			}

		} // Subarea had points, needs node
		
	} // Next subArea

	// Done
	return true;

}

//-------------------------------------------------------------------------

bool CDarkmodHidingSpotTree::subDivideAreas
(
	unsigned int maxPointsPerArea
)
{
	// Ride the list of areas
	TDarkmodHidingSpotAreaNode* p_firstAreaNeedingSubDivision = p_firstArea;

	while (p_firstAreaNeedingSubDivision != NULL)
	{
		// Make a pass through the tree
		TDarkmodHidingSpotAreaNode* p_originalAreaCursor = p_firstAreaNeedingSubDivision;
		TDarkmodHidingSpotAreaNode* p_originalAreaNext = NULL;

		// No areas are known to need sub-division now
		p_firstAreaNeedingSubDivision = NULL;

		// Iterate the list
		while (p_originalAreaCursor != NULL)
		{
			// Remember the next original area we will process
			p_originalAreaNext = p_originalAreaCursor->p_nextSibling;

			// Subdivide this original area if it has more points that the limit
			// given by the caller
			if (p_originalAreaCursor->count > maxPointsPerArea)
			{
			
				// Sub-divide the area
				unsigned int numSubAreasWithPoints = 0;
				TDarkmodHidingSpotAreaNode* p_subAreas[8];

				if (!subDivideArea
				(
					p_originalAreaCursor,
					numSubAreasWithPoints,
					p_subAreas
				))
				{
					// Failed to sub-divide this area due to lack of memory
					return false;
				}

				// is this the first area that we found this pass that needed sub-division?
				if ((numSubAreasWithPoints > 1) && (p_firstAreaNeedingSubDivision == NULL))
				{
					p_firstAreaNeedingSubDivision = p_originalAreaCursor;
				}


			} // Done subdividing this area
            
			// On to next original area
			p_originalAreaCursor = p_originalAreaNext;
		}

	} // As long as some area needed sub-division, we will do another pass

	// Done
	return true;
}

//-------------------------------------------------------------------------

TDarkmodHidingSpotAreaNode* CDarkmodHidingSpotTree::getFirstArea
(
	TDarkModHidingSpotTreeIterationHandle& out_iterationHandle
)
{
	out_iterationHandle = (TDarkModHidingSpotTreeIterationHandle) p_firstArea;
	return p_firstArea;
}

//-------------------------------------------------------------------------

TDarkmodHidingSpotAreaNode* CDarkmodHidingSpotTree::getNextArea
(
	TDarkModHidingSpotTreeIterationHandle& inout_iterationHandle
)
{
	TDarkmodHidingSpotAreaNode* p_cursor = (TDarkmodHidingSpotAreaNode*) inout_iterationHandle;
	if (p_cursor != NULL)
	{
		p_cursor = p_cursor->p_nextSibling;
	}

    inout_iterationHandle = (TDarkModHidingSpotTreeIterationHandle) p_cursor;
	return p_cursor;

}

//-------------------------------------------------------------------------

darkModHidingSpot_t* CDarkmodHidingSpotTree::getFirstHidingSpotInArea
(
	TDarkModHidingSpotTreeIterationHandle& inout_areaIterationHandle,
	TDarkModHidingSpotTreeIterationHandle& out_spotHandle
)
{
	TDarkmodHidingSpotAreaNode* p_areaCursor = (TDarkmodHidingSpotAreaNode*) inout_areaIterationHandle;
	if (p_areaCursor == NULL)
	{
		return NULL;
	}

	out_spotHandle = (TDarkModHidingSpotTreeIterationHandle) (p_areaCursor->p_firstSpot);
	if (p_areaCursor->p_firstSpot == NULL)
	{
		return NULL;
	}

	return &(p_areaCursor->p_firstSpot->spot);

}

//-------------------------------------------------------------------------

darkModHidingSpot_t* CDarkmodHidingSpotTree::getNextHidingSpotInArea
(
	TDarkModHidingSpotTreeIterationHandle& inout_spotHandle
)
{
	darkModHidingSpotNode* p_cursor = (darkModHidingSpotNode*) inout_spotHandle;
	if (p_cursor == NULL)
	{
		return NULL;
	}

	p_cursor = p_cursor->p_next;
	inout_spotHandle = (TDarkModHidingSpotTreeIterationHandle) p_cursor;

	if (p_cursor != NULL)
	{
		return &(p_cursor->spot);
	}
	else
	{
		return NULL;
	}
}

//-------------------------------------------------------------------------

darkModHidingSpot_t* CDarkmodHidingSpotTree::getNthSpotInternal
(
	unsigned int index,
	idBounds& out_areaNodeBounds
)
{
	unsigned int accumulatedIndex = 0;

	TDarkmodHidingSpotAreaNode* p_areaCursor = p_firstArea;

	// Find correct area
	while (p_areaCursor != NULL)
	{
		if ((accumulatedIndex + p_areaCursor->count) > index)
		{
			break;
		}
		else
		{
			accumulatedIndex += p_areaCursor->count;
			p_areaCursor = p_areaCursor->p_nextSibling;
		}
	}

	// Is index beyond end of tree?
	if (p_areaCursor == NULL)
	{
		return NULL;
	}

	// Report bounds of area node to caller
	out_areaNodeBounds = p_areaCursor->bounds;

	// How many spots within is this?
	unsigned int chosenSpotIndex = index - accumulatedIndex;
	unsigned int spotIndex = 0;
	darkModHidingSpotNode* p_spot = p_areaCursor->p_firstSpot;

	while ((p_spot != NULL) && (spotIndex < chosenSpotIndex))
	{
		spotIndex ++;
		p_spot = p_spot->p_next;
	}

	if (p_spot == NULL)
	{
		return NULL;
	}

	// Found it
	return &(p_spot->spot);

}

//-------------------------------------------------------------------------

darkModHidingSpot_t* CDarkmodHidingSpotTree::getNthSpot
(
	unsigned int index
)
{
	/*
	* lastIndex_indexRetrieval The index of the spot for which the handles
	*	are being supplied. If < index this is treated as meaning that
	*   the handles are not initialized as input parameters and a slower
	*	search must be used.  If this and "index" are both 0, the same
	*	is assumed.
	*
	* lastAreaHandle_indexRetrieval On entry, the handle corresponding to the values
	*	returned for the parameter "indexForHandles". 
	*	On exit, will be the value corresponding to the parameter "index".
	*
	* lastSpotHandle_indexRetrieval On entry, handle corresponding to the values
	*	returned for the parameter "indexForHandles". 
	*	On exit, will be the value corresponding to the parameter "index".
	*/

	unsigned int spotDelta = 0;
	TDarkmodHidingSpotAreaNode* p_areaCursor;
	darkModHidingSpotNode* p_spotCursor;

	// Determine where to start search
	if 
	(
		(index < lastIndex_indexRetrieval) || 
		(
			(index == lastIndex_indexRetrieval) && 
			(index == 0) 
		)
	)
	{
		spotDelta = index;
		p_areaCursor = p_firstArea;
		if (p_firstArea != NULL)
		{
			p_spotCursor = (p_firstArea->p_firstSpot);
		}
		else
		{
			p_spotCursor = NULL;
		}
	}
	else
	{
		spotDelta = index - lastIndex_indexRetrieval;
		p_areaCursor = (TDarkmodHidingSpotAreaNode*) lastAreaHandle_indexRetrieval;
		p_spotCursor = (darkModHidingSpotNode*) lastSpotHandle_indexRetrieval;
	}

	// There can't be a spot cursor if there is no area cursor
	if (p_areaCursor == NULL) 
	{
		p_spotCursor = NULL;
	}

	// Handle special case of already being at the point
	

	// Iterate to correct point
	while (p_areaCursor != NULL)
	{
		// Check if we can just skip this entire area
		if 
		(
			(p_spotCursor == p_areaCursor->p_firstSpot) &&
			(spotDelta >= p_areaCursor->count)
		)
		{
			spotDelta -= p_areaCursor->count;
			p_areaCursor = p_areaCursor->p_nextSibling;
			if (p_areaCursor == NULL)
			{
				lastAreaHandle_indexRetrieval = NULL;
				lastSpotHandle_indexRetrieval = NULL;
				lastIndex_indexRetrieval = 0;
				return NULL;
			}

			p_spotCursor = p_areaCursor->p_firstSpot;
		}
		else
		{
			// No we can't skip the entire area, so advance spot cursor
			while ((spotDelta > 0) && (p_spotCursor != NULL))
			{
				p_spotCursor = p_spotCursor->p_next;
				spotDelta --;
			}

			// Is this the spot?
			if ((spotDelta == 0) && (p_spotCursor != NULL))
			{
				// Found it
				lastAreaHandle_indexRetrieval = (TDarkModHidingSpotTreeIterationHandle) p_areaCursor;
				lastSpotHandle_indexRetrieval = (TDarkModHidingSpotTreeIterationHandle) p_spotCursor;
				lastIndex_indexRetrieval = index;
				return &(p_spotCursor->spot);
			}

			// We have to go on to the next area
			p_areaCursor = p_areaCursor->p_nextSibling;
			if (p_areaCursor != NULL)
			{
				p_spotCursor = p_areaCursor->p_firstSpot;
			}
		}
	}

	// Index requested is out of bounds
	lastAreaHandle_indexRetrieval = NULL;
	lastSpotHandle_indexRetrieval = NULL;
	lastIndex_indexRetrieval = 0;
	return NULL;

}

//-------------------------------------------------------------------------


darkModHidingSpot_t* CDarkmodHidingSpotTree::getNthSpotWithAreaNodeBounds
(
	unsigned int index,
	idBounds& out_areaNodeBounds
)
{
	/*
	* lastIndex_indexRetrieval The index of the spot for which the handles
	*	are being supplied. If < index this is treated as meaning that
	*   the handles are not initialized as input parameters and a slower
	*	search must be used.  If this and "index" are both 0, the same
	*	is assumed.
	*
	* lastAreaHandle_indexRetrieval On entry, the handle corresponding to the values
	*	returned for the parameter "indexForHandles". 
	*	On exit, will be the value corresponding to the parameter "index".
	*
	* lastSpotHandle_indexRetrieval On entry, handle corresponding to the values
	*	returned for the parameter "indexForHandles". 
	*	On exit, will be the value corresponding to the parameter "index".
	*/

	unsigned int spotDelta = 0;
	TDarkmodHidingSpotAreaNode* p_areaCursor;
	darkModHidingSpotNode* p_spotCursor;

	// Determine where to start search
	if 
	(
		(index < lastIndex_indexRetrieval) || 
		(
			(index == lastIndex_indexRetrieval) && 
			(index == 0) 
		)
	)
	{
		spotDelta = index;
		p_areaCursor = p_firstArea;
		if (p_firstArea != NULL)
		{
			p_spotCursor = (p_firstArea->p_firstSpot);
		}
		else
		{
			p_spotCursor = NULL;
		}
	}
	else
	{
		spotDelta = index - lastIndex_indexRetrieval;
		p_areaCursor = (TDarkmodHidingSpotAreaNode*) lastAreaHandle_indexRetrieval;
		p_spotCursor = (darkModHidingSpotNode*) lastSpotHandle_indexRetrieval;
	}

	// There can't be a spot cursor if there is no area cursor
	if (p_areaCursor == NULL) 
	{
		p_spotCursor = NULL;
	}

	// Handle special case of already being at the point
	

	// Iterate to correct point
	while (p_areaCursor != NULL)
	{
		// Check if we can just skip this entire area
		if 
		(
			(p_spotCursor == p_areaCursor->p_firstSpot) &&
			(spotDelta >= p_areaCursor->count)
		)
		{
			spotDelta -= p_areaCursor->count;
			p_areaCursor = p_areaCursor->p_nextSibling;
			if (p_areaCursor == NULL)
			{
				lastAreaHandle_indexRetrieval = NULL;
				lastSpotHandle_indexRetrieval = NULL;
				lastIndex_indexRetrieval = 0;
				out_areaNodeBounds.Clear();
				return NULL;
			}

			p_spotCursor = p_areaCursor->p_firstSpot;
		}
		else
		{
			// No we can't skip the entire area, so advance spot cursor
			while ((spotDelta > 0) && (p_spotCursor != NULL))
			{
				p_spotCursor = p_spotCursor->p_next;
				spotDelta --;
			}

			// Is this the spot?
			if ((spotDelta == 0) && (p_spotCursor != NULL))
			{
				// Found it
				lastAreaHandle_indexRetrieval = (TDarkModHidingSpotTreeIterationHandle) p_areaCursor;
				lastSpotHandle_indexRetrieval = (TDarkModHidingSpotTreeIterationHandle) p_spotCursor;
				lastIndex_indexRetrieval = index;
				out_areaNodeBounds = p_areaCursor->bounds;
				return &(p_spotCursor->spot);
			}

			// We have to go on to the next area
			p_areaCursor = p_areaCursor->p_nextSibling;
			if (p_areaCursor != NULL)
			{
				p_spotCursor = p_areaCursor->p_firstSpot;
			}
		}
	}

	// Index requested is out of bounds
	lastAreaHandle_indexRetrieval = NULL;
	lastSpotHandle_indexRetrieval = NULL;
	lastIndex_indexRetrieval = 0;
	out_areaNodeBounds.Clear();
	return NULL;

}

//-------------------------------------------------------------------------

bool CDarkmodHidingSpotTree::copy
(
	CDarkmodHidingSpotTree* p_out_otherTree
)
{
	if (p_out_otherTree == NULL)
	{
		return false;
	}

	p_out_otherTree->clear();

	TDarkmodHidingSpotAreaNode* p_areaCursor = p_firstArea;
	while (p_areaCursor != NULL)
	{
		// Get or make area
		TDarkmodHidingSpotAreaNode* p_otherArea = p_out_otherTree->getArea (p_areaCursor->aasAreaIndex);
		if (p_otherArea == NULL)
		{
			p_otherArea = p_out_otherTree->insertArea (p_areaCursor->aasAreaIndex);
			if (p_otherArea == NULL)
			{
				return false;
			}
		}

		// Insert points
		darkModHidingSpotNode* p_pointCursor = p_areaCursor->p_firstSpot;
		while (p_pointCursor != NULL)
		{
			p_out_otherTree->insertHidingSpot
			(
				p_otherArea,
				p_pointCursor->spot.goal,
				p_pointCursor->spot.hidingSpotTypes,
				p_pointCursor->spot.lightQuotient,
				p_pointCursor->spot.qualityWithoutDistanceFactor,
				p_pointCursor->spot.quality,
				-1.0 // No redundancy combination
			);

			// Next point
			p_pointCursor = p_pointCursor->p_next;
		}

		// Next area
		p_areaCursor = p_areaCursor->p_nextSibling;
		
	}

	// Done
	return true;
}

//-------------------------------------------------------------------------

bool CDarkmodHidingSpotTree::getOneNth
(
	unsigned int N,
	CDarkmodHidingSpotTree* p_out_otherTree
)
{
	// Test parameters
	if (p_out_otherTree == NULL)
	{
		return false;
	}

	unsigned int numPointsMoved = 0;

	// Other tree should be empty
	p_out_otherTree->clear();

	// Test params
	if (N == 0)
	{
		return true;
	}

	// Split up our areas, as that is what human's would typically do, rather
	// than splitting up points within areas.
	if (numAreas > 1)
	{
		// Split number of areas
		unsigned int areaSplit = numAreas - (numAreas / N);

		TDarkmodHidingSpotAreaNode* p_areaCursor = p_firstArea;
		TDarkmodHidingSpotAreaNode* p_areaTrailer = NULL;

		unsigned int areaIndex = 0;
		while (areaIndex < areaSplit) 
		{
			p_areaTrailer = p_areaCursor;
			p_areaCursor = p_areaCursor->p_nextSibling;

			areaIndex ++;

			if (p_areaCursor == NULL)
			{
				// Bad area count in data structure
				return false;
			}
		}

		// Add areas to other list
		p_out_otherTree->numAreas = (numAreas - areaIndex);
		p_out_otherTree->p_firstArea = p_areaCursor;

		// Remove areas from this list
		if (p_areaTrailer != NULL)
		{
			p_areaTrailer->p_nextSibling = NULL;
			p_lastArea = p_areaTrailer;
		}
		else
		{
			p_lastArea = NULL;
			p_firstArea = NULL;
		}
		numAreas = areaIndex;
		
		
		// How many points were in the areas moved?
		TDarkmodHidingSpotAreaNode* p_countCursor = p_out_otherTree->p_firstArea;
		numPointsMoved = 0;
		while (p_countCursor != NULL)
		{
			numPointsMoved += p_countCursor->count;
			p_countCursor = p_countCursor->p_nextSibling;
		}

	}
	else if (numAreas == 0)
	{
		// Done, no hiding spots to split
		return true;
	}
	else
	{

		// Split points in the one and only area
		TDarkmodHidingSpotAreaNode* p_area = p_firstArea;
		if (p_area == NULL)
		{
			return false;
		}

		// Create new area in other tree
		TDarkmodHidingSpotAreaNode* p_otherArea = p_out_otherTree->insertArea (p_area->aasAreaIndex);
		if (p_otherArea == NULL)
		{
			return false;
		}

		unsigned int splitPointIndex = p_area->count - (p_area->count / N);
		unsigned int pointIndex = 0;
		darkModHidingSpotNode* p_spotCursor = p_area->p_firstSpot;
		darkModHidingSpotNode* p_spotTrailer = NULL;
		while (pointIndex < splitPointIndex)
		{
			p_spotTrailer = p_spotCursor;
			p_spotCursor = p_spotCursor->p_next;

			pointIndex ++;
			if (p_spotCursor == NULL)
			{
				// Bad spot count in area
				return false;
			}
		}

		// Everything from p_spotCursor onward goes in other tree
		if (p_spotCursor != NULL)
		{
			p_otherArea->p_lastSpot = p_area->p_lastSpot;
			p_otherArea->p_firstSpot = p_spotCursor;
		}
		else
		{
			p_otherArea->p_lastSpot = NULL;
			p_otherArea->p_firstSpot = NULL;
		}
		

		numPointsMoved = p_area->count - splitPointIndex;
		p_otherArea->count = numPointsMoved;

		// Evertyhing up until p_spotTrailer stays in this tree
		if (p_spotTrailer != NULL)
		{
			p_area->p_lastSpot = p_spotTrailer;
			p_spotTrailer->p_next = NULL;
		}
		else
		{
			p_area->p_lastSpot = NULL;
			p_area->p_firstSpot = NULL;
		}
		p_area->count -= numPointsMoved;

	}

	// Set point totals in both list based on number of points moved
	numSpots -= numPointsMoved;
	p_out_otherTree->numSpots = numPointsMoved;


	// Done
	return true;

}

//------------------------------------------------------------------------------------------

bool CDarkmodHidingSpotTree::sortForNewCenter
(
	idVec3 center,
	float searchRadius
)
{

	// Run through entire tree, and recalculate quality of each point given new distance from center
	unsigned long numSpots = getNumSpots();
	unsigned long spotIndex = 0;

	for (spotIndex = 0; spotIndex < numSpots; spotIndex ++)
	{
		darkModHidingSpot_t* p_spot = getNthSpot(spotIndex);
		
		if (p_spot != NULL)
		{
			float quality;

			// What is distance
			float distanceFromCenter = (p_spot->goal.origin - center).Length();

			// Perform distance fall-off
			if ((searchRadius > 0.0) && (p_spot->qualityWithoutDistanceFactor > 0.0))
			{
				float falloff = ((searchRadius - distanceFromCenter) / searchRadius);
				
				// Use power of 2 fallof
				quality =  p_spot->qualityWithoutDistanceFactor * falloff * falloff;
				if (quality < 0.0)
				{
					quality = 0.0;
				}
			}
			else
			{
				quality = 0.0;
			}

			// Update spot
			p_spot->quality = quality;
				
		}

	} // Next spot

	// Sort each area
	/*
	TDarkModHidingSpotTreeIterationHandle areaIterator;
	TDarkmodHidingSpotAreaNode* p_areaNode = getFirstArea (areaIterator);
	while (p_areaNode != NULL)
	{
		if (p_areaNode->p_firstSpot != NULL)
		{
			quicksortHidingSpotList (p_areaNode->p_firstSpot, p_areaNode->count);
			p_areaNode->bestSpotQuality = p_areaNode->p_firstSpot->spot.quality;
		}

		p_areaNode = getNextArea (areaIterator);
	}
	*/

	// Sort areas
	quicksortAreaList
	(
		p_firstArea,
		numAreas
	);

	// Find last area (ugh)
	TDarkmodHidingSpotAreaNode* p_cursor = p_firstArea;
	if (p_cursor != NULL)
	{
		p_lastArea = NULL;
	}
	else
	{
		while (p_cursor->p_nextSibling != NULL)
		{
				p_cursor = p_cursor->p_nextSibling;
		}
		p_lastArea = p_cursor;
	}

	// Clear index retrieval tracking since tree is completely changed
	clearIndexRetrievalTracking();

	// Done
	return true;

}


//----------------------------------------------------------------------------------------

void CDarkmodHidingSpotTree::quicksortHidingSpotList
(
	darkModHidingSpotNode*& inout_p_firstNode,
	unsigned long numSpots
)
{
	// If list is empty or only one node long , we are done
	if (inout_p_firstNode == NULL)
	{
		return;
	}
	else if (inout_p_firstNode->p_next == NULL)
	{
		return;
	}

	// We use median point as pivote
	darkModHidingSpotNode* p_pivot = inout_p_firstNode;
	for (unsigned long rideCount = 0; rideCount < (numSpots/2); rideCount ++)
	{
		p_pivot = p_pivot->p_next;
		if (p_pivot == NULL)
		{
			// Crap, list is broken, can't sort
			return;
		}
	}

	// Make two sub lists, those greater than pivot and those less than pivot.
	// We'll put equal into the greater list
	darkModHidingSpotNode* p_firstGreaterOrEqual = NULL;
	darkModHidingSpotNode* p_firstLess = NULL;
	unsigned long numGreaterOrEqual = 0;
	unsigned long numLess = 0;

	darkModHidingSpotNode* p_cursor = inout_p_firstNode;
	while (p_cursor != NULL)
	{
		if (p_cursor->spot.quality >= p_pivot->spot.quality)
		{
			p_cursor->p_next = p_firstGreaterOrEqual;
			p_firstGreaterOrEqual = p_cursor;
			numGreaterOrEqual ++;
		}	
		else
		{
			p_cursor->p_next = p_firstLess;
			p_firstLess = p_cursor;
			numLess ++;
		}

		p_cursor = p_cursor->p_next;
	}

	// Sort each sub list
	quicksortHidingSpotList (p_firstGreaterOrEqual, numGreaterOrEqual);
	quicksortHidingSpotList (p_firstLess, numLess);

	// Merge the sub lists, greater then lesser
	if (p_firstGreaterOrEqual != NULL)
	{
		p_cursor = p_firstGreaterOrEqual;
		while (p_cursor->p_next != NULL)
		{
			p_cursor = p_cursor->p_next;
		}

		// Join lists
		p_cursor->p_next = p_firstLess;

		// First is first in greater and equal list
		inout_p_firstNode = p_firstGreaterOrEqual;
	}
	else
	{
		inout_p_firstNode = p_firstLess;
	}

	// Done
	
}

//---------------------------------------------------------------------------------------------------

void CDarkmodHidingSpotTree::quicksortAreaList
(
	TDarkmodHidingSpotAreaNode*& inout_p_firstNode,
	unsigned long numAreas
)
{
	// If list is empty or only one node long , we are done
	if (inout_p_firstNode == NULL)
	{
		return;
	}
	else if (inout_p_firstNode->p_nextSibling == NULL)
	{
		return;
	}

	// We use median point as pivote
	TDarkmodHidingSpotAreaNode* p_pivot = inout_p_firstNode;
	for (unsigned long rideCount = 0; rideCount < (numAreas/2); rideCount ++)
	{
		p_pivot = p_pivot->p_nextSibling;
		if (p_pivot == NULL)
		{
			// Crap, list is broken, can't sort
			return;
		}
	}

	// Make two sub lists, those greater than pivot and those less than pivot.
	// We'll put equal into the greater list
	TDarkmodHidingSpotAreaNode* p_firstGreaterOrEqual = NULL;
	TDarkmodHidingSpotAreaNode* p_firstLess = NULL;
	unsigned long numGreaterOrEqual = 0;
	unsigned long numLess = 0;

	TDarkmodHidingSpotAreaNode* p_cursor = inout_p_firstNode;
	while (p_cursor != NULL)
	{
		if (p_cursor->bestSpotQuality >= p_pivot->bestSpotQuality)
		{
			p_cursor->p_nextSibling = p_firstGreaterOrEqual;
			p_firstGreaterOrEqual = p_cursor;
			numGreaterOrEqual ++;
		}	
		else
		{
			p_cursor->p_nextSibling = p_firstLess;
			p_firstLess = p_cursor;
			numLess ++;
		}

		p_cursor = p_cursor->p_nextSibling;
	}

	// Sort each sub list
	quicksortAreaList (p_firstGreaterOrEqual, numGreaterOrEqual);
	quicksortAreaList (p_firstLess, numLess);

	// Merge the sub lists, greater then lesser
	if (p_firstGreaterOrEqual != NULL)
	{
		p_cursor = p_firstGreaterOrEqual;
		while (p_cursor->p_nextSibling != NULL)
		{
			p_cursor = p_cursor->p_nextSibling;
		}

		// Join lists
		p_cursor->p_nextSibling = p_firstLess;

		// First is first in greater and equal list
		inout_p_firstNode = p_firstGreaterOrEqual;
	}
	else
	{
		inout_p_firstNode = p_firstLess;
	}

	// Done

}



