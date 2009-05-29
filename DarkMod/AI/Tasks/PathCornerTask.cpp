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
#include "PathCornerTask.h"
#include "../Library.h"

namespace ai
{

PathCornerTask::PathCornerTask() :
	PathTask(),
	_moveInitiated(false)
{}

PathCornerTask::PathCornerTask(idPathCorner* path) :
	PathTask(path),
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
	PathTask::Init(owner, subsystem);

	// Check the "run" spawnarg of this path entity
	owner->AI_RUN = (_path.GetEntity()->spawnArgs.GetBool("run", "0"));
}

bool PathCornerTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("Path Corner Task performing.\r");

	idPathCorner* path = _path.GetEntity();
	idAI* owner = _owner.GetEntity();

	// This task may not be performed with empty entity pointers
	assert(path != NULL && owner != NULL);

	// TODO: ai_darkmod_base::playCustomCycle? needed? "anim" spawnarg?

	if (_moveInitiated)
	{
		if (owner->AI_MOVE_DONE)
		{
			if(owner->ReachedPos(path->GetPhysics()->GetOrigin(), MOVE_TO_POSITION))
			{
				// Trigger path targets, now that we've reached the corner
				owner->ActivateTargets(owner);
				NextPath();

				// Move is done, fall back to PatrolTask
				DM_LOG(LC_AI, LT_INFO)LOGSTRING("Move is done.\r");

				return true; // finish this task
			}
			else
			{
				owner->MoveToPosition(path->GetPhysics()->GetOrigin(), _accuracy);
			}
		}	
		if (owner->AI_DEST_UNREACHABLE)
		{
			// Unreachable, fall back to PatrolTask
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Destination is unreachable, skipping.\r");
			NextPath();
			return true; // finish this task
		}

		// Move...
	}
	else
	{
		// moveToEntity() not yet called, do it now
		owner->StopMove(MOVE_STATUS_DEST_NOT_FOUND);
		owner->MoveToPosition(path->GetPhysics()->GetOrigin(), _accuracy);

		_moveInitiated = true;
	}

	return false; // not finished yet
}


// Save/Restore methods
void PathCornerTask::Save(idSaveGame* savefile) const
{
	PathTask::Save(savefile);

	savefile->WriteBool(_moveInitiated);
}

void PathCornerTask::Restore(idRestoreGame* savefile)
{
	PathTask::Restore(savefile);

	savefile->ReadBool(_moveInitiated);
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
