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

#include "Game_local.h"
#include "SearchManager.h"
#include "Misc.h"
#include "ai/Memory.h"

#define SEARCH_RADIUS_FACTOR 0.707107f	  // sqrt(0.5)
#define SEARCH_RADIUS_ONE_SEARCHER 126.0f // If xy radius of entire search is less than this, only allow one searcher
#define SEARCH_MAX_GUARD_SPOT_DISTANCE 500.0f // don't consider guard spot entities beyond this distance from search origin
#define SEARCH_MIN_OBS_DISTANCE 200.0f // minimum observation distance

// Constructor
CSearchManager::CSearchManager()
{
	_uniqueSearchID = 1; // the next unique id to assign to a new search
}

CSearchManager::~CSearchManager()
{
	Clear();
}

void CSearchManager::Clear()
{
	for ( int i = 0 ; i < _searches.Num() ; i++ )
	{
		Search *search = _searches[i];
		destroyCurrentHidingSpotSearch(search); // Destroy the list of hiding spots
		search->_assignments.Clear();
		delete _searches[i];
		_searches[i] = NULL;
	}

	_searches.Clear();
	_uniqueSearchID = 1;
}

Search* CSearchManager::StartNewSearch(idAI* ai)
{
	ai::Memory& memory = ai->GetMemory();

	idVec3 searchPoint = memory.alertPos;
	idVec3 searchVolume = memory.alertSearchVolume;
	idVec3 searchExclusionVolume = memory.alertSearchExclusionVolume;

	idVec3 minBounds(memory.alertPos - searchVolume);
	idVec3 maxBounds(memory.alertPos + searchVolume);

	idVec3 minExclusionBounds(memory.alertPos - searchExclusionVolume);
	idVec3 maxExclusionBounds(memory.alertPos + searchExclusionVolume);

	idBounds searchBounds = idBounds(minBounds, maxBounds);
	idBounds searchExclusionBounds = idBounds(minExclusionBounds, maxExclusionBounds);

	// grayman #2422 - to prevent AI from going upstairs or downstairs
	// to search spots over/under where they should be searching,
	// limit the search to the floor where the alert occurred.

	AdjustSearchLimits(searchBounds);

	Search* search = CreateSearch(_uniqueSearchID++);

	search->_hidingSpotSearchHandle = NULL_HIDING_SPOT_SEARCH_HANDLE;
	search->_origin = searchPoint;  // center of search area, location of alert stimulus
	search->_limits = searchBounds; // boundary of the search
	search->_exclusion_limits = searchExclusionBounds; // exclusion boundary of the search
	search->_outerRadius = 0.0f; // outer radius of search boundary
	search->_referenceAlertLevel = ai->AI_AlertLevel; // used when allocating alert levels to AI responding to help requests
	search->_hidingSpots.clear(); // The hiding spots for this search
	search->_hidingSpotsReady = false; // whether the hiding spot list is complete or still being built
	search->_guardSpots.Clear(); // spots to send guards to
	search->_guardSpotsReady = false; // whether the list of guard spots is ready for use or not
	search->_searcherCount = 0; // number of searchers

	if (ai->spawnArgs.GetBool("canSearchCooperatively", "1"))
	{
		search->_isCoopSearch = true;
		switch (memory.alertType)
		{
		case ai::EAlertTypeSuspicious:
		case ai::EAlertTypeSuspiciousVisual:
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
		case ai::EAlertTypeHitByMoveable:
			search->_assignmentFlags = SEARCH_MOVEABLE;
			break;
		case ai::EAlertTypePickedPocket:
			search->_assignmentFlags = SEARCH_PICKED_POCKET;
			break;
		default: // grayman #3857
		case ai::EAlertTypeNone:
			search->_assignmentFlags = SEARCH_NOTHING;
			break;
		}
	}
	else // swarm search
	{
		search->_isCoopSearch = false;
		search->_assignmentFlags = SEARCH_SWARM; // unlimited searchers
	}

	search->_eventID = memory.currentSearchEventID; // the ID of the suspicious event that caused this search
	//DebugPrintSearch(search);

	return search;
}

