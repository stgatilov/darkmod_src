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
#include "WaitTask.h"
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

	_exitTime = -1;

	// Get a shortcut reference
	Memory& memory = owner->GetMemory();

	// Stop previous moves
	owner->StopMove(MOVE_STATUS_DONE);

	if (memory.currentSearchSpot != idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY))
	{
		if (owner->CanSeePositionExt(memory.currentSearchSpot, false, true))
		{
			DM_LOG(LC_AI, LT_DEBUG).LogVector("I can see the point...\r", memory.currentSearchSpot);

			if (!owner->CheckFOV(memory.currentSearchSpot))
			{
				// Search spot is not within FOV, turn towards the position
				owner->TurnToward(memory.currentSearchSpot);
			}

			// In any case, look at the point to investigate
			owner->Event_LookAtPosition(memory.currentSearchSpot, 2.0f);

			// Wait about half a sec.
			_exitTime = gameLocal.time + 600*(1 + gameLocal.random.RandomFloat()*0.2f);
		}
		else 
		{
			// Let's move
			owner->MoveToPosition(memory.currentSearchSpot);
		}

		// AI_MOVE_DONE and AI_DEST_UNREACHABLE flags are checked in Perform()
	}
	else
	{
		// Invalid hiding spot, terminate task
		DM_LOG(LC_AI, LT_DEBUG).LogString("memory.currentSearchSpot not set to something valid, terminating task.\r");
		subsystem.FinishTask();
	}
}

bool InvestigateSpotTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("InvestigateSpotTask performing.\r");

	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	if (_exitTime > 0)
	{
		// Return TRUE if the time is over, else FALSE (continue)
		if (gameLocal.time > _exitTime)
		{
			DM_LOG(LC_AI, LT_INFO).LogString("ExitTime has passed: %d!\r", gameLocal.time);
			return true;
		}
		else
		{
			return false;
		}
	}
	
	// No exit time set, continue with ordinary process
	Memory& memory = owner->GetMemory();

	if (owner->AI_MOVE_DONE)
	{
		if (owner->AI_DEST_UNREACHABLE)
		{
			DM_LOG(LC_AI, LT_INFO).LogVector("Hiding spot unreachable.\r", memory.currentSearchSpot);
		}

		DM_LOG(LC_AI, LT_INFO).LogVector("Hiding spot investigated: \r", memory.currentSearchSpot);

		// Move is done
		return true;
	}
	else
	{
		// Still moving, check distance
		float dist = (memory.currentSearchSpot - owner->GetEyePosition()).LengthSqr();

		// For distances larger than 300, set run to TRUE
		owner->AI_RUN = (dist > 90000); // dist > 300^2
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
