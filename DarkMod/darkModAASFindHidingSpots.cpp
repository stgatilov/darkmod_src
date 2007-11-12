/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include "darkModAASFindHidingSpots.h"
#include "DarkModGlobals.h"
#include "darkModLAS.h"
#include "../sys/sys_public.h"

#define HIDE_GRID_SPACING 40.0

// Quality of a hiding spot ranges from 0.0 (HIDING_SPOT_MAX_LIGHT_QUOTIENT) to 1.0 (pitch black)
#define OCCLUSION_HIDING_SPOT_QUALITY 1.0

// The distance at which hiding spots will be combined if they have the same "type" properties
#define HIDING_SPOT_COMBINATION_DISTANCE 100.0f

// When doing the octant sub-division of the tree at the end, this is the minimum number of points
// in an octant to require it to be subdivided.
#define NUM_POINTS_PER_AREA_FOR_SUBDIVISION 8

// This is the distance inward from an AAS edge to move test points, so that we don't test inside objects or other
// walls
#define WALL_MARGIN_SIZE 1.0

// Static member for debugging hiding spot results
idList<darkModHidingSpot> darkModAASFindHidingSpots::DebugDrawList;


//----------------------------------------------------------------------------

darkModAASFindHidingSpots::darkModAASFindHidingSpots()
{
	// Default value
	hidingSpotRedundancyDistance = 50.0;

	// Start empty
	h_hideFromPVS.i = -1;
	h_hideFromPVS.h = 0;

	// Remember the hide form position
	hideFromPosition = vec3_origin;

	// Set search parameters
	p_aas = NULL;
	hidingHeight = 0;
	searchLimits[0] = vec3_origin;
	searchLimits[1] = vec3_origin;
	searchCenter = vec3_origin;
	searchRadius = 0.0;
	hidingSpotTypesAllowed = 0;
	p_ignoreEntity = NULL;
	lastProcessingFrameNumber = -1;

	// No hiding spot PVS areas identified yet
	numPVSAreas = 0;
	numPVSAreasIterated = 0;

	numHideFromPVSAreas = 0;
}

//----------------------------------------------------------------------------

// Constructor
darkModAASFindHidingSpots::darkModAASFindHidingSpots
(
	const idVec3 &hideFromPos, 
	//idAAS* in_p_aas, 
	float in_hidingHeight,
	idBounds in_searchLimits, 
	idBounds in_searchExcludeLimits, 
	int in_hidingSpotTypesAllowed, 
	idEntity* in_p_ignoreEntity
)
{

	// Default value
	hidingSpotRedundancyDistance = 50.0;

	// Start empty
	h_hideFromPVS.i = -1;
	h_hideFromPVS.h = 0;

	// Remember the hide form position
	hideFromPosition = hideFromPos;

	// Get the aas from the LAS
	p_aas = gameLocal.GetAAS (LAS.getAASName());
	if (p_aas == NULL)
	{
		DM_LOG(LC_AI, LT_ERROR).LogString("AAS with name %s not found\n", LAS.getAASName().c_str());
	}

	// Set search parameters
	hidingHeight = in_hidingHeight;
	searchLimits = in_searchLimits;
	searchIgnoreLimits = in_searchExcludeLimits;
	searchCenter = searchLimits.GetCenter();
	searchRadius = searchLimits.GetRadius();
	hidingSpotTypesAllowed = in_hidingSpotTypesAllowed;
	p_ignoreEntity = in_p_ignoreEntity;
	lastProcessingFrameNumber = -1;

	// No hiding spot PVS areas identified yet
	numPVSAreas = 0;
	numPVSAreasIterated = 0;

	// Have the PVS system identify locations containing the hide from position
	hideFromPVSAreas[0] = gameLocal.pvs.GetPVSArea(hideFromPosition);
	numHideFromPVSAreas = 1;

    // Setup our local copy of the pvs node graph
	h_hideFromPVS = gameLocal.pvs.SetupCurrentPVS
	(
		hideFromPVSAreas, 
		numHideFromPVSAreas 
	);

}


//----------------------------------------------------------------------------

bool darkModAASFindHidingSpots::initialize
(
	const idVec3 &hideFromPos , 
	//idAAS* in_p_aas, 
	float in_hidingHeight,
	idBounds in_searchLimits,
	idBounds in_searchIgnoreLimits,
	int in_hidingSpotTypesAllowed, 
	idEntity* in_p_ignoreEntity
)
{
	// Be certain we free our PVS node graph
	if ((h_hideFromPVS.h != 0) || (h_hideFromPVS.i != -1))
	{
		gameLocal.pvs.FreeCurrentPVS( h_hideFromPVS );
		h_hideFromPVS.h = 0;
		h_hideFromPVS.i = -1;
	}

	// Remember the hide form position
	hideFromPosition = hideFromPos;

	// Get the aas from teh LAS
	p_aas = gameLocal.GetAAS (LAS.getAASName());
	if (p_aas == NULL)
	{
		DM_LOG(LC_AI, LT_ERROR).LogString("AAS with name %s not found\n", LAS.getAASName().c_str());
		return false;
	}

	// Set search parameters
	hidingHeight = in_hidingHeight;
	searchLimits = in_searchLimits;
	searchIgnoreLimits = in_searchIgnoreLimits;
	searchCenter = searchLimits.GetCenter();
	searchRadius = searchLimits.GetRadius();
	hidingSpotTypesAllowed = in_hidingSpotTypesAllowed;
	p_ignoreEntity = in_p_ignoreEntity;
	lastProcessingFrameNumber = -1;

	// No hiding spot PVS areas identified yet
	numPVSAreas = 0;
	numPVSAreasIterated = 0;

	// Have the PVS system identify locations containing the hide from position
	hideFromPVSAreas[0] = gameLocal.pvs.GetPVSArea(hideFromPosition);
	numHideFromPVSAreas = 1;

    // Setup our local copy of the pvs node graph
	h_hideFromPVS = gameLocal.pvs.SetupCurrentPVS
	(
		hideFromPVSAreas, 
		numHideFromPVSAreas 
	);
	
	// Done
	return true;
}

