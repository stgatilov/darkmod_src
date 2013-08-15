/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod (http://www.thedarkmod.com/)
 
 $Revision$ (Revision of last commit) 
 $Date$ (Date of last commit)
 $Author$ (Author of last commit)
 
******************************************************************************/

#include "precompiled_game.h"
#pragma hdrstop

static bool versioned = RegisterVersionedFile("$Id$");

#include "SearchingState.h"
#include "../Memory.h"
#include "../Tasks/InvestigateSpotTask.h"
#include "../Tasks/SingleBarkTask.h"
#include "../Tasks/RepeatedBarkTask.h" // grayman #3472
#include "../Library.h"
#include "IdleState.h"
#include "AgitatedSearchingState.h"
#include "../../AbsenceMarker.h"
#include "../../AIComm_Message.h"

namespace ai
{

// Get the name of this state
const idStr& SearchingState::GetName() const
{
	static idStr _name(STATE_SEARCHING);
	return _name;
}

bool SearchingState::CheckAlertLevel(idAI* owner)
{
	if (!owner->m_canSearch) // grayman #3069 - AI that can't search shouldn't be here
	{
		owner->SetAlertLevel(owner->thresh_3 - 0.1);
	}

	if (owner->AI_AlertIndex < ESearching)
	{
		owner->GetMemory().agitatedSearched = false; // grayman #3496 - clear this if descending
		
		// Alert index is too low for this state, fall back
		owner->Event_CloseHidingSpotSearch();
		owner->GetMind()->EndState();
		return false;
	}

	// grayman #3009 - can't enter this state if sitting, sleeping,
	// sitting down, lying down, or getting up from sitting or sleeping

	moveType_t moveType = owner->GetMoveType();
	if ( moveType == MOVETYPE_SIT      || 
		 moveType == MOVETYPE_SLEEP    ||
		 moveType == MOVETYPE_SIT_DOWN ||
		 moveType == MOVETYPE_LAY_DOWN )
	{
		owner->GetUp(); // it's okay to call this multiple times
		owner->GetMind()->EndState();
		return false;
	}

	if ( ( moveType == MOVETYPE_GET_UP ) ||	( moveType == MOVETYPE_GET_UP_FROM_LYING ) )
	{
		owner->GetMind()->EndState();
		return false;
	}

	if (owner->AI_AlertIndex > ESearching)
	{
		// Alert index is too high, switch to the higher State
		owner->GetMind()->PushState(owner->backboneStates[EAgitatedSearching]);
		return false;
	}

	// Alert Index is matching, return OK
	return true;
}

void SearchingState::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("SearchingState initialised.\r");
	assert(owner);

	// Ensure we are in the correct alert level
	if ( !CheckAlertLevel(owner) )
	{
		return;
	}

	if (owner->GetMoveType() == MOVETYPE_SIT || owner->GetMoveType() == MOVETYPE_SLEEP)
	{
		owner->GetUp();
	}

	// Shortcut reference
	Memory& memory = owner->GetMemory();

	float alertTime = owner->atime3 + owner->atime3_fuzzyness * (gameLocal.random.RandomFloat() - 0.5);

	_alertLevelDecreaseRate = (owner->thresh_4 - owner->thresh_3) / alertTime;

	if ( owner->AlertIndexIncreased() || memory.mandatory ) // grayman #3331
	{
		// Setup a new hiding spot search
		StartNewHidingSpotSearch(owner);
	}

	if ( owner->AlertIndexIncreased() )
	{
		// grayman #3423 - when the alert level is ascending, kill the repeated bark task
		owner->commSubsystem->ClearTasks();

		// Play bark if alert level is ascending

		// grayman #3496 - enough time passed since last alert bark?
		if ( gameLocal.time >= memory.lastTimeAlertBark + MIN_TIME_BETWEEN_ALERT_BARKS )
		{
			idStr bark;

			if ((memory.alertedDueToCommunication == false) && ((memory.alertType == EAlertTypeSuspicious) || (memory.alertType == EAlertTypeEnemy)))
			{
				bool friendsNear = ( (MS2SEC(gameLocal.time - memory.lastTimeFriendlyAISeen)) <= MAX_FRIEND_SIGHTING_SECONDS_FOR_ACCOMPANIED_ALERT_BARK );
				if ( (memory.alertClass == EAlertVisual_1) ||
					 (memory.alertClass == EAlertVisual_2) ||      // grayman #2603, #3424 
					 // (memory.alertClass == EAlertVisual_3) ) || // grayman #3472 - no longer needed
					 (memory.alertClass == EAlertVisual_4) )       // grayman #3498
				{
					if ( friendsNear )
					{
						bark = "snd_alert3sc";
					}
					else
					{
						bark = "snd_alert3s";
					}
				}
				else if (memory.alertClass == EAlertAudio)
				{
					if ( friendsNear )
					{
						bark = "snd_alert3hc";
					}
					else
					{
						bark = "snd_alert3h";
					}
				}
				else if ( friendsNear )
				{
					bark = "snd_alert3c";
				}
				else
				{
					bark = "snd_alert3";
				}

				// Allocate a SingleBarkTask, set the sound and enqueue it

				owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new SingleBarkTask(bark)));

				memory.lastTimeAlertBark = gameLocal.time; // grayman #3496

				if (cv_ai_debug_transition_barks.GetBool())
				{
					gameLocal.Printf("%d: %s rises to Searching state, barks '%s'\n",gameLocal.time,owner->GetName(),bark.c_str());
				}
			}
		}
		else
		{
			if (cv_ai_debug_transition_barks.GetBool())
			{
				gameLocal.Printf("%d: %s rises to Searching state, can't bark 'snd_alert3{s/sc/h/hc/c}' yet\n",gameLocal.time,owner->GetName());
			}
		}
	}
	else if (memory.alertType == EAlertTypeEnemy)
	{
		// reduce the alert type, so we can react to other alert types (such as a dead person)
		memory.alertType = EAlertTypeSuspicious;
	}
	
	// grayman #3472 - When ascending, set up a repeated bark

	if ( owner->AlertIndexIncreased() )
	{
		owner->commSubsystem->AddSilence(5000 + gameLocal.random.RandomInt(3000)); // grayman #3424

		// This will hold the message to be delivered with the bark
		CommMessagePtr message(new CommMessage(
			CommMessage::DetectedEnemy_CommType, 
			owner, NULL,// from this AI to anyone 
			NULL,
			idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY),
			0
		));

		int minTime = SEC2MS(owner->spawnArgs.GetFloat("searchbark_delay_min", "10"));
		int maxTime = SEC2MS(owner->spawnArgs.GetFloat("searchbark_delay_max", "15"));
		owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new RepeatedBarkTask("snd_state3", minTime, maxTime, message)));
	}
	else // descending
	{
		// Allow repeated barks from Agitated Searching to continue.
	}

	if (!owner->HasSeenEvidence())
	{
		owner->SheathWeapon();
		owner->UpdateAttachmentContents(false);
	}
	else
	{
		// Let the AI update their weapons (make them solid)
		owner->UpdateAttachmentContents(true);
	}
}

