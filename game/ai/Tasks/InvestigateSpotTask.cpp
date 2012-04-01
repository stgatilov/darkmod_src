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

#include "InvestigateSpotTask.h"
#include "WaitTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

const int INVESTIGATE_SPOT_TIME_REMOTE = 2000; // ms (grayman #2640 - change from 600 -> 2000)
const int INVESTIGATE_SPOT_TIME_STANDARD = 300; // ms
const int INVESTIGATE_SPOT_TIME_CLOSELY = 2500; // ms

const int INVESTIGATE_SPOT_STOP_DIST = 100; // grayman #2640 - even if you can see the spot, keep moving if farther away than this
const int INVESTIGATE_SPOT_MIN_DIST  =  20;
const int INVESTIGATE_SPOT_CLOSELY_MAX_DIST = 100; // grayman #2928

const float MAX_TRAVEL_DISTANCE_WALKING = 300; // units?

InvestigateSpotTask::InvestigateSpotTask() :
	_investigateClosely(false),
	_moveInitiated(false)
{}

InvestigateSpotTask::InvestigateSpotTask(bool investigateClosely) :
	_investigateClosely(investigateClosely),
	_moveInitiated(false)
{}

// Get the name of this task
const idStr& InvestigateSpotTask::GetName() const
{
	static idStr _name(TASK_INVESTIGATE_SPOT);
	return _name;
}

void InvestigateSpotTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);

	// Get a shortcut reference
	Memory& memory = owner->GetMemory();

	// Stop previous moves
	//owner->StopMove(MOVE_STATUS_DONE);

	if (memory.currentSearchSpot != idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY))
	{
		// Set the goal position
		SetNewGoal(memory.currentSearchSpot);
	}
	else
	{
		// Invalid hiding spot, terminate task
		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("memory.currentSearchSpot not set to something valid, terminating task.\r");
		subsystem.FinishTask();
	}
}

