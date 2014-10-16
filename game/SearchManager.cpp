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
#include "Misc.h"
#include "ai/Memory.h"

#define SEARCH_RADIUS_FACTOR 0.707107f	  // sqrt(0.5)
#define SEARCH_RADIUS_ONE_SEARCHER 126.0f // If xy radius of entire search is less than this, only allow one searcher
#define SEARCH_MAX_GUARD_SPOT_DISTANCE 500.0f // don't consider guard spot entities beyond this distance from search origin
#define SEARCH_MIN_OBS_DISTANCE 200.0f // minimum observation distance

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
		search->_assignments.Clear();
	}

	_searches.Clear();
	uniqueSearchID = 0;
}

int CSearchManager::StartNewHidingSpotSearch(idAI* ai)
{
	// The AI wants to start a new hiding spot search.
	
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::StartNewHidingSpotSearch - %s wants to start a new hiding spot search\r",ai->GetName()); // grayman debug
	ai::Memory& memory = ai->GetMemory();
	idVec3 searchPoint = memory.alertPos;
	idVec3 searchVolume = memory.alertSearchVolume;
	idVec3 searchExclusionVolume = memory.alertSearchExclusionVolume;
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::StartNewHidingSpotSearch - %s searchPoint = [%s], searchVolume = [%s], searchExclusionVolume = [%s]\r",ai->GetName(),searchPoint.ToString(), searchVolume.ToString(), searchExclusionVolume.ToString()); // grayman debug

	Search *search = NULL;
	int searchID = -1;

	// See if there's a search underway for the provided memory.currentSearchEventID

	search = GetSearchWithEventID(memory.currentSearchEventID,ai);
	if (search)
	{
		searchID = search->_searchID;
	}

	// If there's no existing search that ai can join based on event id,
	// see if there's one at or near the search location.

	if (searchID < 0)
	{
		search = GetSearchAtLocation(searchPoint,ai);
		if (search)
		{
			searchID = search->_searchID;
		}
	}

	// If we found an existing search, we don't need to start
	// a new one. Just copy some data from the existing search
	// to the ai's memory.

	if (searchID >= 0)
	{
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::StartNewHidingSpotSearch - %s found search %d, which is already underway\r",ai->GetName(),search->_searchID); // grayman debug

		// reset ai's data
		memory.alertPos = search->_origin;
		memory.alertSearchVolume = search->_limits.GetSize()/2.0f;
		memory.alertSearchExclusionVolume = search->_exclusion_limits.GetSize()/2.0f;
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("   resetting alertPos = [%s], alertSearchVolume = [%s], searchExclusionVolume = [%s]\r",memory.alertPos.ToString(), memory.alertSearchVolume.ToString(), memory.alertSearchExclusionVolume.ToString()); // grayman debug
		return searchID;
	}

	if (searchPoint.Compare(idVec3(0,0,0)) || searchPoint.Compare(idVec3(idMath::INFINITY,idMath::INFINITY,idMath::INFINITY)))
	{
		// No data to start a search, so drop back to Suspicious
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::StartNewHidingSpotSearch - %s can't start searching: no eventID, no location match, no alertPos\r",ai->GetName()); // grayman debug
		ai->SetAlertLevel(ai->thresh_3 - 0.1);
		return -1;
	}

	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::StartNewHidingSpotSearch - %s no existing search, create a new one\r",ai->GetName()); // grayman debug
	// no existing search, so start a new one

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
	search->_guardSpots.Clear(); // spots to send guards to
	search->_guardSpotsReady = false; // whether the list of guard spots is ready for use or not
	search->_searcherCount = 0; // number of searchers

	switch(memory.alertType)
	{
	case ai::EAlertTypeSuspicious:
		search->_assignmentFlags = SEARCH_SUSPICIOUS;
		break;
	case ai::EAlertTypeEnemy:
		search->_assignmentFlags = SEARCH_ENEMY;
		break;
	case ai::EAlertTypeFailedKO:
		search->_assignmentFlags = SEARCH_FAILED_KO;
		break;
	case ai::EAlertTypeWeapon:
		search->_assignmentFlags = SEARCH_WEAPON;
		break;
	case ai::EAlertTypeBlinded:
		search->_assignmentFlags = SEARCH_BLINDED;
		break;
	case ai::EAlertTypeDeadPerson:
		search->_assignmentFlags = SEARCH_DEAD;
		break;
	case ai::EAlertTypeUnconsciousPerson:
		search->_assignmentFlags = SEARCH_UNCONSCIOUS;
		break;
	case ai::EAlertTypeBlood:
		search->_assignmentFlags = SEARCH_BLOOD;
		break;
	case ai::EAlertTypeLightSource:
		search->_assignmentFlags = SEARCH_LIGHT;
		break;
	case ai::EAlertTypeMissingItem:
		search->_assignmentFlags = SEARCH_MISSING;
		break;
	case ai::EAlertTypeBrokenItem:
		search->_assignmentFlags = SEARCH_BROKEN;
		break;
	case ai::EAlertTypeDoor:
		search->_assignmentFlags = SEARCH_DOOR;
		break;
	case ai::EAlertTypeSuspiciousItem:
		search->_assignmentFlags = SEARCH_SUSPICIOUSITEM;
		break;
	case ai::EAlertTypeRope:
		search->_assignmentFlags = SEARCH_ROPE;
		break;
	case ai::EAlertTypeHitByProjectile:
		search->_assignmentFlags = SEARCH_PROJECTILE;
		break;
	default: // grayman debug
		search->_assignmentFlags = SEARCH_NOTHING;
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::StartNewHidingSpotSearch - memory.alertType '%d' has no SEARCH flags\r",(int)memory.alertType); // grayman debug
		break;
	}

	search->_eventID = memory.currentSearchEventID; // the ID of the suspicious event that caused this search

	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::StartNewHidingSpotSearch - search->_assignmentFlags = %x\r",search->_assignmentFlags); // grayman debug
	DebugPrintSearch(search); // grayman debug - comment when done

	// Add search to list
	_searches.Append(*search);

	return search->_searchID;
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
	}
	else // grayman debug
	{
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::PerformHidingSpotSearch - %s hiding spot search NOT completed\r",ai->GetName()); // grayman debug
	}
}

