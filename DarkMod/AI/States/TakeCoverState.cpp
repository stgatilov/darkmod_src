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

static bool init_version = FileVersionList("$Id: TakeCoverState.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "TakeCoverState.h"
#include "../Memory.h"
#include "../Tasks/CombatSensoryTask.h"
#include "../Tasks/MoveToCoverTask.h"
#include "../Tasks/WaitTask.h"
#include "../Tasks/EmergeFromCoverTask.h"
#include "../Library.h"

namespace ai
{

// Get the name of this state
const idStr& TakeCoverState::GetName() const
{
	static idStr _name(STATE_TAKE_COVER);
	return _name;
}

void TakeCoverState::Init(idAI* owner)
{
	DM_LOG(LC_AI, LT_INFO).LogString("TakeCoverState initialised.\r");
	assert(owner);

	// Shortcut reference
	Memory& memory = owner->GetMind()->GetMemory();

	// Remember current position
	memory.positionBeforeTakingCover = owner->GetPhysics()->GetOrigin();


	// Fill the subsystems with their tasks

	// The movement subsystem should run to Cover position, 
	// wait there for some time and then emerge to have a look.
	owner->GetSubsystem(SubsysMovement)->ClearTasks();
	owner->GetSubsystem(SubsysMovement)->PushTask(MoveToCoverTask::CreateInstance());

	int coverDelayMin = SEC2MS(owner->spawnArgs.GetFloat("emerge_from_cover_delay_min"));
	int coverDelayMax = SEC2MS(owner->spawnArgs.GetFloat("emerge_from_cover_delay_max"));
	int emergeDelay = coverDelayMin + gameLocal.random.RandomFloat() * (coverDelayMax - coverDelayMin);
	WaitTaskPtr waitInCover = WaitTask::CreateInstance();
	waitInCover->SetTime(emergeDelay);
	owner->GetSubsystem(SubsysMovement)->QueueTask(waitInCover);

	owner->GetSubsystem(SubsysMovement)->QueueTask(EmergeFromCoverTask::CreateInstance());

	// The communication system 
	owner->GetSubsystem(SubsysCommunication)->ClearTasks();

	// The sensory system 
	owner->GetSubsystem(SubsysSenses)->ClearTasks();
//	owner->GetSubsystem(SubsysSenses)->PushTask(CombatSensoryTask::CreateInstance());

	// No action
	owner->GetSubsystem(SubsysAction)->ClearTasks();
}

// Gets called each time the mind is thinking
void TakeCoverState::Think(idAI* owner)
{

}

StatePtr TakeCoverState::CreateInstance()
{
	return StatePtr(new TakeCoverState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar takeCoverStateRegistrar(
	STATE_TAKE_COVER, // Task Name
	StateLibrary::CreateInstanceFunc(&TakeCoverState::CreateInstance) // Instance creation callback
);

} // namespace ai
