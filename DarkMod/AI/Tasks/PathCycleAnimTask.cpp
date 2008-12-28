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

static bool init_version = FileVersionList("$Id: PathCycleAnimTask.cpp 3086 2008-12-14 07:30:30Z angua $", init_version);

#include "../Memory.h"
#include "PathCycleAnimTask.h"
#include "PathTurnTask.h"
#include "../Library.h"

namespace ai
{

PathCycleAnimTask::PathCycleAnimTask() 
{}

PathCycleAnimTask::PathCycleAnimTask(idPathCorner* path)
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
	// Just init the base class
	Task::Init(owner, subsystem);

	idPathCorner* path = _path.GetEntity();

	if (path == NULL) {
		gameLocal.Error("PathCycleAnimTask: Path Entity not set before Init()");
	}

	// Parse animation spawnargs here
	idStr animName = path->spawnArgs.GetString("anim");
	if (animName.IsEmpty())
	{
		gameLocal.Warning("path_anim entity without 'anim' spawnarg found.\n");
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
	idPathCorner* path = _path.GetEntity();

	// Store the new path entity into the AI's mind
	idPathCorner* next = idPathCorner::RandomPath(path, NULL);
	owner->GetMind()->GetMemory().currentPath = next;

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

void PathCycleAnimTask::SetTargetEntity(idPathCorner* path) 
{
	assert(path);
	_path = path;
}

// Save/Restore methods
void PathCycleAnimTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	_path.Save(savefile);

	savefile->WriteInt(_waitEndTime);
}

void PathCycleAnimTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	_path.Restore(savefile);

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
