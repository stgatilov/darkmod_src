/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod (http://www.thedarkmod.com/)
 
 $Revision$ (Revision of last commit) 
 $Date$ (Date of last commit)
 $Author$ (Author of last commit)
 
******************************************************************************/

#include "precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include "../Memory.h"
#include "PathWaitTask.h"
#include "../Library.h"

namespace ai
{

PathWaitTask::PathWaitTask() :
	PathTask()
{}

PathWaitTask::PathWaitTask(idPathCorner* path) : 
	PathTask(path)
{
	_path = path;
}

// Get the name of this task
const idStr& PathWaitTask::GetName() const
{
	static idStr _name(TASK_PATH_WAIT);
	return _name;
}

void PathWaitTask::Init(idAI* owner, Subsystem& subsystem)
{
	PathTask::Init(owner, subsystem);

	idPathCorner* path = _path.GetEntity();

	float waittime = path->spawnArgs.GetFloat("wait","0");
	float waitmax = path->spawnArgs.GetFloat("wait_max", "0");

	if (waitmax > 0)
	{
		waittime += (waitmax - waittime) * gameLocal.random.RandomFloat();
	}

	waittime = SEC2MS(waittime);

	_endtime = waittime + gameLocal.time;
}

bool PathWaitTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("PathWaitTask performing.\r");

	idPathCorner* path = _path.GetEntity();
	idAI* owner = _owner.GetEntity();

	// This task may not be performed with empty entity pointers
	assert(path != NULL && owner != NULL);

	if (gameLocal.time >= _endtime)
	{
		// Trigger path targets, now that we've reached the corner
		owner->ActivateTargets(owner);

		// NextPath();

		// Wait is done, fall back to PatrolTask
		DM_LOG(LC_AI, LT_INFO)LOGSTRING("Wait is done.\r");

		return true; // finish this task
	}
	return false;
}


// Save/Restore methods
void PathWaitTask::Save(idSaveGame* savefile) const
{
	PathTask::Save(savefile);

	savefile->WriteFloat(_endtime);
}

void PathWaitTask::Restore(idRestoreGame* savefile)
{
	PathTask::Restore(savefile);

	savefile->ReadFloat(_endtime);
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
