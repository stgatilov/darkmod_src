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

static bool init_version = FileVersionList("$Id: EmptyState.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "EmptyState.h"
#include "../Tasks/EmptyTask.h"
#include "../Library.h"

namespace ai
{

// Get the name of this state
const idStr& EmptyState::GetName() const
{
	static idStr _name(STATE_EMPTY);
	return _name;
}

void EmptyState::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO).LogString("EmptyState initialised.\r");
	assert(owner);

	// Disable all subsystems
	owner->GetSubsystem(SubsysMovement)->ClearTasks();
	owner->GetSubsystem(SubsysCommunication)->ClearTasks();
	owner->GetSubsystem(SubsysAction)->ClearTasks();
	owner->GetSubsystem(SubsysSenses)->ClearTasks();
}

// Gets called each time the mind is thinking
void EmptyState::Think(idAI* owner)
{

}

StatePtr EmptyState::CreateInstance()
{
	return StatePtr(new EmptyState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar emptyStateRegistrar(
	STATE_EMPTY, // Task Name
	StateLibrary::CreateInstanceFunc(&EmptyState::CreateInstance) // Instance creation callback
);

} // namespace ai