int CSearchManager::ObtainSearchID(idAI* ai)
{
	// The AI wants to start a new hiding spot search.

	// If the AI has already searched the search he's requesting, he can't join it.

	ai::Memory& memory = ai->GetMemory();

	if (memory.currentSearchEventID < 0)
	{
		// Since all 'events' get an eventID, what's happening here is that
		// the AI's alert level is going up due to incremental player sightings.
		// To get the AI interested in a specific location and to start a search,
		// we'll log a new suspicious event.
		memory.currentSearchEventID = ai->LogSuspiciousEvent( E_EventTypeMisc, memory.alertPos, NULL, false ); // grayman #3857
	}

	if ( ai->HasSearchedEvent(memory.currentSearchEventID) )
	{
		return -1; // has already searched this event, abort request
	}

	int searchID = -1;

	// See if there's a search underway for the provided memory.currentSearchEventID.
	// grayman #3857 - TODO: there still might be a problem here. Multiple events in close proximity
	// will cause multiple searches in the same area. Example: Gassing N AI with one arrow should
	// cause 1 search, not N. The only saving grace is that as each KO'ed body stims an AI, the
	// AI will leave his previous search and create a search for the new KO'ed body, so maybe
	// this isn't a big problem.
	Search *search = GetSearchWithEventID(memory.currentSearchEventID);
	bool canSearchCooperatively = ai->spawnArgs.GetBool("canSearchCooperatively", "1");
	if (search)
	{
		// match the AI's preference with the search type
		if (search->_isCoopSearch && canSearchCooperatively)
		{
			searchID = search->_searchID;
		}
		else if (!search->_isCoopSearch && !canSearchCooperatively)
		{
			searchID = search->_searchID;
		}
	}

	if (searchID < 0) // no search yet
	{
		// See if there's a search at or near the search location using the correct event type.

		idVec3 searchPoint = memory.alertPos;
		bool searchPointIsValid = (!searchPoint.Compare(idVec3(0,0,0)) && (searchPoint.x != idMath::INFINITY));
		SuspiciousEvent* se = gameLocal.FindSuspiciousEvent(memory.currentSearchEventID);
		if ((se == NULL) || !searchPointIsValid)
		{
			// Drop the ai's alert level
			ai->SetAlertLevel(ai->thresh_3 - 0.1);
			memory.currentSearchEventID = -1;
			return -1;
		}

		EventType eventType = se->type;
		search = GetSearchAtLocation(eventType,searchPoint);
		if (search)
		{
			if ( ai->HasSearchedEvent(search->_eventID) )
			{
				return -1; // has already searched this event, abort request
			}

			// match the AI's preference with the search type
			if (search->_isCoopSearch && canSearchCooperatively)
			{
				searchID = search->_searchID;
			}
			else if (!search->_isCoopSearch && !canSearchCooperatively)
			{
				searchID = search->_searchID;
			}
		}
	}

	// We either have an existing search to join, or none, which means we have to create a new one.

	// kill any investigation/guard/milling/observing tasks
	memory.stopHidingSpotInvestigation = true;
	memory.stopGuarding = true;
	memory.stopMilling = true;

	// If we found an existing search, we don't need to start
	// a new one. Just copy some data from the existing search
	// to the ai's memory, and we'll join the search momentarily.

	if (searchID > 0)
	{
		// reset ai's data
		memory.alertPos = search->_origin;
		memory.alertSearchVolume = search->_limits.GetSize()/2.0f;
		memory.alertSearchExclusionVolume = search->_exclusion_limits.GetSize()/2.0f;
		memory.currentSearchEventID = search->_eventID;
		return searchID;
	}

	// no existing search, so start a new one

	search = StartNewSearch(ai);

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
		// search completed
		memory.hidingSpotSearchDone = true;

		// Hiding spot test is done
		memory.hidingSpotTestStarted = false;
	}
}

void CSearchManager::RandomizeHidingSpotList(Search* search)
{
	if (search == NULL)
	{
		return; // invalid search
	}

	int numSpots = search->_hidingSpots.getNumSpots();
	search->_hidingSpotIndexes.clear(); // clear any existing array elements

	// Fill the array with integers from 0 to numSpots-1.
	for ( int i = 0 ; i < numSpots ; i++ )
	{
		search->_hidingSpotIndexes.push_back(i);
	}

	// grayman #3857 - no longer randomize spot selection, and rely on
	// the quality work done during list creation to move the searchers about
}

Search* CSearchManager::GetSearch(int searchID) // returns a pointer to the requested search
{
	if (searchID < 0)
	{
		return NULL; // invalid ID
	}

	for ( int i = 0 ; i < _searches.Num() ; i++ )
	{
		Search *search = _searches[i];
		if ( ( search->_searchID > 0 ) && ( search->_searchID == searchID) )
		{
			return search;
		}
	}

	return NULL;
}

Search* CSearchManager::GetSearchWithEventID(int eventID)
{
	if (eventID < 0)
	{
		return NULL; // invalid ID
	}

	for ( int i = 0 ; i < _searches.Num() ; i++ )
	{
		Search *search = _searches[i];
		if ( ( search->_searchID > 0 ) && ( search->_eventID == eventID) )
		{
			return search;
		}
	}

	return NULL;
}

