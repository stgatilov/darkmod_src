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

#include "PainState.h"
#include "../Tasks/SingleBarkTask.h"
#include "../Memory.h"
#include "../Library.h"

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
	
	// Set the alert position 50 units in the attacking direction
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
	if (gameLocal.time >= _stateEndTime || 
		idStr(owner->WaitState(ANIMCHANNEL_TORSO)) != "pain") 
	{
		Memory& memory = owner->GetMemory();

		// Alert this AI
		memory.alertClass = EAlertTactile;
		memory.alertType = EAlertTypeEnemy;
	
		// Set the alert position 50 units in the attacking direction
		memory.alertPos = owner->GetPhysics()->GetOrigin();

		memory.countEvidenceOfIntruders++;
		memory.alertedDueToCommunication = false;

		// Alert the AI
		owner->AlertAI("tact", owner->thresh_5*2);

		// End this state
		owner->GetMind()->EndState();
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
