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
#include "../Tasks/RandomHeadturnTask.h"
#include "../Tasks/ChaseEnemyTask.h"
#include "../Tasks/SingleBarkTask.h"
#include "../Tasks/MeleeCombatTask.h"
#include "../Tasks/RangedCombatTask.h"
#include "../Tasks/ChaseEnemyRangedTask.h"
#include "LostTrackOfEnemyState.h"
#include "AgitatedSearchingState.h"
#include "FleeState.h"
#include "../Library.h"

namespace ai
{

// Get the name of this state
const idStr& CombatState::GetName() const
{
	static idStr _name(STATE_COMBAT);
	return _name;
}

bool CombatState::CheckAlertLevel(idAI* owner)
{
	// Return TRUE if the Alert Index is matching AND the combat check passes
	return (SwitchOnMismatchingAlertIndex(4, "") && owner->GetMind()->PerformCombatCheck());
}

void CombatState::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO).LogString("CombatState initialised.\r");
	assert(owner);

	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner)) return;

	// Store the enemy entity locally
	_enemy = owner->GetEnemy();
	idActor* enemy = _enemy.GetEntity();

	// Shortcut reference
	Memory& memory = owner->GetMemory();

	// Issue a communication stim
	owner->IssueCommunication_Internal(
		static_cast<float>(CAIComm_Message::DetectedEnemy_CommType), 
		YELL_STIM_RADIUS, 
		NULL,
		enemy,
		memory.lastEnemyPos
	);

	_criticalHealth = owner->spawnArgs.GetInt("health_critical", "0");

	owner->GetSubsystem(SubsysMovement)->ClearTasks();
	owner->GetSubsystem(SubsysSenses)->ClearTasks();
	owner->GetSubsystem(SubsysCommunication)->ClearTasks();
	owner->GetSubsystem(SubsysAction)->ClearTasks();

	_meleePossible = owner->GetNumMeleeWeapons() > 0;
	_rangedPossible = owner->GetNumRangedWeapons() > 0;
	// greebo: Check for weapons and flee if we are unarmed.
	if (!_meleePossible && !_rangedPossible)
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

	// Fill the subsystems with their tasks

	// The communication system 
	owner->GetSubsystem(SubsysCommunication)->PushTask(
		TaskPtr(new SingleBarkTask("snd_combat"))
	);

	// Ranged combat
	if (_rangedPossible)
	{
		if (_meleePossible && 
			(owner->GetPhysics()->GetOrigin()-enemy->GetPhysics()->GetOrigin()).LengthFast() < 3 * owner->GetMeleeRange())
		{
			ChaseEnemyTaskPtr chaseEnemy = ChaseEnemyTask::CreateInstance();
			chaseEnemy->SetEnemy(enemy);
			owner->GetSubsystem(SubsysMovement)->PushTask(chaseEnemy);

			owner->GetSubsystem(SubsysAction)->PushTask(MeleeCombatTask::CreateInstance());
			_combatType = COMBAT_MELEE;
		}
		else
		{
			owner->GetSubsystem(SubsysAction)->PushTask(RangedCombatTask::CreateInstance());
			owner->GetSubsystem(SubsysMovement)->PushTask(ChaseEnemyRangedTask::CreateInstance());
			_combatType = COMBAT_RANGED;
		}
	}
	// Melee combat
	else
	{
		// The movement subsystem should start running to the last enemy position
		ChaseEnemyTaskPtr chaseEnemy = ChaseEnemyTask::CreateInstance();
		chaseEnemy->SetEnemy(enemy);
		owner->GetSubsystem(SubsysMovement)->PushTask(chaseEnemy);

		owner->GetSubsystem(SubsysAction)->PushTask(MeleeCombatTask::CreateInstance());
		_combatType = COMBAT_MELEE;
	}
}

// Gets called each time the mind is thinking
void CombatState::Think(idAI* owner)
{
	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner)) return;

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

	if (owner->health < _criticalHealth)
	{
		DM_LOG(LC_AI, LT_INFO).LogString("I'm badly hurt, I'm afraid!\r");
		owner->GetMind()->SwitchState(STATE_FLEE);
		return;
	}

	// Switch between melee and ranged combat based on enemy distance
	float enemyDist = (owner->GetPhysics()->GetOrigin()-enemy->GetPhysics()->GetOrigin()).LengthFast();

	if (_combatType == COMBAT_MELEE && _rangedPossible && enemyDist > 3 * owner->GetMeleeRange())
	{
		owner->GetSubsystem(SubsysMovement)->ClearTasks();
		owner->GetSubsystem(SubsysAction)->ClearTasks();

		owner->GetSubsystem(SubsysAction)->PushTask(RangedCombatTask::CreateInstance());
		owner->GetSubsystem(SubsysMovement)->PushTask(ChaseEnemyRangedTask::CreateInstance());
		_combatType = COMBAT_RANGED;
	}

	if (_combatType == COMBAT_RANGED && _meleePossible && enemyDist <= 3 * owner->GetMeleeRange())
	{
		owner->GetSubsystem(SubsysMovement)->ClearTasks();
		owner->GetSubsystem(SubsysAction)->ClearTasks();

		ChaseEnemyTaskPtr chaseEnemy = ChaseEnemyTask::CreateInstance();
		chaseEnemy->SetEnemy(enemy);
		owner->GetSubsystem(SubsysMovement)->PushTask(chaseEnemy);

		owner->GetSubsystem(SubsysAction)->PushTask(MeleeCombatTask::CreateInstance());
		_combatType = COMBAT_MELEE;
	}

	// Check the distance to the enemy, the subsystem tasks need it.
	memory.canHitEnemy = owner->CanHitEntity(enemy, _combatType);

	if (!owner->AI_ENEMY_VISIBLE && 
		(( _combatType == COMBAT_MELEE  && !memory.canHitEnemy) || _combatType == COMBAT_RANGED))
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
			return;
		}
	}
}

void CombatState::Save(idSaveGame* savefile) const
{
	State::Save(savefile);

	savefile->WriteInt(_criticalHealth);
	savefile->WriteBool(_meleePossible);
	savefile->WriteBool(_rangedPossible);

	savefile->WriteInt(static_cast<int>(_combatType));

	_enemy.Save(savefile);
}

void CombatState::Restore(idRestoreGame* savefile)
{
	State::Restore(savefile);

	savefile->ReadInt(_criticalHealth);
	savefile->ReadBool(_meleePossible);
	savefile->ReadBool(_rangedPossible);

	int temp;
	savefile->ReadInt(temp);
	_combatType = static_cast<ECombatType>(temp);

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
