/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 3086 $
 * $Date: 2008-12-14 08:30:30 +0100 (So, 14 Dez 2008) $
 * $Author: angua $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: PathSleepTask.cpp 3086 2008-12-14 07:30:30Z angua $", init_version);

#include "../Memory.h"
#include "PathSleepTask.h"
#include "PathTurnTask.h"
#include "WaitTask.h"
#include "RepeatedbarkTask.h"
#include "../States/IdleSleepState.h"
#include "../Library.h"

namespace ai
{

PathSleepTask::PathSleepTask() 
{}

PathSleepTask::PathSleepTask(idPathCorner* path)
{
	_path = path;
}

// Get the name of this task
const idStr& PathSleepTask::GetName() const
{
	static idStr _name(TASK_PATH_SLEEP);
	return _name;
}

void PathSleepTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);

	idPathCorner* path = _path.GetEntity();

	if (path == NULL) {
		gameLocal.Error("PathSleepTask: Path Entity not set before Init()");
	}

	idStr waitState = owner->WaitState();

	if (owner->GetMoveType() != MOVETYPE_SLEEP && waitState != "lay_down")
	{
		owner->LayDown();

		owner->GetMind()->SwitchState(STATE_IDLE_SLEEP);
	}
}

void PathSleepTask::OnFinish(idAI* owner)
{
	idPathCorner* path = _path.GetEntity();

	// Store the new path entity into the AI's mind
	idPathCorner* next = idPathCorner::RandomPath(path, NULL, owner);
	owner->GetMind()->GetMemory().currentPath = next;
}

bool PathSleepTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("PathSleepTask performing.\r");

	idAI* owner = _owner.GetEntity();

	// This task may not be performed with an empty owner pointer
	assert(owner != NULL);

	if (owner->GetMoveType() == MOVETYPE_SLEEP)
	{
		return true;
	}
	return false;
}

void PathSleepTask::SetTargetEntity(idPathCorner* path) 
{
	assert(path);
	_path = path;
}

// Save/Restore methods
void PathSleepTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	_path.Save(savefile);
}

void PathSleepTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	_path.Restore(savefile);
}

PathSleepTaskPtr PathSleepTask::CreateInstance()
{
	return PathSleepTaskPtr(new PathSleepTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar pathSleepTaskRegistrar(
	TASK_PATH_SLEEP, // Task Name
	TaskLibrary::CreateInstanceFunc(&PathSleepTask::CreateInstance) // Instance creation callback
);

} // namespace ai
