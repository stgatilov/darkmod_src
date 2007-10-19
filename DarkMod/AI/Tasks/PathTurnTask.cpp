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

static bool init_version = FileVersionList("$Id: PathTurnTask.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "../Memory.h"
#include "PatrolTask.h"
#include "PathTurnTask.h"
#include "../Library.h"

namespace ai
{

PathTurnTask::PathTurnTask() :
	_moveInitiated(false)
{}

// Get the name of this task
const idStr& PathTurnTask::GetName() const
{
	static idStr _name(TASK_PATH_TURN);
	return _name;
}

void PathTurnTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);

	if (_path.GetEntity() == NULL) {
		gameLocal.Error("PathTurnTask: Path Entity not set before Init()");
	}

}

bool PathTurnTask::Perform(Subsystem& subsystem)
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

			// Advance to the next path entity pointer
			idPathCorner* next = idPathCorner::RandomPath(path, NULL);

			if (next == NULL)
			{
				DM_LOG(LC_AI, LT_INFO).LogString("Cannot advance path pointer, no more targets.\r");
				return true; // finish this task
			}

			// Store the new path entity into the AI's mind
			owner->GetMind()->GetMemory().currentPath = next;

			// Fall back to the PatrolTask now we're done here
			subsystem.QueueTask(PatrolTask::CreateInstance());

			return true; // finish this task
		}

		// Move...
	}
	else
	{
		// moveToEntity() not yet called, do it now
		owner->StopMove(MOVE_STATUS_DEST_NOT_FOUND);
		owner->MoveToEntity(path);

		_moveInitiated = true;
	}

	return false; // not finished yet
}

void PathTurnTask::SetTargetEntity(idPathCorner* path) 
{
	assert(path);
	_path = path;
}

// Save/Restore methods
void PathTurnTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	savefile->WriteBool(_moveInitiated);
	_path.Save(savefile);
}

void PathTurnTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	savefile->ReadBool(_moveInitiated);
	_path.Restore(savefile);
}

PathTurnTaskPtr PathTurnTask::CreateInstance()
{
	return PathTurnTaskPtr(new PathTurnTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar pathTurnTaskRegistrar(
	TASK_PATH_TURN, // Task Name
	TaskLibrary::CreateInstanceFunc(&PathTurnTask::CreateInstance) // Instance creation callback
);

} // namespace ai
