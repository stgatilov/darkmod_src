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

static bool init_version = FileVersionList("$Id: ChaseEnemyTask.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "ChaseEnemyTask.h"
#include "../Memory.h"
#include "../Library.h"

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

	owner->AI_RUN = true;
}

bool ChaseEnemyTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("Patrol Task performing.\r");

	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	if (owner->AI_ENEMY_DEAD) {
		// TODO enemy_dead();
	}

	owner->UpdateEnemyPosition();

	// Look at the enemy
	if (owner->AI_ENEMY_IN_FOV) {
		owner->Event_LookAtEnemy(1);
	}

	owner->MoveToEnemy();

	/*if (!owner->AI_ENEMY_REACHABLE)
	{
		// Try to set up movement path to enemy
		owner->AI_RUN = true;
		owner->MoveToEnemy();
	}
	else
	{
		// Enemy reachable;
		DM_LOG(LC_AI, LT_INFO).LogString("Enemy reachable.\r");;
	}*/

	/*attack_flags = check_attacks();
	if ( attack_flags ) {
		do_attack( attack_flags );
		return true;
	}

	if ( check_blocked() ) {
		return true;
	}

	range = enemyRange();
	if ( !AI_ENEMY_VISIBLE || ( range > run_distance ) ) {
		do_run = true;
	}
	
	delta = getTurnDelta();
	if ( ( delta > walk_turn ) || ( delta < -walk_turn ) ) {
		AI_RUN = false;
	} else {
		AI_RUN = do_run;
	}*/
	
	return false; // not finished yet
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
