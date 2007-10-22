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

	owner->AI_RUN = true;

	_enemy = owner->GetEnemy();

	if (!owner->MoveToPosition(owner->lastVisibleReachableEnemyPos))
	{
		// TODO: Destination unreachable?
	}
}

bool MeleeCombatTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("Chase Enemy Task performing.\r");

	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	idActor* enemy = _enemy.GetEntity();
	if (enemy == NULL)
	{
		DM_LOG(LC_AI, LT_ERROR).LogString("No enemy, terminating task!\r");
		return true;
	}

	// Can we damage the enemy already?
	if (owner->CanHitEntity(enemy))
	{
		// Yes, stop the move!
		owner->StopMove(MOVE_STATUS_DONE);
		gameLocal.Printf("Enemy is reachable!\n");
	}
	// no, push the AI forward and try to get to the last visible reachable enemy position
	else if (owner->MoveToPosition(owner->lastVisibleReachableEnemyPos))
	{
		if (owner->AI_MOVE_DONE)
		{
			// Position has been reached
			gameLocal.Printf("Position reached!\n");
		}
		else
		{
			// AI is moving, this is ok
			
		}
	}
	else
	{
		// Destination unreachable!
		DM_LOG(LC_AI, LT_INFO).LogString("Destination unreachable!\r");
		gameLocal.Printf("Destination unreachable... \n");
	}

	return false; // not finished yet
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
