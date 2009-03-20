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
#include "PathHideTask.h"
#include "../Library.h"

namespace ai
{

PathHideTask::PathHideTask() :
	PathTask()
{}

PathHideTask::PathHideTask(idPathCorner* path) :
	PathTask(path)
{
	_path = path;
}

// Get the name of this task
const idStr& PathHideTask::GetName() const
{
	static idStr _name(TASK_PATH_HIDE);
	return _name;
}

void PathHideTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Init the base class
	PathTask::Init(owner, subsystem);

	// Make invisible and nonsolid
	owner->Hide();
}

bool PathHideTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("Path Hide task performing.\r");

	idPathCorner* path = _path.GetEntity();
	idAI* owner = _owner.GetEntity();

	// This task may not be performed with empty entity pointers
	assert(path != NULL && owner != NULL);

	// Move on to next target
	if (owner->IsHidden())
	{
		// Trigger path targets, now that we've reached the corner
		owner->ActivateTargets(owner);

		NextPath();

		// Move is done, fall back to PatrolTask
		DM_LOG(LC_AI, LT_INFO)LOGSTRING("entity is hidden.\r");

		return true; // finish this task
	}
	return false;
}

PathHideTaskPtr PathHideTask::CreateInstance()
{
	return PathHideTaskPtr(new PathHideTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar pathHideTaskRegistrar(
	TASK_PATH_HIDE, // Task Name
	TaskLibrary::CreateInstanceFunc(&PathHideTask::CreateInstance) // Instance creation callback
);

} // namespace ai