//----------------------------------------------------------------------------

// Destructor
darkModAASFindHidingSpots::~darkModAASFindHidingSpots(void)
{

	// Be certain we free our PVS node graph
	if ((h_hideFromPVS.h != 0) || (h_hideFromPVS.i != -1))
	{
		gameLocal.pvs.FreeCurrentPVS( h_hideFromPVS );
		h_hideFromPVS.h = 0;
		h_hideFromPVS.i = -1;
	}
}


void darkModAASFindHidingSpots::Save( idSaveGame *savefile ) const
{
	savefile->WriteFloat(hidingSpotRedundancyDistance);
	savefile->WriteInt(static_cast<int>(searchState));
	savefile->WriteVec3(hideFromPosition);

	for (int i = 0; i < idEntity::MAX_PVS_AREAS; i++)
	{
		savefile->WriteInt(hideFromPVSAreas[i]);
	}

	savefile->WriteInt(numHideFromPVSAreas);
	
	savefile->WriteInt(h_hideFromPVS.i);
	savefile->WriteUnsignedInt(h_hideFromPVS.h);

	// PVS areas we need to test as good hiding spots
	savefile->WriteInt(numPVSAreas);

	for (int i = 0; i < idEntity::MAX_PVS_AREAS; i++)
	{
		savefile->WriteInt(PVSAreas[i]);
	}
	
	savefile->WriteInt(numPVSAreasIterated);

	savefile->WriteInt(aasAreaIndices.Num());
	for (int i = 0; i < aasAreaIndices.Num(); i++)
	{
		savefile->WriteInt(aasAreaIndices[i]);
	}
	
	savefile->WriteInt(numAASAreaIndicesSearched);
    
	// p_aas gets restored using the LAS singleton class

	savefile->WriteFloat(hidingHeight);
	savefile->WriteBounds(searchLimits);
	savefile->WriteVec3(searchCenter);
	savefile->WriteFloat(searchRadius);
	savefile->WriteBounds(searchIgnoreLimits);
	savefile->WriteInt(hidingSpotTypesAllowed);

	p_ignoreEntity.Save(savefile);

	savefile->WriteInt(lastProcessingFrameNumber);

	savefile->WriteInt(currentGridSearchAASAreaNum);
	savefile->WriteBounds(currentGridSearchBounds);
	savefile->WriteVec3(currentGridSearchBoundMins);
	savefile->WriteVec3(currentGridSearchBoundMaxes);
	savefile->WriteVec3(currentGridSearchPoint);
}

void darkModAASFindHidingSpots::Restore( idRestoreGame *savefile )
{
	savefile->ReadFloat(hidingSpotRedundancyDistance);

	int tempInt;
	savefile->ReadInt(tempInt);
	searchState = static_cast<TDarkModHidingSpotSearchState>(tempInt);

	savefile->ReadVec3(hideFromPosition);

	for (int i = 0; i < idEntity::MAX_PVS_AREAS; i++)
	{
		savefile->ReadInt(hideFromPVSAreas[i]);
	}

	savefile->ReadInt(numHideFromPVSAreas);
	
	savefile->ReadInt(h_hideFromPVS.i);
	savefile->ReadUnsignedInt(h_hideFromPVS.h);

	// PVS areas we need to test as good hiding spots
	savefile->ReadInt(numPVSAreas);

	for (int i = 0; i < idEntity::MAX_PVS_AREAS; i++)
	{
		savefile->ReadInt(PVSAreas[i]);
	}
	
	savefile->ReadInt(numPVSAreasIterated);

	int num;
	savefile->ReadInt(num);
	aasAreaIndices.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		savefile->ReadInt(aasAreaIndices[i]);
	}
	
	savefile->ReadInt(numAASAreaIndicesSearched);
    
	// Restore the AAS pointer from the LAS singleton
	p_aas = gameLocal.GetAAS(LAS.getAASName());

	savefile->ReadFloat(hidingHeight);
	savefile->ReadBounds(searchLimits);
	savefile->ReadVec3(searchCenter);
	savefile->ReadFloat(searchRadius);
	savefile->ReadBounds(searchIgnoreLimits);
	savefile->ReadInt(hidingSpotTypesAllowed);

	p_ignoreEntity.Restore(savefile);

	savefile->ReadInt(lastProcessingFrameNumber);

	savefile->ReadInt(currentGridSearchAASAreaNum);
	savefile->ReadBounds(currentGridSearchBounds);
	savefile->ReadVec3(currentGridSearchBoundMins);
	savefile->ReadVec3(currentGridSearchBoundMaxes);
	savefile->ReadVec3(currentGridSearchPoint);
}

//-------------------------------------------------------------------------------------------------------

