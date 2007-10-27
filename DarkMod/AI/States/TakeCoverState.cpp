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

static bool init_version = FileVersionList("$Id: TakeCoverState.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "TakeCoverState.h"
#include "../Memory.h"
#include "../../AIComm_Message.h"
#include "../Tasks/EmptyTask.h"
#include "../Tasks/SingleBarkTask.h"
#include "../Tasks/CombatSensoryTask.h"
#include "../Tasks/ThrowObjectTask.h"
#include "../Tasks/MoveToCoverTask.h"
#include "../Library.h"

namespace ai
{

// Get the name of this state
const idStr& TakeCoverState::GetName() const
{
	static idStr _name(STATE_TAKE_COVER);
	return _name;
}

void TakeCoverState::Init(idAI* owner)
{
	DM_LOG(LC_AI, LT_INFO).LogString("TakeCoverState initialised.\r");
	assert(owner);

	// Shortcut reference
	Memory& memory = owner->GetMind()->GetMemory();


	// Fill the subsystems with their tasks

	// The movement subsystem should start running to the last enemy position
	owner->GetSubsystem(SubsysMovement)->ClearTasks();
	owner->GetSubsystem(SubsysMovement)->QueueTask(MoveToCoverTask::CreateInstance());

	// The communication system is barking 
	owner->GetSubsystem(SubsysCommunication)->ClearTasks();


	// The sensory system does its Combat Sensory tasks
	owner->GetSubsystem(SubsysSenses)->ClearTasks();
//	owner->GetSubsystem(SubsysSenses)->QueueTask(CombatSensoryTask::CreateInstance());

	// Object throwing
	owner->GetSubsystem(SubsysAction)->ClearTasks();
}

StatePtr TakeCoverState::CreateInstance()
{
	return StatePtr(new TakeCoverState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar takeCoverStateRegistrar(
	STATE_TAKE_COVER, // Task Name
	StateLibrary::CreateInstanceFunc(&TakeCoverState::CreateInstance) // Instance creation callback
);

} // namespace ai
