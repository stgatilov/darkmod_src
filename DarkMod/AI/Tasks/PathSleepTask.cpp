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

	float waittime = path->spawnArgs.GetFloat("wait","0");
	float waitmax = path->spawnArgs.GetFloat("wait_max", "0");

	if (waitmax > 0)
	{
		waittime += (waitmax - waittime) * gameLocal.random.RandomFloat();
	}

	if (waittime > 0)
	{
		_waitEndTime = gameLocal.time + SEC2MS(waittime);
	}
	else
	{
		_waitEndTime = -1;
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

	idStr waitState = owner->WaitState();

	if (owner->GetMoveType() != MOVETYPE_SLEEP && waitState != "lay_down")
	{
		owner->LayDown();

		// no more head turning, no more idle anims
		owner->GetSubsystem(SubsysSenses)->ClearTasks();
		owner->GetSubsystem(SubsysAction)->ClearTasks();

		// stop idle barks and start sleeping sounds after delay
		owner->GetSubsystem(SubsysCommunication)->ClearTasks();
		owner->GetSubsystem(SubsysCommunication)->PushTask(TaskPtr(new WaitTask(5000)));
	
		int idleBarkIntervalMin = SEC2MS(owner->spawnArgs.GetInt("idle_bark_interval_min", "45"));
		int idleBarkIntervalMax = SEC2MS(owner->spawnArgs.GetInt("idle_bark_interval_max", "180"));

		owner->GetSubsystem(SubsysCommunication)->QueueTask(
			TaskPtr(new RepeatedBarkTask("snd_sleeping", idleBarkIntervalMin, idleBarkIntervalMax))
		);

	}


	// we have waiting time set, end this task when waiting is finished
	if (_waitEndTime >= 0)
	{
		if(gameLocal.time >= _waitEndTime)
		{
			// Exit when the waitstate is not "get up" anymore
			if (waitState != "get_up_from_lying_down")
			{
				if (owner->GetMoveType() == MOVETYPE_SLEEP)
				{
					owner->GetUpFromLyingDown();
				}
				else
				{
					return true;
				}
			}
		}
	}
	else if (owner->GetMoveType() == MOVETYPE_SLEEP)
	{
		// no waiting time set, end this task and keep sleeping
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

	savefile->WriteInt(_waitEndTime);
}

void PathSleepTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	_path.Restore(savefile);

	savefile->ReadInt(_waitEndTime);
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
