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

static bool init_version = FileVersionList("$Id: MoveToPositionTask.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "../Memory.h"
#include "MoveToPositionTask.h"
#include "../Library.h"

namespace ai
{

// This should be unreachable if no target position is specified.
MoveToPositionTask::MoveToPositionTask() :
	_targetPosition(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY),
	_targetYaw(idMath::INFINITY)
{}

MoveToPositionTask::MoveToPositionTask(const idVec3 targetPosition, float targetYaw) :
	_targetPosition(targetPosition),
	_targetYaw(targetYaw)
{}


// Get the name of this task
const idStr& MoveToPositionTask::GetName() const
{
	static idStr _name(TASK_MOVE_TO_POSITION);
	return _name;
}

void MoveToPositionTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);
}

bool MoveToPositionTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("run to position Task performing.\r");

	idAI* owner = _owner.GetEntity();

	// This task may not be performed with empty entity pointer
	assert(owner != NULL);

	if (!owner->MoveToPosition(_targetPosition))
	{
		// Destination unreachable, end task
		return true;
	}
		
	if (owner->AI_MOVE_DONE)
	{
		// Position reached, turn to the given yaw, if valid
		if (_targetYaw != idMath::INFINITY)
		{
			owner->TurnToward(_targetYaw);
		}
		return true;
	}
		 
	return false; // not finished yet
}

void MoveToPositionTask::SetPosition(idVec3 position)
{
	_targetPosition = position;
}

// Save/Restore methods
void MoveToPositionTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	savefile->WriteVec3(_targetPosition);
	savefile->WriteFloat(_targetYaw);
}

void MoveToPositionTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	savefile->ReadVec3(_targetPosition);
	savefile->ReadFloat(_targetYaw);
}

MoveToPositionTaskPtr MoveToPositionTask::CreateInstance()
{
	return MoveToPositionTaskPtr(new MoveToPositionTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar moveToPositionTaskRegistrar(
	TASK_MOVE_TO_POSITION, // Task Name
	TaskLibrary::CreateInstanceFunc(&MoveToPositionTask::CreateInstance) // Instance creation callback
);

} // namespace ai
