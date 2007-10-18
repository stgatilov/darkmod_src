/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: SingleBarkTask.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "SingleBarkTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

// Get the name of this task
const idStr& SingleBarkTask::GetName() const
{
	static idStr _name(TASK_SINGLE_BARK);
	return _name;
}

void SingleBarkTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);

	// If the last chat time is not yet set, initialise it to play the sound now
	/*if (owner->GetMind()->GetMemory().lastPatrolChatTime == -1)
	{
		owner->GetMind()->GetMemory().lastPatrolChatTime = gameLocal.time - _barkRepeatInterval;
	}*/
}

bool SingleBarkTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("Idle Bark Task performing.\r");

	idAI* owner = _owner.GetEntity();

	// This task may not be performed with empty entity pointers
	assert(owner != NULL);

	owner->PlayAndLipSync(_soundName.c_str(), "talk1");

	// Disable the patrol barking for now.
	owner->GetMind()->GetMemory().lastPatrolChatTime = gameLocal.time;

	/*if (gameLocal.time - owner->GetMind()->GetMemory().lastPatrolChatTime > _barkRepeatInterval)
	{
		// The time has come, bark now
		owner->PlayAndLipSync("snd_patrol_idle", "talk1");

		// Reset the timer
		owner->GetMind()->GetMemory().lastPatrolChatTime = gameLocal.time;
	}*/

	return true; // finished!
}

void SingleBarkTask::SetSound(const idStr& soundName)
{
	_soundName = soundName;
}

// Save/Restore methods
void SingleBarkTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);
	savefile->WriteString(_soundName);
}

void SingleBarkTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);
	savefile->ReadString(_soundName);
}

SingleBarkTaskPtr SingleBarkTask::CreateInstance()
{
	return SingleBarkTaskPtr(new SingleBarkTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar singleBarkTaskRegistrar(
	TASK_SINGLE_BARK, // Task Name
	TaskLibrary::CreateInstanceFunc(&SingleBarkTask::CreateInstance) // Instance creation callback
);

} // namespace ai
