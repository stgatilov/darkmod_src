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

static bool init_version = FileVersionList("$Id: IdleState.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "IdleState.h"
#include "../Tasks/EmptyTask.h"
#include "../Tasks/IdleSensoryTask.h"
#include "../Tasks/PatrolTask.h"
#include "../Tasks/PatrolBarkTask.h"
#include "../Library.h"

namespace ai
{

// Get the name of this state
const idStr& IdleState::GetName() const
{
	static idStr _name(STATE_IDLE);
	return _name;
}

void IdleState::Init(idAI* owner)
{
	DM_LOG(LC_AI, LT_INFO).LogString("IdleState initialised.\r");
	assert(owner);

	// Fill the subsystems with their tasks

	// The movement subsystem should start patrolling
	owner->GetSubsystem(SubsysMovement)->InstallTask(PatrolTask::CreateInstance());

	// The communication system is barking in regular intervals
	owner->GetSubsystem(SubsysCommunication)->InstallTask(PatrolBarkTask::CreateInstance());

	// The sensory system does its Idle tasks
	owner->GetSubsystem(SubsysSenses)->InstallTask(IdleSensoryTask::CreateInstance());

	// No action so far
	owner->GetSubsystem(SubsysAction)->InstallTask(EmptyTask::CreateInstance());

	// Initialise the animation state
	owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", 0);
	owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Idle", 0);
}

StatePtr IdleState::CreateInstance()
{
	return StatePtr(new IdleState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar idleStateRegistrar(
	STATE_IDLE, // Task Name
	StateLibrary::CreateInstanceFunc(&IdleState::CreateInstance) // Instance creation callback
);

} // namespace ai
