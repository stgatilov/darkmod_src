/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 3222 $
 * $Date: 2009-03-04 14:37:27 +0100 (Mi, 04 Mär 2009) $
 * $Author: angua $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: IdleSleepState.cpp 3222 2009-03-04 13:37:27Z angua $", init_version);

#include "IdleSleepState.h"
#include "IdleState.h"
#include "AlertIdleState.h"
#include "../Memory.h"
#include "../Tasks/RandomHeadturnTask.h"
#include "../Tasks/PatrolTask.h"
#include "../Tasks/AnimalPatrolTask.h"
#include "../Tasks/SingleBarkTask.h"
#include "../Tasks/RepeatedBarkTask.h"
#include "../Tasks/MoveToPositionTask.h"
#include "../Tasks/IdleAnimationTask.h"
#include "../Tasks/WaitTask.h"
#include "ObservantState.h"
#include "../Library.h"

namespace ai
{


// Get the name of this state
const idStr& IdleSleepState::GetName() const
{
	static idStr _name(STATE_IDLE_SLEEP);
	return _name;
}

bool IdleSleepState::CheckAlertLevel(idAI* owner)
{
	if (owner->AI_AlertIndex > 0)
	{
		// Alert index is too high, switch to the higher State
		owner->GetMind()->PushState(STATE_OBSERVANT);
		return false;
	}

	// Alert Index is matching, return OK
	return true;
}

void IdleSleepState::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("IdleSleepState initialised.\r");
	assert(owner);

	// Memory shortcut
	Memory& memory = owner->GetMemory();
	memory.alertClass = EAlertNone;
	memory.alertType = EAlertTypeNone;

	if (owner->HasSeenEvidence())
	{
		owner->GetMind()->SwitchState(STATE_ALERT_IDLE);
		return;
	}

	if (owner->GetMoveType() != MOVETYPE_SLEEP && 
		(!owner->ReachedPos(memory.idlePosition, MOVE_TO_POSITION) 
			|| owner->GetCurrentYaw() != memory.idleYaw))
	{
		// we need to get to the bed first before starting to sleep, back to idle state
		owner->GetMind()->SwitchState(STATE_IDLE);
		return;
	}

	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner)) return;

	_startSleeping = owner->spawnArgs.GetBool("sleeping", "0");
	_startSitting = owner->spawnArgs.GetBool("sitting", "0");

	_alertLevelDecreaseRate = 0.01f;

	// owner->SheathWeapon();

	owner->actionSubsystem->ClearTasks();
	owner->senseSubsystem->ClearTasks();
//	owner->GetSubsystem(SubsysCommunication)->ClearTasks();// TODO_AI

	InitialiseMovement(owner);

	int idleBarkIntervalMin = SEC2MS(owner->spawnArgs.GetInt("idle_bark_interval_min", "45"));
	int idleBarkIntervalMax = SEC2MS(owner->spawnArgs.GetInt("idle_bark_interval_max", "180"));

/*	owner->GetSubsystem(SubsysCommunication)->QueueTask(TaskPtr(new WaitTask(5000)));
	owner->GetSubsystem(SubsysCommunication)->QueueTask(
		TaskPtr(new RepeatedBarkTask("snd_sleeping", idleBarkIntervalMin, idleBarkIntervalMax))
	);*/// TODO_AI

	// Let the AI update their weapons (make them nonsolid)
	owner->UpdateAttachmentContents(false);

}

// Gets called each time the mind is thinking
void IdleSleepState::Think(idAI* owner)
{
	Memory& memory = owner->GetMemory();
	idStr waitState = owner->WaitState();

	if (_startSleeping && owner->GetMoveType() == MOVETYPE_ANIM)
	{
		if (owner->ReachedPos(memory.idlePosition, MOVE_TO_POSITION) 
			&& owner->GetCurrentYaw() == memory.idleYaw)
		{
			owner->LayDown();
		}
	}
	else if (owner->GetMoveType() == MOVETYPE_GET_UP_FROM_LYING)
	{
		owner->GetMind()->SwitchState(STATE_IDLE);
//		owner->GetSubsystem(SubsysCommunication)->ClearTasks();// TODO_AI
		return;
	}


	UpdateAlertLevel();

	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner)) return;

}


StatePtr IdleSleepState::CreateInstance()
{
	return StatePtr(new IdleSleepState);
}

void IdleSleepState::OnChangeTarget(idAI* owner)
{
	// re-initialize movement subsystem to catch new path_corners
	InitialiseMovement(owner);
}

// Register this state with the StateLibrary
StateLibrary::Registrar idleSleepStateRegistrar(
	STATE_IDLE_SLEEP, // Task Name
	StateLibrary::CreateInstanceFunc(&IdleSleepState::CreateInstance) // Instance creation callback
);

} // namespace ai