void SearchingState::OnSubsystemTaskFinished(idAI* owner, SubsystemId subSystem)
{
/* grayman #2560 - InvestigateSpotTask is the only task this was needed
   for, and this code has been moved to a new OnFinish() for that task. No longer
   needed here.

	Memory& memory = owner->GetMemory();

	if (memory.hidingSpotInvestigationInProgress && subSystem == SubsysAction)
	{
		// The action subsystem has finished investigating the spot, set the
		// boolean back to false, so that the next spot can be chosen
		memory.hidingSpotInvestigationInProgress = false;
	}
 */
}

// Gets called each time the mind is thinking
void SearchingState::Think(idAI* owner)
{
	UpdateAlertLevel();

	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner))
	{
		return;
	}

	// grayman #3063 - move up so it gets done each time,
	// regardless of what state the hiding spot search is in.
	// Let the AI check its senses
	owner->PerformVisualScan();

	if (owner->GetMoveType() == MOVETYPE_SIT 
		|| owner->GetMoveType() == MOVETYPE_SLEEP
		|| owner->GetMoveType() == MOVETYPE_SIT_DOWN
		|| owner->GetMoveType() == MOVETYPE_LAY_DOWN)
	{
		owner->GetUp();
		return;
	}

	Memory& memory = owner->GetMemory();

	owner->MarkEventAsSearched(memory.currentSearchEventID); // grayman #3424

	// grayman #3520 - look at alert spots
	if ( owner->m_lookAtAlertSpot )
	{
		owner->m_lookAtAlertSpot = false;
		idVec3 alertSpot = owner->m_lookAtPos;
		if ( alertSpot.x != idMath::INFINITY ) // grayman #3438
		{
			if ( !owner->CheckFOV(alertSpot) )
			{
				// Search spot is not within FOV, turn towards the position
				owner->TurnToward(alertSpot);
				owner->Event_LookAtPosition(alertSpot, 2.0f);
			}
			else
			{
				owner->Event_LookAtPosition(alertSpot, 2.0f);
			}
		}
		owner->m_lookAtPos = idVec3(idMath::INFINITY,idMath::INFINITY,idMath::INFINITY);
	}

	// grayman #3200 - if asked to restart the hiding spot search, don't continue with the current hiding spot search
	if (memory.restartSearchForHidingSpots)
	{
		// We should restart the search (probably due to a new incoming stimulus)
		// Setup a new hiding spot search
		StartNewHidingSpotSearch(owner);
	}
	else if (!memory.hidingSpotSearchDone) // Do we have an ongoing hiding spot search?
	{
		// Let the hiding spot search do its task
		PerformHidingSpotSearch(owner);

		// Let the AI check its senses
//		owner->PerformVisualScan(); // grayman #3063 - moved to front
/*
		// angua: commented this out, problems with getting up from sitting
		idStr waitState(owner->WaitState());
		if (waitState.IsEmpty())
		{
			// Waitstate is not matching, this means that the animation 
			// can be started.
			owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_LookAround", 5);
			//owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_LookAround", 5);

			// Set the waitstate, this gets cleared by 
			// the script function when the animation is done.
			owner->SetWaitState("look_around");
		}
*/
	}
	// Is a hiding spot search in progress?
	else if (!memory.hidingSpotInvestigationInProgress)
	{
		// Spot search and investigation done, what next?

		// Have run out of hiding spots?

		if (memory.noMoreHidingSpots) 
		{
			if ( gameLocal.time >= memory.nextTime2GenRandomSpot )
			{
				memory.nextTime2GenRandomSpot = gameLocal.time + DELAY_RANDOM_SPOT_GEN*(1 + (gameLocal.random.RandomFloat() - 0.5)/3);

				// grayman #2422
				// Generate a random search point, but make sure it's inside an AAS area
				// and that it's also inside the search volume.

				idVec3 p;		// random point
				int areaNum;	// p's area number
				idVec3 searchSize = owner->m_searchLimits.GetSize();
				idVec3 searchCenter = owner->m_searchLimits.GetCenter();
				
				//gameRenderWorld->DebugBox(colorWhite, idBox(owner->m_searchLimits), MS2SEC(memory.nextTime2GenRandomSpot - gameLocal.time));

				bool validPoint = false;
				for ( int i = 0 ; i < 6 ; i++ )
				{
					p = searchCenter;
					p.x += gameLocal.random.RandomFloat()*(searchSize.x) - searchSize.x/2;
					p.y += gameLocal.random.RandomFloat()*(searchSize.y) - searchSize.y/2;
					p.z += gameLocal.random.RandomFloat()*(searchSize.z) - searchSize.z/2;
					//p.z += gameLocal.random.RandomFloat()*(searchSize.z/2) - searchSize.z/4;
					areaNum = owner->PointReachableAreaNum( p );
					if ( areaNum == 0 )
					{
						//gameRenderWorld->DebugArrow(colorRed, owner->GetEyePosition(), p, 1, MS2SEC(memory.nextTime2GenRandomSpot - gameLocal.time));
						continue;
					}
					owner->GetAAS()->PushPointIntoAreaNum( areaNum, p ); // if this point is outside this area, it will be moved to one of the area's edges
					if ( !owner->m_searchLimits.ContainsPoint(p) )
					{
						//gameRenderWorld->DebugArrow(colorPink, owner->GetEyePosition(), p, 1, MS2SEC(memory.nextTime2GenRandomSpot - gameLocal.time));
						continue;
					}

					//gameRenderWorld->DebugArrow(colorGreen, owner->GetEyePosition(), p, 1, MS2SEC(memory.nextTime2GenRandomSpot - gameLocal.time));
					validPoint = true;
					break;
				}

				if ( validPoint )
				{
					// grayman #2422 - the point chosen 
					memory.currentSearchSpot = p;
			
					// Choose to investigate spots closely on a random basis
					// grayman #2801 - and only if you weren't hit by a projectile

					memory.investigateStimulusLocationClosely = ( ( gameLocal.random.RandomFloat() < 0.3f ) && ( memory.alertType != EAlertTypeHitByProjectile ) );

					owner->actionSubsystem->PushTask(TaskPtr(InvestigateSpotTask::CreateInstance()));
					//gameRenderWorld->DebugArrow(colorGreen, owner->GetEyePosition(), memory.currentSearchSpot, 1, 500);

					// Set the flag to TRUE, so that the sensory scan can be performed
					memory.hidingSpotInvestigationInProgress = true;
				}

				if ( !validPoint ) // no valid random point found
				{
					// Stop moving, the algorithm will choose another spot the next round
					owner->StopMove(MOVE_STATUS_DONE);
					memory.stopRelight = true; // grayman #2603 - abort a relight in progress
					memory.stopExaminingRope = true; // grayman #2872 - stop examining rope
					memory.stopReactingToHit = true; // grayman #2816

					// grayman #2422 - at least turn toward and look at the last invalid point some of the time
					// grayman #3492 - do it every time
					//if ( gameLocal.random.RandomFloat() < 0.5 )
					//{
					p.z += 60; // look up a bit, to simulate searching for the player's head
					if (!owner->CheckFOV(p))
					{
						owner->TurnToward(p);
					}
					owner->Event_LookAtPosition(p,MS2SEC(memory.nextTime2GenRandomSpot - gameLocal.time + 100));
					//gameRenderWorld->DebugArrow(colorPink, owner->GetEyePosition(), p, 1, MS2SEC(memory.nextTime2GenRandomSpot - gameLocal.time + 100));
					//}
				}
			}
		}
		// We should have more hiding spots, try to get the next one
		else if (!ChooseNextHidingSpotToSearch(owner))
		{
			// No more hiding spots to search
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("No more hiding spots!\r");

			// Stop moving, the algorithm will choose another spot the next round
			owner->StopMove(MOVE_STATUS_DONE);
			memory.stopRelight = true; // grayman #2603 - abort a relight in progress
			memory.stopExaminingRope = true; // grayman #2872 - stop examining rope
			memory.stopReactingToHit = true; // grayman #2816
		}
		else
		{
			// ChooseNextHidingSpot returned TRUE, so we have memory.currentSearchSpot set

			//gameRenderWorld->DebugArrow(colorBlue, owner->GetEyePosition(), memory.currentSearchSpot, 1, 2000);

			// Delegate the spot investigation to a new task, this will take the correct action.
			owner->actionSubsystem->PushTask(InvestigateSpotTask::CreateInstance());

			// Prevent falling into the same hole twice
			memory.hidingSpotInvestigationInProgress = true;
		}
	}