bool darkModAASFindHidingSpots::findMoreHidingSpots
(
	//idList<darkModHidingSpot>& inout_hidingSpots,
	CDarkmodHidingSpotTree& inout_hidingSpots,
	int numPointsToTestThisPass,
	int& inout_numPointsTestedThisPass
)
{

	// Holds the center of an AAS area during testing
	idVec3	areaCenter;

	// Make sure search wasn't destroyed
	if ((h_hideFromPVS.h == 0) && (h_hideFromPVS.i == -1))
	{
		// Search was destroyed, there are no more hiding spots
		return false;
	}

	// Branch based on state until search is done or we have tested enough points this pass
	bool b_searchNotDone = (searchState != done_searchState);
	while ((b_searchNotDone) && (inout_numPointsTestedThisPass < numPointsToTestThisPass))
	{
		if (searchState == newPVSArea_searchState)
		{
			b_searchNotDone = testNewPVSArea 
			(
				inout_hidingSpots,
				numPointsToTestThisPass,
				inout_numPointsTestedThisPass
			);
		}
		else if (searchState == iteratingNonVisibleAASAreas_searchState)
		{
			b_searchNotDone = testingAASAreas_InNonVisiblePVSArea
			(
				inout_hidingSpots,
				numPointsToTestThisPass,
				inout_numPointsTestedThisPass
			);
		}
		else if (searchState == iteratingVisibleAASAreas_searchState)
		{
			b_searchNotDone = testingAASAreas_InVisiblePVSArea
			(
				inout_hidingSpots,
				numPointsToTestThisPass,
				inout_numPointsTestedThisPass
			);
		}
		else if (searchState == testingInsideVisibleAASArea_searchState)
		{
			b_searchNotDone = testingInsideVisibleAASArea
			(
				inout_hidingSpots,
				numPointsToTestThisPass,
				inout_numPointsTestedThisPass
			);
		}
		else if (searchState == done_searchState)
		{
			b_searchNotDone = false;
		}
	}

	// Done
	return b_searchNotDone;
}

//-------------------------------------------------------------------------------------------------------

bool darkModAASFindHidingSpots::testNewPVSArea 
(
	//idList<darkModHidingSpot>& inout_hidingSpots,
	CDarkmodHidingSpotTree& inout_hidingSpots,
	int numPointsToTestThisPass,
	int& inout_numPointsTestedThisPass
)
{

	// Loop until we change states (go to test inside a PVS area)
 	while (searchState == newPVSArea_searchState)
	{

		// Test if all PVS areas have been iterated
		if (numPVSAreasIterated >= numPVSAreas)
		{
			// Search is done
			// Combine redundant hiding spots
			CombineRedundantHidingSpots ( inout_hidingSpots, HIDING_SPOT_COMBINATION_DISTANCE);
			searchState = done_searchState;

			return false;
		}

		DM_LOG(LC_AI, LT_DEBUG).LogString("Testing PVS area %d, which is %d out of %d in the set\r", PVSAreas[numPVSAreasIterated], numPVSAreasIterated+1, numPVSAreas);

		// Our current PVS given by h_hidePVS holds the list of areas visible from
		// the "hide from" point.
		// If the area is not in our h_hidePVS set, then it cannot be seen, and it is 
		// thus considered hidden.
		if ( !gameLocal.pvs.InCurrentPVS( h_hideFromPVS, PVSAreas[numPVSAreasIterated]) )
		{

			// Only put these in here if PVS based hiding spots are allowed
			if ((hidingSpotTypesAllowed & PVS_AREA_HIDING_SPOT_TYPE) != 0)
			{
				// Get AAS areas in this visible PVS area
				aasAreaIndices.Clear();
				LAS.pvsToAASMappingTable.getAASAreasForPVSArea (PVSAreas[numPVSAreasIterated], aasAreaIndices);
				DM_LOG(LC_AI, LT_DEBUG).LogString("Non-visible PVS area %d contains %d AAS areas\r", PVSAreas[numPVSAreasIterated], aasAreaIndices.Num());

				// None searched yet
				numAASAreaIndicesSearched = 0;

				// Now searching AAS areas in non-visible PVS area
				searchState = iteratingNonVisibleAASAreas_searchState;

			}
			else
			{
				// Finished searching this PVS area
				// On to next PVS area
				numPVSAreasIterated ++;
			}

		} // end non-visible pvs area
		else
		{
			// PVS area is visible, get its AAS areas
			aasAreaIndices.Clear();
			LAS.pvsToAASMappingTable.getAASAreasForPVSArea (PVSAreas[numPVSAreasIterated], aasAreaIndices);
			DM_LOG(LC_AI, LT_DEBUG).LogString("Visible PVS area %d contains %d AAS areas\r", PVSAreas[numPVSAreasIterated], aasAreaIndices.Num());

			// None searched yet
			numAASAreaIndicesSearched = 0;

			// Now searching AAS areas in visible PVS area
			searchState = iteratingVisibleAASAreas_searchState;


		}
	}

	// More to do
	return true;

}

//-------------------------------------------------------------------------------------------------------

