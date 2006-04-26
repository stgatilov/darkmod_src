#include "../idlib/precompiled.h"
#pragma hdrstop

#include ".\darkmodaasfindhidingspots.h"
#include "..\darkmod\darkmodglobals.h"
#include "..\darkmod\darkModLAS.h"

// What amount of light is acceptable for a minimal quality hiding spot
#define HIDING_SPOT_MAX_LIGHT_QUOTIENT 0.05f

// Quality of a hiding spot ranges from 0.0 (HIDING_SPOT_MAX_LIGHT_QUOTIENT) to 1.0 (pitch black)
#define OCCLUSION_HIDING_SPOT_QUALITY 0.5


// Static member for debugging hiding spot results
idList<darkModHidingSpot_t> darkModAASFindHidingSpots::DebugDrawList;


//----------------------------------------------------------------------------

// Constructor
darkModAASFindHidingSpots::darkModAASFindHidingSpots(const idVec3 &hideFromPos, idAAS* p_aas)
{
	/*
	* Note that most of this code is similar to idAASFindCover for now
	*/

	int numPVSAreas = 0;

	// Start empty
	h_hidePVS.i = -1;
	h_hidePVS.h = NULL;

	// Remember the hide form position
	hideFromPosition = hideFromPos;

	// Have the PVS system identify locations containing the hide from position
	PVSAreas[0] = gameLocal.pvs.GetPVSArea(hideFromPosition);
	numPVSAreas = 1;
	DM_LOG(LC_AI, LT_DEBUG).LogString("PVS Setup found %d areas", numPVSAreas);

	// Setup our local copy of the pvs node graph
	h_hidePVS = gameLocal.pvs.SetupCurrentPVS
	(
		PVSAreas, 
		numPVSAreas 
	);

}

//----------------------------------------------------------------------------

// Destructor
darkModAASFindHidingSpots::~darkModAASFindHidingSpots(void)
{
	// Be certain we free our PVS node graph
	if ((h_hidePVS.h != NULL) || (h_hidePVS.i != -1))
	{
		gameLocal.pvs.FreeCurrentPVS( h_hidePVS );
		h_hidePVS.h = NULL;
		h_hidePVS.i = -1;
	}
}

//----------------------------------------------------------------------------