Search* CSearchManager::GetSearchAtLocation(EventType type, idVec3 location) // returns a pointer to the requested search
{
	// Try to find an event of the given type near the given location.

	for ( int i = 0 ; i < _searches.Num() ; i++ )
	{
		Search *search = _searches[i];
		if ( search->_searchID > 0 )
		{
			EventType eventType = gameLocal.FindSuspiciousEvent(search->_eventID)->type;
			if ( ( eventType == type ) && ( (search->_origin - location).LengthSqr() < 16384 ) )
			{
				// This search is close to where the AI wants to start
				// searching, so we could assign him to this existing
				// search.

				return search;
			}
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

	CDarkmodHidingSpotTree *tree = &search->_hidingSpots; // The hiding spots for this search
	int numSpots = tree->getNumSpots();

	Assignment* assignment = GetAssignment(search,ai);

	if (assignment == NULL)
	{
		return false; // no assignment for this AI in this search
	}

	int index = assignment->_lastSpotAssigned; // the most recent spot assigned to this searcher; index into _hidingSpotIndexes
	index++;

	if (index >= numSpots)
	{
		ai->GetMemory().noMoreHidingSpots = true;
		return false; // no spots left
	}

	// find a valid spot that's inside the ai's search area

	for ( int i = index ; i < numSpots ; i++ )
	{
		int treeIndex = search->_hidingSpotIndexes[i];
		assignment->_lastSpotAssigned = i;

		if ( treeIndex == -1 )
		{
			continue; // skip bad or used spots
		}

		// validate spot

		idBounds areaNodeBounds;
		darkModHidingSpot* hidingSpot = tree->getNthSpotWithAreaNodeBounds(treeIndex, areaNodeBounds);

		if (hidingSpot != NULL)
		{
			// grayman #2422 - this routine might return (0,0,0), but we don't
			// want AI traveling there.

			if ( !hidingSpot->goal.origin.Compare(idVec3(0,0,0)) )
			{
				// grayman #2422 - to keep AI from searching the floor above
				// or below, only return hiding spots that are inside the
				// requested search volume.

				nextSpot = hidingSpot->goal.origin; // point is good so far

				if (assignment->_limits.ContainsPoint(nextSpot))
				{
					// can we walk to this spot?

					idVec3 aiOrigin = ai->GetPhysics()->GetOrigin();
					int aiAreaNum = ai->PointReachableAreaNum(aiOrigin, 1.0f);
					aasPath_t path;
					int nextSpotAreaNum = ai->PointReachableAreaNum(nextSpot);
					if (nextSpotAreaNum > 0)
					{
						if (ai->PathToGoal(path, aiAreaNum, aiOrigin, nextSpotAreaNum, nextSpot, ai))
						{
							search->_hidingSpotIndexes[i] = -1; // don't assign this good spot again
							return true; // calling method will read nextSpot
						}
					}
				}

				return false; // didn't find one this time, maybe next time we'll find one
			}
			else
			{
				// hidingSpot is [0,0,0], so it's NG
				search->_hidingSpotIndexes[i] = -1; // no one should use this spot
			}
		}
		else
		{
			// hidingSpot is NULL, so it's NG
			search->_hidingSpotIndexes[i] = -1; // no one should use this spot
		}
	}

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
		return 1;
	}

	// No more processing to do at this point

	if (!search->_hidingSpotsReady)
	{
		unsigned int refCount;

		// Get finder we just referenced
		p_hidingSpotFinder = CHidingSpotSearchCollection::Instance().getSearchAndReferenceCountByHandle 
			(
				search->_hidingSpotSearchHandle,
				refCount
			);

		search->_hidingSpots.clear();

		p_hidingSpotFinder->hidingSpotList.copy(&search->_hidingSpots);
		//p_hidingSpotFinder->hidingSpotList.getOneNth(refCount,search->_hidingSpots); // grayman #3857 - don't do this any more
		search->_hidingSpotsReady = true; // the hiding spot list is complete

		// randomize the indices into the hiding spot list
		// grayman #3857 - no longer randomizes, but the list is useful for marking
		// spots that have been searched or which are NG to anyone
		RandomizeHidingSpotList(search);

		//DebugPrintHidingSpots(search);

		// Done with search object, dereference so other AIs know how many
		// AIs will still be retrieving points from the search
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

bool CSearchManager::JoinSearch(int searchID, idAI* ai)
{
	Search *search = GetSearch(searchID);
	if (search == NULL)
	{
		return false;
	}

	// this ai can only be an active searcher
	// if he can walk to the search origin

	int toAreaNum = ai->PointReachableAreaNum(search->_origin);
	bool canBeActiveSearcher = true;
	if (toAreaNum > 0)
	{
		idVec3 aiOrigin = ai->GetPhysics()->GetOrigin();
		int aiAreaNum = ai->PointReachableAreaNum(aiOrigin, 1.0f);
		aasPath_t path;
		if (!ai->PathToGoal(path, aiAreaNum, aiOrigin, toAreaNum, search->_origin, ai))
		{
			canBeActiveSearcher = false;
		}
	}
	else
	{
		canBeActiveSearcher = false;
	}

/*	A "swarm" search allows unlimited active searchers. AI such as zombies,
	which have no cooperative abilities, would use this type of search.

	AI that can search cooperatively will be assigned as active searchers,
	guards, or observers. A cooperative search can have at most 2 active searchers.

	For swarm searches:

		All searchers are given the same search information, so
		they will share the list of hiding spots. The hiding spots
		will be assigned individually to each requesting searcher,
		so that no two searchers with search the same spot.

	For cooperative searches:

		If no one's been assigned yet, the first AI to join will be
		given an "active search" based on the origin and boundaries
		set up when the AI was alerted.

		If the search already has a single searcher, the search area
		will be divided equally between the two searchers.

		If more than 2 AI join this search, they will be given guard
		assignments that will place them at nearby doors or visportals
		that lead out of the area, or "guard entities" placed by the
		mapper. Once the guard assignments are filled, new searchers
		will be placed randomly outside the search area as observers.
*/

	float outerRadius = 0;
	idBounds searchBounds;
	idBounds searchExclusionBounds;

	// How many assignments does this search have?

	int numAssignments = search->_assignments.Num();
	int takeAssignmentIndex = -1; // if taking over an abandoned assignment, fill in this index

	bool canSearchCooperatively = ai->spawnArgs.GetBool("canSearchCooperatively", "1");
	smRole_t searcherRole = E_ROLE_NONE; // no assignment yet

	if (search->_isCoopSearch && canSearchCooperatively)
	{
		// If there are 0 or 1 assignment slots for
		// this search, and you can't be an active searcher, then you
		// can't join in this frame. Once the number of slots goes above 2,
		// the active searcher roles are assigned, and you can join as a guard
		// or an observer.
		if ((numAssignments < 2) && !canBeActiveSearcher)
		{
			// Can't join this search. Sorry.
			return false;
		}

		// Active searchers are allowed to be armed or unarmed/civilian.

		// Does this search require active searchers? All searches should
		// require active searchers, but just in case ...

		if (search->_assignmentFlags & (SEARCH_SEARCHER_MILL | SEARCH_SEARCH))
		{
			if (numAssignments == 0)
			{
				// A single AI gets to search the entire search area.

				idVec3 searchSize = search->_limits.GetSize();
				float xRad = searchSize.x / 2.0f;
				float yRad = searchSize.y / 2.0f;
				outerRadius = idMath::Sqrt(xRad*xRad + yRad*yRad);
				search->_outerRadius = outerRadius;
				searchBounds = search->_limits;
				searcherRole = E_ROLE_SEARCHER;
			}
			else if (numAssignments == 1)
			{
				// Is the first assignment slot available?
				if (search->_assignments[0]._searcher == NULL)
				{
					takeAssignmentIndex = 0;
					searcherRole = search->_assignments[0]._searcherRole;
				}
				else if (search->_outerRadius <= SEARCH_RADIUS_ONE_SEARCHER)
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

					outerRadius = search->_assignments[0]._outerRadius; // inherit the outer radius from the first searcher

					// Split search area in half.

					idBounds searchBounds1;
					idBounds searchBounds2;
					searchBounds1 = searchBounds2 = search->_limits;
					idVec3 size = searchBounds1.GetSize();
					if (size.x > size.y)
					{
						searchBounds1[1].x -= size.x / 2.0f;
						searchBounds2[0].x += size.x / 2.0f;
					}
					else
					{
						searchBounds1[1].y -= size.y / 2.0f;
						searchBounds2[0].y += size.y / 2.0f;
					}
					search->_assignments[0]._limits = searchBounds1;
					searchBounds = searchBounds2;
					searcherRole = E_ROLE_SEARCHER;
				}
			}
			else if (numAssignments >= 2)
			{
				// Is the first assignment slot available?
				if ((search->_assignments[0]._searcher == NULL) && canBeActiveSearcher)
				{
					takeAssignmentIndex = 0;
					searcherRole = search->_assignments[0]._searcherRole;
				}
				// Is the second assignment slot available?
				else if ((search->_assignments[1]._searcher == NULL) && canBeActiveSearcher)
				{
					takeAssignmentIndex = 1;
					searcherRole = search->_assignments[1]._searcherRole;
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
			float xRad = searchSize.x / 2.0f;
			float yRad = searchSize.y / 2.0f;

			// outerRadius defines the search perimeter, where observers will stand
			outerRadius = Max(SEARCH_MIN_OBS_DISTANCE, idMath::Sqrt(xRad*xRad + yRad*yRad));
			if (!ai->IsAfraid()) // armed AI becomes a guard
			{
				// Do we need guards for this search?

				if (search->_assignmentFlags & (SEARCH_GUARD_MILL | SEARCH_GUARD))
				{
					searcherRole = E_ROLE_GUARD;
					searchBounds = search->_limits; // set, but not used by a guard
				}
			}

			if (searcherRole == E_ROLE_NONE)
			{
				// Everyone else is unfit to guard a spot and is relegated to observing.
				// Do we need observers for this search?
				if (search->_assignmentFlags & (SEARCH_OBSERVER_MILL | SEARCH_OBSERVE))
				{
					searcherRole = E_ROLE_OBSERVER;
					searchBounds = search->_limits; // set, but not used by an observer
				}
			}
		}
	}
	else if (!search->_isCoopSearch && !canSearchCooperatively)
	{
		if (!canBeActiveSearcher)
		{
			// Can't join this search. Sorry.
			return false;
		}

		// By definition, swarm searchers only involve active
		// searchers that don't mill first.

		// All AI get to search the entire search area.

		idVec3 searchSize = search->_limits.GetSize();
		float xRad = searchSize.x / 2.0f;
		float yRad = searchSize.y / 2.0f;
		outerRadius = idMath::Sqrt(xRad*xRad + yRad*yRad);
		search->_outerRadius = outerRadius;
		searchBounds = search->_limits;
		searcherRole = E_ROLE_SEARCHER;

		// find the first available assignment, if any have been abandoned

		takeAssignmentIndex = -1;
		for ( int i = 0 ; i < numAssignments ; i++ )
		{
			if (search->_assignments[i]._searcher == NULL)
			{
				takeAssignmentIndex = i;
				break;
			}
		}
	}

	if (searcherRole == E_ROLE_NONE)
	{
		// Can't join this search. Sorry.
		return false;
	}

	// Leave your current search, if there is one.

	if (ai->m_searchID > 0)
	{
		LeaveSearch(ai->m_searchID, ai);
	}

	// Either reuse an existing assignment, or create a new one

	Assignment *assignment;

	if ( takeAssignmentIndex >= 0 )
	{
		assignment = &search->_assignments[takeAssignmentIndex];
		assignment->_searcher = ai; // reactivate the deactivated assignment
	}
	else // create new assignment
	{
		Assignment newAssignment;
		assignment = &newAssignment;

		assignment->_origin = search->_origin; // center of search area, location of alert stimulus
		assignment->_outerRadius = outerRadius;
		assignment->_limits = searchBounds;
		assignment->_searcher = ai; // AI who owns this assignment
		assignment->_lastSpotAssigned = -1; // if actively searching, the most recent spot assigned to a searcher; index into _hidingSpotIndexes
		assignment->_searcherRole = searcherRole; // the AI's role

		search->_assignments.Append(*assignment); // add this new assignment to the search
	}

	//DebugPrintAssignment(assignment);

	search->_searcherCount++; // number of searchers (includes active searchers, guards, and observers)

	ai->m_searchID = searchID;

	ai::Memory& memory = ai->GetMemory();
	memory.lastAlertPosSearched = search->_origin;
	memory.alertSearchCenter = search->_origin;
	memory.alertPos = search->_origin;
	memory.currentSearchEventID = search->_eventID;
	ai->AddSuspiciousEvent(search->_eventID);

	memory.shouldMill = false;
	if (searcherRole == E_ROLE_SEARCHER)
	{
		if (search->_assignmentFlags & SEARCH_SEARCHER_MILL)
		{
			memory.shouldMill = true;
		}
	}
	else if (searcherRole == E_ROLE_GUARD)
	{
		if (search->_assignmentFlags & SEARCH_GUARD_MILL)
		{
			memory.shouldMill = true;
		}
	}
	else // observer
	{
		if (search->_assignmentFlags & SEARCH_OBSERVER_MILL)
		{
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

		ai->AddWarningEvent(searcher,search->_eventID);
	}

	// If this search involves a dead body, and that body has spilled blood
	// associated with it, look for the blood's event and mark yourself as
	// knowing about it and searched it. Also, stop the blood from stimming you.

	SuspiciousEvent* se = gameLocal.FindSuspiciousEvent(search->_eventID);
	if (se->type == E_EventTypeDeadPerson)
	{
		idEntity* body = se->entity.GetEntity();
		if (body)
		{
			idEntity* blood = static_cast<idAI*>(body)->GetBlood();
			if (blood)
			{
				int eventID = gameLocal.FindSuspiciousEvent( E_EventTypeMisc, idVec3(0,0,0), blood, 0 );
				if (eventID < 0)
				{
					eventID = ai->LogSuspiciousEvent( E_EventTypeMisc, blood->GetPhysics()->GetOrigin(), blood, false ); // grayman #3857
				}
				ai->MarkEventAsSearched(eventID); // will add an event if ai doesn't already know about it
				blood->IgnoreResponse(ST_VISUAL, ai); // blood won't stim this AI in the future
			}
		}
	}

	// grayman #3857 - "snd_helpSearch" will get overidden by the rising alert bark when entering
	// Searching or Agitated Searching state in the same frame. Since the AI joining this search is not necessarily
	// the result of being requested to, or responding to a call for help, it isn't really needed.
	//ai->Bark("snd_helpSearch"); // Bark that you're joining the search

	return true;
}

// Searcher leaves the search. The searcher's assignment
// isn't deleted, though, in case others join the search before
// it ends, and we want to know what previous searchers were assigned.
// If the last searcher leaves a search, the search is destroyed
// when SearchManager->ProcessSearches() is run.

void CSearchManager::LeaveSearch(int searchID, idAI* ai)
{
	Search *search = GetSearch(searchID);
	if (search == NULL)
	{
		return;
	}

	int numAssignments = search->_assignments.Num();

	if (numAssignments == 0)
	{
		return;
	}

	bool assignmentFound = false;
	for (int i = 0 ; i < numAssignments ; i++)
	{
		Assignment *assignment = &search->_assignments[i];
		if (assignment->_searcher == ai)
		{
			assignmentFound = true;
			assignment->_searcher = NULL; // deactivate the assignment
			search->_searcherCount--;
			break;
		}
	}

	if (!assignmentFound)
	{
		return;
	}

	// Now that you've left the search, stop whatever task you were doing
	// for that search

	ai::Memory& memory = ai->GetMemory();
	memory.currentSearchSpot = idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY);
	// greebo: Clear the initial alert position
	memory.alertSearchCenter = idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY);
	memory.currentSearchEventID = -1;

	ai->searchSubsystem->ClearTasks(); // clears investigate spot task or guard spot task
	memory.stopHidingSpotInvestigation = true;
	memory.stopGuarding = true;
	memory.stopMilling = true;
	memory.shouldMill = false;
	memory.hidingSpotInvestigationInProgress = false;
	memory.guardingInProgress = false;
	memory.millingInProgress = false;

	ai->m_searchID = -1; // leave the search
}

// grayman #2422 - adjust search limits to better fit the vertical
// space in which the alert occurred

// bounds is in absolute coordinates

void CSearchManager::AdjustSearchLimits(idBounds& bounds)
{
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
	// Set search bounds
	idBounds searchBounds = search->_limits;
	idBounds searchExclusionBounds = search->_exclusion_limits;

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
			continue;
		}

		tdmPathGuard* guardEntity = static_cast<tdmPathGuard*>( ent );
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
		}
	}

	// Find exits from the cluster the search is in.
	gameLocal.GetPortals(search,ai);
	search->_guardSpotsReady = true;
}

