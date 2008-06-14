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
#include "ObservantState.h"
#include "../Library.h"

namespace ai
{


// Get the name of this state
const idStr& IdleState::GetName() const
{
	static idStr _name(STATE_IDLE);
	return _name;
}

bool IdleState::CheckAlertLevel(idAI* owner)
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

void IdleState::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("IdleState initialised.\r");
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

	_alertLevelDecreaseRate = 0.01f;

	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner)) return;

	owner->SheathWeapon();

	_startSitting = owner->spawnArgs.GetBool("sitting", "0");

	// Initialise the animation state
	if (_startSitting && memory.idlePosition == idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY))
	{
		owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Idle_Sit", 0);
		owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Idle_Sit", 0);
		owner->Event_SetMoveType(MOVETYPE_SIT);
	}
	else
	{
		owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", 0);
		owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Idle", 0);
	}

	// The action subsystem plays the idle anims (scratching, yawning...)
	owner->GetSubsystem(SubsysAction)->ClearTasks();
	owner->GetSubsystem(SubsysAction)->PushTask(IdleAnimationTask::CreateInstance());

	// The sensory system does its Idle tasks
	owner->GetSubsystem(SubsysSenses)->ClearTasks();
	owner->GetSubsystem(SubsysSenses)->PushTask(RandomHeadturnTask::CreateInstance());

	InitialiseMovement(owner);

	InitialiseCommunication(owner);

	int idleBarkIntervalMin = SEC2MS(owner->spawnArgs.GetInt("idle_bark_interval_min", "45"));
	int idleBarkIntervalMax = SEC2MS(owner->spawnArgs.GetInt("idle_bark_interval_max", "180"));
	// Push the regular patrol barking to the list too
	owner->GetSubsystem(SubsysCommunication)->QueueTask(
		TaskPtr(new RepeatedBarkTask("snd_relaxed", idleBarkIntervalMin, idleBarkIntervalMax))
	);
}

// Gets called each time the mind is thinking
void IdleState::Think(idAI* owner)
{
	Memory& memory = owner->GetMemory();

	if (_startSitting && owner->GetMoveType() != MOVETYPE_SIT)
	{
		if (owner->ReachedPos(memory.idlePosition, MOVE_TO_POSITION) 
			&& owner->GetCurrentYaw() == memory.idleYaw)
		{
			owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Idle_Sit", 0);
			owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Idle_Sit", 0);
			owner->Event_SetMoveType(MOVETYPE_SIT);
		}
	}
	UpdateAlertLevel();

	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner)) return;

	// Let the AI check its senses
	owner->PerformVisualScan();
}

void IdleState::InitialiseMovement(idAI* owner)
{
	Memory& memory = owner->GetMemory();

	owner->AI_RUN = false;

	// The movement subsystem should start patrolling
	owner->GetSubsystem(SubsysMovement)->ClearTasks();

	// greebo: Choose the patrol task depending on the spawnargs.
	TaskPtr patrolTask = TaskLibrary::Instance().CreateInstance(
		owner->spawnArgs.GetBool("animal_patrol", "0") ? TASK_ANIMAL_PATROL : TASK_PATROL
	);
	owner->GetSubsystem(SubsysMovement)->PushTask(patrolTask);

	// Check if the owner has patrol routes set
	idPathCorner* path = idPathCorner::RandomPath(owner, NULL);

	if (path == NULL)
	{
		// We don't have any patrol routes, so we're supposed to stand around
		// where the mapper has put us.
		if (memory.idlePosition == idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY))
		{
			// No idle position saved yet, take the current one
			memory.idlePosition = owner->GetPhysics()->GetOrigin();
			memory.idleYaw = owner->GetCurrentYaw();
		}
		else
		{
			// We already HAVE an idle position set, this means that we are
			// supposed to be there, let's move
			owner->GetSubsystem(SubsysMovement)->PushTask(
				TaskPtr(new MoveToPositionTask(memory.idlePosition, memory.idleYaw))
			);
		}
	}
}

void IdleState::InitialiseCommunication(idAI* owner)
{
	// Push a single bark to the communication subsystem first, it fires only once
	owner->GetSubsystem(SubsysCommunication)->QueueTask(
		TaskPtr(new SingleBarkTask(GetInitialIdleBark(owner)))
	);
}


void IdleState::Save(idSaveGame* savefile) const
{
	State::Save(savefile);

	savefile->WriteBool(_startSitting);
}

void IdleState::Restore(idRestoreGame* savefile)
{
	State::Restore(savefile);

	savefile->ReadBool(_startSitting);
}

idStr IdleState::GetInitialIdleBark(idAI* owner)
{
	// greebo: Ported from ai_darkmod_base::task_Idle written by SZ

	Memory& memory = owner->GetMemory();

	// Decide what sound it is appropriate to play
	idStr soundName("");

	if (owner->m_maxAlertLevel >= owner->thresh_1)
	{
		if (owner->m_lastAlertLevel < owner->thresh_4)
		{
			if (memory.alertClass == EAlertVisual)
			{
				soundName = "snd_alertdown0s";
			}
			else if (memory.alertClass == EAlertAudio)
			{
				soundName = "snd_alertdown0h";
			}
			else
			{
				soundName = "snd_alertdown0";
			}
		}
	}
	return soundName;
}

StatePtr IdleState::CreateInstance()
{
	return StatePtr(new IdleState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar idleStateRegistrar(
	STATE_IDLE, // Task Name
	StateLibrary::CreateInstanceFunc(&IdleState::CreateInstance) // Instance creation callback
);

} // namespace ai