// The primary interface
void darkModAASFindHidingSpots::FindHidingSpots
(
	idList<darkModHidingSpot_t>& inout_hidingSpots, 
	const idAAS *aas, 
	float hidingHeight,
	idBounds searchLimits, 
	int hidingSpotTypesAllowed, 
	idEntity* p_ignoreEntity
) 
{
	// Holds the center of the AAS area
	idVec3	areaCenter;

	// The number of PVS areas held in local testing
	int		numPVSAreas;

	// An array for testing PVS areas
	int		PVSAreas[ idEntity::MAX_PVS_AREAS ];

	// Ensure the PVS to AAS table is initialized
	// If already initialized, this returns right away.
	LAS.pvsToAASMappingTable.buildMappings(0);

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

	// Iterate the PVS areas
	DM_LOG(LC_AI, LT_DEBUG).LogString("Iterating PVS areas, there are %d in the set\n", numPVSAreas);

	for (int pvsResultIndex = 0; pvsResultIndex < numPVSAreas; pvsResultIndex ++)
	{
		
		// Our current PVS given by h_hidePVS holds the list of areas visible from
		// the "hide from" point.
		// If the area is not in our h_hidePVS set, then it cannot be seen, and it is 
		// thus considered hidden.
		if ( !gameLocal.pvs.InCurrentPVS( h_hidePVS, PVSAreas[pvsResultIndex]) )
		{
			// Only put these in here if PVS based hiding spots are allowed
			if ((hidingSpotTypesAllowed & PVS_AREA_HIDING_SPOT_TYPE) != 0)
			{
				// Add a goal for the center of each area within this pvs area
				idList<int> aasAreaIndices;
				LAS.pvsToAASMappingTable.getAASAreasForPVSArea (PVSAreas[pvsResultIndex], aasAreaIndices);

				DM_LOG(LC_AI, LT_DEBUG).LogString("Non-visible PVS area %d contains %d AAS areas\n", PVSAreas[pvsResultIndex], aasAreaIndices.Num());

				for (int ia = 0; ia < aasAreaIndices.Num(); ia ++)
				{
					int aasAreaIndex = aasAreaIndices[ia];

					// This whole area is not visible
					// Add its center and we are done
					darkModHidingSpot_t hidingSpot;
					hidingSpot.goal.areaNum = aasAreaIndex;
					hidingSpot.goal.origin = aas->AreaCenter(aasAreaIndex);
					hidingSpot.hidingSpotTypes = PVS_AREA_HIDING_SPOT_TYPE;
					hidingSpot.quality = 1.0; // Least suspicious assumption
					insertHidingSpotWithQualitySorting (hidingSpot, inout_hidingSpots);
	
					DM_LOG(LC_AI, LT_DEBUG).LogString("Hiding spot added for PVS non-visible area %d, AAS area %d\n", PVSAreas[pvsResultIndex], hidingSpot.goal.areaNum);
				}

			}
		}
		else
		{

			// The area is visible through the PVS system.

			// Test each AAS area within the pvs area
			idList<int> aasAreaIndices;
			LAS.pvsToAASMappingTable.getAASAreasForPVSArea (PVSAreas[pvsResultIndex], aasAreaIndices);

			DM_LOG(LC_AI, LT_DEBUG).LogString("Visible PVS area %d contains %d AAS areas\n", PVSAreas[pvsResultIndex], aasAreaIndices.Num());

			for (int ia = 0; ia < aasAreaIndices.Num(); ia ++)
			{
				// Get AAS area index for this AAS area
				int aasAreaIndex = aasAreaIndices[ia];

				// Check area flags
				int areaFlags = aas->AreaFlags (aasAreaIndex);

				if ((areaFlags & AREA_FLOOR) != 0)
				{
					// This area is traversable by the hiding entity
					// Test for other reasons for hidability such as lighting, visual occlusion etc...
					FindHidingSpotsInVisibleAASArea 
					(
						inout_hidingSpots,
						hidingHeight,
						aas, 
						aasAreaIndex,
						searchLimits,
						hidingSpotTypesAllowed,
						p_ignoreEntity
					);
				}
			}

		} // PVS area is not fully occluded, search within it for local hiding regions

	} // Consider next PVS area within our search limits

	// Done
}

//----------------------------------------------------------------------------