bool darkModAASFindHidingSpots::testingAASAreas_InNonVisiblePVSArea
(
	//idList<darkModHidingSpot>& inout_hidingSpots,
	CDarkmodHidingSpotTree& inout_hidingSpots,
	int numPointsToTestThisPass,
	int& inout_numPointsTestedThisPass
)
{
	for (; numAASAreaIndicesSearched < aasAreaIndices.Num(); numAASAreaIndicesSearched ++)
	{
		int aasAreaIndex = aasAreaIndices[numAASAreaIndicesSearched];

		// No aas area in hiding spot tree yet
		TDarkmodHidingSpotAreaNode* p_hidingAreaNode = NULL;
	

		// This whole area is not visible
		// Add its center and we are done
		darkModHidingSpot hidingSpot;
		hidingSpot.goal.areaNum = aasAreaIndex;
		hidingSpot.goal.origin = p_aas->AreaCenter(aasAreaIndex);
		hidingSpot.hidingSpotTypes = PVS_AREA_HIDING_SPOT_TYPE;

		// Since there is total occlusion, base quality on the distance from the center
		// of the search compared to the total search radius
		float distanceFromCenter = (searchCenter - hidingSpot.goal.origin).Length();
		if (searchRadius > 0.0)
		{
			// Use power of 2 fallof
			hidingSpot.quality = (searchRadius - distanceFromCenter) / searchRadius;
			hidingSpot.quality *= hidingSpot.quality;
			if (hidingSpot.quality < 0.0)
			{
				hidingSpot.quality = (float) 0.0;
			}
		}
		else
		{
			hidingSpot.quality = (float) 0.1;
		}

		// Test if it is inside the exclusion bounds
		if (searchIgnoreLimits.ContainsPoint (hidingSpot.goal.origin))
		{
			hidingSpot.quality = -1.0;
		}

		// Insert if it is any good
		if (hidingSpot.quality > 0.0)
		{
			// ensure area index is in hiding spot tree
			if (p_hidingAreaNode == NULL)
			{
				p_hidingAreaNode = inout_hidingSpots.getArea
				(
					aasAreaIndex
				);
				if (p_hidingAreaNode == NULL)
				{
					p_hidingAreaNode = inout_hidingSpots.insertArea(aasAreaIndex);
					if (p_hidingAreaNode == NULL)
					{
						return false;
					}
				}
			}
			
			// Add spot under this index in the hiding spot tree
			inout_hidingSpots.insertHidingSpot
			(
				p_hidingAreaNode, 
				hidingSpot.goal, 
				hidingSpot.hidingSpotTypes,
				hidingSpot.lightQuotient,
				hidingSpot.qualityWithoutDistanceFactor,
				hidingSpot.quality,
				hidingSpotRedundancyDistance
			);
			
			DM_LOG(LC_AI, LT_DEBUG).LogString("Hiding spot added for PVS non-visible area %d, AAS area %d, quality \r", PVSAreas[numPVSAreasIterated], hidingSpot.goal.areaNum);
		}

		// This counts as a point tested
		inout_numPointsTestedThisPass ++;
		if (inout_numPointsTestedThisPass >= numPointsToTestThisPass)
		{
			// This area was iterated (we aren't looping around to do this)
			if (numAASAreaIndicesSearched < aasAreaIndices.Num() -1)
			{
				// Filled point quota, but need to keep searching this AAS list next time
				return true;
			}
		}

	} // Loop to next AAS area in the PVS area we are testing

	// Done searching AAS areas in this PVS area
	aasAreaIndices.Clear();
	numAASAreaIndicesSearched = 0;

	// Finished this PVS area
	numPVSAreasIterated ++;

	// On to next PVS area
	searchState = newPVSArea_searchState;

	// Potentially more PVS areas to search
	return true;

}

//-------------------------------------------------------------------------------------------------------

bool darkModAASFindHidingSpots::testingAASAreas_InVisiblePVSArea 
(
	//idList<darkModHidingSpot>& inout_hidingSpots,
	CDarkmodHidingSpotTree& inout_hidingSpots,
	int numPointsToTestThisPass,
	int& inout_numPointsTestedThisPass
)
{

	// The PVS area is visible through the PVS system.
	
	// Iterate the aas area indices
	for (; numAASAreaIndicesSearched < aasAreaIndices.Num(); numAASAreaIndicesSearched ++)
	{
		// Get AAS area index for this AAS area
		int aasAreaIndex = aasAreaIndices[numAASAreaIndicesSearched];

		// Check area flags
		int areaFlags = p_aas->AreaFlags (aasAreaIndex);


		if ((areaFlags & AREA_REACHABLE_WALK) != 0)
		{
			// Initialize grid search for inside visible AAS area
			idBounds currentAASAreaBounds = p_aas->GetAreaBounds (aasAreaIndex);
			currentGridSearchBounds = searchLimits.Intersect (currentAASAreaBounds);
			currentGridSearchAASAreaNum = aasAreaIndex;
			currentGridSearchBoundMins = currentGridSearchBounds[0];
			currentGridSearchBoundMaxes = currentGridSearchBounds[1];
			currentGridSearchPoint = currentGridSearchBoundMins;
			currentGridSearchPoint.x += WALL_MARGIN_SIZE;
			

			// We are now searching for hiding spots inside a visible AAS area
			searchState = testingInsideVisibleAASArea_searchState;

			// There is more to do
			return true; 
		}

		// See if we have filled our point quota
		if (inout_numPointsTestedThisPass >= numPointsToTestThisPass)
		{
			// This area was iterated (we aren't looping around to do this)
			if (numAASAreaIndicesSearched < aasAreaIndices.Num() -1)
			{
				// Filled point quota, but need to keep searching this AAS list next time
				return true;
			}
		}
	
	} // loop to next AAS area in this PVS area

	// Done searching AAS areas in this PVS area
	aasAreaIndices.Clear();
	numAASAreaIndicesSearched = 0;

	// Finished this PVS area
	numPVSAreasIterated ++;

	// On to next PVS area
	searchState = newPVSArea_searchState;

	// Potentially more PVS areas to search
	return true;
}

