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
#include "../Tasks/InvestigateSpotTask.h"
#include "../Tasks/SingleBarkTask.h"
#include "../Library.h"
#include "IdleState.h"
#include "AgitatedSearchingState.h"
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

	if (!CheckAlertLevel(2, STATE_AGITATED_SEARCHING))
	{
		return;
	}

	// Shortcut reference
	Memory& memory = owner->GetMemory();

	// Stop moving
	owner->StopMove(MOVE_STATUS_DONE);

	// Setup a new hiding spot search
	StartNewHidingSpotSearch(owner);
	
	// No current search completed that we know of
	memory.numPossibleHidingSpotsSearched = 0;
	memory.currentHidingSpotListSearchMaxDuration = -1;

	// Clear all the ongoing tasks
	owner->GetSubsystem(SubsysSenses)->ClearTasks();
	owner->GetSubsystem(SubsysAction)->ClearTasks();
	owner->GetSubsystem(SubsysMovement)->ClearTasks();

	// If we are supposed to search the stimulus location do that instead 
	// of just standing around while the search completes
	if (memory.stimulusLocationItselfShouldBeSearched)
	{
		// The InvestigateSpotTask will take this point as first hiding spot
		memory.currentSearchSpot = memory.alertPos;

		// Delegate the spot investigation to a new task, this will take the correct action.
		owner->GetSubsystem(SubsysAction)->PushTask(InvestigateSpotTask::CreateInstance());

		// Prevent overwriting this hiding spot in the upcoming Think() call
		memory.hidingSpotInvestigationInProgress = true;
	}
	else
	{
		// AI is not moving, wait for spot search to complete
		memory.hidingSpotInvestigationInProgress = false;
	}

	// No starting time yet
	memory.currentHidingSpotListSearchStartTime = -1;

	// Clear the communication system
	owner->GetSubsystem(SubsysCommunication)->ClearTasks();
	// Allocate a singlebarktask, set the sound and enqueue it
	owner->GetSubsystem(SubsysCommunication)->PushTask(
		TaskPtr(new SingleBarkTask("snd_somethingSuspicious"))
	);
}

void SearchingState::OnSubsystemTaskFinished(idAI* owner, SubsystemId subSystem)
{
	Memory& memory = owner->GetMemory();

	if (memory.hidingSpotInvestigationInProgress && subSystem == SubsysAction)
	{
		// The action subsystem has finished investigating its task, set the
		// boolean back to false, so that the next spot can be chosen
		memory.hidingSpotInvestigationInProgress = false;
	}
}

// Gets called each time the mind is thinking
void SearchingState::Think(idAI* owner)
{
	if(!CheckAlertLevel(2, STATE_AGITATED_SEARCHING))
	{
		return;
	}


	Memory& memory = owner->GetMemory();

	// Check the maximum search duration, if there is one set
	if (memory.currentHidingSpotListSearchStartTime > 0 &&
		gameLocal.time > memory.currentHidingSpotListSearchStartTime + memory.currentHidingSpotListSearchMaxDuration)
	{
		// No more hiding spots to search
		DM_LOG(LC_AI, LT_INFO).LogString("Search time expired!\r");

		owner->StopMove(MOVE_STATUS_DONE);

		// End the state, fall back to idle if nothing left
		// TODO: wander around
		owner->Event_SetAlertLevel(owner->thresh_1 + (owner->thresh_2 - owner->thresh_1) * 0.5);
		owner->GetMind()->EndState();
		return;
	}

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
			// TODO: wander around
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

bool SearchingState::ChooseNextHidingSpotToSearch(idAI* owner)
{
	Memory& memory = owner->GetMemory();

	int numSpots = owner->m_hidingSpots.getNumSpots();
	DM_LOG(LC_AI, LT_INFO).LogString("Found hidings spots: %d\r", numSpots);

	// Choose randomly
	if (numSpots > 0)
	{
		if (memory.firstChosenHidingSpotIndex == -1)
		{
			// No hiding spot chosen yet, initialise
			/*
			// Get visual acuity
			float visAcuity = getAcuity("vis");
				
			// Since earlier hiding spots are "better" (ie closer to stimulus and darker or more occluded)
			// higher visual acuity should bias toward earlier in the list
			float bias = 1.0 - visAcuity;
			if (bias < 0.0)
			{
				bias = 0.0;
			}
			*/
			// greebo: TODO? This isn't random choosing...

			int spotIndex = 0; 

			// Remember which hiding spot we have chosen at start
			memory.firstChosenHidingSpotIndex = spotIndex;
				
			// Note currently chosen hiding spot
			memory.currentChosenHidingSpotIndex = spotIndex;
			
			// Get location
			memory.chosenHidingSpot = owner->GetNthHidingSpotLocation(spotIndex);
			memory.currentSearchSpot = memory.chosenHidingSpot;
			
			DM_LOG(LC_AI, LT_INFO).LogString(
				"First spot chosen is index %d of %d spots.\r", 
				memory.firstChosenHidingSpotIndex, numSpots
			);
		}
		else 
		{
			// First hiding spot index is valid, so get the next one
			// TODO: Copy from task_IteratingHidingSpotSearch
			memory.numPossibleHidingSpotsSearched++;

			// Make sure we stay in bounds
			memory.currentChosenHidingSpotIndex++;
			if (memory.currentChosenHidingSpotIndex >= numSpots)
			{
				memory.currentChosenHidingSpotIndex = 0;
			}

			// Have we wrapped around to first one searched?
			if (memory.currentChosenHidingSpotIndex == memory.firstChosenHidingSpotIndex || 
				memory.currentChosenHidingSpotIndex < 0)
			{
				// No more hiding spots
				DM_LOG(LC_AI, LT_INFO).LogString("No more hiding spots to search.\r");
				memory.hidingSpotSearchDone = false;
				memory.chosenHidingSpot = idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY);
				memory.currentChosenHidingSpotIndex = -1;
				memory.firstChosenHidingSpotIndex = -1;
				return false;
			}
			else
			{
				// Index is valid, let's acquire the position
				DM_LOG(LC_AI, LT_INFO).LogString("Next spot chosen is index %d of %d, first was %d.\r", 
					memory.currentChosenHidingSpotIndex, numSpots-1, memory.firstChosenHidingSpotIndex);

				memory.chosenHidingSpot = owner->GetNthHidingSpotLocation(memory.currentChosenHidingSpotIndex);
				memory.currentSearchSpot = memory.chosenHidingSpot;
				memory.hidingSpotSearchDone = true;
			}
		}
	}
	else
	{
		DM_LOG(LC_AI, LT_INFO).LogString("Didn't find any hiding spots near stimulus");
		memory.firstChosenHidingSpotIndex = -1;
		memory.currentChosenHidingSpotIndex = -1;
		memory.chosenHidingSpot = idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY);
		memory.currentSearchSpot = idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY);
		return false;
	}

	return true;
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
		searchTimeSpan += owner->atime2 * 0.6f;
	}

	if (owner->AI_AlertNum >= owner->thresh_3)
	{
		searchTimeSpan += owner->atime3 * 0.5f;
	}

	// Randomize duration by up to 20% increase
	memory.currentHidingSpotListSearchMaxDuration = 
		SEC2MS(searchTimeSpan * (1 + gameLocal.random.RandomFloat()*0.2f));

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
