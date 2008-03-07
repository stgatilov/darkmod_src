/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: angua $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: HandleDoorTask.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "../Memory.h"
#include "HandleDoorTask.h"

namespace ai
{

// Get the name of this task
const idStr& HandleDoorTask::GetName() const
{
	static idStr _name(TASK_HANDLE_DOOR);
	return _name;
}

void HandleDoorTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Init the base class
	Task::Init(owner, subsystem);

	Memory& memory = owner->GetMemory();

	// Let the owner save its move
	owner->Event_SaveMove();

	CBinaryFrobMover* frobMover = memory.doorRelated.frobMover.GetEntity();
	if (frobMover == NULL)
	{
		return;
	}

	_wasLocked = false;

	if (frobMover->IsLocked())
	{
		_wasLocked = true;
		if (!owner->CanUnlock(frobMover))
		{
			owner->StopMove(MOVE_STATUS_DEST_UNREACHABLE);
			owner->AI_DEST_UNREACHABLE = true;
			subsystem.FinishTask();
			return;
		}
	}

	const idVec3& frobMoverOrg = frobMover->GetPhysics()->GetOrigin();

	idBounds bounds = owner->GetPhysics()->GetBounds();
	float size = bounds[1][0];

	const idVec3& openDir = frobMover->GetOpenDir();
	const idVec3& openPos = frobMover->GetOpenPos();
	const idVec3& closedPos = frobMover->GetClosedPos();

	idVec3 dir = closedPos - frobMoverOrg;
	dir.z = 0;
	idVec3 dirNorm = dir;
	dirNorm.NormalizeFast();
	float dist = dir.LengthFast();

	idVec3 openDirNorm = openDir;
	openDirNorm.z = 0;
	openDirNorm.NormalizeFast();

	//calculate where to stand when the door swings away from us
	idVec3 parallelAwayOffset = dirNorm;
	parallelAwayOffset *= size * 1.4f;

	idVec3 normalAwayOffset = openDirNorm;
	normalAwayOffset *= size * 2.5;

	idVec3 awayPos = closedPos - parallelAwayOffset - normalAwayOffset;
	awayPos.z = frobMoverOrg.z;

	// calculate where to stand when the door swings towards us
	// next to the door
	idTraceModel trm(bounds);
	idClipModel clip(trm);

	idVec3 parallelTowardOffset = dirNorm;
	parallelTowardOffset *= dist + size * 2;

	idVec3 normalTowardOffset = openDirNorm;
	normalTowardOffset *= size * 2;

	idVec3 towardPos = frobMoverOrg + parallelTowardOffset + normalTowardOffset;

	// check if we can stand at this position
	int contents = gameLocal.clip.Contents(towardPos, &clip, mat3_identity, CONTENTS_SOLID, owner);

	if (contents)
	{
		if (cv_ai_door_show.GetBool())
		{
			gameRenderWorld->DebugBounds(colorRed, bounds, towardPos, 10000);
		}

		// at 45° swinging angle
		parallelTowardOffset = dirNorm;

		normalTowardOffset = openDirNorm;

		towardPos = parallelTowardOffset + normalTowardOffset;
		towardPos.NormalizeFast();
		towardPos *= (dist + size * 2);
		towardPos += frobMoverOrg;

		int contents = gameLocal.clip.Contents(towardPos, &clip, mat3_identity, CONTENTS_SOLID, owner);

		if (contents)
		{
			if (cv_ai_door_show.GetBool())
			{
				gameRenderWorld->DebugBounds(colorRed, bounds, towardPos, 10000);
			}

			// far away from the door
			parallelTowardOffset = dirNorm * size * 1.2f;

			normalTowardOffset = openDirNorm;
			normalTowardOffset *= dist + size;

			towardPos = frobMoverOrg + parallelTowardOffset + normalTowardOffset;

			int contents = gameLocal.clip.Contents(towardPos, &clip, mat3_identity, CONTENTS_SOLID, owner);

			if (contents)
			{
				// TODO: no suitable position found
				if (cv_ai_door_show.GetBool())
				{
					gameRenderWorld->DebugBounds(colorGreen, bounds, towardPos, 10000);
				}
			}
			else if (cv_ai_door_show.GetBool())
			{
				gameRenderWorld->DebugBounds(colorGreen, bounds, towardPos, 10000);
			}
		}
		else if (cv_ai_door_show.GetBool())
		{
			gameRenderWorld->DebugBounds(colorGreen, bounds, towardPos, 10000);
		}
	}
	else if (cv_ai_door_show.GetBool())
	{
		 gameRenderWorld->DebugBounds(colorGreen, bounds, towardPos, 10000);
	}

	// check if the door swings towards or away from us
	if (openDir * (owner->GetPhysics()->GetOrigin() - frobMoverOrg) > 0)
	{
		// Door opens towards us
		_frontPos = towardPos;
		_backPos = awayPos;
	}
	else
	{
		// Door opens away from us
		_frontPos = awayPos;
		_backPos = towardPos;
	}

	_doorHandlingState = EStateNone;
}

