/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: angua $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: RangedCombatTask.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "RangedCombatTask.h"
#include "WaitTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

// Get the name of this task
const idStr& RangedCombatTask::GetName() const
{
	static idStr _name(TASK_RANGED_COMBAT);
	return _name;
}

void RangedCombatTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Init the base class
	Task::Init(owner, subsystem);

	_enemy = owner->GetEnemy();
}

bool RangedCombatTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("RangedCombatTask performing.\r");

	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	idActor* enemy = _enemy.GetEntity();
	if (enemy == NULL)
	{
		DM_LOG(LC_AI, LT_ERROR).LogString("No enemy, terminating task!\r");
		return false; // terminate me
	}

	// Can we damage the enemy already? (this flag is set by the combat state)
	if (owner->GetMemory().canHitEnemy)
	{
		idStr waitState(owner->WaitState());
		if (waitState != "bow_fire")
		{
			// Waitstate is not matching, this means that the animation 
			// can be started.
			owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_BowFire", 5);
			// greebo: Set the waitstate, this gets cleared by 
			// the script function when the animation is done.
			owner->SetWaitState("bow_fire");
			subsystem.PushTask(TaskPtr(new WaitTask(2000)));
		}
	}
	else
	{
		owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", 5);
		owner->SetWaitState("");
	}

	return false; // not finished yet
}

void RangedCombatTask::OnFinish(idAI* owner)
{
	owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", 5);
	owner->SetWaitState("");
}


void RangedCombatTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	_enemy.Save(savefile);
}

void RangedCombatTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	_enemy.Restore(savefile);
}

RangedCombatTaskPtr RangedCombatTask::CreateInstance()
{
	return RangedCombatTaskPtr(new RangedCombatTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar rangedCombatTaskRegistrar(
	TASK_RANGED_COMBAT, // Task Name
	TaskLibrary::CreateInstanceFunc(&RangedCombatTask::CreateInstance) // Instance creation callback
);

} // namespace ai
