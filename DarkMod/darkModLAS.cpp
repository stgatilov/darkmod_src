/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/
/*!
* Implementation file for the darkModLAS class
* SophisticatedZombie, using SparHawk's lightgem code
*
*/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include "./darkModLAS.h"
#include "../game/pvs.h"
#include "../renderer/renderworld.h"
#include "../DarkMod/DarkModGlobals.h"
#include "../DarkMod/Intersection.h"

//----------------------------------------------------------------------------

// Global instance of LAS
darkModLAS LAS;

//----------------------------------------------------------------------------

/*!
* Constructor
*/
darkModLAS::darkModLAS(void)
{
	// No areas
	m_numAreas = 0;
	m_pp_areaLightLists = NULL;
}

/*!
* Destructor
*/
darkModLAS::~darkModLAS(void)
{
	shutDown();
}

//----------------------------------------------------------------------------

__inline bool darkModLAS::moveLightBetweenAreas (darkModLightRecord_t* p_LASLight, int oldAreaNum, int newAreaNum )
{
	assert ((oldAreaNum >= 0) && (oldAreaNum < m_numAreas));
	assert (((newAreaNum >= 0) && (newAreaNum < m_numAreas)) || newAreaNum == -1);

	// Remove from old area
	idLinkList<darkModLightRecord_t>* p_cursor = m_pp_areaLightLists[oldAreaNum];
	while (p_cursor != NULL)
	{
		darkModLightRecord_t* p_thisLASLight = (darkModLightRecord_t*) (p_cursor->Owner());
		if (p_thisLASLight == p_LASLight)
		{
			// angua: Check if this is the only light in this area. 
			if (p_cursor->ListHead() == p_cursor && p_cursor->NextNode() == NULL) 
			{
				// set this list to empty.
				m_pp_areaLightLists[oldAreaNum] = NULL;
			}
			// Remove this node from its list
			p_cursor->RemoveHeadsafe();
			break;
		}
		else
		{
			p_cursor = p_cursor->NextNode();
		}
	}

	// Test for not found
	if (p_cursor == NULL)
	{
		// Log error
		DM_LOG(LC_LIGHT, LT_ERROR).LogString("Failed to remove LAS light record from list for area %d", oldAreaNum);

		// Failed
		return false;
	}

	if (newAreaNum == -1)
	{
		// Light is now in the void, return without doing anything
		return false;
	}

	// Add to new area
	if (m_pp_areaLightLists[newAreaNum] != NULL)
	{
		DM_LOG(LC_LIGHT, LT_DEBUG).LogString("Added to existing area %d\r", newAreaNum);
		p_cursor->AddToEnd (*m_pp_areaLightLists[newAreaNum]);
	}
	else
	{
		DM_LOG(LC_LIGHT, LT_DEBUG).LogString("Created new area %d\r", newAreaNum);
		m_pp_areaLightLists[newAreaNum] = p_cursor;
	}
	
	// Update the area index on the light after the move
	p_LASLight->areaIndex = newAreaNum;
	p_LASLight->p_idLight->LASAreaIndex = newAreaNum;

	DM_LOG(LC_LIGHT, LT_DEBUG).LogString("Area %d has now %d elements\r", oldAreaNum, 
		m_pp_areaLightLists[oldAreaNum] ? m_pp_areaLightLists[oldAreaNum]->Num() : 0);
	DM_LOG(LC_LIGHT, LT_DEBUG).LogString("Area %d has now %d elements\r", newAreaNum, m_pp_areaLightLists[newAreaNum]->Num());

	// Done
	DM_LOG(LC_LIGHT, LT_DEBUG).LogString("Light '%s' was moved from area %d to area %d ", p_cursor->Owner()->p_idLight->name.c_str(), oldAreaNum, newAreaNum);

	return true;
	

}

//----------------------------------------------------------------------------