void CSearchManager::Save(idSaveGame* savefile)
{
	savefile->WriteInt(_uniqueSearchID);

	// save searches
	int numSearches = _searches.Num();
	savefile->WriteInt(numSearches);
	for (int i = 0 ; i < numSearches ; i++)
	{
		Search* search = _searches[i];

		savefile->WriteInt(search->_searchID);
		savefile->WriteInt(search->_eventID);
		savefile->WriteBool(search->_isCoopSearch);
		savefile->WriteInt(search->_hidingSpotSearchHandle);
		savefile->WriteVec3(search->_origin);
		savefile->WriteBounds(search->_limits);
		savefile->WriteBounds(search->_exclusion_limits);
		savefile->WriteFloat(search->_outerRadius);
		savefile->WriteFloat(search->_referenceAlertLevel);
		search->_hidingSpots.Save(savefile);
		savefile->WriteBool(search->_hidingSpotsReady);

		int num = search->_hidingSpotIndexes.size();
		savefile->WriteInt(num);
		for ( int j = 0 ; j < num ; j++ )
		{
			savefile->WriteInt(search->_hidingSpotIndexes[j]);
		}

		num = search->_guardSpots.Num();
		savefile->WriteInt(num);
		for ( int j = 0 ; j < num ; j++ )
		{
			savefile->WriteVec4(search->_guardSpots[j]);
		}

		savefile->WriteBool(search->_guardSpotsReady);
		savefile->WriteUnsignedInt(search->_assignmentFlags);
		savefile->WriteInt(search->_searcherCount);

		// save assignments
		num = search->_assignments.Num();
		savefile->WriteInt(num);
		for ( int j = 0 ; j < num ; j++ )
		{
			Assignment& assignment = search->_assignments[j];

			savefile->WriteVec3(assignment._origin);
			savefile->WriteFloat(assignment._outerRadius);
			savefile->WriteBounds(assignment._limits);
			savefile->WriteObject(assignment._searcher);
			savefile->WriteInt(assignment._lastSpotAssigned);
			savefile->WriteInt(static_cast<int>(assignment._searcherRole));

			//DebugPrintAssignment(&assignment);
		}

		//DebugPrintSearch(search);
	}
}

