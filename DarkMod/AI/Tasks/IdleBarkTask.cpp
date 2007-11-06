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

static bool init_version = FileVersionList("$Id: IdleBarkTask.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "IdleBarkTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

IdleBarkTask::IdleBarkTask() :
	_soundName("")
{}

IdleBarkTask::IdleBarkTask(const idStr& soundName) :
	_soundName(soundName)
{}

// Get the name of this task
const idStr& IdleBarkTask::GetName() const
{
	static idStr _name(TASK_IDLE_BARK);
	return _name;
}

void IdleBarkTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);

	// Get the repeat interval in seconds, convert to ms
	_barkRepeatInterval = SEC2MS(owner->spawnArgs.GetFloat("bark_repeat_patrol", "45"));

	Memory& memory = owner->GetMemory();

	// If the last chat time is not yet set, initialise it to play the sound now
	if (memory.lastPatrolChatTime == -1)
	{
		memory.lastPatrolChatTime = gameLocal.time - _barkRepeatInterval;
		// greebo: Add some random offset of up to 20 seconds before barking the first time
		// This prevents guards barking in choirs.
		memory.lastPatrolChatTime += SEC2MS(gameLocal.random.RandomFloat()*10);
	}
}

bool IdleBarkTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("Idle Bark Task performing.\r");

	idAI* owner = _owner.GetEntity();

	// This task may not be performed with empty entity pointers
	assert(owner != NULL);

	Memory& memory = owner->GetMemory();

	if (gameLocal.time - memory.lastPatrolChatTime > _barkRepeatInterval)
	{
		// The time has come, bark now
		owner->PlayAndLipSync(_soundName.c_str(), "talk1");

		// Reset the timer
		memory.lastPatrolChatTime = gameLocal.time;
	}

	return false; // not finished yet
}

// Save/Restore methods
void IdleBarkTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);
	savefile->WriteInt(_barkRepeatInterval);
	savefile->WriteString(_soundName);
}

void IdleBarkTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);
	savefile->ReadInt(_barkRepeatInterval);
	savefile->ReadString(_soundName);
}

IdleBarkTaskPtr IdleBarkTask::CreateInstance()
{
	return IdleBarkTaskPtr(new IdleBarkTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar idleBarkTaskRegistrar(
	TASK_IDLE_BARK, // Task Name
	TaskLibrary::CreateInstanceFunc(&IdleBarkTask::CreateInstance) // Instance creation callback
);

} // namespace ai