//-------------------------------------------------------------------------------------------------------

bool darkModAASFindHidingSpots::testingInsideVisibleAASArea
(
	//idList<darkModHidingSpot>& inout_hidingSpots,
	CDarkmodHidingSpotTree& inout_hidingSpots,
	int numPointsToTestThisPass,
	int& inout_numPointsTestedThisPass
)
{

	//idVec3 areaCenter = aas->AreaCenter (AASAreaNum);

	// Get search area properties
	idVec3 searchCenter = searchLimits.GetCenter();
	float searchRadius = searchLimits.GetRadius();

	// Iterate a gridding within these bounds
	float hideSearchGridSpacing = HIDE_GRID_SPACING;
	
	// Iterate the coordinates to search
	// We don't use for loops here so that we can control the end of the iteration
	// to check up against the boundary regardless of divisibility

	// No hiding spot area node yet used
	TDarkmodHidingSpotAreaNode* p_hidingAreaNode = NULL;

	// Iterate X grid
	while (currentGridSearchPoint.x <= (currentGridSearchBoundMaxes.x - (WALL_MARGIN_SIZE + 0.1) ))
	{
		while (currentGridSearchPoint.y <= (currentGridSearchBoundMaxes.y - (WALL_MARGIN_SIZE + 0.1) ) )
		{
			// See if we have filled our point quota
			if (inout_numPointsTestedThisPass >= numPointsToTestThisPass)
			{
				// Filled point quota, but we need to keep iterating this grid next time
				return true;
			}

			// For now, only consider top of floor
			currentGridSearchPoint.z = currentGridSearchBoundMaxes.z + WALL_MARGIN_SIZE;

			darkModHidingSpot hidingSpot;

			// Test if it is inside the exclusion bounds
			if (searchIgnoreLimits.ContainsPoint (currentGridSearchPoint))
			{
				hidingSpot.quality = -1.0;
				hidingSpot.hidingSpotTypes = NONE_HIDING_SPOT_TYPE;
			}
			else
			{
				// Not inside exclusion bounds, must test it
				hidingSpot.hidingSpotTypes = TestHidingPoint 
				(
					currentGridSearchPoint, 
					searchCenter,
					searchRadius,
					hidingHeight,
					hidingSpotTypesAllowed,
					p_ignoreEntity.GetEntity(),
					hidingSpot.lightQuotient,
					hidingSpot.qualityWithoutDistanceFactor,
					hidingSpot.quality
				);
			}

			// If there are any hiding qualities, insert a hiding spot
			if 
			(
				(hidingSpot.hidingSpotTypes != NONE_HIDING_SPOT_TYPE) && 
				(hidingSpot.quality > 0.0)
			)
			{
				// Insert a hiding spot for this test point
				hidingSpot.goal.areaNum = currentGridSearchAASAreaNum;
				hidingSpot.goal.origin = currentGridSearchPoint;

				// ensure area index is in hiding spot tree
				if (p_hidingAreaNode == NULL)
				{
					p_hidingAreaNode = inout_hidingSpots.getArea
					(
						currentGridSearchAASAreaNum
					);
					if (p_hidingAreaNode == NULL)
					{
						p_hidingAreaNode = inout_hidingSpots.insertArea(currentGridSearchAASAreaNum);
						if (p_hidingAreaNode == NULL)
						{
							return false;
						}
					}
				}
				
				// Add spot under this index in the hiding spot tree
				inout_hidingSpots.insertHidingSpot
				(
					p_hidingAreaNode, 
					hidingSpot.goal, 
					hidingSpot.hidingSpotTypes,
					hidingSpot.lightQuotient,
					hidingSpot.qualityWithoutDistanceFactor,
					hidingSpot.quality,
					hidingSpotRedundancyDistance
				);

				DM_LOG(LC_AI, LT_DEBUG).LogString("Found hiding spot within AAS area %d at (X:%f, Y:%f, Z:%f) with type bitflags %d, quality %f\r", currentGridSearchAASAreaNum, currentGridSearchPoint.x, currentGridSearchPoint.y, currentGridSearchPoint.z, hidingSpot.hidingSpotTypes, hidingSpot.quality);
			}

			// One more point tested
			inout_numPointsTestedThisPass ++;

			// Increase search coordinate. Ensure we search along bounds, which might be a
			// wall or other cover providing surface.
			if ((currentGridSearchPoint.y < (currentGridSearchBoundMaxes.y-WALL_MARGIN_SIZE) ) && ( (currentGridSearchPoint.y + hideSearchGridSpacing) > (currentGridSearchBoundMaxes.y - WALL_MARGIN_SIZE ) ) )
			{
				currentGridSearchPoint.y = currentGridSearchBoundMaxes.y - WALL_MARGIN_SIZE;
			}
			else
			{
				currentGridSearchPoint.y += hideSearchGridSpacing;
			}



		} // Y iteration

		// Increase search coordinate. Ensure we search along bounds, which might be a
		// wall or other cover providing surface.
		if ((currentGridSearchPoint.x < (currentGridSearchBoundMaxes.x- WALL_MARGIN_SIZE) ) && ( (currentGridSearchPoint.x + hideSearchGridSpacing) > (currentGridSearchBoundMaxes.x - WALL_MARGIN_SIZE) ) )
		{
			currentGridSearchPoint.x = currentGridSearchBoundMaxes.x - WALL_MARGIN_SIZE;
		}
		else
		{
			currentGridSearchPoint.x += hideSearchGridSpacing;
		}

		// Reset y iteration
		currentGridSearchPoint.y = currentGridSearchBoundMins.y + WALL_MARGIN_SIZE;

	} // X iteration

	DM_LOG(LC_AI, LT_DEBUG).LogString("Finished hide grid iteration for AAS area %d\r", currentGridSearchAASAreaNum);

	// One more AAS area searched
	numAASAreaIndicesSearched ++;

	// Go back to iterating the list of AAS areas in this visible PVS area
	searchState = iteratingVisibleAASAreas_searchState;

	// There may be more searching to do
	return true;

}

