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

#include "ChaseEnemyTask.h"
#include "../Memory.h"
#include "../Library.h"
#include "../../MultiStateMover.h"
#include "../States/UnreachableTargetState.h"

namespace ai
{

// Get the name of this task
const idStr& ChaseEnemyTask::GetName() const
{
	static idStr _name(TASK_CHASE_ENEMY);
	return _name;
}

void ChaseEnemyTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Init the base class
	Task::Init(owner, subsystem);

	idActor* enemy = _enemy.GetEntity();
	if (!enemy)
	{
		_enemy = owner->GetEnemy();
	}

	_reachEnemyCheck = 0;
	owner->AI_RUN = true;
	owner->MoveToPosition(owner->lastVisibleEnemyPos);
}

bool ChaseEnemyTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("Chase Enemy Task performing.\r");

	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	Memory& memory = owner->GetMemory();

	idActor* enemy = _enemy.GetEntity();
	if (enemy == NULL)
	{
		DM_LOG(LC_AI, LT_ERROR).LogString("No enemy, terminating task!\r");
		return true;
	}

	// Can we damage the enemy already? (this flag is set by the combat state)
	if (memory.canHitEnemy)
	{
		_reachEnemyCheck = 0;

		// Yes, stop the move!
		owner->StopMove(MOVE_STATUS_DONE);
		//gameLocal.Printf("Enemy is reachable!\n");
		// Turn to the player
		owner->TurnToward(enemy->GetEyePosition());
	}
	// no, push the AI forward and try to get to the last visible reachable enemy position
	else if (owner->MoveToPosition(owner->lastVisibleReachableEnemyPos))
	{
		_reachEnemyCheck = 0;

		if (owner->GetMoveStatus() == MOVE_STATUS_DONE)
		{
			// Position has been reached, turn to player, if visible
			if (owner->AI_ENEMY_VISIBLE)
			{
				// angua: Turn to the player
				owner->TurnToward(enemy->GetEyePosition());
			}
		}
	}
	else if (owner->AI_MOVE_DONE && !owner->m_HandlingDoor)
	{
		// Enemy position itself is not reachable, try to find a position within melee range around the enemy
		if (_reachEnemyCheck < 4)
		{
			idVec3 enemyDirection = owner->GetPhysics()->GetOrigin() - enemy->GetPhysics()->GetOrigin();
			enemyDirection.z = 0;

			// test direction rotates 90° each time the check is performed
			enemyDirection.NormalizeFast();
			float angle = _reachEnemyCheck * 90;
			float sinAngle = idMath::Sin(angle);
			float cosAngle = idMath::Cos(angle);
			idVec3 targetDirection = enemyDirection;
			targetDirection.x = enemyDirection.x * cosAngle + enemyDirection.y * sinAngle;
			targetDirection.y = enemyDirection.y * cosAngle + enemyDirection.x * sinAngle;

			idVec3 targetPoint = enemy->GetPhysics()->GetOrigin() 
					+ (targetDirection * owner->GetMeleeRange());
			idVec3 bottomPoint = targetPoint;
			bottomPoint.z -= 70;
			
			trace_t result;
			if (gameLocal.clip.TracePoint(result, targetPoint, bottomPoint, MASK_OPAQUE, NULL))
			{
				targetPoint.z = result.endpos.z + 1;
				idVec3 forward = owner->viewAxis.ToAngles().ToForward();
				targetPoint -= 10 * forward;

				if (!owner->MoveToPosition(targetPoint))
				{
					_reachEnemyCheck ++;
				}
			}
			else
			{
				_reachEnemyCheck ++;
			}
		}
		else
		{
			// Unreachable by walking, check if the opponent is on an elevator
			CMultiStateMover* mover = enemy->OnElevator();
			if (mover != NULL)
			{
				//gameRenderWorld->DebugArrow(colorRed, owner->GetPhysics()->GetOrigin(), mover->GetPhysics()->GetOrigin(), 1, 48);

				// greebo: Check if we can fetch the elevator back to our floor
				if (CanFetchElevator(mover, owner))
				{
					// Push to an InteractionTask

					return false;
				}
			}

			// Destination unreachable!
			DM_LOG(LC_AI, LT_INFO).LogString("Destination unreachable!\r");
			gameLocal.Printf("Destination unreachable... \n");
			owner->GetMind()->SwitchState(STATE_UNREACHABLE_TARGET);
			return true;
		}
	}

	return false; // not finished yet
}

bool ChaseEnemyTask::CanFetchElevator(CMultiStateMover* mover, idAI* owner)
{
	

	return false;
}

void ChaseEnemyTask::SetEnemy(idActor* enemy)
{
	_enemy = enemy;
}

void ChaseEnemyTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	_enemy.Save(savefile);
	savefile->WriteInt(_reachEnemyCheck);
}

void ChaseEnemyTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	_enemy.Restore(savefile);
	savefile->ReadInt(_reachEnemyCheck);
}

ChaseEnemyTaskPtr ChaseEnemyTask::CreateInstance()
{
	return ChaseEnemyTaskPtr(new ChaseEnemyTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar chaseEnemyTaskRegistrar(
	TASK_CHASE_ENEMY, // Task Name
	TaskLibrary::CreateInstanceFunc(&ChaseEnemyTask::CreateInstance) // Instance creation callback
);

} // namespace ai
