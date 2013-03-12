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

#include "PainState.h"
#include "../Tasks/SingleBarkTask.h"
#include "../Memory.h"
#include "../Library.h"
#include "FleeState.h"	 // grayman #3331

namespace ai
{

// Get the name of this state
const idStr& PainState::GetName() const
{
	static idStr _name(STATE_PAIN);
	return _name;
}

void PainState::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("PainState initialised.\r");
	assert(owner);

	Memory& memory = owner->GetMemory();

	// Play the animation
	owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Pain", 4);
	owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Pain", 4);

	owner->SetWaitState(ANIMCHANNEL_TORSO, "pain");
	owner->SetWaitState(ANIMCHANNEL_LEGS, "pain");

	// Set end time
	_stateEndTime = gameLocal.time + 5000;
	
	memory.alertPos = owner->GetPhysics()->GetOrigin();

	// Do a single bark and assemble an AI message
	CommMessagePtr message = CommMessagePtr(new CommMessage(
		CommMessage::DetectedEnemy_CommType, 
		owner, NULL, // from this AI to anyone
		NULL,
		memory.alertPos
	));

	owner->commSubsystem->AddCommTask(
		CommunicationTaskPtr(new SingleBarkTask("snd_pain_large", message))
	);
}

// Gets called each time the mind is thinking
void PainState::Think(idAI* owner)
{
	if ( ( gameLocal.time >= _stateEndTime ) || 
		 ( idStr(owner->WaitState(ANIMCHANNEL_TORSO)) != "pain" ) ) 
	{
		// grayman #3331 - civilians and unarmed AI should flee
		if ( ( ( owner->GetNumMeleeWeapons()  == 0 ) && ( owner->GetNumRangedWeapons() == 0 ) ) ||
				owner->spawnArgs.GetBool("is_civilian", "0") )
		{
			owner->fleeingEvent = true; // I'm fleeing the scene, not fleeing an enemy
			owner->GetMind()->SwitchState(STATE_FLEE);
		}
		else
		{
			// End this state
			owner->GetMind()->EndState();
		}
	}
}

// Save/Restore methods
void PainState::Save(idSaveGame* savefile) const
{
	State::Save(savefile);

	savefile->WriteInt(_stateEndTime);
}

void PainState::Restore(idRestoreGame* savefile) 
{
	State::Restore(savefile);
	savefile->ReadInt(_stateEndTime);
}

StatePtr PainState::CreateInstance()
{
	return StatePtr(new PainState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar painStateRegistrar(
	STATE_PAIN, // Task Name
	StateLibrary::CreateInstanceFunc(&PainState::CreateInstance) // Instance creation callback
);

} // namespace ai
