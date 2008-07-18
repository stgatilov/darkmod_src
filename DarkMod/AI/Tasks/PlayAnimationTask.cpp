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
#include "PlayAnimationTask.h"
#include "../Library.h"

namespace ai
{

PlayAnimationTask::PlayAnimationTask() 
{}

PlayAnimationTask::PlayAnimationTask(const idStr& animName) :
	_animName(animName)
{}

// Get the name of this task
const idStr& PlayAnimationTask::GetName() const
{
	static idStr _name(TASK_PLAY_ANIMATION);
	return _name;
}

void PlayAnimationTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);

	// Parse animation spawnargs here
	if (_animName.IsEmpty())
	{
		gameLocal.Warning("Cannot start PlayAnimationTask with empty animation name.\n");
		subsystem.FinishTask();
	}

	// Synchronise the leg channel
	owner->Event_OverrideAnim(ANIMCHANNEL_LEGS);

	// Play the anim on the TORSO channel (will override the LEGS channel)
	owner->Event_PlayAnim(ANIMCHANNEL_TORSO, _animName);
	
	// Set the name of the state script
	owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_CustomAnim", 2);
	
	// greebo: Set the waitstate, this gets cleared by 
	// the script function when the animation is done.
	owner->SetWaitState("customAnim");
}

void PlayAnimationTask::OnFinish(idAI* owner)
{
	owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", 5);
	owner->SetWaitState("");
}

bool PlayAnimationTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("PlayAnimationTask performing.\r");

	idAI* owner = _owner.GetEntity();

	// This task may not be performed with an empty owner pointer
	assert(owner != NULL);

	// Exit when the waitstate is not "customAnim" anymore
	idStr waitState(owner->WaitState());
	return (waitState != "customAnim");
}

// Save/Restore methods
void PlayAnimationTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	savefile->WriteString(_animName);
}

void PlayAnimationTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	savefile->ReadString(_animName);
}

PlayAnimationTaskPtr PlayAnimationTask::CreateInstance()
{
	return PlayAnimationTaskPtr(new PlayAnimationTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar playAnimationTaskRegistrar(
	TASK_PLAY_ANIMATION, // Task Name
	TaskLibrary::CreateInstanceFunc(&PlayAnimationTask::CreateInstance) // Instance creation callback
);

} // namespace ai
