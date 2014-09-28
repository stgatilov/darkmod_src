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
#include "../Tasks/GuardSpotTask.h" // grayman debug
#include "../Tasks/SingleBarkTask.h"
#include "../Tasks/RepeatedBarkTask.h" // grayman #3472
#include "../Library.h"
#include "IdleState.h"
#include "AgitatedSearchingState.h"
#include "../../AbsenceMarker.h"
#include "../../AIComm_Message.h"
#include "FleeState.h" // grayman #3317

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
		if (owner->m_searchID >= 0) // grayman debug
		{
			gameLocal.m_searchManager->LeaveSearch(owner->m_searchID,owner); // grayman debug
		}
		//owner->Event_CloseHidingSpotSearch();
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

	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("SearchingState::Init - %s wants to start searching\r",owner->GetName()); // grayman debug
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("SearchingState initialised.\r");
	assert(owner);

	// Ensure we are in the correct alert level
	if ( !CheckAlertLevel(owner) )
	{
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("SearchingState::Init - %s leaving SearchingState\r",owner->GetName()); // grayman debug
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

	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("SearchingState::Init - %s AlertIndexIncreased() = %d\r",owner->GetName(),owner->AlertIndexIncreased()); // grayman debug
	if ( owner->AlertIndexIncreased() || memory.mandatory ) // grayman #3331
	{
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("SearchingState::Init - %s calling StartNewHidingSpotSearch()\r",owner->GetName()); // grayman debug
		// Setup a new hiding spot search
		StartNewHidingSpotSearch(owner); // grayman debug - AI gets his assignment
	}
	else // grayman debug
	{
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("SearchingState::Init - %s NOT calling StartNewHidingSpotSearch()\r",owner->GetName()); // grayman debug
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
	//owner->PerformVisualScan(); // grayman debug - uncomment soon

	if (owner->GetMoveType() == MOVETYPE_SIT 
		|| owner->GetMoveType() == MOVETYPE_SLEEP
		|| owner->GetMoveType() == MOVETYPE_SIT_DOWN
		|| owner->GetMoveType() == MOVETYPE_LAY_DOWN)
	{
		owner->GetUp();
		return;
	}

	Memory& memory = owner->GetMemory();

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

		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("SearchingState::Think - %s calling StartNewHidingSpotSearch() to restart a search\r",owner->GetName()); // grayman debug
		// Set up a new hiding spot search
		StartNewHidingSpotSearch(owner); // grayman debug - AI gets his assignment
		return;
	}

	// Do we have an ongoing hiding spot search?
	if (!memory.hidingSpotSearchDone)
	{
		// Let the hiding spot search do its task
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("SearchingState::Think - %s calling PerformHidingSpotSearch() to assign a spot\r",owner->GetName()); // grayman debug
		gameLocal.m_searchManager->PerformHidingSpotSearch(owner->m_searchID,owner); // grayman debug
		return;
	}

	// Is a hiding spot investigation in progress by this AI (InvestigateSpotTask())?
	if (memory.hidingSpotInvestigationInProgress)
	{
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("SearchingState::Think - %s hiding spot investigation underway\r",owner->GetName()); // grayman debug
		// Do nothing here. Wait for the InvestigateSpot task to complete
		return;
	}

	// Is a guard spot task in progress by this AI (GuardSpotTask())?
	if (memory.guardingInProgress)
	{
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("SearchingState::Think - %s guard spot task underway\r",owner->GetName()); // grayman debug
		// Do nothing here. Wait for the GuardSpot task to complete
		return;
	}

	// Hiding spot list is complete, but we don't have an assignment yet.
	// Get something to do based on our assignment.

	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("SearchingState::Think - %s start your assignment\r",owner->GetName()); // grayman debug

	// What is my role? If I'm a guard, I should do one thing. If a searcher, ask for another hiding spot to go to.

	Assignment* assignment = gameLocal.m_searchManager->GetAssignment(gameLocal.m_searchManager->GetSearch(owner->m_searchID),owner);

	if (assignment)
	{
		smRole_t role = assignment->_searcherRole;
		if (role == E_ROLE_SEARCHER)
		{
			// Have we run out of hiding spots?

			if (memory.noMoreHidingSpots) 
			{
				DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("SearchingState::Think - %s there are no more hiding spots\r",owner->GetName()); // grayman debug
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

						DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("SearchingState::Think 1 - %s hidingSpotInvestigationInProgress set to true\r",owner->GetName()); // grayman debug
						// Set the flag to TRUE, so that the sensory scan can be performed
						memory.hidingSpotInvestigationInProgress = true;
					}

					if ( !validPoint ) // no valid random point found
					{
						// Stop moving, the algorithm will choose another spot the next round
						owner->StopMove(MOVE_STATUS_DONE);
						memory.StopReacting(); // grayman #3559

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
			else if (!gameLocal.m_searchManager->GetNextHidingSpot(gameLocal.m_searchManager->GetSearch(owner->m_searchID),owner,memory.currentSearchSpot)) // grayman debug
			{
				DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("SearchingState::Think - %s no more hiding spots to search, stop moving\r",owner->GetName()); // grayman debug
				// No more hiding spots to search
				DM_LOG(LC_AI, LT_INFO)LOGSTRING("No more hiding spots!\r");

				// Stop moving, the algorithm will choose another spot the next round
				owner->StopMove(MOVE_STATUS_DONE);
				memory.StopReacting(); // grayman #3559
			}
			else
			{
				// ChooseNextHidingSpot() returned TRUE, so we have memory.currentSearchSpot set
				DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("SearchingState::Think - %s assigned the next hiding spot, memory.currentSearchSpot = [%s]\r",owner->GetName(),memory.currentSearchSpot.ToString()); // grayman debug

				//gameRenderWorld->DebugArrow(colorBlue, owner->GetEyePosition(), memory.currentSearchSpot, 1, 2000);

				// Delegate the spot investigation to a new task, this will take the correct action.
				owner->actionSubsystem->PushTask(InvestigateSpotTask::CreateInstance());

				DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("SearchingState::Think 2 - %s hidingSpotInvestigationInProgress set to true\r",owner->GetName()); // grayman debug
				// Prevent falling into the same hole twice
				memory.hidingSpotInvestigationInProgress = true;
			}
		}
		else if (role == E_ROLE_GUARD)
		{
			// Pick a spot to guard

			// TODO: Pick a real spot. For now, force a spot that fits bug3857b's architecture
			memory.currentSearchSpot = idVec3(384,0,-127); // spot to guard
			owner->actionSubsystem->PushTask(TaskPtr(GuardSpotTask::CreateInstance()));
		}
	}
}

bool SearchingState::OnAudioAlert(idStr soundName) // grayman #3847
{
	// Firt, call the base class
	if (!State::OnAudioAlert(soundName)) // grayman #3847
	{
		return true;
	}

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
			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("SearchingState::OnAudioAlert 1 - %s memory.restartSearchForHidingSpots set to %d\r",owner->GetName(),memory.restartSearchForHidingSpots); // grayman debug

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
			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("SearchingState::OnAudioAlert 2 - %s memory.restartSearchForHidingSpots set to %d\r",owner->GetName(),memory.restartSearchForHidingSpots); // grayman debug

			//gameRenderWorld->DebugArrow(colorRed, owner->GetEyePosition(), memory.alertPos, 1, 2000);
		}
	}
	return true;
}

