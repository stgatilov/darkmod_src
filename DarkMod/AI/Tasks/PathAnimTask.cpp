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
#include "PathAnimTask.h"
#include "PathTurnTask.h"
#include "../Library.h"

namespace ai
{

PathAnimTask::PathAnimTask() 
{}

PathAnimTask::PathAnimTask(idPathCorner* path)
{
	_path = path;
}

// Get the name of this task
const idStr& PathAnimTask::GetName() const
{
	static idStr _name(TASK_PATH_ANIM);
	return _name;
}

void PathAnimTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);

	idPathCorner* path = _path.GetEntity();

	if (path == NULL) {
		gameLocal.Error("PathAnimTask: Path Entity not set before Init()");
	}

	// Parse animation spawnargs here
	idStr animName = path->spawnArgs.GetString("anim");
	if (animName.IsEmpty())
	{
		gameLocal.Warning("path_anim entity without 'anim' spawnarg found.\n");
		subsystem.FinishTask();
	}

	int blendIn = path->spawnArgs.GetInt("blend_in");
	
	// Play the anim on the TORSO channel (will override the LEGS channel)
	owner->Event_PlayAnim(ANIMCHANNEL_TORSO, animName);
	owner->Event_PlayAnim(ANIMCHANNEL_LEGS, animName);
	
	// Set the name of the state script
	owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_CustomAnim", blendIn);
	owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_CustomAnim", blendIn);
	
	// greebo: Set the waitstate, this gets cleared by 
	// the script function when the animation is done.
	owner->SetWaitState("customAnim");

	owner->GetMind()->GetMemory().playIdleAnimations = false;
}

void PathAnimTask::OnFinish(idAI* owner)
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

bool PathAnimTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("PathAnimTask performing.\r");

	idAI* owner = _owner.GetEntity();

	// This task may not be performed with an empty owner pointer
	assert(owner != NULL);

	// Exit when the waitstate is not "customAnim" anymore
	idStr waitState(owner->WaitState());
	return (waitState != "customAnim");
}

void PathAnimTask::SetTargetEntity(idPathCorner* path) 
{
	assert(path);
	_path = path;
}

// Save/Restore methods
void PathAnimTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	_path.Save(savefile);
}

void PathAnimTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	_path.Restore(savefile);
}

PathAnimTaskPtr PathAnimTask::CreateInstance()
{
	return PathAnimTaskPtr(new PathAnimTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar pathAnimTaskRegistrar(
	TASK_PATH_ANIM, // Task Name
	TaskLibrary::CreateInstanceFunc(&PathAnimTask::CreateInstance) // Instance creation callback
);

} // namespace ai