void CSearchManager::Restore(idRestoreGame* savefile)
{
	savefile->ReadInt(_uniqueSearchID);

	_searches.Clear();
	int numSearches;
	savefile->ReadInt(numSearches);
	for (int i = 0 ; i < numSearches ; i++)
	{
		Search* search = new Search;

		savefile->ReadInt(search->_searchID);
		savefile->ReadInt(search->_eventID);
		savefile->ReadBool(search->_isCoopSearch);
		savefile->ReadInt(search->_hidingSpotSearchHandle);
		savefile->ReadVec3(search->_origin);
		savefile->ReadBounds(search->_limits);
		savefile->ReadBounds(search->_exclusion_limits);
		savefile->ReadFloat(search->_outerRadius);
		savefile->ReadFloat(search->_referenceAlertLevel);
		search->_hidingSpots.Restore(savefile);
		savefile->ReadBool(search->_hidingSpotsReady);
		int num;
		savefile->ReadInt(num);
		search->_hidingSpotIndexes.clear();
		for ( int j = 0 ; j < num ; j++ )
		{
			int n;
			savefile->ReadInt(n);
			search->_hidingSpotIndexes.push_back(n);
		}

		savefile->ReadInt(num);
		search->_guardSpots.SetGranularity(1);
		search->_guardSpots.SetNum(num);
		for ( int j = 0 ; j < num ; j++ )
		{
			savefile->ReadVec4(search->_guardSpots[j]);
		}

		savefile->ReadBool(search->_guardSpotsReady);
		savefile->ReadUnsignedInt(search->_assignmentFlags);
		savefile->ReadInt(search->_searcherCount);

		// restore assignments
		search->_assignments.Clear();
		savefile->ReadInt(num);
		search->_assignments.SetNum(num);
		for (int j = 0 ; j < num ; j++)
		{
			Assignment& assignment = search->_assignments[j];

			savefile->ReadVec3(assignment._origin);
			savefile->ReadFloat(assignment._outerRadius);
			savefile->ReadBounds(assignment._limits);
			savefile->ReadObject(reinterpret_cast<idClass*&>(assignment._searcher));
			savefile->ReadInt(assignment._lastSpotAssigned);

			int n;
			savefile->ReadInt(n);
			assignment._searcherRole = static_cast<smRole_t>(n);

			//DebugPrintAssignment(&assignment);
		}

		_searches.Append(search);

		//DebugPrintSearch(search);
		//DebugPrintHidingSpots(search);
	}
}

