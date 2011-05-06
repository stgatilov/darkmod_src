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

#include "FailedKnockoutState.h"
#include "../Tasks/SingleBarkTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

FailedKnockoutState::FailedKnockoutState() :
	_attacker(NULL),
	_hitHead(false)
{}

FailedKnockoutState::FailedKnockoutState(idEntity* attacker, const idVec3& attackDirection, bool hitHead) :
	_attacker(attacker),
	_attackDirection(attackDirection),
	_hitHead(hitHead)
{}

// Get the name of this state
const idStr& FailedKnockoutState::GetName() const
{
	static idStr _name(STATE_FAILED_KNOCKED_OUT);
	return _name;
}

void FailedKnockoutState::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("FailedKnockoutState initialised.\r");
	assert(owner);

	Memory& memory = owner->GetMemory();

	// Failed KO counts as attack
	memory.hasBeenAttackedByEnemy = true;

	// Play the animation
	owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_FailedKO", 4);
	owner->SetWaitState(ANIMCHANNEL_TORSO, "failed_ko");

	// 800 msec stun time
	_allowEndTime = gameLocal.time + 800;

	// Set end time
	_stateEndTime = gameLocal.time + 3000;
	
	// Set the alert position 50 units in the attacking direction
	memory.alertPos = owner->GetPhysics()->GetOrigin() - _attackDirection * 50;

	// Do a single bark and assemble an AI message
	CommMessagePtr message = CommMessagePtr(new CommMessage(
		CommMessage::DetectedEnemy_CommType, 
		owner, NULL, // from this AI to anyone
		_attacker,
		memory.alertPos
	));

	owner->commSubsystem->AddCommTask(
		CommunicationTaskPtr(new SingleBarkTask("snd_failed_knockout", message))
	);
}

// Gets called each time the mind is thinking
void FailedKnockoutState::Think(idAI* owner)
{
	if (gameLocal.time < _allowEndTime)
	{
		return; // wait...
	}

	if (gameLocal.time >= _stateEndTime || 
		idStr(owner->WaitState(ANIMCHANNEL_TORSO)) != "failed_ko") 
	{
		Memory& memory = owner->GetMemory();

		// Alert this AI
		memory.alertClass = EAlertTactile;
		memory.alertType = EAlertTypeEnemy;
	
		// Set the alert position 50 units in the attacking direction
		memory.alertPos = owner->GetPhysics()->GetOrigin() - _attackDirection * 50;

		memory.countEvidenceOfIntruders++;
		memory.alertedDueToCommunication = false;
		memory.stopRelight = true; // grayman #2603

		// Alert the AI
		owner->AlertAI("tact", owner->thresh_5*2);

		// End this state
		owner->GetMind()->EndState();
	}
}

// Save/Restore methods
void FailedKnockoutState::Save(idSaveGame* savefile) const
{
	State::Save(savefile);

	savefile->WriteInt(_stateEndTime);
	savefile->WriteInt(_allowEndTime);
	savefile->WriteObject(_attacker);
	savefile->WriteVec3(_attackDirection);
}

void FailedKnockoutState::Restore(idRestoreGame* savefile) 
{
	State::Restore(savefile);
	savefile->ReadInt(_stateEndTime);
	savefile->ReadInt(_allowEndTime);
	savefile->ReadObject(reinterpret_cast<idClass*&>(_attacker));
	savefile->ReadVec3(_attackDirection);
}

StatePtr FailedKnockoutState::CreateInstance()
{
	return StatePtr(new FailedKnockoutState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar failedKnockoutStateRegistrar(
	STATE_FAILED_KNOCKED_OUT, // Task Name
	StateLibrary::CreateInstanceFunc(&FailedKnockoutState::CreateInstance) // Instance creation callback
);

} // namespace ai
