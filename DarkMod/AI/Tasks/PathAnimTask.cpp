/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2008-03-27 18:53:28 +0200 (Do, 27 Mar 2008) $
 * $Author: greebo $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: PathAnimTask.cpp 1435 2008-03-27 16:53:28Z greebo $", init_version);

#include "../Memory.h"
#include "PathAnimTask.h"
#include "../Library.h"

namespace ai
{

PathAnimTask::PathAnimTask() 
{}

PathAnimTask::PathAnimTask(idPathCorner* path)
{
	_path = path;
}

// Get the name of this task
const idStr& PathAnimTask::GetName() const
{
	static idStr _name(TASK_PATH_ANIM);
	return _name;
}

void PathAnimTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);

	idPathCorner* path = _path.GetEntity();

	if (path == NULL) {
		gameLocal.Error("PathAnimTask: Path Entity not set before Init()");
	}

	// Parse animation spawnargs here
}

bool PathAnimTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("PathAnimTask performing.\r");

	idPathCorner* path = _path.GetEntity();
	idAI* owner = _owner.GetEntity();

	// This task may not be performed with empty entity pointers
	assert(path != NULL && owner != NULL);

	// Move on to next target when turning is done
	/*if (owner->FacingIdeal())
	{
		// Trigger path targets, now that we've reached the corner
		owner->ActivateTargets(owner);

		// Move is done, fall back to PatrolTask
		DM_LOG(LC_AI, LT_INFO).LogString("Turn is done.\r");

		return true; // finish this task
	}*/
	return false;
}

void PathAnimTask::SetTargetEntity(idPathCorner* path) 
{
	assert(path);
	_path = path;
}

// Save/Restore methods
void PathAnimTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	_path.Save(savefile);
}

void PathAnimTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	_path.Restore(savefile);
}

PathAnimTaskPtr PathAnimTask::CreateInstance()
{
	return PathAnimTaskPtr(new PathAnimTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar pathAnimTaskRegistrar(
	TASK_PATH_ANIM, // Task Name
	TaskLibrary::CreateInstanceFunc(&PathAnimTask::CreateInstance) // Instance creation callback
);

} // namespace ai
