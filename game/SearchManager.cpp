/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod (http://www.thedarkmod.com/)
 
 $Revision: 6097 $ (Revision of last commit) 
 $Date: 2014-09-07 14:53:08 -0400 (Sun, 07 Sep 2014) $ (Date of last commit)
 $Author: grayman $ (Author of last commit)
 
******************************************************************************/

#include "precompiled_game.h"
#pragma hdrstop

static bool versioned = RegisterVersionedFile("$Id: SearchManager.cpp 6097 2014-09-07 18:53:08Z grayman $");

#include "SearchManager.h"
#include "Game_local.h"

#define SEARCH_RADIUS_FACTOR 0.707107	  // sqrt(0.5)
#define SEARCH_RADIUS_ONE_SEARCHER 126.0f // If xy radius of entire search is less than this, only allow one searcher

// Constructor
CSearchManager::CSearchManager()
{
	uniqueSearchID = 0; // the next unique id to assign to a new search
}

CSearchManager::~CSearchManager()
{
	Clear();
}

void CSearchManager::Clear()
{
	// Destroy the list of hiding spots
	// Remove assignments from participating AI
	for (int i = 0 ; i < _searches.Num() ; i++)
	{
		Search *search = &_searches[i];
		destroyCurrentHidingSpotSearch(search);
	}
}

int CSearchManager::StartNewHidingSpotSearch(idAI* ai)
{
	// The AI wants to start a new hiding spot search. See if there's
	// a search already underway for the location specified.

	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::StartNewHidingSpotSearch - %s wants to start a new hiding spot search\r",ai->GetName()); // grayman debug
	ai::Memory& memory = ai->GetMemory();
	idVec3 searchPoint = memory.alertPos;
	idVec3 searchVolume = memory.alertSearchVolume;
	idVec3 searchExclusionVolume = memory.alertSearchExclusionVolume;
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::StartNewHidingSpotSearch - %s searchPoint = [%s], searchVolume = [%s], searchExclusionVolume = [%s]\r",ai->GetName(),searchPoint.ToString(), searchVolume.ToString(), searchExclusionVolume.ToString()); // grayman debug

	Search *search;

	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::StartNewHidingSpotSearch - %s _searches.Num() = %d\r",ai->GetName(),_searches.Num()); // grayman debug
	int searchID = -1;
	for (int i = 0 ; i < _searches.Num() ; i++)
	{
		search = &_searches[i];
		if ((search->_origin - searchPoint).LengthFast() < 64)
		{
			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::StartNewHidingSpotSearch - %s joining search %d, which is already underway\r",ai->GetName(),i); // grayman debug
			searchID = search->_searchID; // search already underway

			// reset ai's data
			memory.alertPos = search->_origin;
			memory.alertSearchVolume = search->_limits.GetSize()/2.0f;
			memory.alertSearchExclusionVolume = search->_exclusion_limits.GetSize()/2.0f;
			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::StartNewHidingSpotSearch - %s resetting alertPos = [%s], alertSearchVolume = [%s], searchExclusionVolume = [%s]\r",ai->GetName(),memory.alertPos.ToString(), memory.alertSearchVolume.ToString(), memory.alertSearchExclusionVolume.ToString()); // grayman debug

			break;
		}
	}

	if (searchID < 0)
	{
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::StartNewHidingSpotSearch - %s no existing search, create a new one\r",ai->GetName()); // grayman debug
		// search is not already underway at this location, start a new one

		idVec3 minBounds(memory.alertPos - searchVolume);
		idVec3 maxBounds(memory.alertPos + searchVolume);

		idVec3 minExclusionBounds(memory.alertPos - searchExclusionVolume);
		idVec3 maxExclusionBounds(memory.alertPos + searchExclusionVolume);

		idBounds searchBounds = idBounds(minBounds,maxBounds);
		idBounds searchExclusionBounds = idBounds(minExclusionBounds,maxExclusionBounds);
	
		// grayman #2422 - to prevent AI from going upstairs or downstairs
		// to search spots over/under where they should be searching,
		// limit the search to the floor where the alert occurred.

		AdjustSearchLimits(searchBounds);

		Search newSearch;

		search = &newSearch;

		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::StartNewHidingSpotSearch - %s searchBounds adjusted to [%s]\r",ai->GetName(),searchBounds.ToString()); // grayman debug
		search->_hidingSpotSearchHandle = NULL_HIDING_SPOT_SEARCH_HANDLE;
		search->_searchID = uniqueSearchID++;
		search->_origin = searchPoint;  // center of search area, location of alert stimulus
		search->_limits = searchBounds; // boundary of the search
		search->_exclusion_limits = searchExclusionBounds; // exclusion boundary of the search
		search->_hidingSpots.clear(); // The hiding spots for this search
		search->_hidingSpotsReady = false; // whether the hiding spot list is complete or still being built
		DebugPrintSearch(search); // grayman debug - comment when done

		// Add search to list
		_searches.Append(*search);
		searchID = search->_searchID;
	}

	return searchID;
}

void CSearchManager::PerformHidingSpotSearch(int searchID, idAI* ai)
{
	// Shortcut reference
	ai::Memory& memory = ai->GetMemory();

	// Increase the frame count
	memory.hidingSpotThinkFrameCount++;

	if (gameLocal.m_searchManager->ContinueSearchForHidingSpots(searchID,ai) == 0)
	{
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::PerformHidingSpotSearch - %s hiding spot search completed\r",ai->GetName()); // grayman debug
		// search completed
		memory.hidingSpotSearchDone = true;

		// Hiding spot test is done
		memory.hidingSpotTestStarted = false;
		
		// Here we transition to the state for handling the behavior of
		// the AI once it thinks it knows where the stimulus may have
		// come from

		//DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::PerformHidingSpotSearch - %s hidingSpotInvestigationInProgress set to false\r",ai->GetName()); // grayman debug
		//memory.hidingSpotInvestigationInProgress = false;
				
		/* grayman debug - I don't think we need to find a hiding spot to go to here.
		// For now, starts searching for the stimulus.

		// Get location
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::PerformHidingSpotSearch - %s calling GetNextHidingSpot() to assign a spot\r",ai->GetName()); // grayman debug
		GetNextHidingSpot(GetSearch(searchID), ai, memory.chosenHidingSpot);
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::PerformHidingSpotSearch - %s GetNextHidingSpot() assigned me memory.chosenHidingSpot = [%s]\r",ai->GetName(),memory.chosenHidingSpot.ToString()); // grayman debug
		*/
	}
}

void CSearchManager::RandomizeHidingSpotList(Search* search)
{
	if (search == NULL)
	{
		return; // invalid search
	}

	int numSpots = search->_hidingSpots.getNumSpots();
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::RandomizeHidingSpotList - randomize the indices for %d spots\r",numSpots); // grayman debug
	search->_randomHidingSpotIndexes.clear(); // clear any existing array elements

	// Fill the array with integers from 0 to numSpots-1.
	for ( int i = 0 ; i < numSpots ; i++ )
	{
		search->_randomHidingSpotIndexes.push_back(i);
	}

    // Shuffle those integers by randomly exchanging pairs.
    for ( int i = 0 ; i < (numSpots-1) ; i++ )
	{
        int r = i + gameLocal.random.RandomInt(numSpots - i); // Random remaining position.
        int temp = search->_randomHidingSpotIndexes[i];
		search->_randomHidingSpotIndexes[i] = search->_randomHidingSpotIndexes[r];
		search->_randomHidingSpotIndexes[r] = temp;
    }
}

Search* CSearchManager::GetSearch(int searchID) // returns a pointer to the requested search
{
	if (searchID < 0)
	{
		return NULL; // invalid ID
	}

	for (int i = 0 ; i < _searches.Num() ; i++)
	{
		Search *search = &_searches[i];
		if (search->_searchID == searchID)
		{
			return search;
		}
	}

	return NULL;
}

Assignment* CSearchManager::GetAssignment(Search* search, idAI* ai) // get ai's assignment for a given search
{
	if ((search == NULL) || (ai == NULL))
	{
		return NULL;
	}

	for (int i = 0 ; i < search->_assignments.Num() ; i++)
	{
		if (search->_assignments[i]._searcher == ai)
		{
			return &search->_assignments[i];
		}
	}

	return NULL; // couldn't find an assignment for this ai for this search
}

int CSearchManager::GetNumHidingSpots(Search *search) // returns the number of hiding spots
{
	if (search == NULL)
	{
		return 0;
	}

	return search->_hidingSpots.getNumSpots();
}

bool CSearchManager::GetNextHidingSpot(Search* search, idAI* ai, idVec3& nextSpot)
{
	if (search == NULL)
	{
		return false; // invalid search
	}

	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::GetNextHidingSpot - %s asking for the next hiding spot\r",ai->GetName()); // grayman debug

	CDarkmodHidingSpotTree *tree = &search->_hidingSpots; // The hiding spots for this search
	int numSpots = tree->getNumSpots();
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::GetNextHidingSpot - %s numSpots = %d\r",ai->GetName(),numSpots); // grayman debug

	Assignment* assignment = GetAssignment(search,ai);

	if (assignment == NULL)
	{
		return false; // no assignment for this AI in this search
	}

	int index = assignment->_lastSpotAssigned; // the most recent spot assigned to this searcher; index into _randomHidingSpotIndexes
	index++;
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::GetNextHidingSpot - %s requested hiding spot index is %d\r",ai->GetName(),index); // grayman debug

	if (index >= numSpots)
	{
		return false; // no spots left
	}

	// find a valid spot that's inside the ai's search area

	for ( int i = index ; i < numSpots ; i++ )
	{
		int treeIndex = search->_randomHidingSpotIndexes[i];
		assignment->_lastSpotAssigned = i;

		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("   %s i = %d, treeIndex = %d\r",ai->GetName(),i,treeIndex); // grayman debug
		if ( treeIndex == -1 )
		{
			continue; // skip bad or used spots
		}

		// validate spot

		idBounds areaNodeBounds;
		darkModHidingSpot* hidingSpot = tree->getNthSpotWithAreaNodeBounds(treeIndex, areaNodeBounds);

		if (hidingSpot != NULL)
		{
			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("   %s hidingSpot = [%s]\r",ai->GetName(),hidingSpot->goal.origin.ToString()); // grayman debug
			// grayman #2422 - this routine might return (0,0,0), but we don't
			// want AI traveling there.

			if ( !hidingSpot->goal.origin.Compare(idVec3(0,0,0)) )
			{
				DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("   %s not [0,0,0], so let's check validity ...\r",ai->GetName()); // grayman debug
				// grayman #2422 - to keep AI from searching the floor above
				// or below, only return hiding spots that are inside the
				// requested search volume.

				if ( search->_limits.ContainsPoint(hidingSpot->goal.origin) )
				{
					nextSpot = hidingSpot->goal.origin; // point is good

					DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("   %s so far, [%s] is good\r",ai->GetName(),nextSpot.ToString()); // grayman debug
					// Make sure this spot is inside a searcher's search area

					float distToSpot = (search->_origin - nextSpot).LengthFast();
					DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("   %s distToSpot = %f\r",ai->GetName(),distToSpot); // grayman debug
					DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("   %s _innerRadius = %f\r",ai->GetName(),assignment->_innerRadius); // grayman debug
					DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("   %s _outerRadius = %f\r",ai->GetName(),assignment->_outerRadius); // grayman debug

					if ((distToSpot <= assignment->_outerRadius) && (distToSpot >= assignment->_innerRadius))
					{
						search->_randomHidingSpotIndexes[i] = -1; // don't assign this good spot again
						DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::GetNextHidingSpot - %s given hiding spot [%s]\r",ai->GetName(),nextSpot.ToString()); // grayman debug
						return true;
					}
					DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::GetNextHidingSpot - %s [%s] is invalid for this AI\r",ai->GetName(),nextSpot.ToString()); // grayman debug
					return false; // didn't find one this time, maybe next time will find one
				}
				else
				{
					// hidingSpot is outside the search area, so it's NG
					DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::GetNextHidingSpot - %s hidingSpot is outside the search area, so it's NG\r",ai->GetName()); // grayman debug
					search->_randomHidingSpotIndexes[i] = -1; // no one should use this spot
				}
			}
			else
			{
				// hidingSpot is [0,0,0], so it's NG
				DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::GetNextHidingSpot - %s hidingSpot is [0,0,0], so it's NG\r",ai->GetName()); // grayman debug
				search->_randomHidingSpotIndexes[i] = -1; // no one should use this spot
			}
		}
		else
		{
			// hidingSpot is NULL, so it's NG
			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::GetNextHidingSpot - %s hidingSpot is NULL, so it's NG\r",ai->GetName()); // grayman debug
			search->_randomHidingSpotIndexes[i] = -1; // no one should use this spot
		}
	}

	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::GetNextHidingSpot - %s couldn't find a valid hiding spot\r",ai->GetName()); // grayman debug
	return false; // can't find a good spot
}

int CSearchManager::ContinueSearchForHidingSpots(int searchID, idAI* ai)
{
	Search *search = GetSearch(searchID);

	if (search == NULL)
	{
		return 0;
	}

	// Get hiding spot search instance from handle
	CDarkmodAASHidingSpotFinder* p_hidingSpotFinder = NULL;
	if (search->_hidingSpotSearchHandle != NULL_HIDING_SPOT_SEARCH_HANDLE)
	{
		p_hidingSpotFinder = CHidingSpotSearchCollection::Instance().getSearchByHandle(
			search->_hidingSpotSearchHandle
		);
	}

	// Make sure search still around
	if (p_hidingSpotFinder == NULL)
	{
		// No hiding spot search to continue
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::ContinueSearchForHidingSpots - %s No current hiding spot search to continue\r",ai->GetName()); // grayman debug
		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("No current hiding spot search to continue\r");
		return 0;
	}

	// Call finder method to continue search
	bool moreProcessingToDo = p_hidingSpotFinder->continueSearchForHidingSpots
	(
		p_hidingSpotFinder->hidingSpotList,
		cv_ai_max_hiding_spot_tests_per_frame.GetInteger(),
		gameLocal.framenum
	);

	// Return result
	if (moreProcessingToDo)
	{
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::ContinueSearchForHidingSpots - %s more processing to do\r",ai->GetName()); // grayman debug
		return 1;
	}

	// No more processing to do at this point

	if (!search->_hidingSpotsReady)
	{
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::ContinueSearchForHidingSpots - %s no more processing to do\r",ai->GetName()); // grayman debug
		unsigned int refCount;

		// Get finder we just referenced
		p_hidingSpotFinder = CHidingSpotSearchCollection::Instance().getSearchAndReferenceCountByHandle 
			(
				search->_hidingSpotSearchHandle,
				refCount
			);

		search->_hidingSpots.clear();

		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::ContinueSearchForHidingSpots - %s copying finished tree to search->_hidingSpots\r",ai->GetName()); // grayman debug
		p_hidingSpotFinder->hidingSpotList.copy(&search->_hidingSpots);
		//p_hidingSpotFinder->hidingSpotList.getOneNth(refCount,search->_hidingSpots);
		search->_hidingSpotsReady = true; // the hiding spot list is complete

		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::ContinueSearchForHidingSpots - %s randomize the indices\r",ai->GetName()); // grayman debug
		// randomize the indices into the hiding spot list
		RandomizeHidingSpotList(search);

		//DebugPrint(search); // grayman debug - comment when done

		// Done with search object, dereference so other AIs know how many
		// AIs will still be retrieving points from the search
		// TODO: understand what this means. With the new design of a central
		// search manager, there should no longer be any splitting of the
		// list of hiding spots
		CHidingSpotSearchCollection::Instance().dereference (search->_hidingSpotSearchHandle);
		search->_hidingSpotSearchHandle = NULL_HIDING_SPOT_SEARCH_HANDLE;

		// DEBUGGING
		if (cv_ai_search_show.GetInteger() >= 1.0)
		{
			// Clear the debug draw list and then fill with our results
			p_hidingSpotFinder->debugClearHidingSpotDrawList();
			p_hidingSpotFinder->debugAppendHidingSpotsToDraw (search->_hidingSpots);
			p_hidingSpotFinder->debugDrawHidingSpots (cv_ai_search_show.GetInteger());
		}
		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Hiding spot search completed\r");
	}

	return 0;
}

// returns the role enum for requesting ai
smRole_t CSearchManager::GetRole(int searchID, idAI* ai)
{
	Search *search = GetSearch(searchID);
	Assignment* assignment = GetAssignment(search,ai);

	if ((search == NULL) || (assignment == NULL))
	{
		return E_ROLE_NONE; // invalid index
	}

	return assignment->_searcherRole;
}

bool CSearchManager::JoinSearch(int searchID, idAI* ai)
{
	Search *search = GetSearch(searchID);
	if (search == NULL)
	{
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::JoinSearch - %s couldn't find search for searchID %d\r",ai->GetName(),searchID); // grayman debug
		return false;
	}

	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::JoinSearch - %s joining searchID %d\r",ai->GetName(),searchID); // grayman debug
	// How many assignments does this search have?

	// If no one's been assigned yet, the first AI to join will be
	// given an "inner search" based on the origin and boundaries
	// set up when the AI was alerted.

	// If the search already has a single searcher, the second searcher
	// is assigned an area that's larger then what was given to the
	// initial searcher. The initial searcher's area is defined as an
	// 'exclusion zone' the second searcher will stay out of.

	// If more than 2 AI join this search, they will be given guard
	// assignments that will place them at nearby doors or visportals
	// that lead out of the area. We might have to limit the number of
	// guards based on the available number of these choke points. Anyone
	// wanting to join after the search is fully populated should be
	// turned away.

	int numAssignments = search->_assignments.Num();
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::JoinSearch - %s this search has %d current assignments\r",ai->GetName(),numAssignments); // grayman debug

	float innerRadius;
	float outerRadius;
	idBounds searchBounds;
	idBounds searchExclusionBounds;
	smRole_t searcherRole = E_ROLE_SEARCHER; // default
	if (numAssignments == 0)
	{
		// A single AI gets to search the entire search area.

		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::JoinSearch - %s first ai to join this search\r",ai->GetName()); // grayman debug
		innerRadius = 0;
		idVec3 searchSize = search->_limits.GetSize();
		float xRad = searchSize.x/2.0f;
		float yRad = searchSize.y/2.0f;
		outerRadius = idMath::Sqrt(xRad*xRad + yRad*yRad);
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::JoinSearch - %s search area for first AI = %f\r",ai->GetName(),idMath::PI*outerRadius*outerRadius); // grayman debug
	}
	else if (numAssignments == 1)
	{
		if (search->_assignments[0]._outerRadius < SEARCH_RADIUS_ONE_SEARCHER)
		{
			// there isn't enough search area to have more than one searcher
			searcherRole = E_ROLE_GUARD;
		}
		else
		{
			// Divide the search area in half between the two searchers.
			// The first AI searches the inner half of the overall area, and
			// the second AI searches the outer half. If one or the other
			// leaves the search, the search area of the remaining AI
			// remains the same.

			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::JoinSearch - %s second ai to join this search\r",ai->GetName()); // grayman debug
			outerRadius = search->_assignments[0]._outerRadius; // inherit the outer radius from the first searcher
			innerRadius = outerRadius*SEARCH_RADIUS_FACTOR; // splits the overall search area in half
			search->_assignments[0]._outerRadius = innerRadius; // pull back first AI's search area
			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::JoinSearch - %s search area for first AI  = %f\r",ai->GetName(),idMath::PI*innerRadius*innerRadius); // grayman debug
			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::JoinSearch - %s search area for second AI = %f\r",ai->GetName(),idMath::PI*outerRadius*outerRadius - idMath::PI*innerRadius*innerRadius); // grayman debug
		}
	}
	else // everyone else becomes a guard
	{
		searcherRole = E_ROLE_GUARD;
	}

	if (searcherRole == E_ROLE_GUARD)
	{
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::JoinSearch - %s joining this search as a guard\r",ai->GetName()); // grayman debug
		// TODO: decide where this guard will stand.
		// Look for visportals and/or doors in the area.
		// Create optional guard location entities? A mapper
		// could add those to places they want guarded during
		// searches. We would find the closest ones here and assign
		// guards to stand at those locations. Gives more control
		// to the mapper.
		innerRadius = 0;
		outerRadius = 0;
	}

	// Create new assignment

	Assignment newAssignment;
	Assignment *assignment = &newAssignment;

	assignment->_origin = search->_origin; // center of search area, location of alert stimulus
	assignment->_innerRadius = innerRadius;
	assignment->_outerRadius = outerRadius;
	assignment->_searcher = ai; // AI searcher assigned to this set of hiding spots
	assignment->_lastSpotAssigned = -1; // the most recent spot assigned to a searcher; index into _randomHidingSpotIndexes
	assignment->_searcherRole = searcherRole; // The role for the AI searcher

	DebugPrintAssignment(assignment); // grayman debug - comment when done

	search->_assignments.Append(*assignment); // add this new assignment to the search
	ai->m_searchID = searchID;
	return true;
}

// Searcher leaves the search. If the last searcher leaves a
// search, the search is destroyed.

void CSearchManager::LeaveSearch(int searchID, idAI* ai)
{
	Search *search = GetSearch(searchID);
	if (search == NULL)
	{
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::LeaveSearch - %s couldn't find search for searchID %d\r",ai->GetName(),searchID); // grayman debug
		return;
	}

	int numSearchers = search->_assignments.Num();
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::LeaveSearch - %s numSearchers = %d\r",ai->GetName(),numSearchers); // grayman debug

	ai->m_searchID = -1; // leave the search

	for (int i = 0 ; i < numSearchers ; i++)
	{
		Assignment *assignment = &search->_assignments[i];
		if (assignment->_searcher == ai)
		{
			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::LeaveSearch - %s removing old assignment from search\r",ai->GetName()); // grayman debug
			search->_assignments.RemoveIndex(i);
			break;
		}
	}
	
	numSearchers = search->_assignments.Num();
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::LeaveSearch - %s numSearchers reduced to %d\r",ai->GetName(),numSearchers); // grayman debug
	if (numSearchers == 0)
	{
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::LeaveSearch - %s no searchers left, destroying search\r",ai->GetName()); // grayman debug
		destroyCurrentHidingSpotSearch(search);

		// find and remove the search
		for (int i = 0 ; i < _searches.Num() ; i++)
		{
			search = &_searches[i];
			if (search->_searchID == searchID)
			{
				_searches.RemoveIndex(i);
				break;
			}
		}
	}

	// Now that you've left the search, stop whatever task you were doing
	// for that search

	ai::Memory& memory = ai->GetMemory();
	if (memory.hidingSpotInvestigationInProgress || memory.guardingInProgress)
	{
		memory.hidingSpotInvestigationInProgress = false;
		memory.guardingInProgress = false;
		memory.currentSearchSpot = idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY);
		ai->actionSubsystem->ClearTasks();
	}
}

