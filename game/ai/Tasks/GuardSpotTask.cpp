/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod (http://www.thedarkmod.com/)
 
 $Revision: 6105 $ (Revision of last commit) 
 $Date: 2014-09-16 10:01:26 -0400 (Tue, 16 Sep 2014) $ (Date of last commit)
 $Author: grayman $ (Author of last commit)
 
******************************************************************************/

#include "precompiled_game.h"
#pragma hdrstop

static bool versioned = RegisterVersionedFile("$Id: GuardSpotTask.cpp 6105 2014-09-16 14:01:26Z grayman $");

#include "GuardSpotTask.h"
#include "WaitTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

const int GUARD_SPOT_TIME_REMOTE = 2000; // ms (grayman #2640 - change from 600 -> 2000)
const int GUARD_SPOT_TIME_STANDARD = 300; // ms
const int GUARD_SPOT_TIME_CLOSELY = 2500; // ms

const int GUARD_SPOT_STOP_DIST = 100; // grayman #2640 - even if you can see the spot, keep moving if farther away than this
const int GUARD_SPOT_MIN_DIST  =  20;
const int GUARD_SPOT_CLOSELY_MAX_DIST = 100; // grayman #2928

const float MAX_TRAVEL_DISTANCE_WALKING = 300; // units?

GuardSpotTask::GuardSpotTask() :
	_moveInitiated(false)
{}

// Get the name of this task
const idStr& GuardSpotTask::GetName() const
{
	static idStr _name(TASK_GUARD_SPOT);
	return _name;
}

void GuardSpotTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);

	// Get a shortcut reference
	Memory& memory = owner->GetMemory();

	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("GuardSpotTask::Init - %s being sent to guard memory.currentSearchSpot = [%s]\r",owner->GetName(),memory.currentSearchSpot.ToString()); // grayman debug
	if (memory.currentSearchSpot != idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY))
	{
		// Set the goal position
		SetNewGoal(memory.currentSearchSpot);
		memory.guardingInProgress = true;
	}
	else
	{
		// Invalid spot, terminate task
		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("memory.currentSearchSpot not set to something valid, terminating task.\r");
		subsystem.FinishTask();
	}
}

