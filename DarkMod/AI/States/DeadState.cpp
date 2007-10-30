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

static bool init_version = FileVersionList("$Id: DeadState.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "DeadState.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

// Get the name of this state
const idStr& DeadState::GetName() const
{
	static idStr _name(STATE_DEAD);
	return _name;
}

void DeadState::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO).LogString("DeadState initialised.\r");
	assert(owner);

	// Stop move!
	owner->StopMove(MOVE_STATUS_DONE);

	owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Death", 0);
	owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Death", 0);

	// greebo: Set the waitstate, this gets cleared by 
	// the script function when the animation is done.
	owner->SetWaitState("dead");

	// Don't do anything else, the death animation will finish in a few frames
	// and the AI is done afterwards.

	// Clear all the subsystems
	owner->GetSubsystem(SubsysMovement)->ClearTasks();
	owner->GetSubsystem(SubsysSenses)->ClearTasks();
	owner->GetSubsystem(SubsysAction)->ClearTasks();
	owner->GetSubsystem(SubsysCommunication)->ClearTasks();
}

// Gets called each time the mind is thinking
void DeadState::Think(idAI* owner)
{
	// Do nothing
}

StatePtr DeadState::CreateInstance()
{
	return StatePtr(new DeadState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar deadStateRegistrar(
	STATE_DEAD, // Task Name
	StateLibrary::CreateInstanceFunc(&DeadState::CreateInstance) // Instance creation callback
);

} // namespace ai