// grayman #2422 - adjust search limits to better fit the vertical
// space in which the alert occurred

// bounds is in absolute coordinates

void CSearchManager::AdjustSearchLimits(idBounds& bounds)
{
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::AdjustSearchLimits ...\r"); // grayman debug
	// trace down

	idBounds newBounds = bounds;

	idVec3 start = bounds.GetCenter();
	idVec3 end = start;
	end.z -= 300;

	trace_t result;
	idEntity *ignore = NULL;
	while ( true )
	{
		gameLocal.clip.TracePoint( result, start, end, MASK_OPAQUE, ignore );
		if ( result.fraction == 1.0f )
		{
			break;
		}

		if ( result.fraction < VECTOR_EPSILON )
		{
			newBounds[0][2] = result.endpos.z; // move the lower bounds
			break;
		}

		// End the trace if we hit the world

		idEntity* entHit = gameLocal.entities[result.c.entityNum];

		if ( entHit == gameLocal.world )
		{
			newBounds[0][2] = result.endpos.z; // move the lower bounds
			break;
		}

		// Continue the trace from the struck point

		start = result.endpos;
		ignore = entHit; // for the next leg, ignore the entity we struck
	}

	// trace up

	end = start;
	end.z += 300;
	ignore = NULL;
	while ( true )
	{
		gameLocal.clip.TracePoint( result, start, end, MASK_OPAQUE, ignore );
		if ( result.fraction == 1.0f )
		{
			break;
		}

		if ( result.fraction < VECTOR_EPSILON )
		{
			if ( newBounds[1][2] > result.endpos.z )
			{
				newBounds[1][2] = result.endpos.z; // move the upper bounds down
			}
			break;
		}

		// End the trace if we hit the world

		idEntity* entHit = gameLocal.entities[result.c.entityNum];

		if ( entHit == gameLocal.world )
		{
			if ( newBounds[1][2] > result.endpos.z )
			{
				newBounds[1][2] = result.endpos.z; // move the upper bounds down
			}
			break;
		}

		// Continue the trace from the struck point

		start = result.endpos;
		ignore = entHit; // for the next leg, ignore the entity we struck
	}

	bounds = newBounds.ExpandSelf(2.0); // grayman #3424 - expand a bit to catch floors
}