bool InvestigateSpotTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("InvestigateSpotTask performing.\r");

	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	if (_exitTime > 0)
	{
		// Return TRUE if the time is over, else FALSE (continue)
		return (gameLocal.time > _exitTime);
	}
	
	// No exit time set, continue with ordinary process

	if (owner->m_HandlingDoor || owner->m_HandlingElevator)
	{
		// Wait, we're busy with a door or elevator
		return false;
	}

	if (!_moveInitiated)
	{
		idVec3 destPos = _searchSpot;

		// greebo: For close investigation, don't step up to the very spot, to prevent the AI
		// from kneeling into bloodspots or corpses
		idVec3 direction = owner->GetPhysics()->GetOrigin() - _searchSpot;
		if (_investigateClosely)
		{
			idVec3 dir = direction;
			dir.NormalizeFast();

			// 20 units before the actual spot
			destPos += dir * 20;
		}

		// grayman #2603 - The fix to #2640 had the AI stopping if he's w/in INVESTIGATE_SPOT_STOP_DIST of the
		// search spot. This caused sudden jerks if he's closer than that at the start of the
		// move. To prevent that, check if he's close and--if so--don't start the move.

		if (!_investigateClosely && (direction.LengthFast() < INVESTIGATE_SPOT_STOP_DIST))
		{
			// Look at the point to investigate
			owner->Event_LookAtPosition(_searchSpot, 2.0f);

			// Wait a bit
			_exitTime = static_cast<int>(
				gameLocal.time + INVESTIGATE_SPOT_TIME_REMOTE*(1 + gameLocal.random.RandomFloat()) // grayman #2640
			);

			return false; // grayman #2422
		}

		// Let's move

		// grayman #2422
		// Here's the root of the problem. PointReachableAreaNum()
		// doesn't always look to the side to find the nearest AAS
		// area at the point's z position. It can move the point to
		// the AAS area above, or the AAS area below. This makes the AI
		// run upstairs or downstairs when all we really want him to do
		// is stay on the same floor.

		// If the AI is searching and not handling a door or handling
		// an elevator or resolving a block: If the spot PointReachableAreaNum()/PushPointIntoAreaNum()
		// wants to move us to is outside the vertical boundaries of the
		// search volume, consider the point bad.
		
		bool pointValid = true;
		idVec3 goal = destPos;
		int toAreaNum = owner->PointReachableAreaNum( goal );
		if ( toAreaNum == 0 )
		{
			pointValid =  false;
		}
		else
		{
			owner->GetAAS()->PushPointIntoAreaNum( toAreaNum, goal ); // if this point is outside this area, it will be moved to one of the area's edges
		
			if ( owner->IsSearching() && !owner->movementSubsystem->IsResolvingBlock() )
			{
				if ( !owner->m_searchLimits.ContainsPoint(goal) )
				{
					pointValid =  false;
				}
			}
		}

		if ( pointValid )
		{
			pointValid = owner->MoveToPosition(goal);
			_moveInitiated = true;
		}

		if ( !pointValid || ( owner->GetMoveStatus() == MOVE_STATUS_DEST_UNREACHABLE) )
		{
			// Hiding spot not reachable, terminate task in the next round
			_exitTime = gameLocal.time;
		}
		else
		{
			// Run if the point is more than MAX_TRAVEL_DISTANCE_WALKING
			// greebo: This is taxing and can be replaced by a simpler distance check 
			// TravelDistance takes about ~0.1 msec on my 2.2 GHz system.

			// grayman #2422 - not the player = walk, player & combat = run, everything else = run
			// Also, travelDist is inaccurate when an AAS area is large, so compare
			// it to the actual distance and use the larger of the two.

			float travelDist = owner->TravelDistance(owner->GetPhysics()->GetOrigin(), _searchSpot);
			float actualDist = (owner->GetPhysics()->GetOrigin() - _searchSpot).LengthFast();
			if ( actualDist > travelDist )
			{
				travelDist = actualDist;
			}

			if ( travelDist <= MAX_TRAVEL_DISTANCE_WALKING ) // close enough to walk?
			{
				owner->AI_RUN = false;
			}
			else if ( owner->GetMemory().visualAlert ) // spotted the player by testing his visibility?
			{
				owner->AI_RUN = false; // when AI's alert index enters Combat mode, that code will get him running
			}
			else // searching for some other reason, and we're far away, so run
			{
				owner->AI_RUN = true;
			}

			//owner->AI_RUN = (travelDist > MAX_TRAVEL_DISTANCE_WALKING); // grayman #2422 - old way
		}

		return false;
	}

	if (owner->GetMoveStatus() == MOVE_STATUS_DEST_UNREACHABLE)
	{
		DM_LOG(LC_AI, LT_INFO)LOGVECTOR("Hiding spot unreachable.\r", _searchSpot);
		return true;
	}
	else if (owner->GetMoveStatus() == MOVE_STATUS_DONE)
	{
		DM_LOG(LC_AI, LT_INFO)LOGVECTOR("Hiding spot investigated: \r", _searchSpot);

		// grayman #2928 - don't kneel down if you're too far from the original stim

		float dist = ( owner->GetPhysics()->GetOrigin() - owner->GetMemory().alertSearchCenter).LengthFast();

		if ( _investigateClosely && ( dist < INVESTIGATE_SPOT_CLOSELY_MAX_DIST ) )
		{
			// Stop previous moves
			owner->StopMove(MOVE_STATUS_WAITING);

			// Check the position of the stim, is it closer to the eyes than to the feet?
			// If it's lower than the eye position, kneel down and investigate
			const idVec3& origin = owner->GetPhysics()->GetOrigin();
			idVec3 eyePos = owner->GetEyePosition();
			if ((_searchSpot - origin).LengthSqr() < (_searchSpot - eyePos).LengthSqr())
			{
				// Close to the feet, kneel down and investigate closely
				owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_KneelDown", 6);
				owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_KneelDown", 6);
			}

			// Wait a bit, setting _exitTime sets the lifetime of this task
			_exitTime = static_cast<int>(
				gameLocal.time + INVESTIGATE_SPOT_TIME_CLOSELY*(1 + gameLocal.random.RandomFloat()*0.2f)
			);
		}
		else
		{
			// Wait a bit, setting _exitTime sets the lifetime of this task
			_exitTime = static_cast<int>(
				gameLocal.time + INVESTIGATE_SPOT_TIME_STANDARD*(1 + gameLocal.random.RandomFloat()*0.2f)
			);
		}
	}
	else
	{
		// Can we already see the point? Only stop moving when the spot shouldn't be investigated closely
		// angua: added distance check to avoid running in circles if the point is too close to a wall.
		// grayman #2640 - keep moving if you're > INVESTIGATE_SPOT_STOP_DIST from a point you can see

		bool stopping = false;
		if (!_investigateClosely)
		{
			float distToSpot = (_searchSpot - owner->GetPhysics()->GetOrigin()).LengthFast();
			if (owner->CanSeePositionExt(_searchSpot, true, true))
			{
				if (distToSpot < INVESTIGATE_SPOT_STOP_DIST) 
				{
					stopping = true;
				}
			}
			else if (distToSpot < INVESTIGATE_SPOT_MIN_DIST)
			{
				stopping = true;
			}
		}
		if (stopping)
		{
			DM_LOG(LC_AI, LT_INFO)LOGVECTOR("Stop, I can see the point now...\r", _searchSpot);

			// Stop moving, we can see the point
			owner->StopMove(MOVE_STATUS_DONE);

			//Look at the point to investigate
			owner->Event_LookAtPosition(_searchSpot, 2.0f);

			// Wait a bit
			_exitTime = static_cast<int>(
				gameLocal.time + INVESTIGATE_SPOT_TIME_REMOTE*(1 + gameLocal.random.RandomFloat()) // grayman #2640
			);
		}
	}

	return false; // not finished yet
}

