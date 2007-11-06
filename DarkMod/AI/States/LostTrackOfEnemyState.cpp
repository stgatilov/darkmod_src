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

static bool init_version = FileVersionList("$Id: LostTrackOfEnemyState.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "LostTrackOfEnemyState.h"
#include "../Memory.h"
#include "../Tasks/EmptyTask.h"
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

	DM_LOG(LC_AI, LT_INFO).LogString("LostTrackOfEnemyState initialised.\r");
	assert(owner);

	// Shortcut reference
	Memory& memory = owner->GetMemory();

	owner->Event_SetAlertLevel((owner->thresh_combat + owner->thresh_3) * 0.5);

	// Draw weapon, if we haven't already
	owner->DrawWeapon();

	// Setup the search parameters
	memory.alertPos = owner->lastVisibleReachableEnemyPos;
	memory.alertRadius = LOST_ENEMY_ALERT_RADIUS;
	memory.alertSearchVolume = LOST_ENEMY_SEARCH_VOLUME;
	memory.alertSearchExclusionVolume.Zero();

	memory.searchingDueToCommunication = false;
	memory.stimulusLocationItselfShouldBeSearched = true;

	// Forget about the enemy, prevent UpdateEnemyPosition from "cheating".
	owner->ClearEnemy();

	// Enqueue a lost track of enemy bark
	owner->GetSubsystem(SubsysCommunication)->PushTask(
		TaskPtr(new SingleBarkTask("snd_lostTrackOfEnemy"))
	);

	// For now, clear the action tasks and movement tasks
	owner->GetSubsystem(SubsysAction)->ClearTasks();
	owner->GetSubsystem(SubsysMovement)->ClearTasks();

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
