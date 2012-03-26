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

#include "precompiled_game.h"
#pragma hdrstop

static bool versioned = RegisterVersionedFile("$Id$");

#include "../Memory.h"
#include "PathCycleAnimTask.h"
#include "PathTurnTask.h"
#include "../Library.h"

namespace ai
{

PathCycleAnimTask::PathCycleAnimTask() :
	PathTask()
{}

PathCycleAnimTask::PathCycleAnimTask(idPathCorner* path) :
	PathTask(path)
{
	_path = path;
}

// Get the name of this task
const idStr& PathCycleAnimTask::GetName() const
{
	static idStr _name(TASK_PATH_CYCLE_ANIM);
	return _name;
}

void PathCycleAnimTask::Init(idAI* owner, Subsystem& subsystem)
{
	PathTask::Init(owner, subsystem);

	idPathCorner* path = _path.GetEntity();

	// Parse animation spawnargs here
	idStr animName = path->spawnArgs.GetString("anim");
	if (animName.IsEmpty())
	{
		gameLocal.Warning("path_anim entity %s without 'anim' spawnarg found.",path->name.c_str());
		subsystem.FinishTask();
	}

	int blendIn = path->spawnArgs.GetInt("blend_in");
	
	owner->Event_PlayCycle(ANIMCHANNEL_TORSO, animName);
	owner->Event_PlayCycle(ANIMCHANNEL_LEGS, animName);
	
	// Set the name of the state script
	owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_CustomAnim", blendIn);
	owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_CustomAnim", blendIn);
	
	// greebo: Set the waitstate, this gets cleared by 
	// the script function when the animation is done.
	owner->SetWaitState("customAnim");

	owner->GetMind()->GetMemory().playIdleAnimations = false;

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

	owner->AI_ACTIVATED = false;
}

void PathCycleAnimTask::OnFinish(idAI* owner)
{
	// NextPath();

	owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", 5);
	owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Idle", 5);

	owner->SetWaitState("");

	owner->GetMind()->GetMemory().playIdleAnimations = true;
}

bool PathCycleAnimTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("PathCycleAnimTask performing.\r");

	idAI* owner = _owner.GetEntity();

	// This task may not be performed with an empty owner pointer
	assert(owner != NULL);


	// Exit when the waittime is over
	if (_waitEndTime >= 0 && gameLocal.time >= _waitEndTime)
	{
		return true;
	}
	else if (owner->AI_ACTIVATED)
	{
		// no wait time is set
		// AI has been triggered, end this task
		owner->AI_ACTIVATED = false;

		return true;
	}


	return false;

}

// Save/Restore methods
void PathCycleAnimTask::Save(idSaveGame* savefile) const
{
	PathTask::Save(savefile);

	savefile->WriteInt(_waitEndTime);
}

void PathCycleAnimTask::Restore(idRestoreGame* savefile)
{
	PathTask::Restore(savefile);

	savefile->ReadInt(_waitEndTime);
}

PathCycleAnimTaskPtr PathCycleAnimTask::CreateInstance()
{
	return PathCycleAnimTaskPtr(new PathCycleAnimTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar pathCycleAnimTaskRegistrar(
	TASK_PATH_CYCLE_ANIM, // Task Name
	TaskLibrary::CreateInstanceFunc(&PathCycleAnimTask::CreateInstance) // Instance creation callback
);

} // namespace ai
