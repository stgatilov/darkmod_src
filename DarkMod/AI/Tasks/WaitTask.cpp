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

static bool init_version = FileVersionList("$Id: WaitTask.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "../Memory.h"
#include "WaitTask.h"
#include "../Library.h"

namespace ai
{

WaitTask::WaitTask() :
	_waitTime(0)
{}

WaitTask::WaitTask(const int waitTime) : 
	_waitTime(waitTime)
{}

// Get the name of this task
const idStr& WaitTask::GetName() const
{
	static idStr _name(TASK_WAIT);
	return _name;
}

void WaitTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);

	_waitEndTime = gameLocal.time + _waitTime;
}

bool WaitTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("WaitTask performing.\r");

	idAI* owner = _owner.GetEntity();

	// This task may not be performed with empty entity pointer
	assert(owner != NULL);

	// This task does nothing but wait until the time is over.
	 if (_waitEndTime <= gameLocal.time)
	{
		return true;
	}
		
	return false; // not finished yet
}

void WaitTask::SetTime(int waitTime)
{
	_waitTime = waitTime;
}

// Save/Restore methods
void WaitTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	savefile->WriteInt(_waitTime);
	savefile->WriteInt(_waitEndTime);
}

void WaitTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	savefile->ReadInt(_waitTime);
	savefile->ReadInt(_waitEndTime);
}

WaitTaskPtr WaitTask::CreateInstance()
{
	return WaitTaskPtr(new WaitTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar waitTaskRegistrar(
	TASK_WAIT, // Task Name
	TaskLibrary::CreateInstanceFunc(&WaitTask::CreateInstance) // Instance creation callback
);

} // namespace ai
