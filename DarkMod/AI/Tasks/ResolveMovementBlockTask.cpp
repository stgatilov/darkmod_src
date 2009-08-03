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

#include "ResolveMovementBlockTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

ResolveMovementBlockTask::ResolveMovementBlockTask()
{}

// Get the name of this task
const idStr& ResolveMovementBlockTask::GetName() const
{
	static idStr _name(TASK_RESOLVE_MOVEMENT_BLOCK);
	return _name;
}

void ResolveMovementBlockTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);
}

bool ResolveMovementBlockTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("ResolveMovementBlockTask Task performing.\r");

	idAI* owner = _owner.GetEntity();

	// This task may not be performed with empty entity pointer
	assert(owner != NULL);

	return false; // not finished yet
}

// Save/Restore methods
void ResolveMovementBlockTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);
}

void ResolveMovementBlockTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);
}

ResolveMovementBlockTaskPtr ResolveMovementBlockTask::CreateInstance()
{
	return ResolveMovementBlockTaskPtr(new ResolveMovementBlockTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar resolveMovementBlockTaskRegistrar(
	TASK_RESOLVE_MOVEMENT_BLOCK, // Task Name
	TaskLibrary::CreateInstanceFunc(&ResolveMovementBlockTask::CreateInstance) // Instance creation callback
);

} // namespace ai