int CSearchManager::StartSearchForHidingSpotsWithExclusionArea
(
	Search* search,
	const idVec3& hideFromLocation,
	int hidingSpotTypesAllowed,
	idAI* p_ignoreAI
)
{
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::StartSearchForHidingSpotsWithExclusionArea - %s\r",p_ignoreAI->GetName()); // grayman debug
	// Set search bounds
	idBounds searchBounds = search->_limits;
	idBounds searchExclusionBounds = search->_exclusion_limits;
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("            searchBounds = [%s]\r",searchBounds.ToString()); // grayman debug
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("   searchExclusionBounds = [%s]\r",searchExclusionBounds.ToString()); // grayman debug

	// Get aas

	idAAS* aas = p_ignoreAI->GetAAS();

	if (aas != NULL)
	{
		// Allocate object that handles the search
		bool b_searchCompleted = false;
		search->_hidingSpotSearchHandle = CHidingSpotSearchCollection::Instance().getOrCreateSearch
		(
			hideFromLocation, 
			aas, 
			HIDING_OBJECT_HEIGHT,
			searchBounds,
			searchExclusionBounds,
			hidingSpotTypesAllowed,
			p_ignoreAI,
			gameLocal.framenum,
			b_searchCompleted
		);

		// Wait at least one frame for other AIs to indicate they want to share
		// this search. Return result indicating search is not done yet.
		return 1;
	}

	DM_LOG(LC_AI, LT_ERROR)LOGSTRING("Cannot perform Event_StartSearchForHidingSpotsWithExclusionArea if no AAS is set for the AI\r");
	
	// Search is done since there is no search
	return 0;
}

