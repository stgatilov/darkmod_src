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

static bool init_version = FileVersionList("$Id: UnreachableTargetState.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "UnreachableTargetState.h"
#include "../Memory.h"
#include "../../AIComm_Message.h"
#include "../Tasks/SingleBarkTask.h"
#include "../Tasks/ThrowObjectTask.h"
#include "../Tasks/MoveToPositionTask.h"
#include "LostTrackOfEnemyState.h"
#include "TakeCoverState.h"
#include "../Library.h"

namespace ai
{

// Get the name of this state
const idStr& UnreachableTargetState::GetName() const
{
	static idStr _name(STATE_UNREACHABLE_TARGET);
	return _name;
}

void UnreachableTargetState::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO).LogString("UnreachableTargetState initialised.\r");
	assert(owner);

	// Shortcut reference
	Memory& memory = owner->GetMemory();

	idActor* enemy = owner->GetEnemy();

	if (!enemy)
	{
		owner->GetMind()->SwitchState(STATE_LOST_TRACK_OF_ENEMY);
		return;
	}

	_enemy = enemy;

	// Issue a communication stim
	owner->IssueCommunication_Internal(
		static_cast<float>(CAIComm_Message::RequestForMissileHelp_CommType), 
		YELL_STIM_RADIUS, 
		NULL,
		enemy,
		memory.lastEnemyPos
	);

	// This checks if taking cover is possible and enabled for this AI
	_takingCoverPossible = false;
	if (owner->spawnArgs.GetBool("taking_cover_enabled","0"))
	{
		aasGoal_t hideGoal;
		if (_takingCoverPossible = owner->LookForCover(hideGoal, enemy, owner->lastVisibleEnemyPos))
		{
			// We should not go into TakeCoverState if we are already at a suitable position
			if (hideGoal.origin == owner->GetPhysics()->GetOrigin() )
			{
				_takingCoverPossible = false;
			}
			DM_LOG(LC_AI, LT_INFO).LogString("Taking Cover Possible: %d \r" , _takingCoverPossible);
		}
	}
	_takeCoverTime = -1;

	// Fill the subsystems with their tasks

	// The communication system is barking 
	owner->GetSubsystem(SubsysCommunication)->ClearTasks();
	owner->GetSubsystem(SubsysCommunication)->PushTask(
		TaskPtr(new SingleBarkTask("snd_cantReachTarget"))
	);

	// The sensory system does nothing so far
	owner->GetSubsystem(SubsysSenses)->ClearTasks();

	owner->StopMove(MOVE_STATUS_DONE);
	owner->GetSubsystem(SubsysMovement)->ClearTasks();

	owner->GetSubsystem(SubsysAction)->ClearTasks();

	_moveRequired = false;

	if (owner->spawnArgs.GetBool("outofreach_projectile_enabled", "0"))
	{
		// Check the distance between AI and the player, if it is too large try to move closer
		// Start throwing objects if we are close enough
		idVec3 enemyDirection = enemy->GetPhysics()->GetOrigin() - owner->GetPhysics()->GetOrigin();
		float dist = (enemyDirection).LengthFast();
		
		//TODO: make not hardcoded
		if (dist > 300)
		{
			_moveRequired = true;

			idVec3 throwPos = enemy->GetPhysics()->GetOrigin() - enemyDirection / dist * 300;

			// TODO: Trace to get floor position
			throwPos.z = owner->GetPhysics()->GetOrigin().z;

			owner->GetSubsystem(SubsysMovement)->PushTask(TaskPtr(new MoveToPositionTask(throwPos)));
			owner->AI_MOVE_DONE = false;
		}
		else 
		{
			owner->FaceEnemy();
			owner->GetSubsystem(SubsysAction)->PushTask(ThrowObjectTask::CreateInstance());

			// Wait at least 3 sec after starting to throw before taking cover
			// TODO: make not hardcoded, some randomness?
			_takeCoverTime = gameLocal.time + 3000;
		}
		DM_LOG(LC_AI, LT_INFO).LogString("move required: %d \r" , _moveRequired);
	}
	else
	{
		owner->GetSubsystem(SubsysMovement)->PushTask(
			TaskPtr(new MoveToPositionTask(owner->lastVisibleReachableEnemyPos))
		);
		_takeCoverTime = gameLocal.time + 3000;
	}

	_reachEnemyCheck = 0;
}