void CSearchManager::RandomizeHidingSpotList(Search* search)
{
	if (search == NULL)
	{
		return; // invalid search
	}

	int numSpots = search->_hidingSpots.getNumSpots();
	//DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::RandomizeHidingSpotList - randomize the indices for %d spots\r",numSpots); // grayman debug
	search->_randomHidingSpotIndexes.clear(); // clear any existing array elements

	// Fill the array with integers from 0 to numSpots-1.
	for ( int i = 0 ; i < numSpots ; i++ )
	{
		search->_randomHidingSpotIndexes.push_back(i);
	}

	// TODO: should we shuffle or not? If not, we don't need _randomHidingSpotIndexes
	/*
    // Shuffle those integers by randomly exchanging pairs.
    for ( int i = 0 ; i < (numSpots-1) ; i++ )
	{
        int r = i + gameLocal.random.RandomInt(numSpots - i); // Random remaining position.
        int temp = search->_randomHidingSpotIndexes[i];
		search->_randomHidingSpotIndexes[i] = search->_randomHidingSpotIndexes[r];
		search->_randomHidingSpotIndexes[r] = temp;
    }*/
}

Search* CSearchManager::GetSearch(int searchID) // returns a pointer to the requested search
{
	if (searchID < 0)
	{
		return NULL; // invalid ID
	}

	for ( int i = 0 ; i < _searches.Num() ; i++ )
	{
		Search *search = &_searches[i];
		if (search->_searchID == searchID)
		{
			return search;
		}
	}

	return NULL;
}

Search* CSearchManager::GetSearchWithEventID(int eventID, idAI* ai)
{
	if (eventID < 0)
	{
		return NULL; // invalid ID
	}

	for ( int i = 0 ; i < _searches.Num() ; i++ )
	{
		Search *search = &_searches[i];
		if (search->_eventID == eventID)
		{
			// If the ai has already participated in this
			// search, he can't rejoin it.

			if ( ai->HasSearchedEvent(search->_eventID) )
			{
				return NULL;
			}

			return search;
		}
	}

	return NULL;
}