void SearchingState::StartNewHidingSpotSearch(idAI* owner) // grayman debug
{
	int newSearchID = gameLocal.m_searchManager->StartNewHidingSpotSearch(owner);

	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("SearchingState::StartNewHidingSpotSearch - %s starting a new search with id %d\r",owner->GetName(),newSearchID); // grayman debug

	// Clear ai flags
	ai::Memory& memory = owner->GetMemory();
	memory.restartSearchForHidingSpots = false;
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("SearchingState::StartNewHidingSpotSearch - %s memory.restartSearchForHidingSpots set to %d\r",owner->GetName(),memory.restartSearchForHidingSpots); // grayman debug
	memory.noMoreHidingSpots = false;
	memory.mandatory = false; // grayman #3331

	// Clear all the ongoing tasks
	owner->senseSubsystem->ClearTasks();
	owner->actionSubsystem->ClearTasks();
	owner->movementSubsystem->ClearTasks();

	// Stop moving
	owner->StopMove(MOVE_STATUS_DONE);
	memory.StopReacting(); // grayman #3559

	owner->MarkEventAsSearched(memory.currentSearchEventID); // grayman #3424

	memory.lastAlertPosSearched = memory.alertPos; // grayman #3492

	// greebo: Remember the initial alert position
	memory.alertSearchCenter = memory.alertPos;

	// If we are supposed to search the stimulus location do that instead 
	// of just standing around while the search completes
	if (memory.stimulusLocationItselfShouldBeSearched)
	{
		// The InvestigateSpotTask will take this point as first hiding spot
		// It's okay for the AI to move toward the alert position, even if he's
		// later assigned to be a guard.
		memory.currentSearchSpot = memory.alertPos;

		// Delegate the spot investigation to a new task, this will take the correct action.
		owner->actionSubsystem->PushTask(
			TaskPtr(new InvestigateSpotTask(memory.investigateStimulusLocationClosely))
		);

		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("SearchingState::StartNewHidingSpotSearch - %s hidingSpotInvestigationInProgress set to true\r",owner->GetName()); // grayman debug
		// Prevent overwriting this hiding spot in the upcoming Think() call
		memory.hidingSpotInvestigationInProgress = true;

		// Reset flag
		memory.investigateStimulusLocationClosely = false;
	}
	else
	{
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("SearchingState::StartNewHidingSpotSearch - %s hidingSpotInvestigationInProgress set to false\r",owner->GetName()); // grayman debug
		// AI is not moving, wait for spot search to complete
		memory.hidingSpotInvestigationInProgress = false;
		memory.currentSearchSpot = idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY);
	}

	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("SearchingState::StartNewHidingSpotSearch - %s leaving old search with id %d, calling LeaveSearch()\r",owner->GetName(),owner->m_searchID); // grayman debug
	// Leave any current search
	gameLocal.m_searchManager->LeaveSearch(owner->m_searchID,owner);

	// Hiding spot test now started
	memory.hidingSpotSearchDone = false;
	memory.hidingSpotTestStarted = true;

	// Invalidate the vector, clear the indices
	//memory.firstChosenHidingSpotIndex = -1;   // grayman debug
	//memory.currentChosenHidingSpotIndex = -1; // grayman debug

	Search *search = gameLocal.m_searchManager->GetSearch(newSearchID);

	// Start search
	// TODO: Is the eye position necessary? Since the hiding spot list can be
	// used by several AI, why is the first AI's eye position relevant
	// to the other AIs' eye positions?
	int res = gameLocal.m_searchManager->StartSearchForHidingSpotsWithExclusionArea(search,owner->GetEyePosition(),255, owner);

	if (res == 0)
	{
		// Search completed on first round
		memory.hidingSpotSearchDone = true;
	}

	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("SearchingState::StartNewHidingSpotSearch - %s joining new search, calling JoinSearch()\r",owner->GetName()); // grayman debug
	gameLocal.m_searchManager->JoinSearch(newSearchID,owner); // gives the ai his assignment
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
