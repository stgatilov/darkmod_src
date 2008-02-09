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

static bool init_version = FileVersionList("$Id: IdleState.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "IdleState.h"
#include "../Memory.h"
#include "../Tasks/RandomHeadturnTask.h"
#include "../Tasks/PatrolTask.h"
#include "../Tasks/SingleBarkTask.h"
#include "../Tasks/IdleBarkTask.h"
#include "../Tasks/MoveToPositionTask.h"
#include "../Tasks/IdleAnimationTask.h"
#include "ObservantState.h"
#include "../Library.h"

namespace ai
{

IdleState::IdleState() :
	_idlePosition(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY),
	_idleYaw(idMath::INFINITY)
{}

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

	DM_LOG(LC_AI, LT_INFO).LogString("IdleState initialised.\r");
	assert(owner);

	_alertLevelDecreaseRate = 0.01f;

	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner)) return;

	owner->AI_RUN = false;

	// Fill the subsystems with their tasks

	// The movement subsystem should start patrolling
	owner->GetSubsystem(SubsysMovement)->ClearTasks();
	owner->GetSubsystem(SubsysMovement)->PushTask(PatrolTask::CreateInstance());

	// The communication system is barking in regular intervals
	owner->GetSubsystem(SubsysCommunication)->ClearTasks();

	// Push a single bark to the communication subsystem first, it fires only once
	owner->GetSubsystem(SubsysCommunication)->PushTask(
		TaskPtr(new SingleBarkTask(GetInitialIdleBark(owner)))
	);

	// No weapons in idle mode, unless we were really alerted before
	if (owner->m_maxAlertLevel < owner->thresh_5)
	{
		owner->SheathWeapon();

		// Push the regular patrol barking to the list too
		owner->GetSubsystem(SubsysCommunication)->QueueTask(
			TaskPtr(new IdleBarkTask("snd_relaxed"))
		);

		// The action subsystem plays the idle anims (scratching, yawning...)
		owner->GetSubsystem(SubsysAction)->ClearTasks();
		owner->GetSubsystem(SubsysAction)->PushTask(IdleAnimationTask::CreateInstance());
	}

	// Initialise the animation state
	owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", 0);
	owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Idle", 0);

	// The sensory system does its Idle tasks
	owner->GetSubsystem(SubsysSenses)->ClearTasks();
	owner->GetSubsystem(SubsysSenses)->PushTask(RandomHeadturnTask::CreateInstance());

	// Check if the owner has patrol routes set
	idPathCorner* path = idPathCorner::RandomPath(owner, NULL);

	if (path == NULL)
	{
		// We don't have any patrol routes, so we're supposed to stand around
		// where the mapper has put us.
		if (_idlePosition == idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY))
		{
			// No idle position saved yet, take the current one
			_idlePosition = owner->GetPhysics()->GetOrigin();
			_idleYaw = owner->GetCurrentYaw();
		}
		else
		{
			// We already HAVE an idle position set, this means that we are
			// supposed to be there, let's move
			owner->GetSubsystem(SubsysMovement)->PushTask(
				TaskPtr(new MoveToPositionTask(_idlePosition, _idleYaw))
			);
		}
	}
}

// Gets called each time the mind is thinking
void IdleState::Think(idAI* owner)
{
	UpdateAlertLevel();

	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner)) return;

	// Let the AI check its senses
	owner->PerformVisualScan();
}

void IdleState::Save(idSaveGame* savefile) const
{
	State::Save(savefile);

	savefile->WriteVec3(_idlePosition);
	savefile->WriteFloat(_idleYaw);
}

void IdleState::Restore(idRestoreGame* savefile)
{
	State::Restore(savefile);

	savefile->ReadVec3(_idlePosition);
	savefile->ReadFloat(_idleYaw);
}

idStr IdleState::GetInitialIdleBark(idAI* owner)
{
	// greebo: Ported from ai_darkmod_base::task_Idle written by SZ

	Memory& memory = owner->GetMemory();

	// Decide what sound it is appropriate to play
	idStr soundName("");
	
	if (owner->AI_AlertLevel <= 0)
	{
		//soundName = "snd_relaxed";
		// greebo: Relaxed sound playing is handled by the IdleBarkTask.
		soundName = "";
	}
	else if (owner->GetMemory().enemiesHaveBeenSeen)
	{
		soundName = "snd_alertdown0SeenEvidence";
	}
	else if (owner->GetMemory().itemsHaveBeenStolen)
	{
		soundName = "snd_alertdown0SeenEvidence";
	}
	else if (owner->GetMemory().countEvidenceOfIntruders >= MIN_EVIDENCE_OF_INTRUDERS_TO_COMMUNICATE_SUSPICION)
	{
		soundName = "snd_alertdown0SeenEvidence";
	}
	else
	{
		// Play its idle sound
		soundName = "snd_alertdown0SeenNoEvidence";
	}

	// Reset the patrol chat time
	memory.lastPatrolChatTime = gameLocal.time;

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