void darkModLAS::accumulateEffectOfLightsInArea 
( 
	float& inout_totalIllumination,
	int areaIndex, 
	idVec3 testPoint1,
	idVec3 testPoint2,
	idEntity* p_ignoredEntity,
	bool b_useShadows
)
{
	/*!
	* Note most of this code is adopted from SparHawk's lightgem alpha code
	*/


	// Set up target segement: Origin and Delta
	idVec3 vTargetSeg[LSG_COUNT];
	vTargetSeg[0] = testPoint1;
	vTargetSeg[1] = testPoint2 - testPoint1;
	if (cv_las_showtraces.GetBool())
	{
		gameRenderWorld->DebugArrow(colorBlue, testPoint1, testPoint2, 2, 1000);
	}

	assert(areaIndex >= 0 && areaIndex < m_numAreas);
	idLinkList<darkModLightRecord_t>* p_cursor = m_pp_areaLightLists[areaIndex];

	// Iterate lights in this area
	while (p_cursor != NULL)
	{
		// Get the light to be tested
		darkModLightRecord_t* p_LASLight = p_cursor->Owner();
		if (p_LASLight == NULL)
		{
			// Log error
			DM_LOG(LC_LIGHT, LT_ERROR).LogString("LASLight record in area %d is NULL", areaIndex);

			// Return what we have so far
			return;
		}

		DM_LOG(LC_LIGHT, LT_DEBUG).LogString
		(
			"accumulateEffectOfLightsInArea (area=%d): accounting for light '%s'", 
			areaIndex,
			p_LASLight->p_idLight->name.c_str()
		);

		/*!
		// What follows in the rest of this method is mostly Sparkhawk's lightgem code
		*/
		idVec3 vLightCone[ELC_COUNT];
		idVec3 vLight;
		EIntersection inter;
		idVec3 vResult[2];


		if(p_LASLight->p_idLight->IsPointlight())
		{
			p_LASLight->p_idLight->GetLightCone
			(
				vLightCone[ELL_ORIGIN], 
				vLightCone[ELA_AXIS], 
				vLightCone[ELA_CENTER]
			);
			inter = IntersectLineEllipsoid
			(
				vTargetSeg, 
				vLightCone, 
				vResult
			);

			// If this is a centerlight we have to move the origin from the original origin to where the
			// center of the light is supposed to be.
			// Centerlight means that the center of the ellipsoid is not the same as the origin. It has to
			// be adjusted because if it casts shadows we have to trace to it, and in this case the light
			// might be inside geometry and would be reported as not being visible even though it casts
			// a visible light outside the geometry it is embedded. If it is not a centerlight and has
			// cast shadows enabled, it wouldn't cast any light at all in such a case because it would
			// be blocked by the geometry.
			vLight = vLightCone[ELL_ORIGIN] + vLightCone[ELA_CENTER];
			DM_LOG(LC_LIGHT, LT_DEBUG).LogString("IntersectLineEllipsoid returned %u\r", inter);
		}
		else
		{
			bool bStump = false;
			p_LASLight->p_idLight->GetLightCone(vLightCone[ELC_ORIGIN], vLightCone[ELA_TARGET], vLightCone[ELA_RIGHT], vLightCone[ELA_UP], vLightCone[ELA_START], vLightCone[ELA_END]);
			inter = IntersectLineCone(vTargetSeg, vLightCone, vResult, bStump);
			DM_LOG(LC_LIGHT, LT_DEBUG).LogString("IntersectLineCone returned %u\r", inter);
		}

		// Do tests to see if we should exclude this light from consideration
		bool b_excludeLight = false;
		float testDistance = 0.0f;
		
		// The line intersection can only return three states. Either the line is passing
		// through the lightcone in which case we will get two intersection points, the line
		// is not passing through which means that the test line is fully outside, or the line
		// is touching the cone in exactly one point. The last case is not really helpfull in
		// our case and doesn't make a difference for the gameplay so we simply ignore it and
		// consider only cases where the testline is at least partially inside the cone.
		if (inter != INTERSECT_FULL) 
		{
			b_excludeLight = true;
		}
		else
		{
			// Determine intensity of this light at the distance from its origin to the
			// test point
			idVec3 testPosNoZ = testPoint1;
			testPosNoZ.z = p_LASLight->lastWorldPos.z;
			testDistance = (p_LASLight->lastWorldPos - testPosNoZ).Length();

			// Fast and cheap test to see if the player could be in the area of the light.
			// Well, it is not exactly cheap, but it is the cheapest test that we can do at this point. :)
			if (testDistance > p_LASLight->p_idLight->m_MaxLightRadius)
			{
				b_excludeLight = true;
				DM_LOG(LC_LIGHT, LT_DEBUG).LogString("Light [%s]: exluded due to max light radius\r", p_LASLight->p_idLight->name.c_str());
			}
			else if (b_useShadows && p_LASLight->p_idLight->CastsShadow()) 
			{
				trace_t trace;
				gameLocal.clip.TracePoint(trace, testPoint1, p_LASLight->lastWorldPos, CONTENTS_OPAQUE, p_ignoredEntity);
				if (cv_las_showtraces.GetBool())
				{
					gameRenderWorld->DebugArrow(
							trace.fraction == 1 ? colorGreen : colorRed, 
							trace.fraction == 1 ? testPoint1 : trace.endpos, 
							p_LASLight->lastWorldPos, 1, 1000);
				}
				DM_LOG(LC_LIGHT, LT_DEBUG).LogString("TraceFraction: %f\r", trace.fraction);
				if(trace.fraction < 1.0f)
				{
					gameLocal.clip.TracePoint (trace, testPoint2, p_LASLight->lastWorldPos, CONTENTS_OPAQUE, p_ignoredEntity);
					if (cv_las_showtraces.GetBool())
					{
						gameRenderWorld->DebugArrow(
							trace.fraction == 1 ? colorGreen : colorRed, 
							trace.fraction == 1 ? testPoint2 : trace.endpos, 
							p_LASLight->lastWorldPos, 1, 1000);
					}
					DM_LOG(LC_LIGHT, LT_DEBUG).LogString("TraceFraction: %f\r", trace.fraction);
					if(trace.fraction < 1.0f)
					{
						DM_LOG(LC_LIGHT, LT_DEBUG).LogString("Light [%s]: test point is in a shadow of the light\r", p_LASLight->p_idLight->name.c_str());
						b_excludeLight = true;
					}
				}
			}
		} 

		// Process light if not excluded
		if (!b_excludeLight)
		{
			// Calcluate where along the test segment the light intersection took place
			int l;
			float fx, fy;
			if (vResult[0].z < vResult[1].z)
			{
				l = 0;
			}
			else
			{
				l = 1;
			}

			if (vResult[l].z < testPoint1.z)
			{
				fx = testPoint1.x;
				fy = testPoint1.y;
			}
			else
			{
				fx = vResult[l].x;
				fy = vResult[l].y;
			}

			// Compute illumination value
			inout_totalIllumination += p_LASLight->p_idLight->GetDistanceColor
			(
				testDistance, 
				vLightCone[ELL_ORIGIN].x - fx, 
				vLightCone[ELL_ORIGIN].y - fy
			);
			DM_LOG(LC_LIGHT, LT_DEBUG).LogString
			(
				"%s in x/y: %f/%f   Distance:   %f/%f   Brightness: %f\r",
				p_LASLight->p_idLight->name.c_str(), 
				fx, 
				fy, 
				inout_totalIllumination, 
				testDistance, 
				p_LASLight->p_idLight->m_MaxLightRadius
			);

		} // Light not excluded

		// If total illumination is 1.0 or greater, we are done
		if (inout_totalIllumination >= 1.0f)
		{
			// Exit early as its really really bright as is
			p_cursor = NULL;
		}
		else
		{
			// Iterate to next light in area
			p_cursor = p_cursor->NextNode();
		}

	} // Next light in area

}