bool HandleDoorTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("HandleDoorTask performing.\r");

	idAI* owner = _owner.GetEntity();
	Memory& memory = owner->GetMemory();

	CBinaryFrobMover* frobMover = memory.doorRelated.frobMover.GetEntity();
	if (frobMover == NULL)
	{
		return true;
	}

	if (frobMover->IsLocked())
	{
		if (!owner->CanUnlock(frobMover))
		{
			owner->StopMove(MOVE_STATUS_DEST_UNREACHABLE);
			owner->AI_DEST_UNREACHABLE = true;
			return true;
		}
	}

	const idVec3& openPos = frobMover->GetOpenPos();
	const idVec3& closedPos = frobMover->GetClosedPos();

	if (cv_ai_door_show.GetBool()) 
	{
		gameRenderWorld->DebugArrow(colorYellow, _frontPos, _frontPos + idVec3(0, 0, 20), 2, 1000);
		gameRenderWorld->DebugArrow(colorGreen, _backPos, _backPos + idVec3(0, 0, 20), 2, 1000);
	}

	// Door is closed or was interrupted
	if (!frobMover->IsOpen() || frobMover->WasInterrupted())
	{
		switch (_doorHandlingState)
		{
			case EStateNone:
				if (!owner->MoveToPosition(_frontPos))
				{
					// TODO: position not reachable, need a better one
				}
				_doorHandlingState = EStateMovingToFrontPos;
				break;

			case EStateMovingToFrontPos:
				if (owner->AI_MOVE_DONE)
				{
					// reached position
					owner->StopMove(MOVE_STATUS_DONE);
					owner->TurnToward(closedPos);
					_waitEndTime = gameLocal.time + 1000;
					_doorHandlingState = EStateWaitBeforeOpen;
				}
				break;

			case EStateWaitBeforeOpen:
				if (gameLocal.time >= _waitEndTime)
				{
					if (frobMover->IsLocked())
					{
						frobMover->Unlock(false);
					}
					frobMover->Open(false);
					// TODO: play anim
					_doorHandlingState = EStateOpeningDoor;
				}
				break;

			case EStateOpeningDoor:
				// we have already started opening the door, but it is closed, try again
				owner->StopMove(MOVE_STATUS_DONE);
				owner->TurnToward(closedPos);
				frobMover->Open(false);
				// TODO: play anim
				break;

			case EStateMovingToBackPos:
				// door has closed while we were attempting to walk through it.
				// end this task (it will be initiated again if we are still in front of the door).
				return true;
				break;
				
			case EStateWaitBeforeClose:
				// door has already closed before we were attempting to do it
				// no need for more waiting
				return true;
				break;

			case EStateClosingDoor:
				// we have moved through the door and closed it
				if (_wasLocked)
				{
					// if the door was locked before, lock it again
					frobMover->Lock(false);
				}
				// continue what we were doing before.
				return true;
				break;

			default:
				break;
		}
	}
	else
	{
		// Door is open
		if (!frobMover->IsChangingState())
		{	
			if (_doorHandlingState == EStateMovingToBackPos && owner->AI_MOVE_DONE)
			{
				// close the door
				if (owner->ShouldCloseDoor(frobMover))
				{
					owner->StopMove(MOVE_STATUS_DONE);
					owner->TurnToward(openPos);
					_waitEndTime = gameLocal.time + 1000;
					_doorHandlingState = EStateWaitBeforeClose;
				}
				else
				{
					return true;
				}
			}

			else if (_doorHandlingState == EStateWaitBeforeClose)
			{
				if (gameLocal.time >= _waitEndTime)
				{
					frobMover->Close(false);
					// TODO: play anim
					_doorHandlingState = EStateClosingDoor;
				}
			}
			else
			{
				owner->MoveToPosition(_backPos);
				_doorHandlingState = EStateMovingToBackPos;
			}
		}
		else if (frobMover->IsBlocked())
		{
			if (frobMover->GetPhysics()->GetBlockingEntity() == owner)
			{
				// we are blocking the door
				owner->StopMove(MOVE_STATUS_DONE);
				owner->TurnToward(closedPos);
				frobMover->Open(false);
				_doorHandlingState = EStateOpeningDoor;
				// TODO: play anim
			}
			else
			{
				// something else is blocking the door
				// possibly the player, another AI or an object
			}
		}

	}
	return false; // not finished yet
}

void HandleDoorTask::OnFinish(idAI* owner)
{
	Memory& memory = owner->GetMemory();

	owner->Event_RestoreMove();
	memory.doorRelated.frobMover = NULL;
	_doorHandlingState = EStateNone;
}


void HandleDoorTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	savefile->WriteVec3(_frontPos);
	savefile->WriteVec3(_backPos);
	savefile->WriteInt(static_cast<int>(_doorHandlingState));
	savefile->WriteInt(_waitEndTime);
	savefile->WriteBool(_wasLocked);
}

void HandleDoorTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	savefile->ReadVec3(_frontPos);
	savefile->ReadVec3(_backPos);
	int temp;
	savefile->ReadInt(temp);
	_doorHandlingState = static_cast<EDoorHandlingState>(temp);
	savefile->ReadInt(_waitEndTime);
	savefile->ReadBool(_wasLocked);
}

HandleDoorTaskPtr HandleDoorTask::CreateInstance()
{
	return HandleDoorTaskPtr(new HandleDoorTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar handleDoorTaskRegistrar(
	TASK_HANDLE_DOOR, // Task Name
	TaskLibrary::CreateInstanceFunc(&HandleDoorTask::CreateInstance) // Instance creation callback
);

} // namespace ai