/*
// prints hiding spots
void CSearchManager::DebugPrintHidingSpots(Search* search)
{
	int numSpots = search->_hidingSpots.getNumSpots();
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("Search %d has %d hiding spots\r",search->_searchID,numSpots);
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

/*
// prints search contents
void CSearchManager::DebugPrintSearch(Search* search)
{
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("============== Search ==============\r");
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("_hidingSpotSearchHandle = %d\r",search->_hidingSpotSearchHandle);
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("              _searchID = %d\r",search->_searchID);
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("               _eventID = %d\r",search->_eventID);
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("          _isCoopSearch = %d\r",search->_isCoopSearch);
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("             event type = %d\r",(int)gameLocal.FindSuspiciousEvent(search->_eventID)->type);
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("                _origin = [%s]\r",search->_origin.ToString());
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("                _limits = [%s]\r",search->_limits.ToString());
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("      _exclusion_limits = [%s]\r",search->_exclusion_limits.ToString());
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("           _outerRadius = %f\r",search->_outerRadius);
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("   _referenceAlertLevel = %f\r",search->_referenceAlertLevel);
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
*/

void CSearchManager::destroyCurrentHidingSpotSearch(Search* search)
{
	// Destroy the list of hiding spots

	// Check to see if there is one
	if (search->_hidingSpotSearchHandle != NULL_HIDING_SPOT_SEARCH_HANDLE)
	{
		// Dereference current search
		CHidingSpotSearchCollection::Instance().dereference(search->_hidingSpotSearchHandle);
		search->_hidingSpotSearchHandle = NULL_HIDING_SPOT_SEARCH_HANDLE;
	}

	// No hiding spots
	search->_hidingSpots.clear();
}

