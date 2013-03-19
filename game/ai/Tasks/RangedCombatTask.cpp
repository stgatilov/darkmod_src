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
	CombatTask::Init(owner, subsystem);

	_enemy = owner->GetEnemy();
}

bool RangedCombatTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("RangedCombatTask performing.\r");

	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	idActor* enemy = _enemy.GetEntity();
	if (enemy == NULL)
	{
		DM_LOG(LC_AI, LT_ERROR)LOGSTRING("No enemy, terminating task!\r");
		return false; // terminate me
	}

	// Can we damage the enemy already? (this flag is set by the combat state)
	if (owner->GetMemory().canHitEnemy)
	{
		idStr waitState(owner->WaitState());
		if (waitState != "ranged_attack")
		{
			// Waitstate is not matching, this means that the animation 
			// can be started.
			owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_RangedAttack", 5);
			owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_RangedAttack", 5);

			// greebo: Set the waitstate, this gets cleared by 
			// the script function when the animation is done.
			owner->SetWaitState("ranged_attack");

			if (_lastCombatBarkTime == -1)
			{
				// grayman #3343 - accommodate different barks for human and non-human enemies

				idStr bark = "";
				idStr enemyAiUse = enemy->spawnArgs.GetString("AIUse");
				if ( ( enemyAiUse == AIUSE_MONSTER ) || ( enemyAiUse == AIUSE_UNDEAD ) )
				{
					bark = "snd_combat_ranged_monster";
				}
				else
				{
					bark = "snd_combat_ranged";
				}
				EmitCombatBark(owner, bark);
			}
		}
		else
		{
			idAnimator* animator = owner->GetAnimatorForChannel(ANIMCHANNEL_LEGS);
			int animint = animator->CurrentAnim(ANIMCHANNEL_LEGS)->AnimNum();
			int length = animator->AnimLength(animint);

			int padding = gameLocal.random.RandomInt(4000) + 1000;

			owner->actionSubsystem->PushTask(TaskPtr(new WaitTask(length + padding)));
		}
	}

	return false; // not finished yet
}

void RangedCombatTask::OnFinish(idAI* owner)
{
	owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", 5);
	owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Idle", 5);
	owner->SetWaitState("");
}

void RangedCombatTask::Save(idSaveGame* savefile) const
{
	CombatTask::Save(savefile);
}

void RangedCombatTask::Restore(idRestoreGame* savefile)
{
	CombatTask::Restore(savefile);
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
