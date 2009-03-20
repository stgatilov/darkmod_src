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

#include "../Memory.h"
#include "PatrolTask.h"
#include "PathWaitForTriggerTask.h"
#include "../Library.h"

namespace ai
{

PathWaitForTriggerTask::PathWaitForTriggerTask() :
	PathTask()
{}

PathWaitForTriggerTask::PathWaitForTriggerTask(idPathCorner* path) : 
	PathTask(path)
{
	_path = path;
}

// Get the name of this task
const idStr& PathWaitForTriggerTask::GetName() const
{
	static idStr _name(TASK_PATH_WAIT_FOR_TRIGGER);
	return _name;
}

void PathWaitForTriggerTask::Init(idAI* owner, Subsystem& subsystem)
{
	PathTask::Init(owner, subsystem);

	owner->AI_ACTIVATED = false;
}

bool PathWaitForTriggerTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("Path WaitForTrigger Task performing.\r");

	idPathCorner* path = _path.GetEntity();
	idAI* owner = _owner.GetEntity();

	// This task may not be performed with empty entity pointers
	assert(path != NULL && owner != NULL);

	if (owner->AI_ACTIVATED)
	{
		owner->AI_ACTIVATED = false;

		// Trigger path targets, now that we've reached the corner
		owner->ActivateTargets(owner);

		NextPath();

		// Move is done, fall back to PatrolTask
		DM_LOG(LC_AI, LT_INFO)LOGSTRING("Waiting for trigger is done.\r");

		return true; // finish this task
	}
	return false;
}

PathWaitForTriggerTaskPtr PathWaitForTriggerTask::CreateInstance()
{
	return PathWaitForTriggerTaskPtr(new PathWaitForTriggerTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar pathWaitForTriggerTaskRegistrar(
	TASK_PATH_WAIT_FOR_TRIGGER, // Task Name
	TaskLibrary::CreateInstanceFunc(&PathWaitForTriggerTask::CreateInstance) // Instance creation callback
);

} // namespace ai