void CSearchManager::Save(idSaveGame* savefile) const
{
	// save searches
	int numSearches = _searches.Num();
	savefile->WriteInt(numSearches);
	for (int i = 0 ; i < numSearches ; i++)
	{
		const Search *search = &_searches[i];

		savefile->WriteInt(search->_searchID);
		savefile->WriteInt(search->_hidingSpotSearchHandle);
		savefile->WriteVec3(search->_origin);
		savefile->WriteBounds(search->_limits);
		savefile->WriteBounds(search->_exclusion_limits);
		search->_hidingSpots.Save(savefile);
		savefile->WriteBool(search->_hidingSpotsReady);
		int num = search->_randomHidingSpotIndexes.size();
		savefile->WriteInt(num);
		for ( int j = 0 ; j < num ; j++ )
		{
			savefile->WriteInt(search->_randomHidingSpotIndexes[j]);
		}

		// save assignments
		num = search->_assignments.Num();
		savefile->WriteInt(num);
		for (int j = 0 ; j < num ; j++)
		{
			const Assignment *assignment = &search->_assignments[j];

			savefile->WriteVec3(assignment->_origin);
			savefile->WriteFloat(assignment->_innerRadius);
			savefile->WriteFloat(assignment->_outerRadius);
			savefile->WriteObject(assignment->_searcher);
			savefile->WriteInt(assignment->_lastSpotAssigned);
			savefile->WriteInt(static_cast<int>(assignment->_searcherRole));
		}
	}
}

