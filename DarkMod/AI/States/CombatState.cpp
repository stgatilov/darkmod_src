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

static bool init_version = FileVersionList("$Id: CombatState.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "CombatState.h"
#include "../Memory.h"
#include "../../AIComm_Message.h"
#include "../Tasks/EmptyTask.h"
#include "../Tasks/IdleSensoryTask.h"
#include "../Tasks/ChaseEnemyTask.h"
#include "../Tasks/SingleBarkTask.h"
#include "../Tasks/MeleeCombatTask.h"
#include "../Tasks/RangedCombatTask.h"
#include "../Tasks/ChaseEnemyRangedTask.h"
#include "LostTrackOfEnemyState.h"
#include "FleeState.h"
#include "ReactingToStimulusState.h"
#include "../Library.h"

namespace ai
{

// Get the name of this state
const idStr& CombatState::GetName() const
{
	static idStr _name(STATE_COMBAT);
	return _name;
}

void CombatState::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO).LogString("CombatState initialised.\r");
	assert(owner);

	if (!CheckAlertLevel(4, "") || !owner->GetMind()->PerformCombatCheck())
	{
		return;
	}

	// Shortcut reference
	Memory& memory = owner->GetMemory();

	// Issue a communication stim
	owner->IssueCommunication_Internal(
		static_cast<float>(CAIComm_Message::DetectedEnemy_CommType), 
		YELL_STIM_RADIUS, 
		NULL,
		owner->GetEnemy(),
		memory.lastEnemyPos
	);

	// Store the enemy entity locally
	_enemy = owner->GetEnemy();
	_criticalHealth = owner->spawnArgs.GetInt("health_critical", "0");

	owner->GetSubsystem(SubsysMovement)->ClearTasks();
	owner->GetSubsystem(SubsysSenses)->ClearTasks();
	owner->GetSubsystem(SubsysCommunication)->ClearTasks();
	owner->GetSubsystem(SubsysAction)->ClearTasks();


	// greebo: Check for weapons and flee if we are unarmed.
	if (owner->GetNumMeleeWeapons() == 0 && owner->GetNumRangedWeapons() == 0)
	{
		DM_LOG(LC_AI, LT_INFO).LogString("I'm unarmed, I'm afraid!\r");
		owner->GetMind()->SwitchState(STATE_FLEE);
		return;
	}

	// greebo: Check for civilian AI, which will always flee in face of a combat (this is a temporary query)
	if (owner->spawnArgs.GetBool("is_civilian", "0"))
	{
		DM_LOG(LC_AI, LT_INFO).LogString("I'm civilian. I'm afraid.\r");
		owner->GetMind()->SwitchState(STATE_FLEE);

		return;
	}

	owner->DrawWeapon();

	/*if (!owner->AI_DEST_UNREACHABLE && owner->CanReachEnemy())
	{
		//pushTaskIfHighestPriority("task_Combat", PRIORITY_COMBAT);
	}
	else
	{
		// TODO: find alternate path, etc
		// Do we have a ranged weapon?
		if (owner->GetNumRangedWeapons() > 0)
		{
 			// Just use ranged weapon
 			//pushTaskIfHighestPriority("task_Combat", PRIORITY_COMBAT);
 		}
 		else
 		{
			// Can't reach the target
			// TODO pushTaskIfHighestPriority("task_TargetCannotBeReached", PRIORITY_CANNOTREACHTARGET);
		}
	}*/

	// Fill the subsystems with their tasks


	// The communication system 
	owner->GetSubsystem(SubsysCommunication)->PushTask(
		TaskPtr(new SingleBarkTask("snd_combat"))
	);

	// Ranged combat
	if (owner->GetNumRangedWeapons() > 0)
	{
		owner->GetSubsystem(SubsysAction)->PushTask(RangedCombatTask::CreateInstance());
		owner->GetSubsystem(SubsysMovement)->PushTask(ChaseEnemyRangedTask::CreateInstance());
	}
	// Melee combat
	else
	{
		// The movement subsystem should start running to the last enemy position
		owner->GetSubsystem(SubsysMovement)->PushTask(ChaseEnemyTask::CreateInstance());
		owner->GetSubsystem(SubsysAction)->PushTask(MeleeCombatTask::CreateInstance());
	}
}

// Gets called each time the mind is thinking
void CombatState::Think(idAI* owner)
{
	if (!CheckAlertLevel(4, ""))
	{
		return;
	}


	Memory& memory = owner->GetMemory();

	idActor* enemy = _enemy.GetEntity();
	if (enemy == NULL)
	{
		DM_LOG(LC_AI, LT_ERROR).LogString("No enemy, terminating task!\r");
		return;
	}

	if (owner->AI_ENEMY_DEAD)
	{
		owner->StopMove(MOVE_STATUS_DONE);
		owner->Event_SetAlertLevel(owner->thresh_1 + (owner->thresh_2 - owner->thresh_1) * 0.5);
		return;
	}

	// Check the distance to the enemy, the other subsystem tasks need it.
	memory.canHitEnemy = owner->CanHitEntity(enemy);

	if (!owner->AI_ENEMY_VISIBLE)
	{
		// The enemy is not visible, let's keep track of him for a small amount of time
		if (gameLocal.time - memory.lastTimeEnemySeen < MAX_BLIND_CHASE_TIME)
		{
			// Cheat a bit and take the last reachable position as "visible & reachable"
			owner->lastVisibleReachableEnemyPos = owner->lastReachableEnemyPos;
		}
		else
		{
			// BLIND_CHASE_TIME has expired, we have lost the enemy!
			owner->GetMind()->SwitchState(STATE_LOST_TRACK_OF_ENEMY);
		}
	}

	if (owner->health < _criticalHealth)
	{
		DM_LOG(LC_AI, LT_INFO).LogString("I'm badly hurt, I'm afraid!\r");
		owner->GetMind()->SwitchState(STATE_FLEE);
		return;

	}
}

void CombatState::Save(idSaveGame* savefile) const
{
	State::Save(savefile);

	savefile->WriteInt(_criticalHealth);
	_enemy.Save(savefile);
}

void CombatState::Restore(idRestoreGame* savefile)
{
	State::Restore(savefile);

	savefile->ReadInt(_criticalHealth);
	_enemy.Restore(savefile);
}

StatePtr CombatState::CreateInstance()
{
	return StatePtr(new CombatState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar combatStateRegistrar(
	STATE_COMBAT, // Task Name
	StateLibrary::CreateInstanceFunc(&CombatState::CreateInstance) // Instance creation callback
);

} // namespace ai