//----------------------------------------------------------------------------

/*!
* Initialization
*/
void darkModLAS::initialize()
{	
	DM_LOG(LC_LIGHT, LT_DEBUG).LogString("Initializing Light Awareness System (LAS)");

	// Dispose of any previous information
	if (m_pp_areaLightLists != NULL)
	{
		delete[] m_pp_areaLightLists;
		m_pp_areaLightLists = NULL;
	}

	// Get number of areas in the map
	m_numAreas = gameRenderWorld->NumAreas();

	// Allocate a light record list for each area
	if (m_numAreas > 0)
	{
		m_pp_areaLightLists = new idLinkList<darkModLightRecord_t>*[m_numAreas];
		if (m_pp_areaLightLists == NULL)
		{
			// Log error and exit
			DM_LOG(LC_LIGHT, LT_ERROR).LogString("Failed to allocate LAS area light lists");
			m_numAreas = 0;
			return;
		}
	}

	// Lists all begin empty
	for (int i = 0; i < m_numAreas; i ++)
	{
		m_pp_areaLightLists[i] = NULL;
	}

	// Frame index starts at 0
	m_updateFrameIndex = 0;


	// Log status
	DM_LOG(LC_LIGHT, LT_DEBUG).LogString("LAS initialized for %d map areas", m_numAreas);

	// Build default PVS to AAS Mapping table
	pvsToAASMappingTable.clear();
	if (!pvsToAASMappingTable.buildMappings("aas32"))
	{
		DM_LOG(LC_LIGHT, LT_DEBUG).LogString("Failed to initialize PVS to aas32 mapping table, trying aas48");

		if (!pvsToAASMappingTable.buildMappings("aas48"))
		{
			DM_LOG(LC_LIGHT, LT_DEBUG).LogString("Failed to initialize PVS to aas48 mapping table");
		}
	}
	else
	{
		DM_LOG(LC_LIGHT, LT_DEBUG).LogString("PVS to aas32 mapping table initialized");
	}

}

