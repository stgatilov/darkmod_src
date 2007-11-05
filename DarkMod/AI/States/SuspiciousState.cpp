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

static bool init_version = FileVersionList("$Id: SuspiciousState.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "SuspiciousState.h"
#include "../Memory.h"
#include "../../AIComm_Message.h"
#include "../Library.h"

namespace ai
{

// Get the name of this state
const idStr& SuspiciousState::GetName() const
{
	static idStr _name(STATE_SUSPICIOUS);
	return _name;
}

void SuspiciousState::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO).LogString("SuspiciousState initialised.\r");
	assert(owner);

	if (!CheckAlertLevel(1, "STATE_SEARCHING"))
	{
		return;
	}

	// Shortcut reference
	Memory& memory = owner->GetMemory();

	owner->GetSubsystem(SubsysMovement)->ClearTasks();
	owner->GetSubsystem(SubsysSenses)->ClearTasks();
	owner->GetSubsystem(SubsysCommunication)->ClearTasks();
	owner->GetSubsystem(SubsysAction)->ClearTasks();

}

// Gets called each time the mind is thinking
void SuspiciousState::Think(idAI* owner)
{

	if (!CheckAlertLevel(1, "STATE_SEARCHING"))
	{
		return;
	}

	Memory& memory = owner->GetMemory();


}

StatePtr SuspiciousState::CreateInstance()
{
	return StatePtr(new SuspiciousState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar suspiciousStateRegistrar(
	STATE_SUSPICIOUS, // Task Name
	StateLibrary::CreateInstanceFunc(&SuspiciousState::CreateInstance) // Instance creation callback
);

} // namespace ai
