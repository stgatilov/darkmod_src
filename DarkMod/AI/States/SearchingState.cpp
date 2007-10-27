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

static bool init_version = FileVersionList("$Id: SearchingState.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "SearchingState.h"
#include "../Memory.h"
#include "../Tasks/EmptyTask.h"
#include "../Tasks/SearchTask.h"
#include "../Library.h"

namespace ai
{

// Get the name of this state
const idStr& SearchingState::GetName() const
{
	static idStr _name(STATE_SEARCHING);
	return _name;
}

void SearchingState::Init(idAI* owner)
{
	DM_LOG(LC_AI, LT_INFO).LogString("SearchingState initialised.\r");
	assert(owner);

	// Shortcut reference
	Memory& memory = owner->GetMind()->GetMemory();

	// Stop moving
	owner->StopMove(MOVE_STATUS_DONE);

	// Setup a new hiding spot search
	StartNewHidingSpotSearch(owner);
	
	// No current search completed that we know of
	memory.numPossibleHidingSpotsSearched = 0;
	memory.currentHidingSpotListSearchMaxDuration = -1;

	// If we are supposed to search the stimulus location do that instead 
	// of just standing around while the search completes
	if (memory.stimulusLocationItselfShouldBeSearched)
	{
		// The SearchTask will take this point as first hiding spot
		memory.currentSearchSpot = memory.alertPos;
		memory.hidingSpotInvestigationInProgress = true;
	}
	else
	{
		// AI is not moving, wait for spot search to complete
		memory.hidingSpotInvestigationInProgress = false;
	}
	
	// Pass control to the SearchTask which will keep track of the hiding spot search
	owner->GetSubsystem(SubsysSenses)->ClearTasks();
	owner->GetSubsystem(SubsysSenses)->PushTask(SearchTask::CreateInstance());

	// For now, clear the action tasks
	owner->GetSubsystem(SubsysAction)->ClearTasks();
	owner->GetSubsystem(SubsysAction)->PushTask(EmptyTask::CreateInstance());

	// The SearchTask is responsible of controlling the movement subsystem
	owner->GetSubsystem(SubsysMovement)->ClearTasks();
}

void SearchingState::StartNewHidingSpotSearch(idAI* owner)
{
	Memory& memory = owner->GetMind()->GetMemory();

	idVec3 minBounds(memory.alertPos - memory.alertSearchVolume);
	idVec3 maxBounds(memory.alertPos + memory.alertSearchVolume);

	idVec3 minExclusionBounds(memory.alertPos - memory.alertSearchExclusionVolume);
	idVec3 maxExclusionBounds(memory.alertPos + memory.alertSearchExclusionVolume);
	
	// Close any previous search
	owner->Event_CloseHidingSpotSearch();

	// Hiding spot test now started
	memory.hidingSpotSearchDone = false;
	memory.hidingSpotTestStarted = true;

	// Invalidate the vector, clear the indices
	memory.firstChosenHidingSpotIndex = -1;
	memory.currentChosenHidingSpotIndex = -1;
	memory.currentSearchSpot = idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY);

	// Start search
	int res = owner->StartSearchForHidingSpotsWithExclusionArea(
		owner->GetEyePosition(), 
		minBounds, maxBounds, 
		minExclusionBounds, maxExclusionBounds, 
		255, owner
	);

	if (res == 0)
	{
		// Search completed on first round
		memory.hidingSpotSearchDone = true;
	}
}

StatePtr SearchingState::CreateInstance()
{
	return StatePtr(new SearchingState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar searchingStateRegistrar(
	STATE_SEARCHING, // Task Name
	StateLibrary::CreateInstanceFunc(&SearchingState::CreateInstance) // Instance creation callback
);

} // namespace ai