// Gets called each time the mind is thinking
void UnreachableTargetState::Think(idAI* owner)
{
	Memory& memory = owner->GetMemory();

	idActor* enemy = _enemy.GetEntity();
	if (enemy == NULL)
	{
		DM_LOG(LC_AI, LT_ERROR).LogString("No enemy!\r");
		owner->GetMind()->SwitchState(STATE_LOST_TRACK_OF_ENEMY);
		return;
	}

	if (owner->AI_ENEMY_DEAD)
	{
		owner->StopMove(MOVE_STATUS_DONE);
		owner->Event_SetAlertLevel(owner->thresh_2 + (owner->thresh_3 - owner->thresh_2) * 0.5);
		owner->GetMind()->EndState();
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
			return;
		}
	}

	owner->TurnToward(enemy->GetPhysics()->GetOrigin());
	
	if (owner->spawnArgs.GetBool("outofreach_projectile_enabled", "0") &&
			_moveRequired && (owner->AI_MOVE_DONE || owner->AI_DEST_UNREACHABLE))
	{
		// We are finished moving closer
		// Start throwing now
		_moveRequired = false;

		owner->FaceEnemy();
		owner->GetSubsystem(SubsysAction)->PushTask(ThrowObjectTask::CreateInstance());
		_takeCoverTime = gameLocal.time + 3000;
	}

	// This checks if the enemy is reachable again so we can go into combat state
	if (owner->enemyReachable || owner->TestMelee() || memory.canHitEnemy)
	{
		if (owner->GetMind()->PerformCombatCheck())
		{
			owner->GetMind()->EndState();
			return;
		}
	}

	// This checks for a reachable position within combat range
	idVec3 enemyDirection = owner->GetPhysics()->GetOrigin() - enemy->GetPhysics()->GetOrigin();
	enemyDirection.z = 0;
	enemyDirection.NormalizeFast();
	float angle = (_reachEnemyCheck * 90) % 360;
	float sinAngle = idMath::Sin(angle);
	float cosAngle = idMath::Cos(angle);
	idVec3 targetDirection = enemyDirection;
	targetDirection.x = enemyDirection.x * cosAngle + enemyDirection.y * sinAngle;
	targetDirection.y = enemyDirection.y * cosAngle + enemyDirection.x * sinAngle;

	idVec3 targetPoint = enemy->GetPhysics()->GetOrigin() 
				+ (targetDirection * (owner->spawnArgs.GetFloat("melee_range","64")));
	idVec3 bottomPoint = targetPoint;
	bottomPoint.z -= 70;
	
	trace_t result;
	if (gameLocal.clip.TracePoint(result, targetPoint, bottomPoint, MASK_OPAQUE, NULL))
	{
		targetPoint.z = result.endpos.z + 1;
		int areaNum = owner->PointReachableAreaNum(owner->GetPhysics()->GetOrigin(), 1.0f);
		idVec3 forward = owner->viewAxis.ToAngles().ToForward();
		int targetAreaNum = owner->PointReachableAreaNum(targetPoint, 1.0f, -10*forward);
		aasPath_t path;

		if (owner->PathToGoal(path, areaNum, owner->GetPhysics()->GetOrigin(), targetAreaNum, targetPoint))
		{
			owner->GetMind()->EndState();
			return;
		}
		else
		{
			_reachEnemyCheck ++;
		}
	}
	else
	{
		_reachEnemyCheck ++;
	}
	
	// Wait at least for 3 seconds (_takeCoverTime) after starting to throw before taking cover
	// If a ranged thread from the player is detected (bow is out)
	// take cover if possible after throwing animation is finished
	idStr waitState(owner->WaitState());

	if ( _takingCoverPossible && waitState != "throw" &&
		_takeCoverTime > 0 && gameLocal.time > _takeCoverTime &&
		( enemy->RangedThreatTo(owner) || !owner->spawnArgs.GetBool("taking_cover_only_from_archers","0") ))
	{
		owner->GetMind()->SwitchState(STATE_TAKE_COVER);
	}
}

void UnreachableTargetState::Save(idSaveGame* savefile) const
{
	State::Save(savefile);

	savefile->WriteBool(_takingCoverPossible);
	savefile->WriteInt(_takeCoverTime);
	savefile->WriteBool(_moveRequired);
	savefile->WriteInt(_reachEnemyCheck);
	_enemy.Save(savefile);
}

void UnreachableTargetState::Restore(idRestoreGame* savefile)
{
	State::Restore(savefile);

	savefile->ReadBool(_takingCoverPossible);
	savefile->ReadInt(_takeCoverTime);
	savefile->ReadBool(_moveRequired);
	savefile->ReadInt(_reachEnemyCheck);
	_enemy.Restore(savefile);
}

StatePtr UnreachableTargetState::CreateInstance()
{
	return StatePtr(new UnreachableTargetState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar unreachableTargetStateRegistrar(
	STATE_UNREACHABLE_TARGET, // Task Name
	StateLibrary::CreateInstanceFunc(&UnreachableTargetState::CreateInstance) // Instance creation callback
);

} // namespace ai
