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

static bool init_version = FileVersionList("$Id: InvestigateSpotTask.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "InvestigateSpotTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

// Get the name of this task
const idStr& InvestigateSpotTask::GetName() const
{
	static idStr _name(TASK_INVESTIGATE_SPOT);
	return _name;
}

void InvestigateSpotTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);

	// Get a shortcut reference
	Memory& memory = owner->GetMind()->GetMemory();

	// Stop previous moves
	owner->StopMove(MOVE_STATUS_DONE);

	memory.hidingSpotInvestigationInProgress = true;

	if (memory.currentSearchSpot != idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY))
	{
		// TODO AI_RUN distance check

		// Let's move
		owner->MoveToPosition(memory.currentSearchSpot);

		// AI_MOVE_DONE and AI_DEST_UNREACHABLE flags are checked in Perform()
	}
}

bool InvestigateSpotTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("InvestigateSpotTask performing.\r");

	idAI* owner = _owner.GetEntity();
	
	// This task may not be performed with empty entity pointers
	assert(owner != NULL);

	if (owner->AI_MOVE_DONE)
	{
		// Get a shortcut reference
		Memory& memory = owner->GetMind()->GetMemory();

		if (owner->AI_DEST_UNREACHABLE)
		{
			DM_LOG(LC_AI, LT_INFO).LogVector("Hiding spot unreachable.\r", memory.currentSearchSpot);
		}

		DM_LOG(LC_AI, LT_INFO).LogVector("Hiding spot investigated: \r", memory.currentSearchSpot);

		// Investigation completed
		memory.hidingSpotInvestigationInProgress = false;

		// Move is done
		return true;
	}

	return false; // not finished yet
}

InvestigateSpotTaskPtr InvestigateSpotTask::CreateInstance()
{
	return InvestigateSpotTaskPtr(new InvestigateSpotTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar investigateSpotTaskRegistrar(
	TASK_INVESTIGATE_SPOT, // Task Name
	TaskLibrary::CreateInstanceFunc(&InvestigateSpotTask::CreateInstance) // Instance creation callback
);

} // namespace ai