//----------------------------------------------------------------------------

// Internal helper
int darkModAASFindHidingSpots::TestHidingPoint 
(
	idVec3 testPoint, 
	idVec3 searchCenter,
	float searchRadius,
	float hidingHeight,
	int hidingSpotTypesAllowed, 
	idEntity* p_ignoreEntity,
	float& out_lightQuotient,
	float& out_qualityWithoutDistance,
	float& out_quality
)
{
	int out_hidingSpotTypesThatApply = NONE_HIDING_SPOT_TYPE;
	out_quality = 0.0f; // none found yet

	idVec3 testLineTop = testPoint;
	testLineTop.z += hidingHeight;

	// Is it dark?
	if ((hidingSpotTypesAllowed & DARKNESS_HIDING_SPOT_TYPE) != 0)
	{
		// Test the lighting level of this position
		//DM_LOG(LC_AI, LT_DEBUG).LogString("Testing hiding-spot lighting at point %f,%f,%f\n", testPoint.x, testPoint.y, testPoint.z);


		out_lightQuotient = LAS.queryLightingAlongLine 
		(
			testPoint,
			testLineTop,
			p_ignoreEntity,
			true
		);

		//DM_LOG(LC_AI, LT_DEBUG).LogString("Done testing hiding-spot lighting at point %f,%f,%f\n", testPoint.x, testPoint.y, testPoint.z);
		if ((out_lightQuotient < g_Global.m_hidingSpotMaxLightQuotient) && (out_lightQuotient >= 0.0))
		{
			//DM_LOG(LC_AI, LT_DEBUG).LogString("Found hidable darkness of %f at point %f,%f,%f\n", LightQuotient, testPoint.x, testPoint.y, testPoint.z);
			out_hidingSpotTypesThatApply |= DARKNESS_HIDING_SPOT_TYPE;

			float darknessQuality = 0.0;
			darknessQuality = (g_Global.m_hidingSpotMaxLightQuotient - out_lightQuotient) / g_Global.m_hidingSpotMaxLightQuotient;
			darknessQuality *= 2.0; // Experimental tweak to make it really focus on dark spots
			if (darknessQuality > out_quality)
			{
				out_quality = darknessQuality;
			}

		}
	}

	// Does a ray to the test point from the hide from point get occluded?
	if ((hidingSpotTypesAllowed & VISUAL_OCCLUSION_HIDING_SPOT_TYPE) != 0)
	{
		//idVec3 fakePoint = hideFromPosition;
		//fakePoint.z -= 5.0f;

		// Check a point above the test point to account for the size
		// of a hiding object. Generally, we use the "top" of the hiding
		// object size, because AI's don't expect something to hang
		// from the back of the occluder and pull its feet upward.
		idVec3 occlusionTestPoint = testPoint;
		occlusionTestPoint.z += hidingHeight;


		trace_t rayResult;
		//DM_LOG(LC_AI, LT_DEBUG).LogString("Testing hiding-spot occlusion at point %f,%f,%f\n", testPoint.x, testPoint.y, testPoint.z);
		if (gameLocal.clip.TracePoint 
		(
			rayResult, 
			hideFromPosition,
			testPoint,
			//MASK_SOLID | MASK_WATER | MASK_OPAQUE,
			MASK_SOLID,
			NULL
		))
		{
			// Some sort of occlusion
			//DM_LOG(LC_AI, LT_DEBUG).LogString("Found hiding-spot occlusion at point %f,%f,%f, fraction of %f\n", testPoint.x, testPoint.y, testPoint.z, rayResult.fraction);
			out_hidingSpotTypesThatApply |= VISUAL_OCCLUSION_HIDING_SPOT_TYPE;

			// Occlusions are 50% good
			if (out_quality < OCCLUSION_HIDING_SPOT_QUALITY)
			{
				out_quality = OCCLUSION_HIDING_SPOT_QUALITY;
			}
		}
		//DM_LOG(LC_AI, LT_DEBUG).LogString("Done testing hiding-spot occlusion at point %f,%f,%f\n", testPoint.x, testPoint.y, testPoint.z);
	}


	// Modify by random factor to prevent all searches in same location from being
	// similar
	out_quality += ((gameLocal.random.CRandomFloat() * out_quality) / 3.0);
	if (out_quality > 1.0)
	{
		out_quality = 1.0;
	}
	else if (out_quality < 0.0)
	{
		out_quality = 0.0;
	}

	// Record quality without distance
	out_qualityWithoutDistance = out_quality;

	// Reduce quality by distance from search center
	float distanceFromCenter = (searchCenter - testPoint).Length();
	if ((searchRadius > 0.0) && (out_quality > 0.0))
	{
		float falloff = ((searchRadius - distanceFromCenter) / searchRadius);
		// Use power of 2 fallof
		out_quality = out_quality * falloff * falloff;
		if (out_quality < 0.0)
		{
			out_quality = 0.0;
		}
	}
	else
	{
		out_quality = 0.0;
	}


	// Done
	//DM_LOG(LC_AI, LT_DEBUG).LogString("Done testing for hidability at point %f,%f,%f\n", testPoint.x, testPoint.y, testPoint.z);
	return out_hidingSpotTypesThatApply;
}

