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

static bool init_version = FileVersionList("$Id: IdleSensoryTask.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "IdleSensoryTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

// Get the name of this task
const idStr& IdleSensoryTask::GetName() const
{
	static idStr _name(TASK_IDLE_SENSORY);
	return _name;
}

void IdleSensoryTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);
}

void IdleSensoryTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("IdleSensory Task performing.\r");

	idAI* owner = _owner.GetEntity();

	// This task may not be performed with empty entity pointers
	assert(owner != NULL);

	
}

// Save/Restore methods
void IdleSensoryTask::Save(idSaveGame* savefile) const
{
	// TODO
}

void IdleSensoryTask::Restore(idRestoreGame* savefile)
{
	// TODO
}

IdleSensoryTaskPtr IdleSensoryTask::CreateInstance()
{
	return IdleSensoryTaskPtr(new IdleSensoryTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar idleSensoryTaskRegistrar(
	TASK_IDLE_SENSORY, // Task Name
	TaskLibrary::CreateInstanceFunc(&IdleSensoryTask::CreateInstance) // Instance creation callback
);

} // namespace ai