// The Search Manager's "Think" method.

void CSearchManager::ProcessSearches()
{
	// If the player is dead, all searchers should leave their searches.

	idPlayer* player = gameLocal.GetLocalPlayer();

	for ( int i = 0 ; i < _searches.Num() ; i++ )
	{
		Search *search = _searches[i];
		int searchID = search->_searchID;

		// Is this an ongoing search?
		if (searchID == -1)
		{
			continue; // no, so skip it
		}

		// If the player is dead, all searchers should leave their searches.
		if (player->health <= 0)
		{
			for ( int j = 0 ; j < search->_assignments.Num() ; j++ )
			{
				Assignment *assignment = &search->_assignments[j];
				idAI* searcher = assignment->_searcher;
				if (searcher != NULL)
				{
					LeaveSearch(searchID, searcher);
					searcher->SetAlertLevel(searcher->thresh_3 - 0.1f);
				}
			}
		}

		// If there are no searchers remaining in this search, recycle it

		int searcherCount = search->_searcherCount;

		if (searcherCount == 0)
		{
			destroyCurrentHidingSpotSearch(search); // Destroy the list of hiding spots
			search->_assignments.Clear();
			search->_searchID = -1; // mark it for recycling
			continue;
		}

		// There is at least one searcher.

		// Are there no active searchers? If so,
		// backfill with someone else if available. Only backfill if
		// there are no active searchers, otherwise things start to
		// get a bit chaotic. Only backfill with searchers whose
		// alert level is above thresh_3.

		bool atLeastOneSearcher = false;
		for ( int j = 0 ; j < 2 ; j++ )
		{
			Assignment *assignment = &search->_assignments[j];
			if (assignment->_searcher != NULL)
			{
				atLeastOneSearcher = true;
				break;
			}
		}

		// Are there no active searchers?
		if (!atLeastOneSearcher)
		{
			// No active searchers. Is there someone who can step
			// up and backfill as an active searcher?

			int assignmentCount = search->_assignments.Num();

			if (assignmentCount >= 3)
			{
				// Are there any observer candidates who can backfill?
				// Pick the one with the lowest rank.
				idAI* bestCandidate = NULL;
				Assignment *bestAssignment = NULL;
				int lowestRank = 1000;
				for ( int j = 2 ; j < assignmentCount ; j++ )
				{
					Assignment *assignment = &search->_assignments[j];
					idAI* searcher = assignment->_searcher;
					if (searcher != NULL)
					{
						// this ai can only be an active searcher
						// if he can walk to the search origin
						int toAreaNum = searcher->PointReachableAreaNum(search->_origin);
						bool canBeActiveSearcher = (toAreaNum != 0);

						if (canBeActiveSearcher &&
							(assignment->_searcherRole == E_ROLE_OBSERVER) &&
							(searcher->rank < lowestRank) &&
							(searcher->AI_AlertLevel >= searcher->thresh_3)) // must still be searching
						{
							bestCandidate = searcher;
							bestAssignment = assignment;
							lowestRank = searcher->rank;
						}
					}
				}

				// If we don't have an observer candidate,
				// are there any guard candidates who can backfill?
				// Pick the one with the lowest rank.
				if (bestCandidate == NULL)
				{
					for ( int j = 2 ; j < assignmentCount ; j++ )
					{
						Assignment *assignment = &search->_assignments[j];
						idAI* searcher = assignment->_searcher;
						if (searcher != NULL)
						{
							// this ai can only be an active searcher
							// if he can walk to the search origin
							int toAreaNum = searcher->PointReachableAreaNum(search->_origin);
							bool canBeActiveSearcher = (toAreaNum != 0);

							if (canBeActiveSearcher &&
								(assignment->_searcherRole == E_ROLE_GUARD) &&
								(searcher->rank < lowestRank) &&
								(searcher->AI_AlertLevel >= searcher->thresh_3)) // must still be searching
							{
								bestCandidate = searcher;
								bestAssignment = assignment;
								lowestRank = searcher->rank;
							}
						}
					}
				}

				// do we have a candidate for backfilling?

				if (bestCandidate)
				{
					search->_assignments[0]._searcher = bestCandidate; // reactivate the first assignment
					bestAssignment->_searcher = NULL; // deactivate the old assignment

					ai::Memory& memory = bestCandidate->GetMemory();
					memory.shouldMill = false; // don't bother milling

					// stop whatever they were doing
					memory.stopHidingSpotInvestigation = true;
					memory.stopGuarding = true;
					memory.stopMilling = true;

					// they'll be given a hiding spot search the next time they think
				}
				else
				{
					// there are no guards who can backfill
				}
			}
			else
			{
				// there are no active searchers, but also no guards or observers
			}

			continue;
		}

		// there is at least 1 active searcher

		// If a higher-ranking AI is actively searching, and
		// there's a lower-ranking AI either guarding or observing,
		// they should swap places. This is to keep the higher-ranking
		// AI out of harm's way should an enemy be found by a searcher.

		// Find the highest rank of the two possible active searchers.
		// There have to be at least 2 searchers and 3 assignments to
		// do any swapping.

		int assignmentCount = search->_assignments.Num();
		if ( ( searcherCount >= 2 ) && ( assignmentCount >= 3) )
		{
			// Who is the highest ranking active searcher and lowest ranking guard or observer?

			int highestActiveSearcherRank = -1;
			int higherRankingSearcherAssignmentIndex = 0;
			int lowestGuardOrObserverRank = 1000;
			int lowestGuardOrObserverAssignmentIndex = 0;
			for ( int j = 0 ; j < assignmentCount ; j++ )
			{
				Assignment *assignment = &search->_assignments[j];
				if (j < 2) // active searchers
				{
					if (assignment->_searcher != NULL)
					{
						if (assignment->_searcher->rank > highestActiveSearcherRank)
						{
							higherRankingSearcherAssignmentIndex = j;
							highestActiveSearcherRank = assignment->_searcher->rank;
						}
					}
				}
				else // guards and observers
				{
					if (assignment->_searcher != NULL)
					{
						if (assignment->_searcher->rank < lowestGuardOrObserverRank)
						{
							// this ai can only be an active searcher
							// if he can walk to the search origin
							int toAreaNum = assignment->_searcher->PointReachableAreaNum(search->_origin);
							if (toAreaNum != 0)
							{
								lowestGuardOrObserverAssignmentIndex = j;
								lowestGuardOrObserverRank = assignment->_searcher->rank;
							}
						}
					}
				}
			}

			// Compare ranks

			if (highestActiveSearcherRank > lowestGuardOrObserverRank)
			{
				Assignment *assignment1 = &search->_assignments[higherRankingSearcherAssignmentIndex];
				Assignment *assignment2 = &search->_assignments[lowestGuardOrObserverAssignmentIndex];
				idAI* searcher1 = assignment1->_searcher;
				idAI* searcher2 = assignment2->_searcher;

				ai::Memory& memory1 = searcher1->GetMemory();
				ai::Memory& memory2 = searcher2->GetMemory();

				bool searcher1shouldMill = memory1.shouldMill || memory1.millingInProgress;
				bool searcher2shouldMill = memory2.shouldMill || memory2.millingInProgress;

				LeaveSearch(searchID, searcher2);
				LeaveSearch(searchID, searcher1);

				JoinSearch(searchID,searcher2);
				JoinSearch(searchID,searcher1);

				memory1.shouldMill = searcher1shouldMill;
				memory2.shouldMill = searcher2shouldMill;
			}
		}
	}
}


// grayman #3857 - Are there any searches on the search list that can be recycled?
// If so, recycle one. Otherwise, create a new one.

Search* CSearchManager::CreateSearch(int searchID)
{
	Search *search = NULL;

	for ( int i = 0 ; i < _searches.Num() ; i++ )
	{
		Search *candidateSearch = _searches[i];
		if (candidateSearch->_searchID == -1)
		{
			search = candidateSearch;
			break;
		}
	}

	if (search == NULL)
	{
		search = new Search;
		_searches.Append(search);
	}

	search->_searchID = searchID;
	return search;
}

CSearchManager* CSearchManager::Instance()
{
	static CSearchManager _manager;
	return &_manager;
}

