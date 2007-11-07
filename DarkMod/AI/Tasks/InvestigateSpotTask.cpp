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

const int INVESTIGATE_SPOT_TIME_REMOTE = 600; // ms
const int INVESTIGATE_SPOT_TIME_STANDARD = 300; // ms
const int INVESTIGATE_SPOT_TIME_CLOSELY = 2500; // ms

const float MAX_TRAVEL_DISTANCE_WALKING = 300; // units?

InvestigateSpotTask::InvestigateSpotTask() :
	_investigateClosely(false)
{}

InvestigateSpotTask::InvestigateSpotTask(bool investigateClosely) :
	_investigateClosely(investigateClosely)
{}

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
		// Check if we can see the point from where we are (only for remote inspection)
		if (!_investigateClosely && owner->CanSeePositionExt(memory.currentSearchSpot, false, true))
		{
			DM_LOG(LC_AI, LT_INFO).LogVector("I can see the point...\r", memory.currentSearchSpot);

			if (!owner->CheckFOV(memory.currentSearchSpot))
			{
				// Search spot is not within FOV, turn towards the position
				owner->TurnToward(memory.currentSearchSpot);
			}

			// In any case, look at the point to investigate
			owner->Event_LookAtPosition(memory.currentSearchSpot, 2.0f);

			// Wait about half a sec.
			_exitTime = gameLocal.time + INVESTIGATE_SPOT_TIME_REMOTE*(1 + gameLocal.random.RandomFloat()*0.2f);
		}
		else 
		{
			// Let's move
			owner->MoveToPosition(memory.currentSearchSpot);

			if (!owner->AI_DEST_UNREACHABLE)
			{
				// Run if the point is more than 500 
				// greebo: This is taxing and can be replaced by a simpler distance check 
				// TravelDistance takes about ~0.1 msec on my 2.2 GHz system.
				float travelDist = owner->TravelDistance(owner->GetPhysics()->GetOrigin(), memory.currentSearchSpot);

				DM_LOG(LC_AI, LT_DEBUG).LogString("TravelDistance is %f.\r", travelDist);
				owner->AI_RUN = (travelDist > MAX_TRAVEL_DISTANCE_WALKING);
			}
		}
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
		return (gameLocal.time > _exitTime);
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

		if (_investigateClosely && !owner->AI_DEST_UNREACHABLE)
		{
			// Stop previous moves
			owner->StopMove(MOVE_STATUS_DONE);

			// We should investigate the spot closely, so kneel_down and do so
			owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_KneelDown", 6);
			owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_KneelDown", 6);

			// Wait a bit, setting _exitTime sets the lifetime of this task
			_exitTime = gameLocal.time + INVESTIGATE_SPOT_TIME_CLOSELY*(1 + gameLocal.random.RandomFloat()*0.2f);
		}
		else
		{
			// Wait a bit, setting _exitTime sets the lifetime of this task
			_exitTime = gameLocal.time + INVESTIGATE_SPOT_TIME_STANDARD*(1 + gameLocal.random.RandomFloat()*0.2f);
		}
	}
	else
	{
		// Can we already see the point? Only stop moving when the spot
		// shouldn't be investigated closely
		if (!_investigateClosely && owner->CanSeePositionExt(memory.currentSearchSpot, true, true))
		{
			DM_LOG(LC_AI, LT_INFO).LogVector("Stop, I can see the point now...\r", memory.currentSearchSpot);

			// Stop moving, we can see the point
			owner->StopMove(MOVE_STATUS_DONE);

			//Look at the point to investigate
			owner->Event_LookAtPosition(memory.currentSearchSpot, 2.0f);

			// Wait about half a sec., this sets the lifetime of this task
			_exitTime = gameLocal.time + 600*(1 + gameLocal.random.RandomFloat()*0.2f);
		}
	}

	return false; // not finished yet
}

void InvestigateSpotTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	savefile->WriteInt(_exitTime);
	savefile->WriteBool(_investigateClosely);
}

void InvestigateSpotTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	savefile->ReadInt(_exitTime);
	savefile->ReadBool(_investigateClosely);
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
