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
	if (owner->AI_AlertIndex < 3)
	{
		// Alert index is too low for this state, fall back
		owner->Event_CloseHidingSpotSearch();
		owner->GetMind()->EndState();
		return false;
	}
	else if (owner->AI_AlertIndex > 3 && owner->GetMind()->PerformCombatCheck())
	{
		// Alert index is too high, switch to the higher State
		owner->GetMind()->PushState(STATE_COMBAT);
		return false;
	}

	// Alert Index is matching, return OK
	return true;
}

void AgitatedSearchingState::Init(idAI* owner)
{
	// Init base classes first (this also calls CheckAlertLevel)
	SearchingState::Init(owner);

	DM_LOG(LC_AI, LT_INFO).LogString("AgitatedSearchingState initialised.\r");
	assert(owner);

	_alertLevelDecreaseRate = (owner->thresh_combat - owner->thresh_3) / owner->atime3;
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
