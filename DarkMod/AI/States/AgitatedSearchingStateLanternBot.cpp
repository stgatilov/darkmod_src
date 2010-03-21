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

#include "AgitatedSearchingStateLanternBot.h"
#include "../Tasks/ScriptTask.h"
#include "../Tasks/RepeatedBarkTask.h"

namespace ai
{

// Get the name of this state
const idStr& AgitatedSearchingStateLanternBot::GetName() const
{
	static idStr _name(STATE_AGITATED_SEARCHING_LANTERN_BOT);
	return _name;
}

void AgitatedSearchingStateLanternBot::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("AgitatedSearchingStateLanternBot initialised.\r");
	assert(owner);

	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner)) return;

	CalculateAlertDecreaseRate(owner);

	// Shortcut reference
	Memory& memory = owner->GetMemory();

	owner->StopMove(MOVE_STATUS_DONE);

	owner->movementSubsystem->ClearTasks();
	owner->senseSubsystem->ClearTasks();
	owner->actionSubsystem->ClearTasks();

	// Fill the subsystems with their tasks

	// This will hold the message to be delivered with the inaudible bark
	CommMessagePtr message(new CommMessage(
		CommMessage::DetectedEnemy_CommType, 
		owner, NULL,// from this AI to anyone 
		NULL,
		memory.alertPos
	));

	// The communication system plays starting bark
	owner->commSubsystem->AddCommTask(
		CommunicationTaskPtr(new RepeatedBarkTask("snd_spotted_noise", 3000, 4000, message))
	);

	// Add the script task blowing the alarm whistle
	owner->actionSubsystem->PushTask(TaskPtr(new ScriptTask("startAlarmWhistle")));
}

// Gets called each time the mind is thinking
void AgitatedSearchingStateLanternBot::Think(idAI* owner)
{
	UpdateAlertLevel();

	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner))
	{
		owner->GetMind()->EndState();
		return;
	}

	Memory& memory = owner->GetMemory();

	// Look at alert position
	owner->TurnToward(memory.alertPos);
}

StatePtr AgitatedSearchingStateLanternBot::CreateInstance()
{
	return StatePtr(new AgitatedSearchingStateLanternBot);
}

// Register this state with the StateLibrary
StateLibrary::Registrar agitatedSearchingStateLanternBotRegistrar(
	STATE_AGITATED_SEARCHING_LANTERN_BOT, // Task Name
	StateLibrary::CreateInstanceFunc(&AgitatedSearchingStateLanternBot::CreateInstance) // Instance creation callback
);

} // namespace ai