void CSearchManager::Restore(idRestoreGame* savefile)
{
	// restore searches
	_searches.Clear();
	int numSearches;
	savefile->ReadInt(numSearches);
	_searches.SetNum(numSearches);
	for (int i = 0 ; i < numSearches ; i++)
	{
		Search search;

		savefile->ReadInt(search._searchID);
		savefile->ReadInt(search._hidingSpotSearchHandle);
		savefile->ReadVec3(search._origin);
		savefile->ReadBounds(search._limits);
		savefile->ReadBounds(search._exclusion_limits);
		search._hidingSpots.Restore(savefile);
		savefile->ReadBool(search._hidingSpotsReady);
		int num;
		savefile->ReadInt(num);
		search._randomHidingSpotIndexes.clear();
		for ( int j = 0 ; j < num ; j++ )
		{
			int n;
			savefile->ReadInt(n);
			search._randomHidingSpotIndexes.push_back(n);
		}

		// restore assignments
		search._assignments.Clear();
		savefile->ReadInt(num);
		search._assignments.SetNum(num);
		for (int j = 0 ; j < num ; j++)
		{
			Assignment assignment;

			savefile->ReadVec3(assignment._origin);
			savefile->ReadFloat(assignment._innerRadius);
			savefile->ReadFloat(assignment._outerRadius);
			savefile->ReadObject(reinterpret_cast<idClass*&>(assignment._searcher));
			savefile->ReadInt(assignment._lastSpotAssigned);

			int n;
			savefile->ReadInt(n);
			assignment._searcherRole = static_cast<smRole_t>(n);

			search._assignments.Append(assignment);
		}

		_searches.Append(search);
	}
}
/*
// prints hiding spots
void CSearchManager::DebugPrint(Search* search)
{
	int numSpots = search->_hidingSpots.getNumSpots();
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("Search has %d hiding spots\r",numSpots); // grayman debug
	for ( int i = 0 ; i < numSpots ; i++ )
	{
		idVec3 p;
		idBounds areaNodeBounds;
		darkModHidingSpot* p_spot = search->_hidingSpots.getNthSpotWithAreaNodeBounds(i, areaNodeBounds);
		if (p_spot != NULL)
		{
			p = p_spot->goal.origin;
			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("%d: [%s]\r",i,p.ToString()); // spot is good
		}
	}
}
*/
// prints search contents
void CSearchManager::DebugPrintSearch(Search* search)
{
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("============== Search ==============\r");
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("_hidingSpotSearchHandle = %d\r",search->_hidingSpotSearchHandle);
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("              _searchID = %d\r",search->_searchID);
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("                _origin = [%s]\r",search->_origin.ToString());
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("                _limits = [%s]\r",search->_limits.ToString());
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("      _exclusion_limits = [%s]\r",search->_exclusion_limits.ToString());
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("            no of spots = %d\r",search->_hidingSpots.getNumSpots());
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("      no of assignments = %d\r",search->_assignments.Num());
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("====================================\r");
}