//-------------------------------------------------------------------------

void darkModLAS::addLight (idLight* p_idLight)
{
	// Get the light position
	idVec3 lightPos(p_idLight->GetPhysics()->GetOrigin());

	// Determine the index of the area containing this light
	int containingAreaIndex = gameRenderWorld->PointInArea (lightPos);
	if (containingAreaIndex < 0)
	{
		// The light isn't in an area
		// TODO: Log error, light is not in an area
		DM_LOG(LC_LIGHT, LT_ERROR).LogString("Light is not contained in an area");
		return;
	}

	// Add it to the appropriate list
	assert (containingAreaIndex < m_numAreas);

	// Make a darkMod light record for it
	darkModLightRecord_t* p_record = new darkModLightRecord_t;
	p_record->lastFrameUpdated = m_updateFrameIndex;
	p_record->lastWorldPos = lightPos;
	p_record->p_idLight = p_idLight;
	p_record->areaIndex = containingAreaIndex;

	if (m_pp_areaLightLists[containingAreaIndex] != NULL)
	{
		idLinkList<darkModLightRecord_t>* p_node = new idLinkList<darkModLightRecord_t>;
		p_node->SetOwner (p_record);
		p_node->AddToEnd (*(m_pp_areaLightLists[containingAreaIndex]));

		DM_LOG(LC_LIGHT, LT_DEBUG).LogString("Light '%s' was added to area %d at end of list, area now has %d lights", p_idLight->name.c_str(), containingAreaIndex, m_pp_areaLightLists[containingAreaIndex]->Num());
	}
	else
	{
		// First in area
		idLinkList<darkModLightRecord_t>* p_first = new idLinkList<darkModLightRecord_t>;
		p_first->SetOwner (p_record);
		if (p_first == NULL)
		{
			DM_LOG(LC_LIGHT, LT_ERROR).LogString("Failed to create node for LASLight record");
			return;
		}
		else
		{
			m_pp_areaLightLists[containingAreaIndex] = p_first;
		}

		DM_LOG(LC_LIGHT, LT_DEBUG).LogString("Light '%s' was added to area %d as first in list", p_idLight->name.c_str(), containingAreaIndex);

	}

	// Note the area we just added the light to
	p_idLight->LASAreaIndex = containingAreaIndex;


}


//------------------------------------------------------------------------------

