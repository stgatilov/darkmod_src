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
	
	// greebo: We do not check for NULL pointers in the owner at this point, 
	// as this method is called by the owner itself.

	idAI* owner = _owner.GetEntity();

	owner->GetSubsystem(SubsysMovement)->PerformTask();
	//owner->GetSubsystem(SubsysCommunication)->PerformTask();
}

// Changes the state
void BasicMind::ChangeState(const idStr& stateName)
{
	
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