// prints assignment contents
void CSearchManager::DebugPrintAssignment(Assignment* assignment)
{
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("============== Assignment ==============\r");
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("          _origin = [%s]\r",assignment->_origin.ToString());
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("     _innerRadius = %f\r",assignment->_innerRadius);
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("     _outerRadius = %f\r",assignment->_outerRadius);
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("               ai = '%s'\r",assignment->_searcher ? assignment->_searcher->GetName():"NULL");
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("_lastSpotAssigned = %d\r",assignment->_lastSpotAssigned);
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("    _searcherRole = %d\r",(int)assignment->_searcherRole);
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("========================================\r");
}

void CSearchManager::destroyCurrentHidingSpotSearch(Search* search)
{
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::destroyCurrentHidingSpotSearch ...\r"); // grayman debug
	// Destroy the list of hiding spots
	// Remove assignments from participating AI

	// Check to see if there is one
	if (search->_hidingSpotSearchHandle != NULL_HIDING_SPOT_SEARCH_HANDLE)
	{
		// Dereference current search
		CHidingSpotSearchCollection::Instance().dereference(search->_hidingSpotSearchHandle);
		search->_hidingSpotSearchHandle = NULL_HIDING_SPOT_SEARCH_HANDLE;
	}

	// No hiding spots
	search->_hidingSpots.clear();

	for (int i = 0 ; i < search->_assignments.Num() ; i++)
	{
		idAI* ai = search->_assignments[i]._searcher;

		if (ai)
		{
			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("   telling '%s' that the search is ending\r",ai->GetName()); // grayman debug
			// greebo: Clear the initial alert position
			ai->GetMemory().alertSearchCenter = idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY);
			ai->m_searchID = -1;
		}
	}
}

// grayman debug - delete when done
void CSearchManager::PrintMe(int n, idAI* owner)
{
	// grayman debug - find where/when tree is getting written over
	Search *search = gameLocal.m_searchManager->GetSearch(owner->m_searchID);
	if (search)
	{
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("PrintMe %d - %s - search exists\r",n,owner->GetName()); // grayman debug
		CDarkmodHidingSpotTree *tree = &search->_hidingSpots; // The hiding spots for this search
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("   tree has %d spots\r",owner->GetName(),tree->getNumSpots()); // grayman debug
		if (tree->getNumSpots() > 0)
		{
			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("       p_firstArea = %x\r",tree->getFirstArea()); // grayman debug
			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("   p_firstArea->id = %d\r",tree->getFirstArea()->id); // grayman debug
		}
	}
	else
	{
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("PrintMe %d - %s - search doesn't exist yet\r",n,owner->GetName()); // grayman debug
	}
	// end grayman debug
}

CSearchManager* CSearchManager::Instance()
{
	static CSearchManager _manager;
	return &_manager;
}

