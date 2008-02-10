/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: angua $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: TakeCoverState.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "StayInCoverState.h"
#include "FleeState.h"
#include "../Memory.h"
#include "../Tasks/MoveToCoverTask.h"
#include "../Tasks/WaitTask.h"
#include "../Tasks/MoveToPositionTask.h"
#include "../Tasks/RandomHeadturnTask.h"
#include "LostTrackOfEnemyState.h"
#include "EmergeFromCoverState.h"
#include "../Library.h"

namespace ai
{

// Get the name of this state
const idStr& StayInCoverState::GetName() const
{
	static idStr _name(STATE_STAY_IN_COVER);
	return _name;
}

void StayInCoverState::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO).LogString("StayInCoverState initialised.\r");
	assert(owner);

	// Shortcut reference
	Memory& memory = owner->GetMemory();

	// Fill the subsystems with their tasks

	// The movement subsystem should do nothing.
	owner->StopMove(MOVE_STATUS_DONE);
	owner->GetSubsystem(SubsysMovement)->ClearTasks();
	
	// Calculate the time we should stay in cover
	int coverDelayMin = SEC2MS(owner->spawnArgs.GetFloat("emerge_from_cover_delay_min"));
	int coverDelayMax = SEC2MS(owner->spawnArgs.GetFloat("emerge_from_cover_delay_max"));
	int waitTime = static_cast<int>(
		coverDelayMin + gameLocal.random.RandomFloat() * (coverDelayMax - coverDelayMin)
	);
	_emergeDelay = gameLocal.time + waitTime;

	// The communication system 
	owner->GetSubsystem(SubsysCommunication)->ClearTasks();

	// The sensory system does random head turning
	owner->GetSubsystem(SubsysSenses)->ClearTasks();
	owner->GetSubsystem(SubsysSenses)->PushTask(RandomHeadturnTask::CreateInstance());

	// Waiting in cover
	owner->GetSubsystem(SubsysAction)->ClearTasks();
}

// Gets called each time the mind is thinking
void StayInCoverState::Think(idAI* owner)
{
	if ((owner->AI_ENEMY_VISIBLE || owner->AI_TACTALERT) && owner->GetMind()->PerformCombatCheck())
	{
		// Get back to combat if we are done moving to cover and encounter the enemy
		owner->GetMind()->EndState();
		return;
	}

	if (gameLocal.time >= _emergeDelay)
	{
		// emerge from cover after waiting is done
		owner->GetMind()->SwitchState(STATE_EMERGE_FROM_COVER);
		return;
	}
}

void StayInCoverState::OnProjectileHit(idProjectile* projectile)
{
	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	// Call the base class first
	State::OnProjectileHit(projectile);

	if ((owner->GetNumMeleeWeapons() == 0 && owner->GetNumRangedWeapons() == 0) ||
		owner->spawnArgs.GetBool("is_civilian"))
	{
		// We are unarmed or civilian and got hit by a projectile while in cover, run!
		owner->GetMind()->SwitchState(STATE_FLEE);
	}
	else
	{
		// We are armed, so let's just end this state and deal with it
		owner->GetMind()->EndState();
	}
}

void StayInCoverState::Save(idSaveGame* savefile) const
{
	State::Save(savefile);

	savefile->WriteInt(_emergeDelay);
}

void StayInCoverState::Restore(idRestoreGame* savefile)
{
	State::Restore(savefile);

	savefile->ReadInt(_emergeDelay);
}

StatePtr StayInCoverState::CreateInstance()
{
	return StatePtr(new StayInCoverState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar stayInCoverStateRegistrar(
	STATE_STAY_IN_COVER, // Task Name
	StateLibrary::CreateInstanceFunc(&StayInCoverState::CreateInstance) // Instance creation callback
);

} // namespace ai
