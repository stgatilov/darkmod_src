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

static bool init_version = FileVersionList("$Id: RandomTurningTask.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "../Memory.h"
#include "PatrolTask.h"
#include "RandomTurningTask.h"
#include "../Library.h"

namespace ai
{

// Get the name of this task
const idStr& RandomTurningTask::GetName() const
{
	static idStr _name(TASK_RANDOM_TURNING);
	return _name;
}

void RandomTurningTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);

	_nextYaw = owner->GetCurrentYaw() + (gameLocal.random.RandomFloat() - 0.5) * 360;
	owner->TurnToward(_nextYaw);
	_turning = true;
	_nextTurningTime = gameLocal.time;

	

}

bool RandomTurningTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("Random Turning Task performing.\r");

	idAI* owner = _owner.GetEntity();

	// This task may not be performed with empty entity pointers
	assert(owner != NULL);

	if (_turning && owner->FacingIdeal())
	{		
		owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Idle", 0);
		owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", 0);

		// TODO: un-hardcode
		int turnDelay = 1000 + gameLocal.random.RandomFloat() * 400;
		
		// Wait a bit before turning again
		_nextTurningTime = gameLocal.time + turnDelay;
		DM_LOG(LC_AI, LT_INFO).LogString("Turn is done.\r");
		_turning = false;
		_nextYaw = owner->GetCurrentYaw() + (gameLocal.random.RandomFloat() - 0.5) * 180;
		
	}

	if (!_turning && gameLocal.time >= _nextTurningTime)
	{
		
		owner->TurnToward(_nextYaw);
		if (_nextYaw < 0)
		{
			owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Turn_Left", 0);
			owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Turn_Left", 0);

		}
		else
		{
			owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Turn_Right", 0);
			owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Turn_Right", 0);

		}
		_turning = true;
	}

	return false;
}


// Save/Restore methods
void RandomTurningTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	savefile->WriteFloat(_nextYaw);
	savefile->WriteBool(_turning);
	savefile->WriteInt(_nextTurningTime);
}

void RandomTurningTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	savefile->ReadFloat(_nextYaw);
	savefile->ReadBool(_turning);
	savefile->ReadInt(_nextTurningTime);
}

RandomTurningTaskPtr RandomTurningTask::CreateInstance()
{
	return RandomTurningTaskPtr(new RandomTurningTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar randomTurningTaskRegistrar(
	TASK_RANDOM_TURNING, // Task Name
	TaskLibrary::CreateInstanceFunc(&RandomTurningTask::CreateInstance) // Instance creation callback
);

} // namespace ai
