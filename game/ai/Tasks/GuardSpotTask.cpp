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

const float MAX_TRAVEL_DISTANCE_WALKING = 300; // units?
const float MAX_YAW = 45; // max yaw (+/-) from original yaw for idle turning
const int TURN_DELAY = 8000; // will make the guard turn every 8-12 seconds
const int TURN_DELAY_DELTA = 4000;
const int MILLING_DELAY = 3500; // will generate milling times between 3.5 and 7 seconds

GuardSpotTask::GuardSpotTask() :
	_nextTurnTime(0)
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
	if (memory.currentSearchSpot == idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY))
	{
		// Invalid spot, terminate task
		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("GuardSpotTask::Init - %s memory.currentSearchSpot not set to something valid, terminating task.\r",owner->GetName()); // grayman debug
		subsystem.FinishTask();
		return;
	}

	// Set the goal position
	SetNewGoal(memory.currentSearchSpot);
	_guardSpotState = EStateSetup;

	// Milling?
	if (owner->GetMemory().millingInProgress)
	{
		// Is there any activity after milling is over?
		// If so, we want a short _exitTime so we can make the run
		// before we drop out of searching mode. If no, we can continue
		// milling until we drop out.

		Search* search = gameLocal.m_searchManager->GetSearch(owner->m_searchID);
		Assignment* assignment = gameLocal.m_searchManager->GetAssignment(search,owner);

		if (assignment)
		{
			_millingOnly = true;
			if (assignment->_searcherRole == E_ROLE_SEARCHER)
			{
				if (search->_assignmentFlags & SEARCH_SEARCH)
				{
					_millingOnly = false;
				}
			}
			else if (assignment->_searcherRole == E_ROLE_GUARD)
			{
				if (search->_assignmentFlags & SEARCH_GUARD)
				{
					_millingOnly = false;
				}
			}
			else // observer
			{
				if (search->_assignmentFlags & SEARCH_OBSERVE)
				{
					_millingOnly = false;
				}
			}
		}
	}
}