//----------------------------------------------------------------------------

/*
void darkModAASFindHidingSpots::insertHidingSpotWithQualitySorting
(
	darkModHidingSpot& hidingSpot,
	//idList<darkModHidingSpot>& inout_hidingSpots
	CDarkmodHidingSpotTree& inout_hidingSpots
)
{
	// Find the right place
	int numSpots = inout_hidingSpots.Num();

	int spotIndex = 0;
	for (spotIndex = 0; spotIndex < numSpots; spotIndex ++)
	{
		if (inout_hidingSpots[spotIndex].quality <= hidingSpot.quality)
		{
			// Insert it before this spot (at this spot and move all rest down)
			break;
		}
	}

	// Do insertion
	inout_hidingSpots.Insert (hidingSpot, spotIndex);
	
}
*/

//----------------------------------------------------------------------------

void darkModAASFindHidingSpots::CombineRedundantHidingSpots
(
	//idList<darkModHidingSpot>& inout_hidingSpots,
	CDarkmodHidingSpotTree& inout_hidingSpots,
	float distanceAtWhichToCombine
)
{
	//idList<darkModHidingSpot> consolidatedList;
	/*
	int listLength = inout_hidingSpots.Num();

	for (int index = 0; index < listLength; index ++)
	{
		// Get the hiding spot
		darkModHidingSpot spot = inout_hidingSpots[index];

		// compare with other hiding spots later in the list
		for (int otherIndex = index+1; otherIndex < listLength; otherIndex ++)
		{
			darkModHidingSpot otherSpot = inout_hidingSpots[otherIndex];
			float distance = abs((spot.goal.origin - otherSpot.goal.origin).Length());
			if ((spot.hidingSpotTypes == otherSpot.hidingSpotTypes) && (distance < distanceAtWhichToCombine))
			{
				// Remove the other spot 
				inout_hidingSpots.RemoveIndex(otherIndex);
				listLength --;

				// A point may have been pulled down into this other index
				otherIndex --;
			}
		}

	}
	*/

}

//----------------------------------------------------------------------------

// Debug functions

void darkModAASFindHidingSpots::debugClearHidingSpotDrawList()
{
	// Clear the list
	darkModAASFindHidingSpots::DebugDrawList.Clear();

}

//----------------------------------------------------------------------------

void darkModAASFindHidingSpots::debugAppendHidingSpotsToDraw 
(
	//const idList<darkModHidingSpot>& hidingSpotsToAppend
	CDarkmodHidingSpotTree& inout_hidingSpots
)
{
	idList<darkModHidingSpot> hidingSpotsToAppend;
	darkModHidingSpot* p_spot;

	unsigned long numSpots = inout_hidingSpots.getNumSpots();

	for (unsigned long spotIndex = 0; spotIndex < numSpots; spotIndex ++)
	{

		p_spot = inout_hidingSpots.getNthSpot (spotIndex);

		darkModHidingSpot spotCopy;
		spotCopy = *p_spot;

		hidingSpotsToAppend.Insert (spotCopy, spotIndex);
	}

	// Append to the list
	darkModAASFindHidingSpots::DebugDrawList.Append (hidingSpotsToAppend);

}

//----------------------------------------------------------------------------

void darkModAASFindHidingSpots::debugDrawHidingSpots(int viewLifetime)
{
	// Set up some depiction values 
	idVec4 DarknessMarkerColor(0.0f, 0.0f, 1.0f, 0.0);
	idVec4 OcclusionMarkerColor(0.0f, 1.0f, 0.0f, 0.0);
	idVec4 PortalMarkerColor(1.0f, 0.0f, 0.0f, 0.0);

	idVec3 markerArrowLength (0.0, 0.0, 25.0f);

	
	// Iterate the hiding spot debug draw list
	size_t spotCount = darkModAASFindHidingSpots::DebugDrawList.Num();
	for (size_t spotIndex = 0; spotIndex < spotCount; spotIndex ++)
	{
		idVec4 markerColor(0.0f, 0.0f, 0.0f, 0.0f);
		
		if ((DebugDrawList[spotIndex].hidingSpotTypes & PVS_AREA_HIDING_SPOT_TYPE) != 0)
		{
			markerColor += PortalMarkerColor;
		}
		
		if ((DebugDrawList[spotIndex].hidingSpotTypes & DARKNESS_HIDING_SPOT_TYPE) != 0)
		{
			markerColor += DarknessMarkerColor;
		}
		
		if ((DebugDrawList[spotIndex].hidingSpotTypes & VISUAL_OCCLUSION_HIDING_SPOT_TYPE) != 0)
		{
			markerColor += OcclusionMarkerColor;
		}

		// Scale from blackness to the color
		for (int i = 0; i < 4; i ++)
		{
			markerColor[i] *= DebugDrawList[spotIndex].quality;
		}

		// Render this hiding spot
		gameRenderWorld->DebugArrow
		(
			markerColor,
			DebugDrawList[spotIndex].goal.origin + markerArrowLength * DebugDrawList[spotIndex].quality,
			DebugDrawList[spotIndex].goal.origin,
			2.0f,
			viewLifetime
		);
	}

	// Done
}

