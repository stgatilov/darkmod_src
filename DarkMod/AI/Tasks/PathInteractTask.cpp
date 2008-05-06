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

static bool init_version = FileVersionList("$Id: PathInteractTask.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "../Memory.h"
#include "PatrolTask.h"
#include "PathInteractTask.h"
#include "../Library.h"

namespace ai
{

PathInteractTask::PathInteractTask()
{}

PathInteractTask::PathInteractTask(idPathCorner* path)
{
	_path = path;
}

// Get the name of this task
const idStr& PathInteractTask::GetName() const
{
	static idStr _name(TASK_PATH_INTERACT);
	return _name;
}

void PathInteractTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);

	idPathCorner* path = _path.GetEntity();

	if (path == NULL) {
		gameLocal.Error("PathInteractTask: Path Entity not set before Init()");
	}

	idStr targetStr = path->spawnArgs.GetString("ent", "");
	_target = gameLocal.FindEntity(targetStr);

	if (_target == NULL)
	{
		return;
	}

	owner->StopMove(MOVE_STATUS_DONE);
	// Turn and look
	owner->TurnToward(_target->GetPhysics()->GetOrigin());
	owner->Event_LookAtEntity(_target, 1);

	// Start anim
	owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Use_righthand", 4);

	_waitEndTime = gameLocal.time + 600;
}

bool PathInteractTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("PathInteractTask performing.\r");

	idPathCorner* path = _path.GetEntity();
	idAI* owner = _owner.GetEntity();

	// This task may not be performed with empty entity pointers
	assert(path != NULL && owner != NULL);

	if (_target == NULL)
	{
		return true;
	}
	
	if (gameLocal.time >= _waitEndTime)
	{
		// Trigger the frob action script
		_target->FrobAction(true);
		return true;
	}
	
	// Debug
	// gameRenderWorld->DebugArrow(colorGreen, owner->GetEyePosition(), _target->GetPhysics()->GetOrigin(), 10, 10000);
	
	// Trigger next path target(s)
	owner->ActivateTargets(owner);
	
	return false; // finish this task
}

// Save/Restore methods
void PathInteractTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	_path.Save(savefile);
	//_target.Save(savefile);
	savefile->WriteInt(_waitEndTime);
}

void PathInteractTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	_path.Restore(savefile);
	//_target.Restore(savefile);

	savefile->ReadInt(_waitEndTime);
}

PathInteractTaskPtr PathInteractTask::CreateInstance()
{
	return PathInteractTaskPtr(new PathInteractTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar pathInteractTaskRegistrar(
	TASK_PATH_INTERACT, // Task Name
	TaskLibrary::CreateInstanceFunc(&PathInteractTask::CreateInstance) // Instance creation callback
);

} // namespace ai
