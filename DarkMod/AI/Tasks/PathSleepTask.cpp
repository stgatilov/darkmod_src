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
#include "RepeatedBarkTask.h"
#include "../States/IdleSleepState.h"
#include "../Library.h"

namespace ai
{

PathSleepTask::PathSleepTask() :
	PathTask()
{}

PathSleepTask::PathSleepTask(idPathCorner* path) :
	PathTask(path)
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
	PathTask::Init(owner, subsystem);

	if (owner->GetMoveType() == MOVETYPE_ANIM)
	{
		owner->LayDown();
	}
}

void PathSleepTask::OnFinish(idAI* owner)
{
	NextPath();
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
