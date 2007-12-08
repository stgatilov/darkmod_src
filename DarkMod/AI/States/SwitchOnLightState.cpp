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

static bool init_version = FileVersionList("$Id: SwitchOnLightState.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "SwitchOnLightState.h"
#include "../Memory.h"

namespace ai
{

SwitchOnLightState::SwitchOnLightState()
{}

SwitchOnLightState::SwitchOnLightState(idEntity* stimSource)
{
	_stimSource = stimSource;
}



// Get the name of this state
const idStr& SwitchOnLightState::GetName() const
{
	static idStr _name(STATE_SWITCH_ON_LIGHT);
	return _name;
}

void SwitchOnLightState::Init(idAI* owner)
{
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO).LogString("SwitchOnLightState initialised.\r");
	assert(owner);

	// Shortcut reference
	Memory& memory = owner->GetMemory();
	idEntity* stimSource = _stimSource.GetEntity();

	idStr lightType = stimSource->spawnArgs.GetString(AIUSE_LIGHTTYPE_KEY);


	if (lightType == AIUSE_LIGHTTYPE_TORCH)
	{

	}




	// Fill the subsystems with their tasks

	// The movement subsystem should wait half a second before starting to run
	owner->GetSubsystem(SubsysMovement)->ClearTasks();


//	owner->GetSubsystem(SubsysMovement)->PushTask(MoveToPositionTask);

	owner->GetSubsystem(SubsysCommunication)->ClearTasks();

	// The sensory system 
	owner->GetSubsystem(SubsysSenses)->ClearTasks();

	// No action
	owner->GetSubsystem(SubsysAction)->ClearTasks();


}

// Gets called each time the mind is thinking
void SwitchOnLightState::Think(idAI* owner)
{
	// Shortcut reference
	Memory& memory = owner->GetMemory();

}

void SwitchOnLightState::Save(idSaveGame* savefile) const
{
	State::Save(savefile);

	_stimSource.Save(savefile);
}

void SwitchOnLightState::Restore(idRestoreGame* savefile)
{
	State::Restore(savefile);

	_stimSource.Restore(savefile);
}

StatePtr SwitchOnLightState::CreateInstance()
{
	return StatePtr(new SwitchOnLightState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar switchOnLightStateRegistrar(
	STATE_SWITCH_ON_LIGHT, // Task Name
	StateLibrary::CreateInstanceFunc(&SwitchOnLightState::CreateInstance) // Instance creation callback
);

} // namespace ai