/* grayman #3200 - moved up
	else if (memory.restartSearchForHidingSpots)
	{
		// We should restart the search (probably due to a new incoming stimulus)
		// Setup a new hiding spot search
		StartNewHidingSpotSearch(owner);
	}

	else // grayman #3063 - moved to front
	{
		// Move to Hiding spot is ongoing, do additional sensory tasks here

		// Let the AI check its senses
		owner->PerformVisualScan();
	}
 */
}

void SearchingState::StartNewHidingSpotSearch(idAI* owner)
{
	Memory& memory = owner->GetMemory();

	// grayman #3438 - exit if there's no focus for the search
	if ( memory.alertPos.x == idMath::INFINITY )
	{
		return;
	}

	// Clear flags
	memory.restartSearchForHidingSpots = false;
	memory.noMoreHidingSpots = false;
	memory.mandatory = false; // grayman #3331

	// Clear all the ongoing tasks
	owner->senseSubsystem->ClearTasks();
	owner->actionSubsystem->ClearTasks();
	owner->movementSubsystem->ClearTasks();

	// Stop moving
	owner->StopMove(MOVE_STATUS_DONE);
	memory.stopRelight = true; // grayman #2603 - abort a relight in progress
	memory.stopExaminingRope = true; // grayman #2872 - stop examining rope
	memory.stopReactingToHit = true; // grayman #2816

	owner->MarkEventAsSearched(memory.currentSearchEventID); // grayman #3424

	memory.lastAlertPosSearched = memory.alertPos; // grayman #3492

	// If we are supposed to search the stimulus location do that instead 
	// of just standing around while the search completes
	if (memory.stimulusLocationItselfShouldBeSearched)
	{
		// The InvestigateSpotTask will take this point as first hiding spot
		memory.currentSearchSpot = memory.alertPos;

		// Delegate the spot investigation to a new task, this will take the correct action.
		owner->actionSubsystem->PushTask(
			TaskPtr(new InvestigateSpotTask(memory.investigateStimulusLocationClosely))
		);

		//gameRenderWorld->DebugArrow(colorPink, owner->GetEyePosition(), memory.currentSearchSpot, 1, 2000);

		// Prevent overwriting this hiding spot in the upcoming Think() call
		memory.hidingSpotInvestigationInProgress = true;

		// Reset the flags
		memory.stimulusLocationItselfShouldBeSearched = false;
		memory.investigateStimulusLocationClosely = false;
	}
	else
	{
		// AI is not moving, wait for spot search to complete
		memory.hidingSpotInvestigationInProgress = false;
		memory.currentSearchSpot = idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY);
	}

	idVec3 minBounds(memory.alertPos - memory.alertSearchVolume);
	idVec3 maxBounds(memory.alertPos + memory.alertSearchVolume);

	idVec3 minExclusionBounds(memory.alertPos - memory.alertSearchExclusionVolume);
	idVec3 maxExclusionBounds(memory.alertPos + memory.alertSearchExclusionVolume);
	
	// Close any previous search
	owner->Event_CloseHidingSpotSearch();

	// Hiding spot test now started
	memory.hidingSpotSearchDone = false;
	memory.hidingSpotTestStarted = true;

	// Invalidate the vector, clear the indices
	memory.firstChosenHidingSpotIndex = -1;
	memory.currentChosenHidingSpotIndex = -1;

	// Start search
	int res = owner->StartSearchForHidingSpotsWithExclusionArea(
		owner->GetEyePosition(), 
		minBounds, maxBounds, 
		minExclusionBounds, maxExclusionBounds, 
		255, owner
	);

	if (res == 0)
	{
		// Search completed on first round
		memory.hidingSpotSearchDone = true;
	}
}

