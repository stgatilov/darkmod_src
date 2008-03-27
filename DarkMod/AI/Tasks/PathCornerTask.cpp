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

static bool init_version = FileVersionList("$Id: PathCornerTask.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "../Memory.h"
#include "PatrolTask.h"
#include "PathCornerTask.h"
#include "../Library.h"

namespace ai
{

PathCornerTask::PathCornerTask() :
	_moveInitiated(false)
{}

PathCornerTask::PathCornerTask(idPathCorner* path) :
	_moveInitiated(false)
{
	_path = path;
}

// Get the name of this task
const idStr& PathCornerTask::GetName() const
{
	static idStr _name(TASK_PATH_CORNER);
	return _name;
}

void PathCornerTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);

	if (_path.GetEntity() == NULL)
	{
		gameLocal.Error("PathCornerTask: Path Entity not set before Init()");
	}

	// Check the "run" spawnarg of this path entity
	owner->AI_RUN = (_path.GetEntity()->spawnArgs.GetBool("run", "0"));
}

bool PathCornerTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("Path Corner Task performing.\r");

	idPathCorner* path = _path.GetEntity();
	idAI* owner = _owner.GetEntity();

	// This task may not be performed with empty entity pointers
	assert(path != NULL && owner != NULL);

	// TODO: ai_darkmod_base::playCustomCycle? needed? "anim" spawnarg?

	if (_moveInitiated)
	{
		if (owner->AI_MOVE_DONE || owner->AI_DEST_UNREACHABLE)
		{
			if (owner->AI_MOVE_DONE)
			{
				// Trigger path targets, now that we've reached the corner
				owner->ActivateTargets(owner);

				// Move is done, fall back to PatrolTask
				DM_LOG(LC_AI, LT_INFO).LogString("Move is done.\r");
			}
			
			if (owner->AI_DEST_UNREACHABLE)
			{
				// Unreachable, fall back to PatrolTask
				DM_LOG(LC_AI, LT_INFO).LogString("Destination is unreachable, skipping.\r");
			}
		
			return true; // finish this task
		}

		// Move...
	}
	else
	{
		// moveToEntity() not yet called, do it now
		owner->StopMove(MOVE_STATUS_DEST_NOT_FOUND);
		owner->MoveToPosition(path->GetPhysics()->GetOrigin());

		_moveInitiated = true;
	}

	return false; // not finished yet
}

void PathCornerTask::SetTargetEntity(idPathCorner* path) 
{
	assert(path);
	_path = path;
}

// Save/Restore methods
void PathCornerTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	savefile->WriteBool(_moveInitiated);
	_path.Save(savefile);
}

void PathCornerTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	savefile->ReadBool(_moveInitiated);
	_path.Restore(savefile);
}

PathCornerTaskPtr PathCornerTask::CreateInstance()
{
	return PathCornerTaskPtr(new PathCornerTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar pathCornerTaskRegistrar(
	TASK_PATH_CORNER, // Task Name
	TaskLibrary::CreateInstanceFunc(&PathCornerTask::CreateInstance) // Instance creation callback
);

} // namespace ai
