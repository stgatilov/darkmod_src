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

#include "SuspiciousState.h"
#include "../Memory.h"
#include "../../AIComm_Message.h"
#include "../Tasks/RandomHeadturnTask.h"
#include "../Tasks/SingleBarkTask.h"
#include "SearchingState.h"
#include "../Library.h"

namespace ai
{

// Get the name of this state
const idStr& SuspiciousState::GetName() const
{
	static idStr _name(STATE_SUSPICIOUS);
	return _name;
}

bool SuspiciousState::CheckAlertLevel(idAI* owner)
{
	if (owner->AI_AlertIndex < 2)
	{
		// Alert index is too low for this state, fall back
		owner->GetMind()->EndState();
		return false;
	}
	else if (owner->AI_AlertIndex > 2)
	{
		// Alert index is too high, switch to the higher State
		owner->GetMind()->PushState(owner->backboneStates[EInvestigating]);
		return false;
	}

	// Alert Index is matching, return OK
	return true;
}

void SuspiciousState::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("SuspiciousState initialised.\r");
	assert(owner);

	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner)) return;

	float alertTime = owner->atime2 + owner->atime2_fuzzyness * (gameLocal.random.RandomFloat() - 0.5);
	_alertLevelDecreaseRate = (owner->thresh_3 - owner->thresh_2) / alertTime;

	// Shortcut reference
	Memory& memory = owner->GetMemory();

	owner->senseSubsystem->ClearTasks();
	owner->actionSubsystem->ClearTasks();

	if (owner->GetMoveType() == MOVETYPE_SLEEP)
	{
		owner->GetUp();
	}
	
	if (gameLocal.random.RandomFloat() > 0.5f)
	{
		owner->movementSubsystem->ClearTasks();
		owner->StopMove(MOVE_STATUS_DONE);
		memory.stopRelight = true; // grayman #2603 - abort a relight in progress

		if (!owner->CheckFOV(memory.alertPos) && owner->GetMoveType() == MOVETYPE_ANIM)
		{
			// Search spot is not within FOV, turn towards the position
			// don't turn while sitting
			owner->TurnToward(memory.alertPos);
		}
	}

	// In any case, look at the point to investigate
	owner->Event_LookAtPosition(memory.alertPos, 2.0f);

	// barking
	idStr bark;

	if (owner->AlertIndexIncreased())
	{
		if (memory.alertClass == EAlertVisual_1)
		{
			bark = "snd_alert1s";
		}
		else if (memory.alertClass == EAlertAudio)
		{
			bark = "snd_alert1h";
		}
		else
		{
			bark = "snd_alert1";
		}

		owner->commSubsystem->AddCommTask(
			CommunicationTaskPtr(new SingleBarkTask(bark))
		);
	}
	else
	{
		owner->commSubsystem->ClearTasks();
	}

	// Let the AI update their weapons (make them nonsolid)
	owner->UpdateAttachmentContents(false);
}

// Gets called each time the mind is thinking
void SuspiciousState::Think(idAI* owner)
{
	UpdateAlertLevel();
	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner)) return;
	
	// Let the AI check its senses
	owner->PerformVisualScan();
}

StatePtr SuspiciousState::CreateInstance()
{
	return StatePtr(new SuspiciousState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar suspiciousStateRegistrar(
	STATE_SUSPICIOUS, // Task Name
	StateLibrary::CreateInstanceFunc(&SuspiciousState::CreateInstance) // Instance creation callback
);

} // namespace ai