void SearchingState::PerformHidingSpotSearch(idAI* owner)
{
	// Shortcut reference
	Memory& memory = owner->GetMemory();

	// Increase the frame count
	memory.hidingSpotThinkFrameCount++;

	if (owner->ContinueSearchForHidingSpots() == 0)
	{
		// search completed
		memory.hidingSpotSearchDone = true;

		// Hiding spot test is done
		memory.hidingSpotTestStarted = false;
		
		// Here we transition to the state for handling the behavior of
		// the AI once it thinks it knows where the stimulus may have
		// come from
		
		// For now, starts searching for the stimulus.  We probably want
		// a way for different AIs to act differently
		// TODO: Morale check etc...

		// Get location
		memory.chosenHidingSpot = owner->GetNthHidingSpotLocation(memory.currentChosenHidingSpotIndex);
	}
}

void SearchingState::RandomizeHidingSpotList(idAI* owner) // grayman #3424
{
	// Randomize an array of ints to be used for indexes into
	// the list of hiding spot.

	int numSpots = owner->m_hidingSpots.getNumSpots();
	owner->m_randomHidingSpotIndexes.clear(); // clear any existing array elements

	// Fill the array with integers from 0 to numSpots-1.
	for ( int i = 0 ; i < numSpots ; i++ )
	{
		owner->m_randomHidingSpotIndexes.push_back(i);
	}

    // Shuffle elements by randomly exchanging pairs.
    for ( int i = 0 ; i < (numSpots-1) ; i++ )
	{
        int r = i + gameLocal.random.RandomInt(numSpots - i); // Random remaining position.
        int temp = owner->m_randomHidingSpotIndexes[i];
		owner->m_randomHidingSpotIndexes[i] = owner->m_randomHidingSpotIndexes[r];
		owner->m_randomHidingSpotIndexes[r] = temp;
    }
}

