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

	// Setup a new hiding spot search
	StartNewHidingSpotSearch(owner);
	
	// No current search completed that we know of
	memory.numPossibleHidingSpotsSearched = 0;
	memory.currentHidingSpotListSearchMaxDuration = -1;

	// Clear out any pending sensory tasks
	owner->GetSubsystem(SubsysSenses)->ClearTasks();

	// If we are supposed to search the stimulus location do that instead 
	// of just standing around while the search completes
	if (memory.stimulusLocationItselfShouldBeSearched)
	{
		// Spot search should go to a state to wait for thinking to complete
		// when done (may transition out of that instantaneously if thinking
		// is done)

		// Enqueue a InvestigateSpot Task before the SearchTask
		memory.currentSearchSpot = memory.alertPos;
		
		// TODO pushTask("task_WaitingForHidingSpotThinkingToComplete", PRIORITY_SEARCH_THINKING);
		
		// Determine the search duration
		// TODO: subFrameTask_determineSearchDuration();
		// TODO: pushTask("task_SearchingSpot", PRIORITY_SEARCHSPOT);
	}
	else
	{
		// TODO: pushTask("task_WaitingForHidingSpotThinkingToComplete", PRIORITY_SEARCH_THINKING);
	}
	// ----- End TODO

	// Pass control to the SearchTask which will keep track of the hiding spot search
	owner->GetSubsystem(SubsysSenses)->QueueTask(SearchTask::CreateInstance());

	// For now, clear the action tasks
	owner->GetSubsystem(SubsysAction)->ClearTasks();
	owner->GetSubsystem(SubsysAction)->QueueTask(EmptyTask::CreateInstance());
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

	// Invalidate the vector
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

		// Moved: ChooseFirstHidingSpotToSearch(owner);
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