Search* CSearchManager::GetSearchAtLocation(idVec3 location, idAI* ai) // returns a pointer to the requested search
{
	for ( int i = 0 ; i < _searches.Num() ; i++ )
	{
		Search *search = &_searches[i];
		if ((search->_origin - location).LengthFast() < 100)
		{
			// This search is close to where the AI wants to start
			// searching, so we could assign him to this existing
			// search. However, if he's already participated in this
			// search, he can't rejoin it.

			if ( ai->HasSearchedEvent(search->_eventID) )
			{
				continue;
			}

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
		ai->GetMemory().noMoreHidingSpots = true;
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

				nextSpot = hidingSpot->goal.origin; // point is good so far

				DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("   %s so far, [%s] is good\r",ai->GetName(),nextSpot.ToString()); // grayman debug

				if (assignment->_limits.ContainsPoint(nextSpot))
				{
					search->_randomHidingSpotIndexes[i] = -1; // don't assign this good spot again
					DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::GetNextHidingSpot - %s given hiding spot [%s]\r",ai->GetName(),nextSpot.ToString()); // grayman debug
					return true;
				}

				DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::GetNextHidingSpot - %s hidingSpot is outside the search area, so it's NG\r",ai->GetName()); // grayman debug
				return false; // didn't find one this time, maybe next time we'll find one
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
	//DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::ContinueSearchForHidingSpots - %s search->_hidingSpotSearchHandle = %d\r",ai->GetName(),search->_hidingSpotSearchHandle); // grayman debug
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
		//DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::ContinueSearchForHidingSpots - %s No current hiding spot search to continue\r",ai->GetName()); // grayman debug
		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("No current hiding spot search to continue\r");
		return 0;
	}

	//DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::ContinueSearchForHidingSpots - %s Call finder method to continue search\r",ai->GetName()); // grayman debug
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

	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::ContinueSearchForHidingSpots - %s no more processing to do\r",ai->GetName()); // grayman debug
	// No more processing to do at this point

	if (!search->_hidingSpotsReady)
	{
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::ContinueSearchForHidingSpots - %s hiding spots aren't ready\r",ai->GetName()); // grayman debug
		unsigned int refCount;

		// Get finder we just referenced
		p_hidingSpotFinder = CHidingSpotSearchCollection::Instance().getSearchAndReferenceCountByHandle 
			(
				search->_hidingSpotSearchHandle,
				refCount
			);

		search->_hidingSpots.clear();

		//DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::ContinueSearchForHidingSpots - %s copying finished tree to search->_hidingSpots\r",ai->GetName()); // grayman debug
		p_hidingSpotFinder->hidingSpotList.copy(&search->_hidingSpots);
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::PerformHidingSpotSearch - %s there are %d hiding spots\r",ai->GetName(),search->_hidingSpots.getNumSpots()); // grayman debug
		//p_hidingSpotFinder->hidingSpotList.getOneNth(refCount,search->_hidingSpots);
		search->_hidingSpotsReady = true; // the hiding spot list is complete

		//DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::ContinueSearchForHidingSpots - %s hiding spots ready\r",ai->GetName()); // grayman debug
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
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::ContinueSearchForHidingSpots - %s Hiding spot search completed\r",ai->GetName()); // grayman debug
	}

	return 0;
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

	float innerRadius = 0;
	float outerRadius = 0;
	idBounds searchBounds;
	idBounds searchExclusionBounds;
	smRole_t searcherRole = E_ROLE_NONE; // no assignment yet

	// Active searchers are allowed to be armed or unarmed/civilian.

	// Does this search require active searchers?

	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::JoinSearch - %s _assignmentFlags = %x\r",ai->GetName(),search->_assignmentFlags); // grayman debug
	if (search->_assignmentFlags & (SEARCH_SEARCHER_MILL|SEARCH_SEARCH))
	{
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::JoinSearch - %s need a searcher\r",ai->GetName()); // grayman debug

		if (numAssignments == 0)
		{
			// A single AI gets to search the entire search area.

			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::JoinSearch - %s first ai to join this search\r",ai->GetName()); // grayman debug
			innerRadius = 0;
			idVec3 searchSize = search->_limits.GetSize();
			float xRad = searchSize.x/2.0f;
			float yRad = searchSize.y/2.0f;
			outerRadius = idMath::Sqrt(xRad*xRad + yRad*yRad);
			search->_outerRadius = outerRadius;
			searchBounds = search->_limits;
			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::JoinSearch - %s search area for first AI = %f\r",ai->GetName(),idMath::PI*outerRadius*outerRadius); // grayman debug
			searcherRole = E_ROLE_SEARCHER;
		}
		else if (numAssignments == 1)
		{
			if (search->_outerRadius <= SEARCH_RADIUS_ONE_SEARCHER)
			{
				// there isn't enough search area to have more than one searcher
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

				// Split search area in half.

				idBounds searchBounds1;
				idBounds searchBounds2;
				searchBounds1 = searchBounds2 = search->_limits;
				idVec3 size = searchBounds1.GetSize();
				if (size.x > size.y)
				{
					searchBounds1[1].x -= size.x/2.0f;
					searchBounds2[0].x += size.x/2.0f;
				}
				else
				{
					searchBounds1[1].y -= size.y/2.0f;
					searchBounds2[0].y += size.y/2.0f;
				}
				search->_assignments[0]._limits = searchBounds1;
				searchBounds = searchBounds2;
				searcherRole = E_ROLE_SEARCHER;
				//DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::JoinSearch - %s Bark('snd_helpSearch')\r",ai->GetName()); // grayman debug
				//ai->Bark("snd_helpSearch"); // Bark that you're joining the search

				DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::JoinSearch - %s search size for first AI  = [%s]\r",ai->GetName(),searchBounds1.GetSize().ToString()); // grayman debug
				DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::JoinSearch - %s search size for second AI = [%s]\r",ai->GetName(),searchBounds2.GetSize().ToString()); // grayman debug
			}
		}
	}

	if (searcherRole == E_ROLE_NONE)
	{
		// No role assigned. You'll either be a guard or an observer.
		// If you're unarmed or a civilian, or your health is low, you
		// can't be a guard, so you'll be an observer until something
		// chases you away. Don't worry here about the number of guard
		// spots available. If SearchingState finds there are none, it
		// will change your role to an observer.

		idVec3 searchSize = search->_limits.GetSize();
		float xRad = searchSize.x/2.0f;
		float yRad = searchSize.y/2.0f;

		// outerRadius defines the search perimeter, where observers will stand
		outerRadius = max(SEARCH_MIN_OBS_DISTANCE,idMath::Sqrt(xRad*xRad + yRad*yRad));
		innerRadius = 0; // no active searching
		if (!ai->IsAfraid()) // armed AI becomes a guard
		{
			// Do we need guards for this search?

			if (search->_assignmentFlags & (SEARCH_GUARD_MILL|SEARCH_GUARD))
			{
				DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::JoinSearch - %s joining this search as a guard\r",ai->GetName()); // grayman debug
				searcherRole = E_ROLE_GUARD;
				//DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::JoinSearch - %s Bark('snd_helpSearch')\r",ai->GetName()); // grayman debug
				//ai->Bark("snd_helpSearch"); // Bark that you're joining the search
			}
		}

		if (searcherRole == E_ROLE_NONE)
		{
			// Everyone else is unfit to guard a spot and is relegated to observing.
			// Do we need observers for this search?
			if (search->_assignmentFlags & (SEARCH_OBSERVER_MILL|SEARCH_OBSERVE))
			{
				DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::JoinSearch - %s joining this search as a civilian observer\r",ai->GetName()); // grayman debug
				searcherRole = E_ROLE_OBSERVER;
				//DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::JoinSearch - %s Bark('snd_helpSearch')\r",ai->GetName()); // grayman debug
				//ai->Bark("snd_helpSearch"); // Bark that you're joining the search
			}
		}
	}

	if (searcherRole == E_ROLE_NONE)
	{
		// Can't join this search. Sorry.
		return false;
	}

	// Leave your current search, if there is one.

	if (ai->m_searchID >= 0)
	{
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::JoinSearch - %s leaving old search with id %d, calling LeaveSearch()\r",ai->GetName(),ai->m_searchID); // grayman debug
		gameLocal.m_searchManager->LeaveSearch(ai->m_searchID,ai);
	}

	// Create new assignment

	Assignment newAssignment;
	Assignment *assignment = &newAssignment;

	assignment->_origin = search->_origin; // center of search area, location of alert stimulus
	assignment->_outerRadius = outerRadius;
	assignment->_limits = searchBounds;
	assignment->_searcher = ai; // AI who owns this assignment
	assignment->_lastSpotAssigned = -1; // if actively searching, the most recent spot assigned to a searcher; index into _randomHidingSpotIndexes
	assignment->_searcherRole = searcherRole; // the AI's role

	DebugPrintAssignment(assignment); // grayman debug - comment when done

	search->_assignments.Append(*assignment); // add this new assignment to the search

	search->_searcherCount++; // number of searchers (includes active searchers, guards, and observers)

	ai->m_searchID = searchID;
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::JoinSearch - %s has joined search %d\r",ai->GetName(),ai->m_searchID); // grayman debug

	ai::Memory& memory = ai->GetMemory();
	memory.lastAlertPosSearched = search->_origin;
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::JoinSearch - %s lastAlertPosSearched = [%s]\r",ai->GetName(),memory.lastAlertPosSearched.ToString()); // grayman debug
	memory.alertSearchCenter = search->_origin;
	memory.alertPos = search->_origin;
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::JoinSearch - %s setting alertPos to [%s]\r",ai->GetName(),memory.alertPos.ToString()); // grayman debug
	memory.currentSearchEventID = search->_eventID;
	ai->AddSuspiciousEvent(search->_eventID);

	// Four activities are available:
	// 1 - actively search
	// 2 - mill about the search origin
	// 3 - guard at portals or mapper-designated locations (via guard entities)
	// 4 - observe from a safe distance at the perimeter of the search area

	// In the following chart, which shows what AI should do when they join a search:
	// "armed" = armed and good health and not a civilian
	// "unarmed" = unarmed and/or poor health and/or a civilian

	// Alert type					| Event						| Active searchers		| Guards					| Observers
	// -----------------------------------------------------------------------------------------------------------------------
	// EAlertTypeHitByProjectile	| Hit By Arrow				| if unarmed, flee, otherwise:
	//															| search				| go to guard spot			| go to observation spot
	// EAlertTypeEnemy				| Enemy, Tactile by Enemy	| armed: enter combat / unarmed: flee
	// EAlertTypeFailedKO			| Failed KO					| armed: enter combat / unarmed: flee
	// EAlertTypeWeapon				| Sword, Blackjack, bow		| mill, search			| mill						| mill
	// EAlertTypeBlinded			| Blinded					| 1 AI: searches		|							|
	//								|							| >1 AI: mill, search	| mill						| mill, go to observation spot
	// EAlertTypeDeadPerson			| Dead Person				| search				| go to guard spot			| armed: go to observation spot / unarmed: flee
	// EAlertTypeUnconsciousPerson	| Unconscious Person		| search				| go to guard spot			| armed: go to observation spot / unarmed: flee
	// EAlertTypeRope				| Rope						| mill, search			| mill, go to guard spot	| mill, go to observation spot
	// EAlertTypeSuspiciousItem		|							| mill, search			| mill, go to guard spot	| mill, go to observation spot
	// EAlertTypeBlood				| Blood						| mill, search			| mill, go to guard spot	| mill, go to observation spot
	// EAlertTypeBrokenItem			| Broken Item				| mill, search			| mill						| mill
	// EAlertTypeMissingItem		| Missing Item				| search				| mill, go to guard spot	| mill, go to observation spot			
	// EAlertTypeDoor				| Door						| 1 AI searches			| n/a						| n/a
	// EAlertTypeLightSource		| Light Source				| 1 AI searches			| n/a						| n/a
	// EAlertTypeSuspicious			| Hit By Moveable			| 1 AI searches			| n/a						| n/a
	//								| Flying arrow or fireball	| 1 AI searches			| n/a						| n/a
	//								| Picked Pocket				| n/a					| n/a						| n/a
	//								| Audio						| 1 AI: searches		|							|
	//								|							| >1 AI: mill, search	| mill						| mill, go to observation spot

	// The assignment flags for the search tell us whether we should mill about or not
	// before executing our roles.

	memory.shouldMill = false;
	if (searcherRole == E_ROLE_SEARCHER)
	{
		if (search->_assignmentFlags & SEARCH_SEARCHER_MILL)
		{
			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::JoinSearch - %s searcher, will mill about\r",ai->GetName()); // grayman debug
			memory.shouldMill = true;
		}
	}
	else if (searcherRole == E_ROLE_GUARD)
	{
		if (search->_assignmentFlags & SEARCH_GUARD_MILL)
		{
			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::JoinSearch - %s guard, will mill about\r",ai->GetName()); // grayman debug
			memory.shouldMill = true;
		}
	}
	else // observer
	{
		if (search->_assignmentFlags & SEARCH_OBSERVER_MILL)
		{
			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::JoinSearch - %s observer, will mill about\r",ai->GetName()); // grayman debug
			memory.shouldMill = true;
		}
	}

	// Mark all warning flags from this ai to those in the search
	// to simulate having already warned about this event.
	// This will prevent future warnings between participants, since
	// each one remembers who else was there.

	for ( int i = 0 ; i < search->_assignments.Num() ; i++ )
	{
		Assignment *assignment = &search->_assignments[i];
		idAI* searcher = assignment->_searcher;
		if ( (searcher == NULL) || (searcher == ai) )
		{
			continue;
		}

		ai->AddWarningEvent(searcher,search->_eventID); // log that a (fake) warning passed between them
		searcher->AddWarningEvent(ai,search->_eventID); // log that a (fake) warning passed between them
	}

	if (!memory.shouldMill) // grayman debug
	{
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::JoinSearch - %s will not mill about\r",ai->GetName()); // grayman debug
	}

	return true;
}

// Searcher leaves the search. If the last searcher leaves a
// search, the search is destroyed. The searcher's assignment
// isn't deleted, though, in case others join the search before
// it ends, and we want to know what previous searchers were assigned.

void CSearchManager::LeaveSearch(int searchID, idAI* ai)
{
	Search *search = GetSearch(searchID);
	if (search == NULL)
	{
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::LeaveSearch - %s couldn't find search for searchID %d\r",ai->GetName(),searchID); // grayman debug
		return;
	}

	int numAssignments = search->_assignments.Num();
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::LeaveSearch - %s numAssignments = %d\r",ai->GetName(),numAssignments); // grayman debug

	for (int i = 0 ; i < numAssignments ; i++)
	{
		Assignment *assignment = &search->_assignments[i];
		if (assignment->_searcher == ai)
		{
			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::LeaveSearch - %s deactivating an abandoned assignment from search\r",ai->GetName()); // grayman debug
			assignment->_searcher = NULL; // deactivate the assignment
			ai->m_searchID = -1; // leave the search
			ai->GetMemory().currentSearchEventID = -1;
			search->_searcherCount--;
			break;
		}
	}
	
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::LeaveSearch - %s _searcherCount reduced to %d\r",ai->GetName(),search->_searcherCount); // grayman debug
	if (search->_searcherCount == 0)
	{
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::LeaveSearch - %s no searchers left, destroying search %d\r",ai->GetName(),search->_searchID); // grayman debug
		destroyCurrentHidingSpotSearch(search);

		// find and remove the search
		for (int i = 0 ; i < _searches.Num() ; i++)
		{
			search = &_searches[i];
			if (search->_searchID == searchID)
			{
				search->_assignments.Clear();
				_searches.RemoveIndex(i);
				break;
			}
		}
	}

	// Now that you've left the search, stop whatever task you were doing
	// for that search

	ai::Memory& memory = ai->GetMemory();
	if (memory.hidingSpotInvestigationInProgress || memory.guardingInProgress || memory.millingInProgress)
	{
		memory.hidingSpotInvestigationInProgress = false;
		memory.guardingInProgress = false;
		memory.millingInProgress = false;
		memory.currentSearchSpot = idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY);
		memory.currentSearchEventID = -1;
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

int SortPathGuardsByPriority( tdmPathGuard* const* a, tdmPathGuard* const* b )
{
	if ( (*a)->m_priority < (*b)->m_priority )
	{
		return 1;
	}
	if ( (*a)->m_priority > (*b)->m_priority )
	{
		return -1;
	}
	return 0;
}

void CSearchManager::CreateListOfGuardSpots(Search* search, idAI* ai)
{
	if (search == NULL)
	{
		return;
	}

	// Create a list of spots to send guards to for the
	// purpose of guarding a spot. These guards aren't searching
	// the area; they're keeping an eye on the perimeter, or perhaps
	// guarding a sensitive area or treasure object.

	// If any of these points lie w/in the search boundary, ignore it.
	// This is done to reduce congestion.

	// Find all guard entities nearby (less than SEARCH_MAX_GUARD_SPOT_DISTANCE away)
	idList<tdmPathGuard*> guardEntities;
	guardEntities.Clear();
	for ( idEntity* ent = gameLocal.spawnedEntities.Next() ; ent != NULL ; ent = ent->spawnNode.Next() )
	{
		if ( !ent || !ent->IsType( tdmPathGuard::Type ) )
		{
			continue;
		}

		if (search->_limits.ContainsPoint(ent->GetPhysics()->GetOrigin()))
		{
			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::CreateListOfGuardSpots - guard entity '%s' is inside the limits [%s]\r",ent->GetName(),search->_limits.ToString()); // grayman debug
			continue;
		}

		tdmPathGuard* guardEntity = static_cast<tdmPathGuard*>( ent );
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::CreateListOfGuardSpots - found guard entity '%s'\r",guardEntity->GetName()); // grayman debug
		float dist = (search->_origin - guardEntity->GetPhysics()->GetOrigin()).LengthFast();
		if (dist < SEARCH_MAX_GUARD_SPOT_DISTANCE)
		{
			guardEntities.Append(guardEntity);
		}
	}

	// Sort the guard entities by priority

	int numGuardEntities = guardEntities.Num();
	if (numGuardEntities > 0)
	{
		if (numGuardEntities > 1)
		{
			// prioritize using the "priority" spawnarg
			guardEntities.Sort( SortPathGuardsByPriority );
		}

		// list is prioritized, so initialize the _guardSpots list with it

		for (int i = 0 ; i < numGuardEntities ; i++)
		{
			tdmPathGuard* ent = guardEntities[i];
			idVec3 entOrigin = ent->GetPhysics()->GetOrigin();
			search->_guardSpots.Append(idVec4(entOrigin.x,entOrigin.y,entOrigin.z,ent->m_angle));
			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("CSearchManager::CreateListOfGuardSpots - added guard entity data, priority = %d\r",ent->m_priority); // grayman debug
		}
	}

	// Find exits from the cluster the search is in.
	gameLocal.GetPortals(search,ai);
	search->_guardSpotsReady = true;
}

void CSearchManager::Save(idSaveGame* savefile) const
{
	savefile->WriteInt(uniqueSearchID);

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
		savefile->WriteFloat(search->_outerRadius);
		search->_hidingSpots.Save(savefile);
		savefile->WriteBool(search->_hidingSpotsReady);
		int num = search->_randomHidingSpotIndexes.size();
		savefile->WriteInt(num);
		for ( int j = 0 ; j < num ; j++ )
		{
			savefile->WriteInt(search->_randomHidingSpotIndexes[j]);
		}

		savefile->WriteInt(search->_guardSpots.Num());
		for ( int j = 0 ; j < search->_guardSpots.Num() ; j++ )
		{
			savefile->WriteVec4(search->_guardSpots[j]);
		}

		savefile->WriteBool(search->_guardSpotsReady);
		savefile->WriteUnsignedInt(search->_assignmentFlags);

		// save assignments
		num = search->_assignments.Num();
		savefile->WriteInt(num);
		for (int j = 0 ; j < num ; j++)
		{
			const Assignment *assignment = &search->_assignments[j];

			savefile->WriteVec3(assignment->_origin);
			savefile->WriteFloat(assignment->_outerRadius);
			savefile->WriteBounds(assignment->_limits);
			savefile->WriteObject(assignment->_searcher);
			savefile->WriteInt(assignment->_lastSpotAssigned);
			savefile->WriteInt(static_cast<int>(assignment->_searcherRole));
		}

		savefile->WriteInt(search->_searcherCount);
		savefile->WriteInt(search->_eventID);
	}
}

void CSearchManager::Restore(idRestoreGame* savefile)
{
	savefile->ReadInt(uniqueSearchID);

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
		savefile->ReadFloat(search._outerRadius);
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

		savefile->ReadInt(num);
		search._guardSpots.SetGranularity(1);
		search._guardSpots.SetNum(num);
		for ( int j = 0 ; j < num ; j++ )
		{
			savefile->ReadVec4(search._guardSpots[j]);
		}

		savefile->ReadBool(search._guardSpotsReady);
		savefile->ReadUnsignedInt(search._assignmentFlags);

		// restore assignments
		search._assignments.Clear();
		savefile->ReadInt(num);
		search._assignments.SetNum(num);
		for (int j = 0 ; j < num ; j++)
		{
			Assignment assignment;

			savefile->ReadVec3(assignment._origin);
			savefile->ReadFloat(assignment._outerRadius);
			savefile->ReadBounds(assignment._limits);
			savefile->ReadObject(reinterpret_cast<idClass*&>(assignment._searcher));
			savefile->ReadInt(assignment._lastSpotAssigned);

			int n;
			savefile->ReadInt(n);
			assignment._searcherRole = static_cast<smRole_t>(n);

			search._assignments.Append(assignment);
		}

		savefile->ReadInt(search._searcherCount);
		savefile->ReadInt(search._eventID);

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
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("               _eventID = %d\r",search->_eventID);
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("                _origin = [%s]\r",search->_origin.ToString());
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("                _limits = [%s]\r",search->_limits.ToString());
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("      _exclusion_limits = [%s]\r",search->_exclusion_limits.ToString());
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("     no of hiding spots = %d\r",search->_hidingSpots.getNumSpots());
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("      no of guard spots = %d\r",search->_guardSpots.Num());
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("      no of assignments = %d\r",search->_assignments.Num());
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("       assignment flags = %x\r",search->_assignmentFlags);
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("====================================\r");

	//gameRenderWorld->DebugBox(colorRed, idBox(search->_limits), 60000);
}

// prints assignment contents
void CSearchManager::DebugPrintAssignment(Assignment* assignment)
{
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("============== Assignment ==============\r");
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("          _origin = [%s]\r",assignment->_origin.ToString());
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("     _outerRadius = %f\r",assignment->_outerRadius);
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("          _limits = [%s]\r",assignment->_limits.ToString());
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("               ai = '%s'\r",assignment->_searcher ? assignment->_searcher->GetName():"NULL");
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("_lastSpotAssigned = %d\r",assignment->_lastSpotAssigned);
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("    _searcherRole = %d\r",(int)assignment->_searcherRole);
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("========================================\r");

	//gameRenderWorld->DebugCircle(idVec4( 0.15f, 0.15f, 0.15f, 1.00f ), assignment->_origin,idVec3(0,0,1), assignment->_outerRadius, 100, 60000);
	//gameRenderWorld->DebugBox(idVec4( 0.15f, 0.15f, 0.15f, 1.00f ), idBox(assignment->_limits), 120000);
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
			ai->GetMemory().currentSearchEventID = -1;
		}
	}
}

/*
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
*/

CSearchManager* CSearchManager::Instance()
{
	static CSearchManager _manager;
	return &_manager;
}

