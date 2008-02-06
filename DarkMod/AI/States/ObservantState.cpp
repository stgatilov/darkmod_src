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

static bool init_version = FileVersionList("$Id: ObservantState.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "ObservantState.h"
#include "../Memory.h"
#include "../../AIComm_Message.h"
#include "../Tasks/RandomHeadturnTask.h"
#include "../Tasks/SingleBarkTask.h"
#include "ObservantState.h"
#include "SuspiciousState.h"
#include "../Library.h"

namespace ai
{

// Get the name of this state
const idStr& ObservantState::GetName() const
{
	static idStr _name(STATE_OBSERVANT);
	return _name;
}

bool ObservantState::CheckAlertLevel(idAI* owner)
{
	if (owner->AI_AlertIndex < 1)
	{
		// Alert index is too low for this state, fall back
		owner->GetMind()->EndState();
		return false;
	}
	else if (owner->AI_AlertIndex > 1)
	{
		// Alert index is too high, switch to the higher State
		owner->GetMind()->PushState(STATE_SUSPICIOUS);
		return false;
	}

	// Alert Index is matching, return OK
	return true;
}

void ObservantState::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO).LogString("ObservantState initialised.\r");
	assert(owner);

	_alertLevelDecreaseRate = (owner->thresh_2 - owner->thresh_1) / owner->atime1;

	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner)) return;

	// Shortcut reference
	Memory& memory = owner->GetMemory();

	// barking
	idStr bark;

	if (owner->AlertIndexIncreased())
	{
		if (memory.alertClass == EAlertVisual)
		{
			bark = "snd_alert1s";
		}
		else if (memory.alertClass == EAlertAudio)
		{
			bark = "snd_alert1h";
		}
		else
		{
			bark = "snd_alert1";
		}
		owner->GetSubsystem(SubsysCommunication)->PushTask(
				TaskPtr(new SingleBarkTask(bark))
			);
	}
	else
	{
		bark = "snd_alertdown1";
		owner->GetSubsystem(SubsysCommunication)->ClearTasks();
		owner->GetSubsystem(SubsysCommunication)->PushTask(
				TaskPtr(new SingleBarkTask(bark))
			);
	}
}

// Gets called each time the mind is thinking
void ObservantState::Think(idAI* owner)
{
	UpdateAlertLevel();
	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner)) return;
	
	// Let the mind check its senses (TRUE = process new stimuli)
	owner->GetMind()->PerformSensoryScan(true);
}

StatePtr ObservantState::CreateInstance()
{
	return StatePtr(new ObservantState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar observantStateRegistrar(
	STATE_OBSERVANT, // Task Name
	StateLibrary::CreateInstanceFunc(&ObservantState::CreateInstance) // Instance creation callback
);

} // namespace ai
