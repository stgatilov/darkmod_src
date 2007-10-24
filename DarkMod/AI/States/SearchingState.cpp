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

	// ----- TODO
	// Implement as private method of this task
	// TODO subFrameTask_startNewHidingSpotSearch (self.getEyePos(), m_alertPos, m_alertSearchVolume, m_alertSearchExclusionVolume);
	
	// No current search completed that we know of
	memory.numPossibleHidingSpotsSearched = 0;
	memory.currentHidingSpotListSearchMaxDuration = -1;

	// If we are supposed to search the stimulus location do that instead of just standing around while the search
	// completes
	if (memory.stimulusLocationItselfShouldBeSearched)
	{
		// Spot search should go to a state to wait for thinking to complete
		// when done (may transition out of that instantaneously if thinking
		// is done)
		
		// TODO pushTask("task_WaitingForHidingSpotThinkingToComplete", PRIORITY_SEARCH_THINKING);
		memory.currentSearchSpot = memory.alertPos;
		
		// Determine the search duration
		// TODO: subFrameTask_determineSearchDuration();
		// TODO: pushTask("task_SearchingSpot", PRIORITY_SEARCHSPOT);
	}		
	else
	{
		// TODO: pushTask("task_WaitingForHidingSpotThinkingToComplete", PRIORITY_SEARCH_THINKING);
	}
	// ----- End TODO

	// Take the idle sensory scan task and plug it into the senses subsystem
	owner->GetSubsystem(SubsysSenses)->ClearTasks();
	owner->GetSubsystem(SubsysSenses)->QueueTask(EmptyTask::CreateInstance());

	// For now, clear the action tasks
	owner->GetSubsystem(SubsysAction)->ClearTasks();
	owner->GetSubsystem(SubsysAction)->QueueTask(EmptyTask::CreateInstance());
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
