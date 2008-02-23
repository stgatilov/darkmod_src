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

#include "../../DarkMod/BinaryFrobMover.h"
#include "../../DarkMod/FrobDoor.h"


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
	assert (frobMover != NULL);

	idBounds bounds = owner->GetPhysics()->GetBounds();
	float size = bounds[1][0];

	idVec3 openDir = frobMover->GetOpenDir();
	idVec3 openPos = frobMover->GetOpenPos();
	idVec3 closedPos = frobMover->GetClosedPos();

	//calculate where to stand when the door swings away from us
	idVec3 parallelAwayOffset = (closedPos - frobMover->GetPhysics()->GetOrigin());
	parallelAwayOffset.z = 0;
	parallelAwayOffset.Normalize();
	parallelAwayOffset *= size * 1.4f;

	idVec3 normalAwayOffset = openDir * size * 2;

	idVec3 awayPos = closedPos - parallelAwayOffset - normalAwayOffset;
	awayPos.z = frobMover->GetPhysics()->GetOrigin().z;

	gameRenderWorld->DebugArrow(colorYellow, awayPos, awayPos + idVec3(0, 0, 20), 2, 100000);

	// calculate where to stand when the door swings towards us
	idVec3 parallelTowardOffset = (closedPos - frobMover->GetPhysics()->GetOrigin());
	parallelTowardOffset.z = 0;
	parallelTowardOffset.Normalize();

	idVec3 normalTowardOffset = openDir;
	normalTowardOffset.z = 0;
	normalTowardOffset.Normalize();

	idVec3 towardPos = parallelTowardOffset + normalTowardOffset;
	towardPos.Normalize();
	idVec3 dist = closedPos - frobMover->GetPhysics()->GetOrigin();
	dist.z = 0;
	towardPos *= (dist.LengthFast() + size * 2);
	towardPos += frobMover->GetPhysics()->GetOrigin();

	gameRenderWorld->DebugArrow(colorGreen, towardPos, towardPos + idVec3(0, 0, 20), 2, 100000);

	// check if the door swings towards or away from us
	if (openDir * (owner->GetPhysics()->GetOrigin() - frobMover->GetPhysics()->GetOrigin()) > 0)
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

	_movingToFrontPos = false;
	_openingDoor = false;
	_movingToBackPos = false;
	_closingDoor = false;
}

bool HandleDoorTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("HandleDoorTask performing.\r");

	idAI* owner = _owner.GetEntity();

	Memory& memory = owner->GetMemory();

	CBinaryFrobMover* frobMover = memory.doorRelated.frobMover.GetEntity();
	assert (frobMover != NULL);

	idVec3 openPos = frobMover->GetOpenPos();
	idVec3 closedPos = frobMover->GetClosedPos();

	// Door is closed
	if (!frobMover->IsOpen() || frobMover->WasInterrupted())
	{
		if (!_movingToFrontPos && !_closingDoor)
		{
			if (!owner->MoveToPosition(_frontPos))
			{
				// position not reachable, need a better one
				int bla = 1;
			}
			owner->AI_MOVE_DONE = false;
			_movingToFrontPos = true;
		}
		else if (_movingToFrontPos && owner->AI_MOVE_DONE)
		{
			// reached position
			owner->StopMove(MOVE_STATUS_WAITING);
			owner->TurnToward(closedPos);
			frobMover->Open(false);
			// TODO: play anim
			_movingToFrontPos = false;
			_openingDoor = true;
		}
		else if (_closingDoor)
		{
			// we have moved through the door and closed it, continue what we were doing before.
			owner->Event_RestoreMove();
			memory.doorRelated.frobMover = NULL;
			return true;
		}
		// moving to front position...
	}
	else
	{
		// Door is open
		if (!frobMover->IsChangingState())
		{	
			if (!_movingToBackPos)
			{
				_openingDoor = false;
				owner->MoveToPosition(_backPos);
				_movingToBackPos = true;
				owner->AI_MOVE_DONE = false;
			}
			else if (_movingToBackPos && owner->AI_MOVE_DONE)
			{
				_movingToBackPos = false;
				// close the door
				//if (owner->ShouldCloseDoor(frobMover)
				//{
				owner->StopMove(MOVE_STATUS_WAITING);
				_movingToBackPos = false;
				owner->TurnToward(openPos);

				frobMover->Close(false);
				_closingDoor = true;
				// TODO: play anim
				//}
			}
		}
	}
	return false; // not finished yet
}

void HandleDoorTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	savefile->WriteVec3(_frontPos);
	savefile->WriteVec3(_backPos);
	savefile->WriteBool(_movingToFrontPos);
	savefile->WriteBool(_openingDoor);
	savefile->WriteBool(_movingToBackPos);
	savefile->WriteBool(_closingDoor);
}

void HandleDoorTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	savefile->ReadVec3(_frontPos);
	savefile->ReadVec3(_backPos);
	savefile->ReadBool(_movingToFrontPos);
	savefile->ReadBool(_openingDoor);
	savefile->ReadBool(_movingToBackPos);
	savefile->ReadBool(_closingDoor);
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
