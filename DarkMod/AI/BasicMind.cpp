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

static bool init_version = FileVersionList("$Id: BasicMind.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "BasicMind.h"
#include "States/IdleState.h"
#include "Library.h"

namespace ai
{

BasicMind::BasicMind(idAI* owner) :
	_alertState(ERelaxed)
{
	// Set the idEntityPtr
	_owner = owner;
}

void BasicMind::Think()
{
	// Thinking
	DM_LOG(LC_AI, LT_INFO).LogString("Mind is thinking...\r");

	if (_state == NULL)
	{
		// We start with the idle state
		ChangeState(STATE_IDLE);
	}

	// greebo: We do not check for NULL pointers in the owner at this point, 
	// as this method is called by the owner itself.

	idAI* owner = _owner.GetEntity();

	switch (gameLocal.framenum % 4) {
		case 0:
			owner->GetSubsystem(SubsysSenses)->PerformTask();
			break;
		case 1:
			owner->GetSubsystem(SubsysMovement)->PerformTask();
			break;
		case 2:
			owner->GetSubsystem(SubsysCommunication)->PerformTask();
			break;
		case 3:
			owner->GetSubsystem(SubsysAction)->PerformTask();
			break;
	};
}

// Changes the state
void BasicMind::ChangeState(const idStr& stateName)
{
	StatePtr newState = StateLibrary::Instance().CreateInstance(stateName.c_str());

	if (newState != NULL)
	{
		// Change the state, the pointer is ok
		_state = newState;

		// Initialise the new state
		_state->Init(_owner.GetEntity());
	}
	else
	{
		gameLocal.Error("BasicMind: Could not change state to %s", stateName.c_str());
	}
}

// Returns the reference to the current state
StatePtr& BasicMind::GetState()
{
	return _state;
}

// Get the current alert state 
EAlertState BasicMind::GetAlertState() const
{
	return _alertState;
}

// Set the current alert state
void BasicMind::SetAlertState(EAlertState newState)
{
	_alertState = newState;
	// TODO: Emit onAlertChange signal
}

void BasicMind::Save(idSaveGame* savefile) const 
{
	// TODO
}

void BasicMind::Restore(idRestoreGame* savefile) 
{
	// TODO
}

} // namespace ai