bool GuardSpotTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("GuardSpotTask performing.\r");

	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	// if we've entered combat mode, we want to
	// end this task.

	if ( owner->AI_AlertIndex == ECombat )
	{
		return true;
	}

	// If Searching is over, end this task.

	if ( owner->AI_AlertIndex < ESearching)
	{
		return true;
	}
	
	if (_exitTime > 0)
	{
		if (gameLocal.time > _exitTime) // grayman debug
		{
			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("GuardSpotTask::Perform - %s _exitTime is up, quitting\r",owner->GetName()); // grayman debug
		}
		// Return TRUE if the time is over, else FALSE (continue)
		return (gameLocal.time > _exitTime);
	}

	// No exit time set, or it hasn't expired, so continue with ordinary process

	if (owner->m_HandlingDoor || owner->m_HandlingElevator)
	{
		// Wait, we're busy with a door or elevator
		return false;
	}

	// grayman #3510
	if (owner->m_RelightingLight)
	{
		// Wait, we're busy relighting a light so we have more light to search by
		return false;
	}
	
	idVec3 ownerOrigin = owner->GetPhysics()->GetOrigin(); // grayman #3492

	if (!_moveInitiated)
	{
		idVec3 destPos = _guardSpot;

		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("GuardSpotTask::Perform - %s move not started yet to [%s]\r",owner->GetName(),destPos.ToString()); // grayman debug

		// Let's move

		// If the AI is searching and not handling a door or handling
		// an elevator or resolving a block: If the spot PointReachableAreaNum()/PushPointIntoAreaNum()
		// wants to move us to is outside the vertical boundaries of the
		// search volume, consider the point bad.
		
		bool pointValid = true;
		idVec3 goal = destPos;
		int toAreaNum = owner->PointReachableAreaNum( goal );
		if ( toAreaNum == 0 )
		{
			pointValid = false;
			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("InvestigateSpotTask::Perform - %s pointValid = false because toAreaNum = 0\r",owner->GetName()); // grayman debug
		}
		else
		{
			owner->GetAAS()->PushPointIntoAreaNum( toAreaNum, goal ); // if this point is outside this area, it will be moved to one of the area's edges
		}

		if ( pointValid )
		{
			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("GuardSpotTask::Perform - %s move to [%s]\r",owner->GetName(),goal.ToString()); // grayman debug
			pointValid = owner->MoveToPosition(goal);
			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("GuardSpotTask::Perform - %s pointValid = %s after MoveToPosition()\r",owner->GetName(),pointValid ? "true":"false"); // grayman debug
			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("GuardSpotTask::Perform - %s moveStatus = %d after MoveToPosition()\r",owner->GetName(),(int)owner->GetMoveStatus()); // grayman debug
		}

		if ( !pointValid || ( owner->GetMoveStatus() == MOVE_STATUS_DEST_UNREACHABLE) )
		{
			// Guard spot not reachable, terminate task in the next round
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

			//gameRenderWorld->DebugArrow(colorYellow, owner->GetEyePosition(), _searchSpot, 1, MS2SEC(_exitTime - gameLocal.time + 100));
			_moveInitiated = true;
			float actualDist = (ownerOrigin - _guardSpot).LengthFast();
			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("GuardSpotTask::Perform - %s actualDist = %f\r",owner->GetName(),actualDist); // grayman debug
			owner->AI_RUN = ( actualDist > MAX_TRAVEL_DISTANCE_WALKING ); // close enough to walk?
		}

		return false;
	}

	// Moving. Have we arrived?

	if (owner->GetMoveStatus() == MOVE_STATUS_DEST_UNREACHABLE)
	{
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("GuardSpotTask::Perform - %s guard spot unreachable\r",owner->GetName()); // grayman debug
		DM_LOG(LC_AI, LT_INFO)LOGVECTOR("Guard spot unreachable.\r", _guardSpot);
		return true;
	}

	if (owner->GetMoveStatus() == MOVE_STATUS_DONE)
	{
		// We've successfully reached the guard spot
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("GuardSpotTask::Perform - %s reached guard spot\r",owner->GetName()); // grayman debug
		DM_LOG(LC_AI, LT_INFO)LOGVECTOR("Guard spot investigated: \r", _guardSpot);

		// Turn toward the origin of the search

		Search* search = gameLocal.m_searchManager->GetSearch(owner->m_searchID);

		if (search)
		{
			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("GuardSpotTask::Perform - %s turning toward search origin [%s]\r",owner->GetName(),search->_origin.ToString()); // grayman debug
			owner->TurnToward(search->_origin);
		}

		// Wait until we exit SearchingState
	}

	return false; // not finished yet
}

void GuardSpotTask::SetNewGoal(const idVec3& newPos)
{
	idAI* owner = _owner.GetEntity();
	if (owner == NULL)
	{
		return;
	}

	// Copy the value
	_guardSpot = newPos;
	// Reset the "move started" flag
	_moveInitiated = false;

	// Set the exit time back to negative default, so that the AI starts walking again
	_exitTime = -1;
}

void GuardSpotTask::OnFinish(idAI* owner) // grayman #2560
{
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("GuardSpotTask::OnFinish - %s guardingInProgress set to false\r",owner->GetName()); // grayman debug
	// The action subsystem has finished guarding the spot, so set the
	// boolean back to false
	owner->GetMemory().guardingInProgress = false;
}

void GuardSpotTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	savefile->WriteInt(_exitTime);
	savefile->WriteVec3(_guardSpot);
}

void GuardSpotTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	savefile->ReadInt(_exitTime);
	savefile->ReadVec3(_guardSpot);
}

GuardSpotTaskPtr GuardSpotTask::CreateInstance()
{
	return GuardSpotTaskPtr(new GuardSpotTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar guardSpotTaskRegistrar(
	TASK_GUARD_SPOT, // Task Name
	TaskLibrary::CreateInstanceFunc(&GuardSpotTask::CreateInstance) // Instance creation callback
);

} // namespace ai
