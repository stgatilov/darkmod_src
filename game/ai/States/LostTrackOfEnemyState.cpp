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

#include "LostTrackOfEnemyState.h"
#include "../Memory.h"
#include "../Tasks/SingleBarkTask.h"
#include "../Library.h"

namespace ai
{

// Get the name of this state
const idStr& LostTrackOfEnemyState::GetName() const
{
	static idStr _name(STATE_LOST_TRACK_OF_ENEMY);
	return _name;
}

void LostTrackOfEnemyState::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("LostTrackOfEnemyState initialised.\r");
	assert(owner);

	// Shortcut reference
	Memory& memory = owner->GetMemory();

	owner->SetAlertLevel((owner->thresh_5 + owner->thresh_4) * 0.5);

	// Draw weapon, if we haven't already
	owner->DrawWeapon();

	// Setup the search parameters
	memory.alertPos = owner->lastVisibleReachableEnemyPos;
	memory.alertRadius = LOST_ENEMY_ALERT_RADIUS;
	memory.alertSearchVolume = LOST_ENEMY_SEARCH_VOLUME;
	memory.alertSearchExclusionVolume.Zero();

	memory.alertedDueToCommunication = false;
	memory.stimulusLocationItselfShouldBeSearched = true;

	// Forget about the enemy, prevent UpdateEnemyPosition from "cheating".
	owner->ClearEnemy();

	// Enqueue a lost track of enemy bark
	owner->commSubsystem->AddCommTask(
		CommunicationTaskPtr(new SingleBarkTask("snd_lostTrackOfEnemy"))
	);

	// For now, clear the action tasks and movement tasks
	owner->actionSubsystem->ClearTasks();
	owner->movementSubsystem->ClearTasks();

	owner->GetMind()->EndState();
}

// Gets called each time the mind is thinking
void LostTrackOfEnemyState::Think(idAI* owner)
{

}

StatePtr LostTrackOfEnemyState::CreateInstance()
{
	return StatePtr(new LostTrackOfEnemyState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar lostTrackOfEnemyStateRegistrar(
	STATE_LOST_TRACK_OF_ENEMY, // Task Name
	StateLibrary::CreateInstanceFunc(&LostTrackOfEnemyState::CreateInstance) // Instance creation callback
);

} // namespace ai
