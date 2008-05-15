/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include "SingleBarkTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

SingleBarkTask::SingleBarkTask() :
	_soundName("")
{}

SingleBarkTask::SingleBarkTask(const idStr& soundName) :
	_soundName(soundName)
{}

// Get the name of this task
const idStr& SingleBarkTask::GetName() const
{
	static idStr _name(TASK_SINGLE_BARK);
	return _name;
}

void SingleBarkTask::Init(idAI* owner, Subsystem& subsystem)
{
	// This task may not be performed with empty entity pointers
	assert(owner != NULL);

	if (!_soundName.IsEmpty())
	{
		int duration = owner->PlayAndLipSync(_soundName.c_str(), "talk1");
		_endTime = gameLocal.time + duration;
	}
	else
	{
		_endTime = gameLocal.time;
	}

}

bool SingleBarkTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("SingleBarkTask performing.\r");

	idAI* owner = _owner.GetEntity();

	if (gameLocal.time >= _endTime)
	{
		return true; // finished!
	}
	return false;
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
	savefile->WriteInt(_endTime);
}

void SingleBarkTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);
	savefile->ReadString(_soundName);
	savefile->ReadInt(_endTime);
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
