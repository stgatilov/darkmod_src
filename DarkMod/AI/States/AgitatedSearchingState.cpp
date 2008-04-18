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

static bool init_version = FileVersionList("$Id: AgitatedSearchingState.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "AgitatedSearchingState.h"
#include "../Memory.h"
#include "../Tasks/InvestigateSpotTask.h"
#include "../Tasks/SingleBarkTask.h"
#include "../Tasks/RepeatedBarkTask.h"
#include "../Tasks/WaitTask.h"
#include "CombatState.h"
#include "../Library.h"
#include "../../idAbsenceMarkerEntity.h"
#include "../../AIComm_Message.h"

namespace ai
{

// Get the name of this state
const idStr& AgitatedSearchingState::GetName() const
{
	static idStr _name(STATE_AGITATED_SEARCHING);
	return _name;
}

bool AgitatedSearchingState::CheckAlertLevel(idAI* owner)
{
	if (owner->AI_AlertIndex < 4)
	{
		// Alert index is too low for this state, fall back
		owner->GetMind()->EndState();
		return false;
	}
	else if (owner->AI_AlertIndex > 4)
	{
		// Alert index is too high, switch to the higher State
		owner->Event_CloseHidingSpotSearch();
		owner->GetMind()->PushState(STATE_COMBAT);
		return false;
	}

	// Alert Index is matching, return OK
	return true;
}

void AgitatedSearchingState::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO).LogString("AgitatedSearchingState initialised.\r");
	assert(owner);

	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner)) return;

	// Shortcut reference
	Memory& memory = owner->GetMemory();

	float alertTime = owner->atime4 + owner->atime4_fuzzyness * (gameLocal.random.RandomFloat() - 0.5);
	_alertLevelDecreaseRate = (owner->thresh_5 - owner->thresh_4) / alertTime;

	// Setup a new hiding spot search
	StartNewHidingSpotSearch(owner);

	if (owner->AlertIndexIncreased())
	{
		if (memory.alertType == EAlertTypeEnemy)
		{
			idStr bark;
			if (memory.alertClass == EAlertVisual)
			{
				bark = "snd_alert4s";
			}
			else
			{
				bark = "snd_alert4";
			}

			owner->GetSubsystem(SubsysCommunication)->ClearTasks();
			owner->GetSubsystem(SubsysCommunication)->PushTask(
				TaskPtr(new SingleBarkTask(bark))
			);
		}
	}
	owner->GetSubsystem(SubsysCommunication)->QueueTask(
		TaskPtr(new WaitTask(5000))
		);

	int minTime = SEC2MS(owner->spawnArgs.GetFloat("searchbark_delay_min", "10"));
	int maxTime = SEC2MS(owner->spawnArgs.GetFloat("searchbark_delay_max", "15"));

	if (owner->HasSeenEvidence())
	{
		owner->GetSubsystem(SubsysCommunication)->QueueTask(
				TaskPtr(new RepeatedBarkTask("snd_state4SeenEvidence", minTime, maxTime))
			);
	}
	else
	{
		owner->GetSubsystem(SubsysCommunication)->QueueTask(
				TaskPtr(new RepeatedBarkTask("snd_state4SeenNoEvidence", minTime, maxTime))
			);
	}


	
	owner->DrawWeapon();
}

// Gets called each time the mind is thinking
void AgitatedSearchingState::Think(idAI* owner)
{
	SearchingState::Think(owner);
}

StatePtr AgitatedSearchingState::CreateInstance()
{
	return StatePtr(static_cast<State*>(new AgitatedSearchingState));
}

// Register this state with the StateLibrary
StateLibrary::Registrar agitatedSearchingStateRegistrar(
	STATE_AGITATED_SEARCHING, // Task Name
	StateLibrary::CreateInstanceFunc(&AgitatedSearchingState::CreateInstance) // Instance creation callback
);

} // namespace ai
