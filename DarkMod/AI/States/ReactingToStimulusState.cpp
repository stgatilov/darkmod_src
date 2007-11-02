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
#include "../Memory.h"
#include "../Tasks/EmptyTask.h"
#include "../Tasks/StimulusSensoryTask.h"
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
	Memory& memory = owner->GetMind()->GetMemory();

	// Look to the alert position
	owner->Event_LookAtPosition(memory.alertPos, 2.0f);

	// Take the stimulus sensory scan task and plug it into the senses subsystem
	owner->GetSubsystem(SubsysSenses)->ClearTasks();
	owner->GetSubsystem(SubsysSenses)->PushTask(StimulusSensoryTask::CreateInstance());

	// For now, clear the action tasks
	owner->GetSubsystem(SubsysAction)->ClearTasks();
}

// Gets called each time the mind is thinking
void ReactingToStimulusState::Think(idAI* owner)
{

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
