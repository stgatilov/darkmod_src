/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: angua $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: FleeDoneState.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "FleeDoneState.h"
#include "../Memory.h"
#include "../../AIComm_Message.h"
#include "../Library.h"
#include "../Tasks/WaitTask.h"
#include "../Tasks/FleeTask.h"
#include "../Tasks/RepeatedBarkTask.h"
#include "../Tasks/RandomHeadturnTask.h"
#include "../Tasks/SingleBarkTask.h"

#include "../Tasks/RandomTurningTask.h"
#include "IdleState.h"

namespace ai
{

// Get the name of this state
const idStr& FleeDoneState::GetName() const
{
	static idStr _name(STATE_FLEE_DONE);
	return _name;
}

void FleeDoneState::Init(idAI* owner)
{
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO).LogString("FleeDoneState initialised.\r");
	assert(owner);

	// Shortcut reference
	Memory& memory = owner->GetMemory();

	// greebo: At this point we should be at a presumably safe place, 
	// start looking for allies
	_searchForFriendDone = false;

	owner->GetSubsystem(SubsysSenses)->ClearTasks();
	owner->GetSubsystem(SubsysAction)->ClearTasks();

	// Slow turning for 5 seconds to look for friends
	owner->StopMove(MOVE_STATUS_DONE);
	_oldTurnRate = owner->GetTurnRate();
	owner->SetTurnRate(90);
	owner->GetSubsystem(SubsysMovement)->ClearTasks();
	owner->GetSubsystem(SubsysMovement)->PushTask(RandomTurningTask::CreateInstance());
	_turnEndTime = gameLocal.time + 5000;

	owner->GetSubsystem(SubsysSenses)->PushTask(RandomHeadturnTask::CreateInstance());
}

// Gets called each time the mind is thinking
void FleeDoneState::Think(idAI* owner)
{
	// Shortcut reference
	Memory& memory = owner->GetMemory();

	if (!_searchForFriendDone)
	{
		idActor* friendlyAI = owner->FindFriendlyAI(-1);
		if ( friendlyAI != NULL)
		{
			// We found a friend, cry for help to him
			DM_LOG(LC_AI, LT_INFO).LogString("Found friendly AI %s \r", friendlyAI->name.c_str());

			_searchForFriendDone = true;
			owner->GetSubsystem(SubsysMovement)->ClearTasks();
			owner->SetTurnRate(_oldTurnRate);

			owner->TurnToward(friendlyAI->GetPhysics()->GetOrigin());
			float distanceToFriend = (friendlyAI->GetPhysics()->GetOrigin() - owner->GetPhysics()->GetOrigin()).LengthFast();

			// Cry for help
			owner->GetSubsystem(SubsysCommunication)->ClearTasks();
			// Placeholder, replace with "snd_help" when available
			owner->GetSubsystem(SubsysCommunication)->PushTask(TaskPtr(new SingleBarkTask("snd_somethingSuspicious")));
			owner->IssueCommunication_Internal(
				static_cast<float>(CAIComm_Message::DetectedEnemy_CommType), 
				distanceToFriend*1.2, 
				friendlyAI,
				owner->GetEnemy(),
				owner->lastVisibleEnemyPos
			);

			// Go back to suspicious, 
			// wait some time before going back to idle
			owner->SetAlertLevel(owner->thresh_3 + (owner->thresh_3 - owner->thresh_2) * 0.5);
			owner->GetSubsystem(SubsysAction)->PushTask(TaskPtr(new WaitTask(10000)));
			owner->GetSubsystem(SubsysSenses)->PushTask(RandomHeadturnTask::CreateInstance());
			return;
		}
	
		else if (gameLocal.time >= _turnEndTime)
		{
			// We didn't find a friend, stop looking for them after some time
			_searchForFriendDone = true;
			owner->GetSubsystem(SubsysMovement)->ClearTasks();
			owner->SetTurnRate(_oldTurnRate);

			// Go back to suspicious (higher level since we didn't find someone to help)
			// wait some time before going back to idle
			owner->SetAlertLevel(owner->thresh_2 + (owner->thresh_3 - owner->thresh_2) * 0.9);

			// Play the cowering animation
			owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Cower", 4);
			owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Cower", 4);

			owner->GetSubsystem(SubsysAction)->PushTask(TaskPtr(new WaitTask(60000)));
			
			return;
		}
	}
	else
	{
		// Let the AI check its senses
		owner->PerformVisualScan();
		if (owner->AI_ALERTED)
		{
			owner->GetMind()->EndState();
		}
	}
}

void FleeDoneState::OnSubsystemTaskFinished(idAI* owner, SubsystemId subSystem)
{
	if (subSystem == SubsysAction)
	{
		owner->GetMind()->EndState();
	}
}

// Save/Restore methods
void FleeDoneState::Save(idSaveGame* savefile) const
{
	State::Save(savefile);

	savefile->WriteFloat(_oldTurnRate);
	savefile->WriteInt(_turnEndTime);
	savefile->WriteBool(_searchForFriendDone);
} 

void FleeDoneState::Restore(idRestoreGame* savefile)
{
	State::Restore(savefile);

	savefile->ReadFloat(_oldTurnRate);
	savefile->ReadInt(_turnEndTime);
	savefile->ReadBool(_searchForFriendDone);
} 

StatePtr FleeDoneState::CreateInstance()
{
	return StatePtr(new FleeDoneState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar fleeDoneStateRegistrar(
	STATE_FLEE_DONE, // Task Name
	StateLibrary::CreateInstanceFunc(&FleeDoneState::CreateInstance) // Instance creation callback
);

} // namespace ai