void darkModLAS::removeLight (idLight* p_idLight)
{
	// Test parameters
	assert (p_idLight != NULL);
	
	if (p_idLight->LASAreaIndex < 0)
	{
		// Log error
		DM_LOG(LC_LIGHT, LT_ERROR).LogString("Attempted to remove the light '%s' with no assigned LAS area index", p_idLight->name.c_str());
		return;
	}
	else if (p_idLight->LASAreaIndex >= m_numAreas)
	{
		// Log error
		DM_LOG(LC_LIGHT, LT_ERROR).LogString("Attempted to remove the light '%s' with out of bounds area index %d", p_idLight->name.c_str(), p_idLight->LASAreaIndex);
		return;
	}


	if (m_pp_areaLightLists == NULL)
	{
		// Log error
		DM_LOG(LC_LIGHT, LT_ERROR).LogString("LAS not initialized. Remove light '%s' request ignored", p_idLight->name.c_str());
		return;
	}

	// Remove the light from the list it should be in
	idLinkList<darkModLightRecord_t>* p_cursor = m_pp_areaLightLists[p_idLight->LASAreaIndex];
	while (p_cursor != NULL)
	{
		if (p_cursor->Owner()->p_idLight == p_idLight)
		{
			// Keep track of header, bass ackward idLinkedList can't
			// update the header pointer on its own because of the inverted
			// way it handles the container arrangement.
			if (m_pp_areaLightLists[p_idLight->LASAreaIndex] == p_cursor)
			{
				m_pp_areaLightLists[p_idLight->LASAreaIndex] = p_cursor->NextNode();
				if (m_pp_areaLightLists[p_idLight->LASAreaIndex] == p_cursor)
				{
					// If only one node left in the list, it makes circular link because idLinkList::Remove doesn't 
					// update the head. Also because of this, on removing the first node in the list, the head
					// of the list is no longer real and all iterations become circular. That is a problem.
					// pointer tracked in each node.  Its a logical error in idLinkList::Remove that
					// really should be fixed.
					m_pp_areaLightLists[p_idLight->LASAreaIndex] = NULL;
				}
			}

			// Remove this node from its list and destroy the record
			p_cursor->RemoveHeadsafe();


			// Light not in an LAS area
			int tempIndex = p_idLight->LASAreaIndex;
			p_idLight->LASAreaIndex = -1;

			// Destroy node
			delete p_cursor;

			// Log status
			DM_LOG(LC_LIGHT, LT_DEBUG).LogString("Light '%s' was removed from LAS area %d", p_idLight->name.c_str(), tempIndex);

			// Done
			return;
		}
		else
		{
			p_cursor = p_cursor->NextNode();
		}
	}

	// Light not found
	// Log error
	DM_LOG(LC_LIGHT, LT_ERROR).LogString("Light '%s' stating it was in LAS area index %d was not found in that LAS Area's list", p_idLight->name.c_str(), p_idLight->LASAreaIndex);

	// Done
}

//----------------------------------------------------------------------------

