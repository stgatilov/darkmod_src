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

static bool init_version = FileVersionList("$Id: PathWaitTask.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "../Memory.h"
#include "PatrolTask.h"
#include "PathWaitTask.h"
#include "../Library.h"

namespace ai
{

// Get the name of this task
const idStr& PathWaitTask::GetName() const
{
	static idStr _name(TASK_PATH_WAIT);
	return _name;
}

void PathWaitTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);

	idPathCorner* path = _path.GetEntity();

	if (path == NULL) {
		gameLocal.Error("PathWaitTask: Path Entity not set before Init()");
	}

	float waittime = path->spawnArgs.GetFloat("wait","0");
	float waitmax = path->spawnArgs.GetFloat("waitmax", "0");

	if (waitmax > 0)
	{
		waittime += (waitmax - waittime) * gameLocal.random.RandomFloat();
	}

	waittime = SEC2MS(waittime);

	_endtime = waittime + gameLocal.time;
}

bool PathWaitTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("Path Corner Task performing.\r");

	idPathCorner* path = _path.GetEntity();
	idAI* owner = _owner.GetEntity();

	// This task may not be performed with empty entity pointers
	assert(path != NULL && owner != NULL);

	if (gameLocal.time >= _endtime)
	{
		// Trigger path targets, now that we've reached the corner
		owner->ActivateTargets(owner);

		// Move is done, fall back to PatrolTask
		DM_LOG(LC_AI, LT_INFO).LogString("Turn is done.\r");

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

void PathWaitTask::SetTargetEntity(idPathCorner* path) 
{
	assert(path);
	_path = path;
}

// Save/Restore methods
void PathWaitTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	savefile->WriteFloat(_endtime);
	_path.Save(savefile);
}

void PathWaitTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	savefile->ReadFloat(_endtime);
	_path.Restore(savefile);
}

PathWaitTaskPtr PathWaitTask::CreateInstance()
{
	return PathWaitTaskPtr(new PathWaitTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar PathWaitTaskRegistrar(
	TASK_PATH_WAIT, // Task Name
	TaskLibrary::CreateInstanceFunc(&PathWaitTask::CreateInstance) // Instance creation callback
);

} // namespace ai
