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

static bool init_version = FileVersionList("$Id: SearchTask.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "SearchTask.h"
#include "../States/IdleState.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

// Get the name of this task
const idStr& SearchTask::GetName() const
{
	static idStr _name(TASK_SEARCH);
	return _name;
}

void SearchTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);

	// nothing so far, remove me (FIXME)
}

bool SearchTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("SearchTask performing.\r");

	idAI* owner = _owner.GetEntity();
	
	// This task may not be performed with empty entity pointers
	assert(owner != NULL);

	// Get a shortcut reference
	Memory& memory = owner->GetMind()->GetMemory();

	// Let the mind check its senses (TRUE = process new stimuli)
	//owner->GetMind()->PerformSensoryScan(true);

	// Do we have a search spot already stored in the Memory?
	if (memory.currentSearchSpot != idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY))
	{
		// We have a chosen hiding spot, this means that the search is completed
		gameRenderWorld->DebugArrow(colorBlue, owner->GetEyePosition(), memory.currentSearchSpot, 1, 20);

	}
	else
	{
		// No hiding spot index chosen so far, is the search still in progress?
		if (memory.hidingSpotSearchDone)
		{
			// Try to get a first hiding spot
			ChooseFirstHidingSpotToSearch(owner);

			// If we have a valid search pool, the chosen index is positive
			if (memory.currentChosenHidingSpotIndex == -1)
			{
				// Whoops, no hiding spots?
				DM_LOG(LC_AI, LT_INFO).LogString("No hiding spots after search is done!\r");
				return true; // finish this task
			}			
		}

		// hiding spot search still running, let it finish, try again next frame
	}

	return false; // not finished yet
}

void SearchTask::ChooseFirstHidingSpotToSearch(idAI* owner)
{
	Memory& memory = owner->GetMind()->GetMemory();

	int numSpots = owner->m_hidingSpots.getNumSpots();
	DM_LOG(LC_AI, LT_INFO).LogString("Found hidings spots: %d\r", numSpots);

	// Choose randomly
	if (numSpots > 0)
	{
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
		
		DM_LOG(LC_AI, LT_INFO).LogString(
			"First spot chosen is index %d of %d spots.\r", 
			memory.firstChosenHidingSpotIndex, numSpots
		);
	}
	else
	{
		DM_LOG(LC_AI, LT_INFO).LogString("Didn't find any hiding spots near stimulus");
		memory.firstChosenHidingSpotIndex = -1;
		memory.currentChosenHidingSpotIndex = -1;
		memory.chosenHidingSpot.Zero();
	}
}

SearchTaskPtr SearchTask::CreateInstance()
{
	return SearchTaskPtr(new SearchTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar searchTaskRegistrar(
	TASK_SEARCH, // Task Name
	TaskLibrary::CreateInstanceFunc(&SearchTask::CreateInstance) // Instance creation callback
);

} // namespace ai
