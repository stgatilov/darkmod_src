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
#include "PathSitTask.h"
#include "PathTurnTask.h"
#include "../Library.h"

namespace ai
{

PathSitTask::PathSitTask() :
	PathTask()
{}

PathSitTask::PathSitTask(idPathCorner* path) :
	PathTask(path)
{
	_path = path;
}

// Get the name of this task
const idStr& PathSitTask::GetName() const
{
	static idStr _name(TASK_PATH_SIT);
	return _name;
}

void PathSitTask::Init(idAI* owner, Subsystem& subsystem)
{
	PathTask::Init(owner, subsystem);

	idPathCorner* path = _path.GetEntity();

	// Parse animation spawnargs here
	
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

	// angua: check whether the AI should turn to a specific angle after sitting down
	if (path->spawnArgs.FindKey("sit_down_angle") != NULL)
	{
		owner->AI_SIT_DOWN_ANGLE = path->spawnArgs.GetFloat("sit_down_angle", "0");
	}
	else
	{
		owner->AI_SIT_DOWN_ANGLE = owner->GetCurrentYaw();
	}
	owner->AI_SIT_UP_ANGLE = owner->GetCurrentYaw();

	owner->AI_SIT_DOWN_ANGLE = idMath::AngleNormalize180(owner->AI_SIT_DOWN_ANGLE);

	
	if (owner->GetMoveType() != MOVETYPE_SIT)
	{
		owner->SitDown();
	}
}

bool PathSitTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("PathSitTask performing.\r");

	idAI* owner = _owner.GetEntity();

	// This task may not be performed with an empty owner pointer
	assert(owner != NULL);


	if (_waitEndTime >= 0)
	{
		if(gameLocal.time >= _waitEndTime)
		{
			// Exit when the waitstate is not "get up" anymore
			idStr waitState(owner->WaitState());
			if (waitState != "get_up")
			{
				if (owner->GetMoveType() == MOVETYPE_SIT)
				{
					owner->GetUp();
				}
				else
				{
					return true;
				}
			}
		}
	}
	else
	{
		return true;
	}

	return false;
}

// Save/Restore methods
void PathSitTask::Save(idSaveGame* savefile) const
{
	PathTask::Save(savefile);

	savefile->WriteInt(_waitEndTime);
}

void PathSitTask::Restore(idRestoreGame* savefile)
{
	PathTask::Restore(savefile);

	savefile->ReadInt(_waitEndTime);
}

PathSitTaskPtr PathSitTask::CreateInstance()
{
	return PathSitTaskPtr(new PathSitTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar pathSitTaskRegistrar(
	TASK_PATH_SIT, // Task Name
	TaskLibrary::CreateInstanceFunc(&PathSitTask::CreateInstance) // Instance creation callback
);

} // namespace ai
