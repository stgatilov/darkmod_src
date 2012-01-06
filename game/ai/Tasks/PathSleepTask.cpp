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

	if(_path.GetEntity()->spawnArgs.GetBool("lay_down_left", "1"))
	{
		owner->AI_LAY_DOWN_LEFT = true;
	}
	else
	{
		owner->AI_LAY_DOWN_LEFT = false;
	}

	if (owner->GetMoveType() == MOVETYPE_ANIM)
	{
		owner->LayDown();
	}
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