bool SearchingState::ChooseNextHidingSpotToSearch(idAI* owner)
{
	Memory& memory = owner->GetMemory();

	int numSpots = owner->m_hidingSpots.getNumSpots();
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("Found hidings spots: %d\r", numSpots);

	// Choose randomly
	if (numSpots > 0)
	{
		if (memory.firstChosenHidingSpotIndex == -1)
		{
			// No hiding spot chosen yet, initialise
			/*
			// Get visual acuity
			float visAcuity = getAcuity("vis");
				
			// Since earlier hiding spots are "better" (ie closer to stimulus and darker or more occluded)
			// higher visual acuity should bias toward earlier in the list
			float bias = 1.0 - visAcuity;
			if (bias < 0.0)
			{
				bias = 0.0;
			}
			*/
			// greebo: TODO? This isn't random choosing...

			// grayman #3424
			// Randomize hiding spot list indexes before selecting anything from the list.
			RandomizeHidingSpotList(owner);

			// grayman #3424 - catch an invalid starting spot early, rather than letting InvestigateSpotTask
			// be queued to run, only to discover the spot is invalid. If all available spots are invalid,
			// quit the search.

			int spotIndex = -1;
			idVec3 spot;
			for ( int i = 0 ; i < numSpots ; i++ )
			{
				// Get location
				spot = owner->GetNthHidingSpotLocation(owner->m_randomHidingSpotIndexes[i]);
				if ( !spot.Compare(idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY) ) )
				{
					spotIndex = i;
					break; // valid spot
				}
			}

			if ( spotIndex == -1 ) // no valid spots?
			{
				DM_LOG(LC_AI, LT_INFO)LOGSTRING("No valid spots to search.\r");
				memory.hidingSpotSearchDone = false;
				memory.chosenHidingSpot = idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY);
				memory.currentChosenHidingSpotIndex = -1;
				memory.firstChosenHidingSpotIndex = -1;
				memory.noMoreHidingSpots = true;
				return false;
			}

			// Remember which hiding spot we have chosen at start
			memory.firstChosenHidingSpotIndex = spotIndex;
				
			// Note currently chosen hiding spot
			memory.currentChosenHidingSpotIndex = spotIndex;
			
			memory.chosenHidingSpot = spot;
			memory.currentSearchSpot = memory.chosenHidingSpot;
			//gameRenderWorld->DebugArrow(colorBlue, owner->GetEyePosition(), memory.currentSearchSpot, 1, 2000);

			DM_LOG(LC_AI, LT_INFO)LOGSTRING(
				"First spot chosen is index %d of %d spots.\r", 
				memory.firstChosenHidingSpotIndex, numSpots
			);
		}
		else 
		{
			// grayman #3424 - catch invalid spots early, rather than letting InvestigateSpotTask
			// be queued to run, only to discover the spot is invalid.

			for ( int i = 0 ; i < numSpots ; i++ )
			{
				// Make sure we stay in bounds
				memory.currentChosenHidingSpotIndex++;
				if (memory.currentChosenHidingSpotIndex >= numSpots)
				{
					memory.currentChosenHidingSpotIndex = 0;
				}

				// Have we wrapped around to first one searched?
				if ( ( memory.currentChosenHidingSpotIndex == memory.firstChosenHidingSpotIndex ) || 
					 ( memory.currentChosenHidingSpotIndex < 0 ) )
				{
					// No more hiding spots
					DM_LOG(LC_AI, LT_INFO)LOGSTRING("No more hiding spots to search.\r");
					memory.hidingSpotSearchDone = false;
					memory.chosenHidingSpot = idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY);
					memory.currentChosenHidingSpotIndex = -1;
					memory.firstChosenHidingSpotIndex = -1;
					memory.noMoreHidingSpots = true;
					return false;
				}

				// Index is valid, let's acquire the position
				DM_LOG(LC_AI, LT_INFO)LOGSTRING("Next spot chosen is index %d of %d\r", 
					owner->m_randomHidingSpotIndexes[memory.currentChosenHidingSpotIndex], numSpots-1);

				idVec3 spot = owner->GetNthHidingSpotLocation(owner->m_randomHidingSpotIndexes[memory.currentChosenHidingSpotIndex]);
				if ( spot.Compare(idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY) ) )
				{
					continue; // skip this spot
				}

				memory.chosenHidingSpot = spot;
				memory.currentSearchSpot = spot;
				memory.hidingSpotSearchDone = true;
				//gameRenderWorld->DebugArrow(colorBlue, owner->GetEyePosition(), spot, 1, 2000);
				break;
			}
		}
	}
	else
	{
		DM_LOG(LC_AI, LT_INFO)LOGSTRING("Didn't find any hiding spots near stimulus.\r");
		memory.firstChosenHidingSpotIndex = -1;
		memory.currentChosenHidingSpotIndex = -1;
		memory.chosenHidingSpot = idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY);
		memory.currentSearchSpot = idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY);
		memory.noMoreHidingSpots = true;
		return false;
	}

	return true;
}