//----------------------------------------------------------------------------

// Test stub
void darkModAASFindHidingSpots::testFindHidingSpots 
(
	idVec3 hideFromLocation, 
	float in_hidingHeight,
	idBounds in_hideSearchBounds, 
	idEntity* in_p_ignoreEntity, 
	idAAS* in_p_aas
)
{

	idBounds emptyExcludeBounds;
	emptyExcludeBounds.Clear();

	darkModAASFindHidingSpots HidingSpotFinder (hideFromLocation, in_hidingHeight, in_hideSearchBounds, emptyExcludeBounds, ANY_HIDING_SPOT_TYPE, in_p_ignoreEntity);
	HidingSpotFinder.searchIgnoreLimits.Clear();

	CDarkmodHidingSpotTree hidingSpotList;

	DM_LOG(LC_AI, LT_DEBUG).LogVector ("Hide search mins", in_hideSearchBounds[0]);
	DM_LOG(LC_AI, LT_DEBUG).LogVector ("Hide search maxes", in_hideSearchBounds[1]);

#define MAX_SPOTS_PER_TEST_ROUND 1000

	bool b_searchContinues;
	b_searchContinues = HidingSpotFinder.startHidingSpotSearch
	(
		hidingSpotList,
		MAX_SPOTS_PER_TEST_ROUND,
		gameLocal.framenum
	);
	while (b_searchContinues)
	{
		b_searchContinues = HidingSpotFinder.continueSearchForHidingSpots
		(
			hidingSpotList,
			MAX_SPOTS_PER_TEST_ROUND,
			gameLocal.framenum
		);
	}

	// Clear the debug list and add these
	darkModAASFindHidingSpots::debugClearHidingSpotDrawList();
	darkModAASFindHidingSpots::debugAppendHidingSpotsToDraw (hidingSpotList);
	darkModAASFindHidingSpots::debugDrawHidingSpots (15000);


	// Done

}

/*
############################################################################################
# Normal public interface
############################################################################################
*/


bool darkModAASFindHidingSpots::isSearchCompleted()
{
	// Make sure search wasn't destroyed
	if ((h_hideFromPVS.h == 0) && (h_hideFromPVS.i == -1))
	{
		// Search was destroyed, search is done
		return true;
	}

	// Doen if in searchDone state
	return  (searchState == done_searchState);
}

//-----------------------------------------------------------------------------

// The search start function
bool darkModAASFindHidingSpots::startHidingSpotSearch
(
	CDarkmodHidingSpotTree& out_hidingSpots,
	int numPointsToTestThisPass,
	int frameNumber
) 
{
	// The number of points this pass
	int numPointsTestedThisPass = 0;

	// Remember last frame that tested points
	lastProcessingFrameNumber = frameNumber;

	// Set search state
	searchState = buildingPVSList_searchState;

	// Ensure the PVS to AAS table is initialized
	// If already initialized, this returns right away.
	if (!LAS.pvsToAASMappingTable.buildMappings("aas32"))
	{
		LAS.pvsToAASMappingTable.buildMappings("aas48");
	}

	// Get the PVS areas intersecting the search bounds
	// Note, the id code below did this by expanding a bound out from the area center, regardless
	// of the size of the area.  This uses our function-local PVSArea array to
	// hold the intersecting PVS Areas.
	numPVSAreas = gameLocal.pvs.GetPVSAreas
	(
		searchLimits, 
		PVSAreas, 
		idEntity::MAX_PVS_AREAS 
	);

	// Haven't iterated any PVS areas yet
	numPVSAreasIterated = 0;
	
	// Iterating PVS areas
	searchState = newPVSArea_searchState;

	// Call the interior function
	if (!findMoreHidingSpots
	(
		out_hidingSpots,
		numPointsToTestThisPass,
		numPointsTestedThisPass
	))
	{
		// Sub divide the tree
		out_hidingSpots.subDivideAreas (NUM_POINTS_PER_AREA_FOR_SUBDIVISION);
		return false;
	}
	else
	{
		// More spots to test
		return true;
	}

}

//-------------------------------------------------------------------------------------------------------

// The search continue function
bool darkModAASFindHidingSpots::continueSearchForHidingSpots
(
	CDarkmodHidingSpotTree& inout_hidingSpots,
	int numPointsToTestThisPass,
	int frameNumber
)
{

	// If we already tested points this frame, don't test any more
	if (frameNumber == lastProcessingFrameNumber)
	{
		return !isSearchCompleted();
	}
	else
	{
		// Remember that we are testing points this frame
		lastProcessingFrameNumber = frameNumber;
	}


	// The number of points this pass
	int numPointsTestedThisPass = 0;

	// Call the interior function
	if (!findMoreHidingSpots
	(
		inout_hidingSpots,
		numPointsToTestThisPass,
		numPointsTestedThisPass
	))
	{
		// Sub divide the tree
		inout_hidingSpots.subDivideAreas (NUM_POINTS_PER_AREA_FOR_SUBDIVISION);
		return false;
	}
	else
	{
		// More spots to test
		return true;
	}

}

