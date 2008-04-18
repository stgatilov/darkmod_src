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
#include "../Tasks/RandomHeadturnTask.h"
#include "../Tasks/SingleBarkTask.h"
#include "SearchingState.h"
#include "../Library.h"

namespace ai
{

// Get the name of this state
const idStr& SuspiciousState::GetName() const
{
	static idStr _name(STATE_SUSPICIOUS);
	return _name;
}

bool SuspiciousState::CheckAlertLevel(idAI* owner)
{
	if (owner->AI_AlertIndex < 2)
	{
		// Alert index is too low for this state, fall back
		owner->GetMind()->EndState();
		return false;
	}
	else if (owner->AI_AlertIndex > 2)
	{
		// Alert index is too high, switch to the higher State
		owner->GetMind()->PushState(STATE_SEARCHING);
		return false;
	}

	// Alert Index is matching, return OK
	return true;
}

void SuspiciousState::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO).LogString("SuspiciousState initialised.\r");
	assert(owner);

	float alertTime = owner->atime2 + owner->atime2_fuzzyness * (gameLocal.random.RandomFloat() - 0.5);
	_alertLevelDecreaseRate = (owner->thresh_3 - owner->thresh_2) / alertTime;

	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner)) return;

	// Shortcut reference
	Memory& memory = owner->GetMemory();

	owner->GetSubsystem(SubsysSenses)->ClearTasks();
	owner->GetSubsystem(SubsysAction)->ClearTasks();

	owner->GetSubsystem(SubsysMovement)->ClearTasks();
	owner->StopMove(MOVE_STATUS_DONE);
	if (!owner->CheckFOV(memory.alertPos))
	{
		// Search spot is not within FOV, turn towards the position
		owner->TurnToward(memory.alertPos);
	}

	// In any case, look at the point to investigate
	owner->Event_LookAtPosition(memory.alertPos, 2.0f);

	// barking
	idStr bark;
	owner->GetSubsystem(SubsysCommunication)->ClearTasks();

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
}

// Gets called each time the mind is thinking
void SuspiciousState::Think(idAI* owner)
{
	UpdateAlertLevel();
	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner)) return;
	
	// Let the AI check its senses
	owner->PerformVisualScan();
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