void SearchingState::OnAudioAlert()
{
	// First, call the base class
	State::OnAudioAlert();

	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);
	Memory& memory = owner->GetMemory();

	if (!memory.alertPos.Compare(memory.currentSearchSpot, 50))
	{
		// The position of the sound is different from the current search spot, so redefine the goal
		TaskPtr curTask = owner->actionSubsystem->GetCurrentTask();
		InvestigateSpotTaskPtr spotTask = boost::dynamic_pointer_cast<InvestigateSpotTask>(curTask);
			
		if (spotTask != NULL)
		{
			// Redirect the owner to a new position
			spotTask->SetNewGoal(memory.alertPos);
			spotTask->SetInvestigateClosely(false);
			memory.restartSearchForHidingSpots = true; // grayman #3200

			//gameRenderWorld->DebugArrow(colorYellow, owner->GetEyePosition(), memory.alertPos, 1, 2000);
		}
	}
	else
	{
		// grayman #3200 - we're about to ignore the new sound and continue with
		// the current search, but we should at least turn toward the new sound
		// to acknowledge having heard it

		// grayman #3424 - If we're on the move, only look at alertPos. Turning toward
		// it disrupts whatever movement we're doing. If we're standing still, turn
		// toward the spot as well as look at it.

		//owner->StopMove(MOVE_STATUS_DONE);
		if ( !owner->AI_FORWARD )
		{
			owner->TurnToward(memory.alertPos);
		}

		idVec3 target = memory.alertPos;
		target.z += 32;
		owner->Event_LookAtPosition(target,MS2SEC(LOOK_AT_AUDIO_SPOT_DURATION + (gameLocal.random.RandomFloat() - 0.5)*1000));
		//gameRenderWorld->DebugArrow(colorBlue, owner->GetEyePosition(), memory.alertPos, 1, 2000);
	}

	if (memory.alertSearchCenter != idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY))
	{
		// We have a valid search center
		float distanceToSearchCenter = (memory.alertPos - memory.alertSearchCenter).LengthSqr();
		if (distanceToSearchCenter > memory.alertSearchVolume.LengthSqr())
		{
			// The alert position is far from the current search center, restart search
			memory.restartSearchForHidingSpots = true;

			//gameRenderWorld->DebugArrow(colorRed, owner->GetEyePosition(), memory.alertPos, 1, 2000);
		}
	}
}

StatePtr SearchingState::CreateInstance()
{
	return StatePtr(new SearchingState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar searchingStateRegistrar(
	STATE_SEARCHING, // Task Name
	StateLibrary::CreateInstanceFunc(&SearchingState::CreateInstance) // Instance creation callback
);

} // namespace ai
