/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include "TakeCoverState.h"
#include "../Memory.h"
#include "../Tasks/MoveToCoverTask.h"
#include "../Tasks/WaitTask.h"
#include "../Tasks/MoveToPositionTask.h"
#include "../Tasks/RandomHeadturnTask.h"
#include "LostTrackOfEnemyState.h"
#include "StayInCoverState.h"
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
	// Init base class first
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO).LogString("TakeCoverState initialised.\r");
	assert(owner);

	// Shortcut reference
	Memory& memory = owner->GetMemory();

	// angua: The last position of the AI before it takes cover, so it can return to it later.
	memory.positionBeforeTakingCover = owner->GetPhysics()->GetOrigin();

	// Fill the subsystems with their tasks

	// The movement subsystem should wait half a second and then run to Cover position, 
	// wait there for some time and then emerge to have a look.
	owner->StopMove(MOVE_STATUS_DONE);
	owner->GetSubsystem(SubsysMovement)->ClearTasks();
	owner->GetSubsystem(SubsysMovement)->PushTask(TaskPtr(new WaitTask(500)));
	owner->GetSubsystem(SubsysMovement)->QueueTask(MoveToCoverTask::CreateInstance());
	owner->AI_MOVE_DONE = false;

	// The communication system 
	owner->GetSubsystem(SubsysCommunication)->ClearTasks();

	// The sensory system 
	owner->GetSubsystem(SubsysSenses)->ClearTasks();

	// No action
	owner->GetSubsystem(SubsysAction)->ClearTasks();
}

// Gets called each time the mind is thinking
void TakeCoverState::Think(idAI* owner)
{
	if (owner->AI_MOVE_DONE && !owner->AI_DEST_UNREACHABLE)
	{
		// When we are done moving to cover position, stay for some time there 
		// then come out and move back to where we were standing before taking cover
		owner->AI_RUN = false;
		owner->TurnToward(owner->lastVisibleEnemyPos);
		owner->GetMind()->SwitchState(STATE_STAY_IN_COVER);
	}
	else if (owner->AI_DEST_UNREACHABLE)
	{
		owner->GetMind()->EndState();
	}
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
