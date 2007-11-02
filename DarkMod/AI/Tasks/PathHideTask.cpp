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

static bool init_version = FileVersionList("$Id: PathHideTask.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "../Memory.h"
#include "PatrolTask.h"
#include "PathHideTask.h"
#include "../Library.h"

namespace ai
{

// Get the name of this task
const idStr& PathHideTask::GetName() const
{
	static idStr _name(TASK_PATH_HIDE);
	return _name;
}

void PathHideTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Init the base class
	Task::Init(owner, subsystem);

	idPathCorner* path = _path.GetEntity();

	if (path == NULL) {
		gameLocal.Error("PathHideTask: Path Entity not set before Init()");
	}

	// Make invisible and nonsolid
	owner->Hide();
}

bool PathHideTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("Path Hide task performing.\r");

	idPathCorner* path = _path.GetEntity();
	idAI* owner = _owner.GetEntity();

	// This task may not be performed with empty entity pointers
	assert(path != NULL && owner != NULL);

	// Move on to next target
	if (owner->IsHidden())
	{
		// Trigger path targets, now that we've reached the corner
		owner->ActivateTargets(owner);

		// Move is done, fall back to PatrolTask
		DM_LOG(LC_AI, LT_INFO).LogString("entity is hidden.\r");

		// Advance to the next path entity pointer
		idPathCorner* next = idPathCorner::RandomPath(path, NULL);

		if (next == NULL)
		{
			DM_LOG(LC_AI, LT_INFO).LogString("Cannot advance path pointer, no more targets.\r");
			return true; // finish this task
		}

		// Store the new path entity into the AI's mind
		owner->GetMind()->GetMemory().currentPath = next;

		return true; // finish this task
	}
	return false;
}

void PathHideTask::SetTargetEntity(idPathCorner* path) 
{
	assert(path);
	_path = path;
}

// Save/Restore methods
void PathHideTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	_path.Save(savefile);
}

void PathHideTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	_path.Restore(savefile);
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