// Internal helper
void darkModAASFindHidingSpots::FindHidingSpotsInVisibleAASArea 
(
	idList<darkModHidingSpot_t>& inout_hidingSpots,
	float hidingHeight,
	const idAAS* aas, 
	int AASAreaNum, 
	idBounds searchLimits,
	int hidingSpotTypesAllowed, 
	idEntity* p_ignoreEntity
)
{
	// The area is visible from the hide-from point. So, we have to do a more complicated
	// search for points of darkness or visual occlusion due to objstacles etc...

	idVec3 areaCenter = aas->AreaCenter (AASAreaNum);
	DM_LOG(LC_AI, LT_DEBUG).LogString("Center of AAS area %d is %f, %f, %f", AASAreaNum, areaCenter.x, areaCenter.y, areaCenter.z);

	// Get the area bounds
	idBounds areaBounds = aas->GetAreaBounds (AASAreaNum);

	// Hiding search bounds is intersection of search bounds and area bounds
	idBounds hidingBounds = searchLimits.Intersect (areaBounds);

	// Iterate a gridding within these bounds
	float hideSearchGridSpacing = 40.0f;
	
	idVec3 boundMins = hidingBounds[0];
	idVec3 boundMaxes = hidingBounds[1];

	DM_LOG(LC_AI, LT_DEBUG).LogString("Iterating hide gridding for AAS area %d,  X:%f to %f, Y:%f to %f, Z:%f to %f \n", AASAreaNum, boundMins.x, boundMaxes.x, boundMins.y, boundMaxes.y, boundMins.z, boundMaxes.z);

	// Iterate the coordinates to search
	// We don't use for loops here so that we can control the end of the iteration
	// to check up against the boundary regardless of divisibility

	// Iterate X grid
	float XPoint = boundMins.x;
	while (XPoint <= boundMaxes.x)
	{
		float YPoint = boundMins.y;
		while (YPoint <= boundMaxes.y)
		{
			// For now, only consider top of floor
			float ZPoint = boundMaxes.z + 1.0;

			// Test the point
			idVec3 testPoint (XPoint, YPoint, ZPoint);

			darkModHidingSpot_t hidingSpot;
			hidingSpot.hidingSpotTypes = TestHidingPoint 
			(
				testPoint, 
				hidingHeight,
				hidingSpotTypesAllowed,
				p_ignoreEntity,
				hidingSpot.quality
			);

			// If there are any hiding qualities, insert a hiding spot
			if (hidingSpot.hidingSpotTypes != NONE_HIDING_SPOT_TYPE)
			{
				// Insert a hiding spot for this test point
				hidingSpot.goal.areaNum = AASAreaNum;
				hidingSpot.goal.origin = hidingSpot.goal.origin = testPoint;
				insertHidingSpotWithQualitySorting (hidingSpot, inout_hidingSpots);
				DM_LOG(LC_AI, LT_DEBUG).LogString("Found hiding spot within AAS area %d at (X:%f, Y:%f, Z:%f) with type bitflags %d\n", AASAreaNum, testPoint.x, testPoint.y, testPoint.z, hidingSpot.hidingSpotTypes);
			}

			// Increase search coordinate. Ensure we search along bounds, which might be a
			// wall or other cover providing surface.
			if ((YPoint < boundMaxes.y) && (YPoint + hideSearchGridSpacing) > (boundMaxes.y))
			{
				YPoint = boundMaxes.y;
			}
			else
			{
				YPoint += hideSearchGridSpacing;
			}

		} // Y iteration

		// Increase search coordinate. Ensure we search along bounds, which might be a
		// wall or other cover providing surface.
		if ((XPoint < boundMaxes.x) && (XPoint + hideSearchGridSpacing) > (boundMaxes.x))
		{
			XPoint = boundMaxes.x;
		}
		else
		{
			XPoint += hideSearchGridSpacing;
		}
		

	} // X iteration

	DM_LOG(LC_AI, LT_DEBUG).LogString("Finished hide grid iteration for AAS area %d\n", AASAreaNum);

	// Done
}

//----------------------------------------------------------------------------

