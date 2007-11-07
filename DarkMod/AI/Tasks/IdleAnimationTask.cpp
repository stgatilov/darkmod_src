/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: IdleAnimationTask.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "IdleAnimationTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

#define ANIM_INTERVAL 25000 // msecs

const char* IDLE_ANIMS[3] = {
	"Torso_Itch",
	"Torso_SniffArm1",
	"Torso_Cough1"
};

IdleAnimationTask::IdleAnimationTask() :
	_nextAnimationTime(-1)
{}

// Get the name of this task
const idStr& IdleAnimationTask::GetName() const
{
	static idStr _name(TASK_IDLE_ANIMATION);
	return _name;
}

void IdleAnimationTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);

	Memory& memory = owner->GetMemory();

	_nextAnimationTime = gameLocal.time + gameLocal.random.RandomFloat()*ANIM_INTERVAL;
}

bool IdleAnimationTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("IdleAnimationTask performing.\r");

	idAI* owner = _owner.GetEntity();

	// This task may not be performed with empty entity pointers
	assert(owner != NULL);

	Memory& memory = owner->GetMemory();

	if (gameLocal.time > _nextAnimationTime)
	{
		// The time has come, determine the animation to play
		int animIdx = gameLocal.random.RandomInt(3);
		owner->SetAnimState(ANIMCHANNEL_TORSO, IDLE_ANIMS[animIdx], 4);
		
		// Reset the timer
		_nextAnimationTime = gameLocal.time + ANIM_INTERVAL*(0.8f + gameLocal.random.RandomFloat()*0.4f);
	}

	return false; // not finished yet
}

// Save/Restore methods
void IdleAnimationTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);
	savefile->WriteInt(_nextAnimationTime);
}

void IdleAnimationTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);
	savefile->ReadInt(_nextAnimationTime);
}

IdleAnimationTaskPtr IdleAnimationTask::CreateInstance()
{
	return IdleAnimationTaskPtr(new IdleAnimationTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar idleAnimationTaskRegistrar(
	TASK_IDLE_ANIMATION, // Task Name
	TaskLibrary::CreateInstanceFunc(&IdleAnimationTask::CreateInstance) // Instance creation callback
);

} // namespace ai