bool GuardSpotTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("GuardSpotTask performing.\r");

	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	// quit if incapable of continuing
	if (owner->AI_DEAD || owner->AI_KNOCKEDOUT)
	{
		return true;
	}

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
		if (gameLocal.time >= _exitTime) // grayman debug
		{
			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("GuardSpotTask::Perform - %s _exitTime is up, quitting\r",owner->GetName()); // grayman debug
		}
		// Return TRUE if the time is over, else FALSE (continue)
		return (gameLocal.time >= _exitTime);
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
	
	switch (_guardSpotState)
	{
	case EStateSetup:
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
				// Guard spot not reachable, terminate task
				return true;
			}

			// Run if the point is more than MAX_TRAVEL_DISTANCE_WALKING

			float actualDist = (owner->GetPhysics()->GetOrigin() - _guardSpot).LengthFast();
			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("GuardSpotTask::Perform - %s actualDist = %f\r",owner->GetName(),actualDist); // grayman debug
			bool shouldRun = actualDist > MAX_TRAVEL_DISTANCE_WALKING;
			owner->AI_RUN = false;
			if (shouldRun)
			{
				owner->AI_RUN = true;
			}
			else if (owner->m_searchID >= 0)
			{
				// When searching, and assigned guard or observer roles, AI should run.
				Search* search = gameLocal.m_searchManager->GetSearch(owner->m_searchID);
				Assignment* assignment = gameLocal.m_searchManager->GetAssignment(search,owner);
				if (search && assignment)
				{
					if (((assignment->_searcherRole == E_ROLE_GUARD) && (search->_assignmentFlags & SEARCH_GUARD)) ||
						((assignment->_searcherRole == E_ROLE_OBSERVER) && (search->_assignmentFlags & SEARCH_OBSERVE)))
					{
						owner->AI_RUN = true;
					}
				}
			}

			_guardSpotState = EStateMoving;
			break;
		}
	case EStateMoving:
		{
			// Moving. Have we arrived?

			if (owner->GetMoveStatus() == MOVE_STATUS_DEST_UNREACHABLE)
			{
				DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("GuardSpotTask::Perform - %s guard spot unreachable\r",owner->GetName()); // grayman debug
				DM_LOG(LC_AI, LT_INFO)LOGVECTOR("Guard spot unreachable.\r", _guardSpot);
				return true;
			}	

			if (owner->GetMoveStatus() == MOVE_STATUS_DONE)
			{
				if (owner->ReachedPos(_guardSpot, MOVE_TO_POSITION))
				{
					// We've successfully reached the spot

					DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("GuardSpotTask::Perform - %s reached spot\r",owner->GetName()); // grayman debug

					// If a facing angle is specified, turn to that angle.
					// If no facing angle is specified, turn toward the origin of the search

					Search* search = gameLocal.m_searchManager->GetSearch(owner->m_searchID);

					if (search)
					{
						if ( owner->GetMemory().guardingAngle == idMath::INFINITY)
						{
							DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("GuardSpotTask::Perform - %s turning toward search origin [%s]\r",owner->GetName(),search->_origin.ToString()); // grayman debug
							owner->TurnToward(search->_origin);
						}
						else
						{
							DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("GuardSpotTask::Perform - %s turning toward yaw %f\r",owner->GetName(),owner->GetMemory().guardingAngle); // grayman debug
							owner->TurnToward(owner->GetMemory().guardingAngle);
						}

						_baseYaw = owner->GetIdealYaw();
						DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("GuardSpotTask::Perform - %s _baseYaw %f\r",owner->GetName(),_baseYaw); // grayman debug

						// Milling?
						if (owner->GetMemory().millingInProgress)
						{
							if (!_millingOnly)
							{
								// leave milling early, so we can get to the following activity
								_exitTime = gameLocal.time + MILLING_DELAY + gameLocal.random.RandomInt(MILLING_DELAY);
								_nextTurnTime = gameLocal.time + (TURN_DELAY + gameLocal.random.RandomInt(TURN_DELAY_DELTA))/6;
							}
							else
							{
								// we can hang around until we drop out of searching mode
								_nextTurnTime = gameLocal.time + TURN_DELAY + gameLocal.random.RandomInt(TURN_DELAY_DELTA);
							}
						}
						else // guarding or observing
						{
							_nextTurnTime = gameLocal.time + TURN_DELAY + gameLocal.random.RandomInt(TURN_DELAY_DELTA);
						}
					}
					_guardSpotState = EStateStanding;
				}
				else
				{
					_guardSpotState = EStateSetup; // try again
				}
			}
			break;
		}
	case EStateStanding:
		{
			if ( (_nextTurnTime > 0) && (gameLocal.time >= _nextTurnTime) )
			{
				// turn randomly in place
				float newYaw = _baseYaw + 2.0f*MAX_YAW*(gameLocal.random.RandomFloat() - 0.5f);
				DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("GuardSpotTask::Perform - %s execute random yaw turn of %d degrees\r",owner->GetName(),newYaw - _baseYaw); // grayman debug
				owner->TurnToward(newYaw);

				// Milling?
				if (owner->GetMemory().millingInProgress)
				{
					if (!_millingOnly)
					{
						_nextTurnTime = gameLocal.time + (TURN_DELAY + gameLocal.random.RandomInt(TURN_DELAY_DELTA))/6;
					}
					else
					{
						_nextTurnTime = gameLocal.time + TURN_DELAY + gameLocal.random.RandomInt(TURN_DELAY_DELTA);
					}
				}
				else
				{
					_nextTurnTime = gameLocal.time + TURN_DELAY + gameLocal.random.RandomInt(TURN_DELAY_DELTA);
				}
			}
			break;
		}
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

	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("GuardSpotTask::SetNewGoal - %s newPos = [%s]\r",owner->GetName(),newPos.ToString()); // grayman debug
	// If newPos is in a portal, there might be a door there. We only care
	// about finding doors if owner is a guard.

	CFrobDoor *door = NULL;

	Search* search = gameLocal.m_searchManager->GetSearch(owner->m_searchID);
	Assignment* assignment = gameLocal.m_searchManager->GetAssignment(search,owner);

	if ( assignment && (assignment->_searcherRole == E_ROLE_GUARD) )
	{
		// Determine if this spot is in or near a door
		idBounds clipBounds = idBounds(newPos);
		clipBounds.ExpandSelf(32.0f);

		// newPos might be sitting on the floor. If it is, we don't
		// want to expand downward and pick up a door on the floor below.
		// Set the .z values accordingly.

		clipBounds[0].z = newPos.z;
		clipBounds[1].z += -32.0f + 8;
		int clipmask = owner->GetPhysics()->GetClipMask();
		idClipModel *clipModel;
		idClipModel *clipModelList[MAX_GENTITIES];
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("GuardSpotTask::SetNewGoal - %s clipBounds = [%s]\r",owner->GetName(),clipBounds.ToString()); // grayman debug
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("GuardSpotTask::SetNewGoal - %s clipmask = %x\r",owner->GetName(),clipmask); // grayman debug
		int numListedClipModels = gameLocal.clip.ClipModelsTouchingBounds( clipBounds, clipmask, clipModelList, MAX_GENTITIES );
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("GuardSpotTask::SetNewGoal - %s numListedClipModels = %d\r",owner->GetName(),numListedClipModels); // grayman debug
		for ( int i = 0 ; i < numListedClipModels ; i++ )
		{
			clipModel = clipModelList[i];
			idEntity* ent = clipModel->GetEntity();

			if (ent == NULL)
			{
				DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("GuardSpotTask::SetNewGoal - %s ent == NULL\r",owner->GetName()); // grayman debug
				continue;
			}

			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("GuardSpotTask::SetNewGoal - %s obEnt = '%s'\r",owner->GetName(),ent->GetName()); // grayman debug
			if (ent->IsType(CFrobDoor::Type))
			{
				door = static_cast<CFrobDoor*>(ent);
				break;
			}
		}
	}

	if (door)
	{
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("GuardSpotTask::SetNewGoal - %s found door '%s'\r",owner->GetName(),door->GetName()); // grayman debug

		idVec3 frontPos = door->GetDoorPosition(owner->GetDoorSide(door),DOOR_POS_FRONT);

		// Can't stand at the front position, because you'll be in the way
		// of anyone wanting to use the door from this side. Move toward the
		// search origin.

		idVec3 dir = gameLocal.m_searchManager->GetSearch(owner->m_searchID)->_origin - frontPos;
		dir.Normalize();
		frontPos += 50*dir;

		_guardSpot = frontPos;
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("GuardSpotTask::SetNewGoal - %s _guardSpot = [%s]\r",owner->GetName(),_guardSpot.ToString()); // grayman debug
	}
	else
	{
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("GuardSpotTask::SetNewGoal - %s no door\r",owner->GetName()); // grayman debug
		_guardSpot = newPos;
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("GuardSpotTask::SetNewGoal - %s _guardSpot = [%s]\r",owner->GetName(),_guardSpot.ToString()); // grayman debug
	}

	_guardSpotState = EStateSetup;

	// Set the exit time back to negative default, so that the AI starts walking again
	_exitTime = -1;
}

void GuardSpotTask::OnFinish(idAI* owner) // grayman #2560
{
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("GuardSpotTask::OnFinish - %s guardingInProgress set to false\r",owner->GetName()); // grayman debug
	// The action subsystem has finished guarding the spot, so set the
	// boolean back to false
	owner->GetMemory().guardingInProgress = false;
	owner->GetMemory().millingInProgress = false;
}

void GuardSpotTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	savefile->WriteInt(_exitTime);
	savefile->WriteInt(static_cast<int>(_guardSpotState));
	savefile->WriteVec3(_guardSpot);
	savefile->WriteInt(_nextTurnTime);
	savefile->WriteFloat(_baseYaw);
	savefile->WriteBool(_millingOnly);
}

void GuardSpotTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	savefile->ReadInt(_exitTime);

	int temp;
	savefile->ReadInt(temp);
	_guardSpotState = static_cast<EGuardSpotState>(temp);

	savefile->ReadVec3(_guardSpot);
	savefile->ReadInt(_nextTurnTime);
	savefile->ReadFloat(_baseYaw);
	savefile->ReadBool(_millingOnly);
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
