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

static bool init_version = FileVersionList("$Id: FleeState.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "FleeState.h"
#include "../Memory.h"
#include "../../AIComm_Message.h"
#include "../Library.h"
#include "../Tasks/WaitTask.h"
#include "../Tasks/FleeTask.h"
#include "../Tasks/RepeatedBarkTask.h"
#include "FleeDoneState.h"

namespace ai
{

// Get the name of this state
const idStr& FleeState::GetName() const
{
	static idStr _name(STATE_FLEE);
	return _name;
}

void FleeState::Init(idAI* owner)
{
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO).LogString("FleeState initialised.\r");
	assert(owner);

	// Shortcut reference
	Memory& memory = owner->GetMemory();
	memory.fleeingDone = false;

	// Cry for help
	owner->IssueCommunication_Internal(
		static_cast<float>(CAIComm_Message::DetectedEnemy_CommType), 
		YELL_STIM_RADIUS, 
		NULL,
		owner->GetEnemy(),
		memory.lastEnemyPos
	);

	// Fill the subsystems with their tasks

	// The movement subsystem should wait half a second before starting to run
	owner->StopMove(MOVE_STATUS_DONE);
	owner->FaceEnemy();
	owner->GetSubsystem(SubsysMovement)->ClearTasks();
	owner->GetSubsystem(SubsysMovement)->PushTask(TaskPtr(new WaitTask(1000)));
	owner->GetSubsystem(SubsysMovement)->QueueTask(FleeTask::CreateInstance());

	// The communication system cries for help
	owner->StopSound(SND_CHANNEL_VOICE, false);
	owner->GetSubsystem(SubsysCommunication)->ClearTasks();
	owner->GetSubsystem(SubsysCommunication)->PushTask(TaskPtr(new WaitTask(200)));
	// Placeholder, replace with "snd_flee" when available
	owner->GetSubsystem(SubsysCommunication)->PushTask(TaskPtr(new RepeatedBarkTask("snd_flee", 4000,8000)));

	// The sensory system 
	owner->GetSubsystem(SubsysSenses)->ClearTasks();

	// No action
	owner->GetSubsystem(SubsysAction)->ClearTasks();

	// Play the surprised animation
	owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Surprise", 5);
	owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Surprise", 5);
	owner->SetWaitState("surprise");

}

// Gets called each time the mind is thinking
void FleeState::Think(idAI* owner)
{
	// Shortcut reference
	Memory& memory = owner->GetMemory();

	if (memory.fleeingDone)
	{
		owner->GetMind()->SwitchState(STATE_FLEE_DONE);
	}
}


StatePtr FleeState::CreateInstance()
{
	return StatePtr(new FleeState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar fleeStateRegistrar(
	STATE_FLEE, // Task Name
	StateLibrary::CreateInstanceFunc(&FleeState::CreateInstance) // Instance creation callback
);

} // namespace ai
