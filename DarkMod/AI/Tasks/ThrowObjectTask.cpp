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

static bool init_version = FileVersionList("$Id: ThrowObjectTask.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "ThrowObjectTask.h"
#include "../States/TakeCoverState.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

// Get the name of this task
const idStr& ThrowObjectTask::GetName() const
{
	static idStr _name(TASK_THROW_OBJECT);
	return _name;
}

void ThrowObjectTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Init the base class
	Task::Init(owner, subsystem);

	_projectileDelayMin = SEC2MS(owner->spawnArgs.GetFloat("outofreach_projectile_delay_min", "7.0"));
	_projectileDelayMax = SEC2MS(owner->spawnArgs.GetFloat("outofreach_projectile_delay_max", "10.0"));

	// First throw immediately
	_nextThrowObjectTime = gameLocal.time;

	// This checks if there is a suitable position for taking cover
	aasGoal_t hideGoal;
	if (_takingCoverPossible = owner->LookForCover(hideGoal, owner->GetEnemy(), owner->lastVisibleEnemyPos))
	{
		// We should not go into TakeCoverState if we are already at a suitable position
		if (!owner->spawnArgs.GetBool("taking_cover_enabled","0") || hideGoal.origin == owner->GetPhysics()->GetOrigin() )
		{
			_takingCoverPossible = false;
		}
		DM_LOG(LC_AI, LT_INFO).LogString("Taking Cover Possible: %d \r" , _takingCoverPossible);
	}
}

bool ThrowObjectTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("Throw Object Task performing.\r");

	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);
	
	Memory& memory = owner->GetMind()->GetMemory();
	idActor* enemy = owner->GetEnemy();

	if (enemy == NULL)
	{
		return true;
	}

	if (owner->AI_ENEMY_VISIBLE)
	{
		// Turn to the player
		// We don't need the check for enemy == NULL, since this should not be the case if the enemy is visible
		idVec3 diff = enemy->GetEyePosition() - owner->GetEyePosition();
		owner->TurnToward(diff.ToAngles().yaw);
	}

	// angua: Throw animation plays after the delay has expired 
	// if the player is visible  and object throwing is enabled for this AI
	if (owner->AI_ENEMY_VISIBLE &&
		_nextThrowObjectTime <= gameLocal.time && 
		owner->spawnArgs.GetBool("outofreach_projectile_enabled", "0"))
	{
		idStr waitState(owner->WaitState());
		if (waitState != "throw")
		{
			// Waitstate is not matching, this means that the animation 
			// can be started.
			owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Throw", 5);
			owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Throw", 5);

			// Set the waitstate, this gets cleared by 
			// the script function when the animation is done.
			owner->SetWaitState("throw");
		}
		// Set next throwing time
		_nextThrowObjectTime = gameLocal.time + _projectileDelayMin
							+ gameLocal.random.RandomFloat() * (_projectileDelayMax - _projectileDelayMin);
	}

	// Take cover when throwing is done if it is possible and a ranged threat from the player is detected
	idStr waitState(owner->WaitState());
	if (_takingCoverPossible && waitState != "throw" && 
			( !owner->spawnArgs.GetBool("taking_cover_only_from_archers","0") || enemy->RangedThreatTo(owner) ) )
	{
		owner->GetMind()->SwitchState(STATE_TAKE_COVER);
	}

	if (owner->MoveToPosition(owner->lastVisibleEnemyPos) || owner->TestMelee())
	{
		owner->GetMind()->PerformCombatCheck();
	}

	return false; // not finished yet
}

void ThrowObjectTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	savefile->WriteInt(_projectileDelayMin);
	savefile->WriteInt(_projectileDelayMax);
	savefile->WriteInt(_nextThrowObjectTime);
	savefile->WriteBool(_takingCoverPossible);
}

void ThrowObjectTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	savefile->ReadInt(_projectileDelayMin);
	savefile->ReadInt(_projectileDelayMax);
	savefile->ReadInt(_nextThrowObjectTime);
	savefile->ReadBool(_takingCoverPossible);
}

ThrowObjectTaskPtr ThrowObjectTask::CreateInstance()
{
	return ThrowObjectTaskPtr(new ThrowObjectTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar throwObjectTaskRegistrar(
	TASK_THROW_OBJECT, // Task Name
	TaskLibrary::CreateInstanceFunc(&ThrowObjectTask::CreateInstance) // Instance creation callback
);

} // namespace ai
