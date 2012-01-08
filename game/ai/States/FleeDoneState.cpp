/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod (http://www.thedarkmod.com/)
 
 $Revision$ (Revision of last commit) 
 $Date$ (Date of last commit)
 $Author$ (Author of last commit)
 
******************************************************************************/

#include "precompiled_game.h"
#pragma hdrstop

static bool versioned = RegisterVersionedFile("$Id$");

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

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("FleeDoneState initialised.\r");
	assert(owner);

	// Shortcut reference
	//Memory& memory = owner->GetMemory();

	// greebo: At this point we should be at a presumably safe place, 
	// start looking for allies
	_searchForFriendDone = false;

	// alert level ramps down
	float alertTime = owner->atime_fleedone + owner->atime_fleedone_fuzzyness * (gameLocal.random.RandomFloat() - 0.5);
	_alertLevelDecreaseRate = (owner->thresh_5 - owner->thresh_3) / alertTime;

	owner->senseSubsystem->ClearTasks();
 	owner->GetSubsystem(SubsysCommunication)->ClearTasks();
	owner->actionSubsystem->ClearTasks();

	// Slow turning for 5 seconds to look for friends
	owner->StopMove(MOVE_STATUS_DONE);
	_oldTurnRate = owner->GetTurnRate();
	owner->SetTurnRate(90);
	owner->movementSubsystem->ClearTasks();
	owner->movementSubsystem->PushTask(RandomTurningTask::CreateInstance());
	_turnEndTime = gameLocal.time + 5000;

	owner->senseSubsystem->PushTask(RandomHeadturnTask::CreateInstance());
}

// Gets called each time the mind is thinking
void FleeDoneState::Think(idAI* owner)
{
	UpdateAlertLevel();
		
	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner)) 
	{
		// terminate FleeDoneState when time is over
		owner->movementSubsystem->ClearTasks();
		owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", 4);
		owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Idle", 4);
		owner->SetTurnRate(_oldTurnRate);

		owner->GetMind()->EndState();
		return;
	}

	// Let the AI check its senses
	owner->PerformVisualScan();
	if (owner->AI_ALERTED)
	{
		// terminate FleeDoneState when the AI is alerted
		owner->movementSubsystem->ClearTasks();
		owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", 4);
		owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Idle", 4);
		owner->SetTurnRate(_oldTurnRate);

		owner->GetMind()->EndState();
	}

	// Shortcut reference
	Memory& memory = owner->GetMemory();

	if (!_searchForFriendDone)
	{
		idActor* friendlyAI = owner->FindFriendlyAI(-1);
		if ( friendlyAI != NULL)
		{
			// We found a friend, cry for help to him
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Found friendly AI %s \r", friendlyAI->name.c_str());

			_searchForFriendDone = true;
			owner->movementSubsystem->ClearTasks();
			owner->SetTurnRate(_oldTurnRate);

			owner->TurnToward(friendlyAI->GetPhysics()->GetOrigin());
			//float distanceToFriend = (friendlyAI->GetPhysics()->GetOrigin() - owner->GetPhysics()->GetOrigin()).LengthFast();

			// Cry for help
			// Create a new help message
			CommMessagePtr message(new CommMessage(
				CommMessage::RequestForHelp_CommType, 
				owner, friendlyAI, NULL, memory.alertPos)
			); 

			CommunicationTaskPtr barkTask(new SingleBarkTask("snd_flee", message));
			owner->commSubsystem->AddCommTask(barkTask);
			memory.lastTimeVisualStimBark = gameLocal.time;

		}
		else if (gameLocal.time >= _turnEndTime)
		{
			// We didn't find a friend, stop looking for them after some time
			_searchForFriendDone = true;
			owner->movementSubsystem->ClearTasks();
			owner->SetTurnRate(_oldTurnRate);

			// Play the cowering animation
			owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Cower", 4);
			owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Cower", 4);
		}
	}
}


void FleeDoneState::OnPersonEncounter(idEntity* stimSource, idAI* owner)
{
	assert(stimSource != NULL && owner != NULL); // must be fulfilled

	Memory& memory = owner->GetMemory();
	
	if (!stimSource->IsType(idActor::Type)) return; // No Actor, quit

	// Hard-cast the stimsource onto an actor 
	idActor* other = static_cast<idActor*>(stimSource);	
	if (owner->IsFriend(other))
	{
		if (gameLocal.time - memory.lastTimeVisualStimBark >= MINIMUM_SECONDS_BETWEEN_STIMULUS_BARKS)
		{

			memory.lastTimeVisualStimBark = gameLocal.time;
			CommMessagePtr message(new CommMessage(
				CommMessage::RequestForHelp_CommType, 
				owner, other, NULL, memory.alertPos)
			); 

			CommunicationTaskPtr barkTask(new SingleBarkTask("snd_flee", message));
			owner->commSubsystem->AddCommTask(barkTask);
			memory.lastTimeVisualStimBark = gameLocal.time;
		}
	}

	State::OnPersonEncounter(stimSource, owner);
}


bool FleeDoneState::CheckAlertLevel(idAI* owner)
{
	// FleeDoneState terminates itself when the AI reaches Suspicious
	if (owner->AI_AlertIndex < 3)
	{
		// Alert index is too low for this state, fall back
		owner->movementSubsystem->ClearTasks();
		owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", 4);
		owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Idle", 4);
		owner->SetTurnRate(_oldTurnRate);

		return false;
	}
	
	// Alert Index is matching, return OK
	return true;
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
