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

#include "CombatStateLanternBot.h"

namespace ai
{

// Get the name of this state
const idStr& CombatStateLanternBot::GetName() const
{
	static idStr _name(STATE_COMBAT_LANTERN_BOT);
	return _name;
}

void CombatStateLanternBot::Init(idAI* owner)
{
	// Init base class first
	CombatState::Init(owner);

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("CombatStateLanternBot initialised.\r");
	assert(owner);

	// TODO
}

// Gets called each time the mind is thinking
void CombatStateLanternBot::Think(idAI* owner)
{
	/*idActor* enemy = _enemy.GetEntity();
	if (enemy == NULL)
	{
		DM_LOG(LC_AI, LT_ERROR)LOGSTRING("No enemy, terminating task!\r");
		owner->GetMind()->EndState();
		return;
	}

	if (enemy->AI_DEAD)
	{
		owner->ClearEnemy();
		owner->StopMove(MOVE_STATUS_DONE);

		// TODO: Check if more enemies are in range
		owner->SetAlertLevel(owner->thresh_2 + (owner->thresh_3 - owner->thresh_2) * 0.9);

		// Emit the killed player bark
		owner->commSubsystem->AddCommTask(
			CommunicationTaskPtr(new SingleBarkTask("snd_killed_enemy"))
		);

		owner->GetMind()->EndState();
		return;
	}

	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner))
	{
		owner->GetMind()->EndState();
		return;
	}
	Memory& memory = owner->GetMemory();

	if (!owner->IsEnemy(enemy))
	{
		// angua: the relation to the enemy has changed, this is not an enemy any more
		owner->StopMove(MOVE_STATUS_DONE);
		owner->SetAlertLevel(owner->thresh_2 + (owner->thresh_3 - owner->thresh_2) * 0.9);
		owner->ClearEnemy();
		owner->GetMind()->EndState();
		
		owner->movementSubsystem->ClearTasks();
		owner->senseSubsystem->ClearTasks();
		owner->actionSubsystem->ClearTasks();

		return;
	}

	// angua: look at ememy
	owner->Event_LookAtPosition(enemy->GetEyePosition(), gameLocal.msec);

	// Flee if we are damaged and the current melee action is finished
	if (owner->health < _criticalHealth && owner->m_MeleeStatus.m_ActionState == MELEEACTION_READY)
	{
		DM_LOG(LC_AI, LT_INFO)LOGSTRING("I'm badly hurt, I'm afraid!\r");
		owner->GetMind()->SwitchState(STATE_FLEE);
		return;
	}

	if (owner->GetMoveType() != MOVETYPE_ANIM)
	{
		owner->GetUp();
		return;
	}

	// Switch between melee and ranged combat based on enemy distance
	float enemyDist = (owner->GetPhysics()->GetOrigin()-enemy->GetPhysics()->GetOrigin()).LengthFast();

	if (_combatType == COMBAT_MELEE && _rangedPossible && enemyDist > 3 * owner->GetMeleeRange())
	{
		owner->movementSubsystem->ClearTasks();
		owner->actionSubsystem->ClearTasks();

		owner->actionSubsystem->PushTask(RangedCombatTask::CreateInstance());
		owner->movementSubsystem->PushTask(ChaseEnemyRangedTask::CreateInstance());
		_combatType = COMBAT_RANGED;
	}

	if (_combatType == COMBAT_RANGED && _meleePossible && enemyDist <= 3 * owner->GetMeleeRange())
	{
		owner->movementSubsystem->ClearTasks();
		owner->actionSubsystem->ClearTasks();

		// Allocate a ChaseEnemyTask
		owner->movementSubsystem->PushTask(TaskPtr(new ChaseEnemyTask(enemy)));

		owner->actionSubsystem->PushTask(MeleeCombatTask::CreateInstance());
		_combatType = COMBAT_MELEE;
	}

	// Check the distance to the enemy, the subsystem tasks need it.
	memory.canHitEnemy = owner->CanHitEntity(enemy, _combatType);
	if( owner->m_bMeleePredictProximity )
		memory.willBeAbleToHitEnemy = owner->WillBeAbleToHitEntity(enemy, _combatType);

	// Check whether the enemy can hit us in the near future
	memory.canBeHitByEnemy = owner->CanBeHitByEntity(enemy, _combatType);

	if (!owner->AI_ENEMY_VISIBLE && 
		(( _combatType == COMBAT_MELEE  && !memory.canHitEnemy) || _combatType == COMBAT_RANGED))
	{
		// The enemy is not visible, let's keep track of him for a small amount of time
		if (gameLocal.time - memory.lastTimeEnemySeen < MAX_BLIND_CHASE_TIME)
		{
			// Cheat a bit and take the last reachable position as "visible & reachable"
			owner->lastVisibleReachableEnemyPos = owner->lastReachableEnemyPos;
		}
		else if (owner->ReachedPos(owner->lastVisibleReachableEnemyPos, MOVE_TO_POSITION)  
			|| gameLocal.time - memory.lastTimeEnemySeen > 2 * MAX_BLIND_CHASE_TIME)
		{
			// BLIND_CHASE_TIME has expired, we have lost the enemy!
			owner->GetMind()->SwitchState(STATE_LOST_TRACK_OF_ENEMY);
			return;
		}
	}*/
}

StatePtr CombatStateLanternBot::CreateInstance()
{
	return StatePtr(new CombatStateLanternBot);
}

// Register this state with the StateLibrary
StateLibrary::Registrar combatStateLanternBotRegistrar(
	STATE_COMBAT_LANTERN_BOT, // Task Name
	StateLibrary::CreateInstanceFunc(&CombatStateLanternBot::CreateInstance) // Instance creation callback
);

} // namespace ai
