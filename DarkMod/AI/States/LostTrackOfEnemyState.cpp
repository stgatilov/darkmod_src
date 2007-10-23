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

static bool init_version = FileVersionList("$Id: LostTrackOfEnemyState.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "LostTrackOfEnemyState.h"
#include "../Memory.h"
#include "../Tasks/EmptyTask.h"
#include "../Tasks/SingleBarkTask.h"
#include "../Library.h"

namespace ai
{

// Get the name of this state
const idStr& LostTrackOfEnemyState::GetName() const
{
	static idStr _name(STATE_LOST_TRACK_OF_ENEMY);
	return _name;
}

void LostTrackOfEnemyState::Init(idAI* owner)
{
	DM_LOG(LC_AI, LT_INFO).LogString("LostTrackOfEnemyState initialised.\r");
	assert(owner);

	// Shortcut reference
	Memory& memory = owner->GetMind()->GetMemory();

	// Draw weapon, if we haven't already
	owner->DrawWeapon();

	// Fill the subsystems with their tasks

	// The movement subsystem should start running to the last enemy position
	owner->GetSubsystem(SubsysMovement)->ClearTasks();
	owner->GetSubsystem(SubsysMovement)->QueueTask(EmptyTask::CreateInstance());

	// The communication system is barking in regular intervals
	owner->GetSubsystem(SubsysCommunication)->ClearTasks();

	SingleBarkTaskPtr barkTask = SingleBarkTask::CreateInstance();
	barkTask->SetSound("snd_lostTrackOfEnemy");
	owner->GetSubsystem(SubsysCommunication)->QueueTask(barkTask);

	// The sensory system does its Idle tasks
	owner->GetSubsystem(SubsysSenses)->ClearTasks();
	owner->GetSubsystem(SubsysSenses)->QueueTask(EmptyTask::CreateInstance());

	// For now, we assume a melee combat (TODO: Ranged combat decision)
	owner->GetSubsystem(SubsysAction)->ClearTasks();
	owner->GetSubsystem(SubsysAction)->QueueTask(EmptyTask::CreateInstance());
}

StatePtr LostTrackOfEnemyState::CreateInstance()
{
	return StatePtr(new LostTrackOfEnemyState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar lostTrackOfEnemyStateRegistrar(
	STATE_LOST_TRACK_OF_ENEMY, // Task Name
	StateLibrary::CreateInstanceFunc(&LostTrackOfEnemyState::CreateInstance) // Instance creation callback
);

} // namespace ai
