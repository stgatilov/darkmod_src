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
#include "InvestigateSpotTask.h"
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

	owner->GetMemory().hidingSpotInvestigationInProgress = false;
}

bool SearchTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("SearchTask performing.\r");

	idAI* owner = _owner.GetEntity();
	
	// This task may not be performed with empty entity pointers
	assert(owner != NULL);

	// Get a shortcut reference
	Memory& memory = owner->GetMemory();

	// Let the mind check its senses (TRUE = process new stimuli)
	owner->GetMind()->PerformSensoryScan(true);

	if (memory.hidingSpotInvestigationInProgress)
	{
		// AI is currently searching, perform some tasks in the meantime?
		DM_LOG(LC_AI, LT_INFO).LogString("Moving to hiding spot...\r");
	}
	else if (memory.hidingSpotSearchDone)
	{
		// Spot search and investigation done, choose a hiding spot
		// Try to get a first hiding spot
		/*if (!ChooseNextHidingSpotToSearch(owner))
		{
			// No more hiding spots to search
			DM_LOG(LC_AI, LT_INFO).LogString("No more hiding spots!\r");

			if (owner->m_hidingSpots.getNumSpots() > 0)
			{
				// Number of hiding spot is greater than zero, so we
				// came here after the search has been finished

				// Rub neck
				// Bark
				// Wait
			}

			// Fall back into idle mode
			owner->GetMind()->SwitchState(STATE_IDLE);

			// TODO: decrease alert level?

			return true; // finish this task
		}

		//gameRenderWorld->DebugArrow(colorBlue, owner->GetEyePosition(), memory.currentSearchSpot, 1, 2000);

		// Enqueue an InvestigateSpot task which should fall back to this one
		owner->GetSubsystem(SubsysAction)->ClearTasks();
		owner->GetSubsystem(SubsysAction)->PushTask(InvestigateSpotTask::CreateInstance());

		// Prevent falling into the same hole twice
		memory.hidingSpotInvestigationInProgress = true;*/
	}
	else
	{
		// Hiding spot search not yet done, wait...
		DM_LOG(LC_AI, LT_INFO).LogString("Hiding spot search not yet done...\r");
	}

	return false; // not finished yet
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
