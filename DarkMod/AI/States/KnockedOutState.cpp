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

static bool init_version = FileVersionList("$Id: KnockedOutState.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "KnockedOutState.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

// Get the name of this state
const idStr& KnockedOutState::GetName() const
{
	static idStr _name(STATE_KNOCKED_OUT);
	return _name;
}

void KnockedOutState::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO).LogString("KnockedOutState initialised.\r");
	assert(owner);

	// Stop move!
	owner->StopMove(MOVE_STATUS_DONE);

	owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_KO", 0);
	owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_KO", 0);

	// greebo: Set the waitstate, this gets cleared by 
	// the script function when the animation is done.
	owner->SetWaitState("knockedout");

	// Don't do anything else, the KO animation will finish in a few frames
	// and the AI is done afterwards.

	// Clear all the subsystems
	owner->GetSubsystem(SubsysMovement)->ClearTasks();
	owner->GetSubsystem(SubsysSenses)->ClearTasks();
	owner->GetSubsystem(SubsysAction)->ClearTasks();
	owner->GetSubsystem(SubsysCommunication)->ClearTasks();
}

// Gets called each time the mind is thinking
void KnockedOutState::Think(idAI* owner)
{
	// Do nothing
}

StatePtr KnockedOutState::CreateInstance()
{
	return StatePtr(new KnockedOutState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar knockedOutStateRegistrar(
	STATE_KNOCKED_OUT, // Task Name
	StateLibrary::CreateInstanceFunc(&KnockedOutState::CreateInstance) // Instance creation callback
);

} // namespace ai
