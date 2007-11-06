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

void AgitatedSearchingState::Init(idAI* owner)
{
	// Init base classes first
	// Note: do NOT initialise the SearchingState base class, this
	// would trigger an infinite loop due to an inappropriate CheckAlertLevel() call.
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO).LogString("AgitatedSearchingState initialised.\r");
	assert(owner);

	// Only switch to higher (combat) state when an AI is set
	if (owner->GetMind()->PerformCombatCheck() && !CheckAlertLevel(3, STATE_COMBAT))
	{
		return;
	}

	// Setup the base class
	SetupSearch(owner);
}

// Gets called each time the mind is thinking
void AgitatedSearchingState::Think(idAI* owner)
{
	if (!CheckAlertLevel(3, STATE_COMBAT))
	{
		return;
	}

	Memory& memory = owner->GetMemory();

	// Do we have an ongoing hiding spot search?
	if (!memory.hidingSpotSearchDone)
	{
		// Let the hiding spot search do its task
		PerformHidingSpotSearch(owner);
	}
	// Is a hiding spot search in progress?
	else if (!memory.hidingSpotInvestigationInProgress)
	{
		// Pick a hiding spot and push the task

		// Spot search and investigation done, choose a hiding spot
		// Try to get a first hiding spot
		if (!ChooseNextHidingSpotToSearch(owner))
		{
			// No more hiding spots to search
			DM_LOG(LC_AI, LT_INFO).LogString("No more hiding spots!\r");

			if (owner->m_hidingSpots.getNumSpots() > 0)
			{
				// Number of hiding spot is greater than zero, so we
				// came here after the search has been finished

				// Rub neck?
				// Bark?
				// Wait?
			}

			owner->StopMove(MOVE_STATUS_DONE);

			// Fall back into the previous state
			// TODO: wander
			owner->Event_SetAlertLevel(owner->thresh_1 + (owner->thresh_2 - owner->thresh_1) * 0.5);
			owner->GetMind()->EndState();

			return; // Exit, state will be switched next frame, we're done here
		}

		// ChooseNextHidingSpot returned TRUE, so we have memory.currentSearchSpot set

		//gameRenderWorld->DebugArrow(colorBlue, owner->GetEyePosition(), memory.currentSearchSpot, 1, 2000);

		// Delegate the spot investigation to a new task, this will take the correct action.
		owner->GetSubsystem(SubsysAction)->PushTask(InvestigateSpotTask::CreateInstance());

		// Prevent falling into the same hole twice
		memory.hidingSpotInvestigationInProgress = true;
	}
	else
	{
		// Move to Hiding spot is ongoing, do additional sensory tasks here

		// Let the mind check its senses (TRUE = process new stimuli)
		owner->GetMind()->PerformSensoryScan(true);
	}
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
