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

static bool init_version = FileVersionList("$Id: MeleeCombatTask.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "MeleeCombatTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

// Get the name of this task
const idStr& MeleeCombatTask::GetName() const
{
	static idStr _name(TASK_MELEE_COMBAT);
	return _name;
}

void MeleeCombatTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Init the base class
	Task::Init(owner, subsystem);

	_enemy = owner->GetEnemy();
}

bool MeleeCombatTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("Melee Combat Task performing.\r");

	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	idActor* enemy = _enemy.GetEntity();
	if (enemy == NULL)
	{
		DM_LOG(LC_AI, LT_ERROR).LogString("No enemy, terminating task!\r");
		return true; // terminate me
	}

	// Can we damage the enemy already? (this flag is set by the sensory task)
	if (owner->GetMind()->GetMemory().canHitEnemy)
	{
		idStr waitState(owner->WaitState());
		if (waitState != "melee_attack")
		{
			// Waitstate is not matching, this means that the animation 
			// can be started.
			StartAttack(owner);

			// greebo: Set the waitstate, this gets cleared by 
			// the script function when the animation is done.
			owner->SetWaitState("melee_attack");
		}
	}

	return false; // not finished yet
}

void MeleeCombatTask::StartAttack(idAI* owner)
{
	if (gameLocal.random.RandomFloat() < 0.5f)
	{
		// Quick melee
		owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_QuickMelee", 5);
	}
	else
	{
		// Long melee
		owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_LongMelee", 5);
	}
}

void MeleeCombatTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	_enemy.Save(savefile);
}

void MeleeCombatTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	_enemy.Restore(savefile);
}

MeleeCombatTaskPtr MeleeCombatTask::CreateInstance()
{
	return MeleeCombatTaskPtr(new MeleeCombatTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar meleeCombatTaskRegistrar(
	TASK_MELEE_COMBAT, // Task Name
	TaskLibrary::CreateInstanceFunc(&MeleeCombatTask::CreateInstance) // Instance creation callback
);

} // namespace ai
