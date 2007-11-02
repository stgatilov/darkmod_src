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
#include "../Tasks/SingleBarkTask.h"
#include "../Library.h"
#include "../../idAbsenceMarkerEntity.h"
#include "../../AIComm_Message.h"

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
	// Init base class first
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO).LogString("SearchingState initialised.\r");
	assert(owner);

	// Shortcut reference
	Memory& memory = owner->GetMemory();

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

	// Clear the communication system
	owner->GetSubsystem(SubsysCommunication)->ClearTasks();
	// Allocate a singlebarktask, set the sound and enqueue it
	owner->GetSubsystem(SubsysCommunication)->PushTask(
		TaskPtr(new SingleBarkTask("snd_somethingSuspicious"))
	);
	
	// Pass control to the SearchTask which will keep track of the hiding spot search
	owner->GetSubsystem(SubsysSenses)->ClearTasks();
	owner->GetSubsystem(SubsysSenses)->PushTask(SearchTask::CreateInstance());

	// For now, clear the action tasks
	owner->GetSubsystem(SubsysAction)->ClearTasks();

	// The SearchTask is responsible of controlling the movement subsystem
	owner->GetSubsystem(SubsysMovement)->ClearTasks();
}

// Gets called each time the mind is thinking
void SearchingState::Think(idAI* owner)
{
	Memory& memory = owner->GetMemory();

	// Do we have an ongoing hiding spot search?
	if (!memory.hidingSpotSearchDone)
	{
		// Let the hiding spot search do its task
		PerformHidingSpotSearch(owner);
	}
}

void SearchingState::StartNewHidingSpotSearch(idAI* owner)
{
	Memory& memory = owner->GetMemory();

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

void SearchingState::PerformHidingSpotSearch(idAI* owner)
{
	// Shortcut reference
	Memory& memory = owner->GetMemory();

	// Increase the frame count
	memory.hidingSpotThinkFrameCount++;

	if (owner->ContinueSearchForHidingSpots() == 0)
	{
		// search completed
		memory.hidingSpotSearchDone = true;

		// Hiding spot test is done
		memory.hidingSpotTestStarted = false;
		
		// Here we transition to the state for handling the behavior of
		// the AI once it thinks it knows where the stimulus may have
		// come from
		
		// For now, starts searching for the stimulus.  We probably want
		// a way for different AIs to act differently
		// TODO: Morale check etc...

		// Determine the search duration
		DetermineSearchDuration(owner);

		// Yell that you noticed something if you are responding directly to a stimulus
		if (!memory.searchingDueToCommunication)
		{
			owner->IssueCommunication_Internal(
				CAIComm_Message::DetectedSomethingSuspicious_CommType, 
				YELL_STIM_RADIUS, 
				NULL,
				NULL,
				memory.alertPos
			);
		}

		// Get location
		memory.chosenHidingSpot = owner->GetNthHidingSpotLocation(memory.currentChosenHidingSpotIndex);

		// Set time search is starting
		memory.currentHidingSpotListSearchStartTime = gameLocal.time;
	}
}

int SearchingState::DetermineSearchDuration(idAI* owner)
{
	Memory& memory = owner->GetMemory();

	// Determine how much time we should spend searching based on the alert times
	int searchTimeSpan(0);

	if (owner->AI_AlertNum >= owner->thresh_1)
	{
		searchTimeSpan += owner->atime1;
	}

	if (owner->AI_AlertNum >= owner->thresh_2)
	{
		searchTimeSpan += owner->atime2;
	}

	if (owner->AI_AlertNum >= owner->thresh_3)
	{
		searchTimeSpan += owner->atime3;
	}

	// Randomize duration by up to 20% increase
	memory.currentHidingSpotListSearchMaxDuration = 
		SEC2MS(searchTimeSpan + (searchTimeSpan * gameLocal.random.RandomFloat()*0.2f));

	DM_LOG(LC_AI, LT_INFO).LogString("Search duration set to %d msec\r", memory.currentHidingSpotListSearchMaxDuration);
	
	// Done
	return memory.currentHidingSpotListSearchMaxDuration;
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