void darkModLAS::shutDown()
{
	// Log activity
	DM_LOG(LC_LIGHT, LT_DEBUG).LogString("LAS shutdown initiated...\n");

	// Delete all records in each list
	for (int areaIndex = 0; areaIndex < m_numAreas; areaIndex ++)
	{
		DM_LOG(LC_LIGHT, LT_DEBUG).LogString("LAS shutdown clearing light records for areaIndex %d...\n", areaIndex);
	
		// Destroy each light record
		idLinkList<darkModLightRecord_t>* p_cursor = m_pp_areaLightLists[areaIndex];
		idLinkList<darkModLightRecord_t>* p_temp;
		while (p_cursor != NULL)
		{
			darkModLightRecord_t* p_LASLight = p_cursor->Owner();
			DM_LOG(LC_LIGHT, LT_DEBUG).LogString("Light list iterating node %d ", p_cursor);

			if (p_LASLight != NULL)
			{
				DM_LOG(LC_LIGHT, LT_DEBUG).LogString("Light list clear is deleting light '%s' ", p_LASLight->p_idLight->GetName());
				delete p_LASLight;
			}

			// Next node
			p_temp = p_cursor->NextNode();
			p_cursor = p_temp;
		}

		DM_LOG(LC_LIGHT, LT_DEBUG).LogString("LAS shutdown destroying node list for areaIndex %d...\n", areaIndex);

		// Clear the list of nodes
		if (m_pp_areaLightLists[areaIndex] != NULL)
		{
			delete m_pp_areaLightLists[areaIndex];
			m_pp_areaLightLists[areaIndex] = NULL;
		}

		DM_LOG(LC_LIGHT, LT_DEBUG).LogString("LAS shutdown destroyed list for for areaIndex %d\n", areaIndex);

	} // Next area

	// Log activity
	DM_LOG(LC_LIGHT, LT_DEBUG).LogString("LAS shutdown deleting array of per-area list pointers...\n");

	// Delete array of list pointers
	if (m_pp_areaLightLists != NULL)
	{
		delete[] m_pp_areaLightLists;
		m_pp_areaLightLists = NULL;
	}

	// No areas
	m_numAreas = 0;

	DM_LOG(LC_LIGHT, LT_DEBUG).LogString("LAS shutdown deleted array of per-area list pointers...\n");

	// Log activity
	DM_LOG(LC_LIGHT, LT_DEBUG).LogString("LAS shut down and empty");

	// Destroy PVS to AAS mapping table contents
	DM_LOG(LC_LIGHT, LT_DEBUG).LogString("Clearing PVS to AAS(0) mapping table ...");
	pvsToAASMappingTable.clear();
	DM_LOG(LC_LIGHT, LT_DEBUG).LogString("PVS to AAS(0) mapping table cleared");


}

//----------------------------------------------------------------------------

void darkModLAS::updateLASState()
{
	// Doing a new update frame
	m_updateFrameIndex ++;

	DM_LOG(LC_LIGHT, LT_DEBUG).LogString("Updating LAS state, new LAS frame index is %d", m_updateFrameIndex);

	// Go through each of the areas and for any light that has moved, see
	// if it has changed areas.

	for (int areaIndex = 0; areaIndex < m_numAreas; areaIndex ++)
	{
		idLinkList<darkModLightRecord_t>* p_cursor = m_pp_areaLightLists[areaIndex];
		while (p_cursor != NULL)
		{
			// Get the darkMod light record
			darkModLightRecord_t* p_LASLight = p_cursor->Owner();

			// Remember next node in the current list
			idLinkList<darkModLightRecord_t>* p_nextTemp = p_cursor->NextNode();

			// Have we updated this light yet?
			if (p_LASLight->lastFrameUpdated != m_updateFrameIndex)
			{
				// Get the light position
				idVec3 lightPos(p_LASLight->p_idLight->GetPhysics()->GetOrigin());
	
				// Check to see if it has moved
				if (p_LASLight->lastWorldPos != lightPos)
				{
					// Update its world pos
					p_LASLight->lastWorldPos = lightPos;

					// This light may have moved between areas
					int newAreaIndex = gameRenderWorld->PointInArea (p_LASLight->lastWorldPos);
					if (newAreaIndex != p_LASLight->areaIndex)
					{
						// Move between areas
						moveLightBetweenAreas (p_LASLight, p_LASLight->areaIndex, newAreaIndex);
					
					}  // Light changed areas
				
				} // Light moved
			
				// Mark light as updated this LAS frame
				p_LASLight->lastFrameUpdated = m_updateFrameIndex;
				
			} // Light not yet updated
	
			// Iterate to next light
			p_cursor = p_nextTemp;

		} // Next light in this area
	
	} // Next area

	// Done
	DM_LOG(LC_LIGHT, LT_DEBUG).LogString("LAS update frame %d complete", m_updateFrameIndex);

}


//----------------------------------------------------------------------------

