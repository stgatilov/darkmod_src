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

static bool init_version = FileVersionList("$Id: PathShowTask.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "../Memory.h"
#include "PatrolTask.h"
#include "PathShowTask.h"
#include "../Library.h"

namespace ai
{

// Get the name of this task
const idStr& PathShowTask::GetName() const
{
	static idStr _name(TASK_PATH_SHOW);
	return _name;
}

void PathShowTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);

	idPathCorner* path = _path.GetEntity();

	if (path == NULL) {
		gameLocal.Error("PathShowTask: Path Entity not set before Init()");
	}
}

bool PathShowTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("Path Show task performing.\r");

	idPathCorner* path = _path.GetEntity();
	idAI* owner = _owner.GetEntity();

	// This task may not be performed with empty entity pointers
	assert(path != NULL && owner != NULL);

	if (owner->CanBecomeSolid())
	{
		owner->Show();
	}

	if (!owner->IsHidden())
	{
		// Trigger path targets, now that we've reached the corner
		owner->ActivateTargets(owner);

		// Move is done, fall back to PatrolTask
		DM_LOG(LC_AI, LT_INFO).LogString("entity is visible.\r");

		// Advance to the next path entity pointer
		idPathCorner* next = idPathCorner::RandomPath(path, NULL);

		if (next == NULL)
		{
			DM_LOG(LC_AI, LT_INFO).LogString("Cannot advance path pointer, no more targets.\r");
			return true; // finish this task
		}

		// Store the new path entity into the AI's mind
		owner->GetMemory().currentPath = next;

		return true; // finish this task
	}
	return false;
}

void PathShowTask::SetTargetEntity(idPathCorner* path) 
{
	assert(path);
	_path = path;
}

// Save/Restore methods
void PathShowTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	_path.Save(savefile);
}

void PathShowTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	_path.Restore(savefile);
}

PathShowTaskPtr PathShowTask::CreateInstance()
{
	return PathShowTaskPtr(new PathShowTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar pathShowTaskRegistrar(
	TASK_PATH_SHOW, // Task Name
	TaskLibrary::CreateInstanceFunc(&PathShowTask::CreateInstance) // Instance creation callback
);

} // namespace ai
