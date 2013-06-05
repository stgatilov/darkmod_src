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

#include "BlindedState.h"
#include "../Tasks/SingleBarkTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

// Get the name of this state
const idStr& BlindedState::GetName() const
{
	static idStr _name(STATE_BLINDED);
	return _name;
}


void BlindedState::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	owner->movementSubsystem->ClearTasks();
	owner->senseSubsystem->ClearTasks();
	owner->actionSubsystem->ClearTasks();

	owner->StopMove(MOVE_STATUS_DONE);

	owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Blinded", 4);
	owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Blinded", 4);

	owner->SetWaitState("blinded"); // grayman #3431
	owner->SetWaitState(ANIMCHANNEL_TORSO, "blinded");
	owner->SetWaitState(ANIMCHANNEL_LEGS, "blinded");

	Memory& memory = owner->GetMemory();
	memory.stopRelight = true; // grayman #2603 - abort a relight in progress
	memory.stopExaminingRope = true; // grayman #2872 - abort a rope examination
	memory.stopReactingToHit = true; // grayman #2816

	CommMessagePtr message(new CommMessage(
		CommMessage::RequestForHelp_CommType, 
		owner, NULL, // from this AI to anyone 
		NULL,
		memory.alertPos,
		0
	));

	owner->commSubsystem->AddCommTask(
		CommunicationTaskPtr(new SingleBarkTask("snd_blinded", message))
	);

	float duration = SEC2MS(owner->spawnArgs.GetFloat("blind_time", "4")) + 
		(gameLocal.random.RandomFloat() - 0.5f) * 2 * SEC2MS(owner->spawnArgs.GetFloat("blind_time_fuzziness", "2"));

	_endTime = gameLocal.time + static_cast<int>(duration);

	// Set alert level a little bit below combat
	if (owner->AI_AlertLevel < owner->thresh_5 - 1)
	{
		owner->SetAlertLevel(owner->thresh_5 - 1);
	}

	_oldVisAcuity = owner->GetAcuity("vis");
	owner->SetAcuity("vis", 0);
	_oldAudAcuity = owner->GetAcuity("aud"); // Smoke #2829
	owner->SetAcuity("aud",_oldAudAcuity*0.25f); // Smoke #2829

	_staring = false; // grayman #3431 (set to true when you stare at the ground)
}

// Gets called each time the mind is thinking
void BlindedState::Think(idAI* owner)
{
	if (gameLocal.time >= _endTime)
	{
		owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", 4);
		owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Idle", 4);

		owner->SetWaitState(ANIMCHANNEL_TORSO, "");
		owner->SetWaitState(ANIMCHANNEL_LEGS, "");

		owner->SetAcuity("vis", _oldVisAcuity);
		owner->SetAcuity("aud", _oldAudAcuity); // Smoke #2829

		owner->GetMind()->EndState();
	}
	else if ( !_staring && ( idStr(owner->WaitState()) != "blinded" ) ) // grayman #3431
	{
		int duration = _endTime - gameLocal.time;
		if ( duration > 0 )
		{
			// Stare at the ground in front of you, as if you're trying to get your sight back

			idVec3 vec = owner->viewAxis.ToAngles().ToForward()*24;
			vec.z = 0;
			idVec3 lookAtMe = owner->GetPhysics()->GetOrigin() + vec;
			owner->Event_LookAtPosition(lookAtMe, MS2SEC(duration));
			_staring = true;
		}
	}
}

void BlindedState::Save(idSaveGame* savefile) const
{
	State::Save(savefile);

	savefile->WriteInt(_endTime);
	savefile->WriteFloat(_oldVisAcuity);
	savefile->WriteFloat(_oldAudAcuity); // Smoke #2829
	savefile->WriteBool(_staring); // grayman #3431
}

void BlindedState::Restore(idRestoreGame* savefile)
{
	State::Restore(savefile);

	savefile->ReadInt(_endTime);
	savefile->ReadFloat(_oldVisAcuity);
	savefile->ReadFloat(_oldAudAcuity); // Smoke #2829
	savefile->ReadBool(_staring); // grayman #3431
}

StatePtr BlindedState::CreateInstance()
{
	return StatePtr(new BlindedState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar blindedStateRegistrar(
	STATE_BLINDED, // Task Name
	StateLibrary::CreateInstanceFunc(&BlindedState::CreateInstance) // Instance creation callback
);

} // namespace ai
