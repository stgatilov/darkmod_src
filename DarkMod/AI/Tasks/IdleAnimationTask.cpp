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

#include <vector>
#include <string>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

namespace ai
{

IdleAnimationTask::IdleAnimationTask() :
	_nextAnimationTime(-1),
	_idleAnimationInterval(-1)
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

	// Read the animation set and interval from the owner's spawnarg
	_idleAnimationInterval = SEC2MS(owner->spawnArgs.GetInt("idle_animations_interval", "-1"));

	// Read the general-purpose animations first
	std::string animStringList(owner->spawnArgs.GetString("idle_animations", ""));

	std::vector<std::string> anims; // will hold the separated strings
	boost::algorithm::split(anims, animStringList, boost::algorithm::is_any_of(" ,"));

	// Copy the strings into the idList<idStr>
	for (std::size_t i = 0; i < anims.size(); i++)
	{
		_idleAnimations.Append(idStr(anims[i].c_str()));
	}

	// Now read the anims for the torso only
	animStringList = owner->spawnArgs.GetString("idle_animations_torso", "");
	boost::algorithm::split(anims, animStringList, boost::algorithm::is_any_of(" ,"));

	// Copy the strings into the idList<idStr>
	for (std::size_t i = 0; i < anims.size(); i++)
	{
		_idleAnimationsTorso.Append(idStr(anims[i].c_str()));
	}

	if (_idleAnimationInterval > 0 && (_idleAnimations.Num() > 0 || _idleAnimationsTorso.Num()))
	{
		_nextAnimationTime = static_cast<int>(gameLocal.time + gameLocal.random.RandomFloat()*_idleAnimationInterval);
	}
	else
	{
		// No idle animation interval set or no animations, finish this task
		subsystem.FinishTask();
	}
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
		// Check if the AI is moving, this determines which channel we can play on
		idStr animState(owner->GetAnimState(ANIMCHANNEL_LEGS));

		if (animState == "Legs_Idle")
		{
			// AI is not walking, play animations affecting all channels
			int animIdx = gameLocal.random.RandomInt(_idleAnimations.Num());

			idStr animName(_idleAnimations[animIdx]);

			owner->SetAnimState(ANIMCHANNEL_TORSO, ("Torso_" + animName).c_str(), 4);
			owner->SetAnimState(ANIMCHANNEL_LEGS, ("Legs_" + animName).c_str(), 4);
		}
		else 
		{
			// AI is walking, only use animations for the Torso channel
			int animIdx = gameLocal.random.RandomInt(_idleAnimationsTorso.Num());

			idStr animName(_idleAnimationsTorso[animIdx]);
			owner->SetAnimState(ANIMCHANNEL_TORSO, ("Torso_" + animName).c_str(), 4);
		}
		
		// Reset the timer
		_nextAnimationTime = static_cast<int>(
			gameLocal.time + _idleAnimationInterval*(0.8f + gameLocal.random.RandomFloat()*0.4f)
		);
	}

	return false; // not finished yet
}

void IdleAnimationTask::OnFinish(idAI* owner)
{
	owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", 5);
	owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Idle", 5);
	owner->SetWaitState("");
}

// Save/Restore methods
void IdleAnimationTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);
	savefile->WriteInt(_nextAnimationTime);
	savefile->WriteInt(_idleAnimationInterval);

	savefile->WriteInt(_idleAnimations.Num());
	for (int i = 0; i < _idleAnimations.Num(); i++)
	{
		savefile->WriteString(_idleAnimations[i].c_str());
	}

	savefile->WriteInt(_idleAnimationsTorso.Num());
	for (int i = 0; i < _idleAnimationsTorso.Num(); i++)
	{
		savefile->WriteString(_idleAnimationsTorso[i].c_str());
	}
}

void IdleAnimationTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);
	savefile->ReadInt(_nextAnimationTime);
	savefile->ReadInt(_idleAnimationInterval);

	int num;
	savefile->ReadInt(num);
	_idleAnimations.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		savefile->ReadString(_idleAnimations[i]);
	}

	savefile->ReadInt(num);
	_idleAnimationsTorso.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		savefile->ReadString(_idleAnimationsTorso[i]);
	}
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