void InvestigateSpotTask::SetNewGoal(const idVec3& newPos)
{
	idAI* owner = _owner.GetEntity();
	if (owner == NULL)
	{
		return;
	}

	// Copy the value
	_searchSpot = newPos;
	// Reset the "move started" flag
	_moveInitiated = false;

	// Set the exit time back to negative default, so that the AI starts walking again
	_exitTime = -1;

	// Check if we can see the point from where we are (only for remote inspection)
	if (!_investigateClosely &&
		((_searchSpot - owner->GetPhysics()->GetOrigin()).LengthFast() < INVESTIGATE_SPOT_STOP_DIST) && // grayman #2640 - reduces AI search freezing
		owner->CanSeePositionExt(_searchSpot, false, true))
	{
		DM_LOG(LC_AI, LT_INFO)LOGVECTOR("I can see the point...\r", _searchSpot);

		if (!owner->CheckFOV(_searchSpot))
		{
			// Search spot is not within FOV, turn towards the position
			owner->TurnToward(_searchSpot);
		}

		// In any case, look at the point to investigate
		owner->Event_LookAtPosition(_searchSpot, 2.0f);

		// Wait a bit
		_exitTime = static_cast<int>(
			gameLocal.time + INVESTIGATE_SPOT_TIME_REMOTE*(1 + gameLocal.random.RandomFloat()) // grayman #2640
		);
	}
}

void InvestigateSpotTask::SetInvestigateClosely(bool closely)
{
	_investigateClosely = closely;
}

void InvestigateSpotTask::OnFinish(idAI* owner) // grayman #2560
{
	// The action subsystem has finished investigating the spot, set the
	// boolean back to false, so that the next spot can be chosen
	owner->GetMemory().hidingSpotInvestigationInProgress = false;
}

void InvestigateSpotTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	savefile->WriteInt(_exitTime);
	savefile->WriteBool(_investigateClosely);
	savefile->WriteVec3(_searchSpot);
}

void InvestigateSpotTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	savefile->ReadInt(_exitTime);
	savefile->ReadBool(_investigateClosely);
	savefile->ReadVec3(_searchSpot);
}

InvestigateSpotTaskPtr InvestigateSpotTask::CreateInstance()
{
	return InvestigateSpotTaskPtr(new InvestigateSpotTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar investigateSpotTaskRegistrar(
	TASK_INVESTIGATE_SPOT, // Task Name
	TaskLibrary::CreateInstanceFunc(&InvestigateSpotTask::CreateInstance) // Instance creation callback
);

} // namespace ai
