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

static bool init_version = FileVersionList("$Id: PathSitTask.cpp 3086 2008-12-14 07:30:30Z angua $", init_version);

#include "../Memory.h"
#include "PathSitTask.h"
#include "PathTurnTask.h"
#include "../Library.h"

namespace ai
{

PathSitTask::PathSitTask() 
{}

PathSitTask::PathSitTask(idPathCorner* path)
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
	// Just init the base class
	Task::Init(owner, subsystem);

	idPathCorner* path = _path.GetEntity();

	if (path == NULL) {
		gameLocal.Error("PathSitTask: Path Entity not set before Init()");
	}

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
	
	if (owner->GetMoveType() != MOVETYPE_SIT)
	{
		owner->SitDown();
	}
}

void PathSitTask::OnFinish(idAI* owner)
{
	idPathCorner* path = _path.GetEntity();

	// Store the new path entity into the AI's mind
	idPathCorner* next = idPathCorner::RandomPath(path, NULL, owner);
	owner->GetMind()->GetMemory().currentPath = next;
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

void PathSitTask::SetTargetEntity(idPathCorner* path) 
{
	assert(path);
	_path = path;
}

// Save/Restore methods
void PathSitTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	_path.Save(savefile);

	savefile->WriteInt(_waitEndTime);
}

void PathSitTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	_path.Restore(savefile);

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
