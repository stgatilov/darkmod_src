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
#include "../Tasks/EmptyTask.h"
#include "../Tasks/SingleBarkTask.h"
#include "../Tasks/CombatSensoryTask.h"
#include "../Tasks/ThrowObjectTask.h"
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
	DM_LOG(LC_AI, LT_INFO).LogString("UnreachableTargetState initialised.\r");
	assert(owner);

	// Shortcut reference
	Memory& memory = owner->GetMind()->GetMemory();

	owner->DrawWeapon();

	// Issue a communication stim
	owner->IssueCommunication_Internal(
		static_cast<float>(CAIComm_Message::RequestForMissileHelp_CommType), 
		YELL_STIM_RADIUS, 
		NULL,
		owner->GetEnemy(),
		memory.lastEnemyPos
	);

	// Fill the subsystems with their tasks

	// The movement subsystem should start running to the last enemy position
	owner->GetSubsystem(SubsysMovement)->ClearTasks();

	// The communication system is barking 
	owner->GetSubsystem(SubsysCommunication)->ClearTasks();

	SingleBarkTaskPtr barkTask = SingleBarkTask::CreateInstance();
	barkTask->SetSound("snd_cantReachTarget");
	owner->GetSubsystem(SubsysCommunication)->QueueTask(barkTask);

	// The sensory system does its Combat Sensory tasks
	owner->GetSubsystem(SubsysSenses)->ClearTasks();
	owner->GetSubsystem(SubsysSenses)->QueueTask(CombatSensoryTask::CreateInstance());

	// Object throwing
	owner->GetSubsystem(SubsysAction)->ClearTasks();
	owner->GetSubsystem(SubsysAction)->QueueTask(ThrowObjectTask::CreateInstance());
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
