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
#include "../Library.h"
#include "../Tasks/WaitTask.h"
#include "../Tasks/FleeTask.h"
#include "../Tasks/IdleSensoryTask.h"

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
	Memory& memory = owner->GetMind()->GetMemory();

	owner->FaceEnemy();
	// Fill the subsystems with their tasks

	// The movement subsystem should wait half a second before starting to run
	owner->GetSubsystem(SubsysMovement)->ClearTasks();
		
//	owner->GetSubsystem(SubsysMovement)->PushTask(taskPtr(new WaitTask(500)));
	owner->GetSubsystem(SubsysMovement)->QueueTask(FleeTask::CreateInstance());

	// The communication system 
	owner->GetSubsystem(SubsysCommunication)->ClearTasks();
	//owner->GetSubsystem(SubsysCommunication)->QueueTask(FleeTask::CreateInstance());


	// The sensory system should do sensory scan
	owner->GetSubsystem(SubsysSenses)->ClearTasks();
//	owner->GetSubsystem(SubsysSenses)->PushTask(IdleSensoryTask::CreateInstance());

	// No action
	owner->GetSubsystem(SubsysAction)->ClearTasks();
}

// Gets called each time the mind is thinking
void FleeState::Think(idAI* owner)
{

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