float darkModLAS::queryLightingAlongLine
(
		idVec3 testPoint1,
		idVec3 testPoint2,
		idEntity* p_ignoreEntity,
		bool b_useShadows
)
{
	idTimer lightingTimer;
	lightingTimer.Clear();
	lightingTimer.Start();
	if (p_ignoreEntity != NULL)
	{
		DM_LOG(LC_LIGHT, LT_DEBUG).LogString
		(
			"queryLightingAlongLine <%f,%f,%f> to <%f,%f,%f>, IgnoredEntity = '%s', UseShadows = %d'", 
			testPoint1.x, testPoint1.y, testPoint1.z,
			testPoint2.x, testPoint2.y, testPoint2.z,
			p_ignoreEntity->name.c_str(),
			b_useShadows
		);
	}
	else
	{
		DM_LOG(LC_LIGHT, LT_DEBUG).LogString
		(
			"queryLightingAlongLine <%f,%f,%f> to <%f,%f,%f>,  UseShadows = %d'", 
			testPoint1.x, testPoint1.y, testPoint1.z,
			testPoint2.x, testPoint2.y, testPoint2.z,
			b_useShadows
		);

	}

	// Total illumination starts at 0
	float totalIllumination = 0.0f;

	// Find the area that the test points are in
//	int testPointAreaIndex = gameRenderWorld->PointInArea (testPoint1);

	// Compute test bounds
	idVec3 mins, maxes;
	if (testPoint1.x < testPoint2.x)
	{
		mins.x = testPoint1.x;
		maxes.x = testPoint2.x;
	}
	else
	{
		mins.x = testPoint2.x;
		maxes.x = testPoint1.x;
	}

	if (testPoint1.y < testPoint2.y)
	{
		mins.y = testPoint1.y;
		maxes.y = testPoint2.y;
	}
	else
	{
		mins.y = testPoint2.y;
		maxes.y = testPoint1.y;
	}

	if (testPoint1.z < testPoint2.z)
	{
		mins.z = testPoint1.z;
		maxes.z = testPoint2.z;
	}
	else
	{
		mins.z = testPoint2.z;
		maxes.z = testPoint1.z;
	}
	
	idBounds testBounds (mins, maxes);

	// Run a local PVS query to determine which other areas are visible (and hence could
	// have lights shing on the target area)
	int pvsTestAreaIndices[idEntity::MAX_PVS_AREAS];
	int numPVSTestAreas = gameLocal.pvs.GetPVSAreas
	(
		testBounds,
		pvsTestAreaIndices,
		idEntity::MAX_PVS_AREAS
	);

	// Set up the graph
	pvsHandle_t h_lightPVS = gameLocal.pvs.SetupCurrentPVS
	(
		pvsTestAreaIndices, 
		numPVSTestAreas
	);

	DM_LOG(LC_LIGHT, LT_DEBUG).LogString
	(
		"queryLightingAlongLine: PVS test results in %d PVS areas", 
		numPVSTestAreas
	);



	// Check all the lights in the PVS areas and factor them in
	for (int pvsTestResultIndex = 0; pvsTestResultIndex < numPVSTestAreas; pvsTestResultIndex ++)
	{
		// Add the effect of lights in this visible area to the effect at the
		// point
		accumulateEffectOfLightsInArea 
		(
			totalIllumination,
			pvsTestAreaIndices[pvsTestResultIndex],
			testPoint1,
			testPoint2,
			p_ignoreEntity,
			b_useShadows
		);
	}

	// Done with PVS test
	gameLocal.pvs.FreeCurrentPVS( h_lightPVS );

	DM_LOG(LC_LIGHT, LT_DEBUG).LogString
	(
		"queryLightingAlongLine <%f,%f,%f> to <%f,%f,%f>,  result is %.2f", 
		testPoint1.x, testPoint1.y, testPoint1.z,
		testPoint2.x, testPoint2.y, testPoint2.z,
		totalIllumination
	);

	lightingTimer.Stop();
	DM_LOG(LC_LIGHT, LT_INFO).LogString("Lighing along line took %lf\r", lightingTimer.Milliseconds());

	// Return total illumination value to the caller
	return totalIllumination;
}

//----------------------------------------------------------------------------

idStr darkModLAS::getAASName()
{
	return pvsToAASMappingTable.getAASName();
}