// Internal helper
int darkModAASFindHidingSpots::TestHidingPoint 
(
	idVec3 testPoint, 
	float hidingHeight,
	int hidingSpotTypesAllowed, 
	idEntity* p_ignoreEntity,
	float& out_quality
)
{
	int out_hidingSpotTypesThatApply = NONE_HIDING_SPOT_TYPE;
	out_quality = 0.0f; // none found yet

	// Is it dark?
	if ((hidingSpotTypesAllowed & DARKNESS_HIDING_SPOT_TYPE) != 0)
	{
		// Test the lighting level of this position
		//DM_LOG(LC_AI, LT_DEBUG).LogString("Testing hiding-spot lighting at point %f,%f,%f\n", testPoint.x, testPoint.y, testPoint.z);
		idVec3 testLineTop = testPoint;
		testLineTop.z += hidingHeight;
		
		float LightQuotient = LAS.queryLightingAlongLine 
		(
			testPoint,
			testLineTop,
			p_ignoreEntity,
			true
		);

		//DM_LOG(LC_AI, LT_DEBUG).LogString("Done testing hiding-spot lighting at point %f,%f,%f\n", testPoint.x, testPoint.y, testPoint.z);
		if (LightQuotient < HIDING_SPOT_MAX_LIGHT_QUOTIENT)
		{
			//DM_LOG(LC_AI, LT_DEBUG).LogString("Found hidable darkness of %f at point %f,%f,%f\n", LightQuotient, testPoint.x, testPoint.y, testPoint.z);
			out_hidingSpotTypesThatApply |= DARKNESS_HIDING_SPOT_TYPE;

			float darknessQuality = 0.0;
			darknessQuality = (HIDING_SPOT_MAX_LIGHT_QUOTIENT - LightQuotient) / HIDING_SPOT_MAX_LIGHT_QUOTIENT;
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

	// Done
	//DM_LOG(LC_AI, LT_DEBUG).LogString("Done testing for hidability at point %f,%f,%f\n", testPoint.x, testPoint.y, testPoint.z);
	return out_hidingSpotTypesThatApply;
}

//----------------------------------------------------------------------------

void darkModAASFindHidingSpots::insertHidingSpotWithQualitySorting
(
	darkModHidingSpot_t& hidingSpot,
	idList<darkModHidingSpot_t>& inout_hidingSpots
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

//----------------------------------------------------------------------------

void darkModAASFindHidingSpots::CombineRedundantHidingSpots
(
	idList<darkModHidingSpot_t>& inout_hidingSpots
)
{
	/*
	int listLength = inout_hidingSpots.Num();

	for (int index = 0; index < listLength; index ++)
	{
		// Get the hiding spot
		darkModHidingSpot_t spot = inout_hidingSpots[index];

	}
	*/

}

//----------------------------------------------------------------------------


// Primary external interface
void darkModAASFindHidingSpots::getNearbyHidingSpots 
(
	idList<darkModHidingSpot_t>& out_hidingSpots,
	idAAS *p_aas, 
	float hidingHeight,
	idBounds searchLimits, 
	int hidingSpotTypesAllowed, 
	idEntity* p_ignoreEntity
)
{
	// Clear hiding spot list
	out_hidingSpots.Clear();

	// Log
	DM_LOG(LC_AI, LT_DEBUG).LogString("Hide from position %f, %f, %f\n", hideFromPosition.x, hideFromPosition.y, hideFromPosition.z);

	// Test paramters
	if (p_aas == NULL)
	{
		// TODO: Log error
		DM_LOG(LC_AI, LT_ERROR).LogString("Parameter p_aas is NULL");
		return;
	}

	// Test the search region for hiding spots
	FindHidingSpots( out_hidingSpots, p_aas, hidingHeight, searchLimits, hidingSpotTypesAllowed, p_ignoreEntity);


	// Done
	return;

 }

//----------------------------------------------------------------------------

// Debug functions

void darkModAASFindHidingSpots::debugClearHidingSpotDrawList()
{
	// Clear the list
	darkModAASFindHidingSpots::DebugDrawList.Clear();

}

//----------------------------------------------------------------------------

void darkModAASFindHidingSpots::debugAppendHidingSpotsToDraw (const idList<darkModHidingSpot_t>& hidingSpotsToAppend)
{
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

	idVec3 markerArrowLength (0.0, 0.0, 1.0f);

	
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
			DebugDrawList[spotIndex].goal.origin + markerArrowLength,
			DebugDrawList[spotIndex].goal.origin,
			1.0f,
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
	float hidingHeight,
	idBounds hideSearchBounds, 
	idEntity* p_ignoreEntity, 
	idAAS* p_aas
)
{

	darkModAASFindHidingSpots HidingSpotFinder (hideFromLocation, p_aas);

	idList<darkModHidingSpot_t> hidingSpotList;

	DM_LOG(LC_AI, LT_DEBUG).LogVector ("Hide search mins", hideSearchBounds[0]);
	DM_LOG(LC_AI, LT_DEBUG).LogVector ("Hide search maxes", hideSearchBounds[1]);

	HidingSpotFinder.getNearbyHidingSpots
	(
		hidingSpotList,
		p_aas,
		hidingHeight,
		hideSearchBounds,
		ANY_HIDING_SPOT_TYPE,
		p_ignoreEntity
	);

	// Clear the debug list and add these
	darkModAASFindHidingSpots::debugClearHidingSpotDrawList();
	darkModAASFindHidingSpots::debugAppendHidingSpotsToDraw (hidingSpotList);
	darkModAASFindHidingSpots::debugDrawHidingSpots (15000);


	// Done
	
}

