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

static bool init_version = FileVersionList("$Id: ReactingToStimulusState.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "ReactingToStimulusState.h"
#include "SearchingState.h"
#include "../Memory.h"
#include "../Tasks/EmptyTask.h"
#include "../Library.h"

namespace ai
{

// Get the name of this state
const idStr& ReactingToStimulusState::GetName() const
{
	static idStr _name(STATE_REACTING_TO_STIMULUS);
	return _name;
}

void ReactingToStimulusState::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO).LogString("ReactingToStimulusState initialised.\r");
	assert(owner);

	// Shortcut reference
	Memory& memory = owner->GetMemory();

	// Look to the alert position
	owner->Event_LookAtPosition(memory.alertPos, 2.0f);
}

// Gets called each time the mind is thinking
void ReactingToStimulusState::Think(idAI* owner)
{
	// Get a shortcut reference
	Memory& memory = owner->GetMemory();

	// Let the mind check its senses (TRUE = process new stimuli)
	owner->GetMind()->PerformSensoryScan(true);

	if (owner->AI_AlertNum >= owner->thresh_combat)
	{
		owner->GetMind()->PerformCombatCheck();
	}
	else if (owner->AI_AlertNum >= owner->thresh_2)
	{
		// Let the AI stop, before going into search mode
		owner->StopMove(MOVE_STATUS_DONE);

		// Look at the location of the alert stimulus. 
		owner->Event_LookAtPosition(memory.alertPos, 3.0f);

		// Turn to it if we are really agitated (body preparedness for combat)
		if (owner->AI_AlertNum >= owner->thresh_3)
		{
			owner->Event_TurnToPos(memory.alertPos);
		}

		// Switch to searching mode, this will take care of the details
		owner->GetMind()->SwitchStateIfHigherPriority(STATE_SEARCHING, PRIORITY_SEARCHING);
	}
	else if (owner->AI_AlertNum <= owner->thresh_1)
	{
		// Fallback to idle, but with increased alertness
		owner->Event_SetAlertLevel(owner->thresh_1 * 0.5f);

		owner->GetMind()->EndState();
	}
}

StatePtr ReactingToStimulusState::CreateInstance()
{
	return StatePtr(new ReactingToStimulusState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar reactingToStimulusStateRegistrar(
	STATE_REACTING_TO_STIMULUS, // Task Name
	StateLibrary::CreateInstanceFunc(&ReactingToStimulusState::CreateInstance) // Instance creation callback
);

} // namespace ai
