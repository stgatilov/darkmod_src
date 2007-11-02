/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: angua $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: TakeCoverState.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "TakeCoverState.h"
#include "../Memory.h"
#include "../Tasks/MoveToCoverTask.h"
#include "../Tasks/WaitTask.h"
#include "../Tasks/MoveToPositionTask.h"
#include "../Tasks/IdleSensoryTask.h"
#include "LostTrackOfEnemyState.h"
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
	Memory& memory = owner->GetMind()->GetMemory();

	// angua: The last position of the AI before it takes cover, so it can return to it later.
	_positionBeforeTakingCover = owner->GetPhysics()->GetOrigin();

	// Fill the subsystems with their tasks

	// The movement subsystem should wait half a second and then run to Cover position, 
	// wait there for some time and then emerge to have a look.
	owner->StopMove(MOVE_STATUS_DONE);
	owner->GetSubsystem(SubsysMovement)->ClearTasks();
	owner->GetSubsystem(SubsysMovement)->PushTask(TaskPtr(new WaitTask(500)));
	owner->GetSubsystem(SubsysMovement)->QueueTask(MoveToCoverTask::CreateInstance());
	_takingCover = true;
	owner->AI_MOVE_DONE = false;

	// Calculate the time we should stay in cover
	int coverDelayMin = SEC2MS(owner->spawnArgs.GetFloat("emerge_from_cover_delay_min"));
	int coverDelayMax = SEC2MS(owner->spawnArgs.GetFloat("emerge_from_cover_delay_max"));
	_emergeDelay = coverDelayMin + gameLocal.random.RandomFloat() * (coverDelayMax - coverDelayMin);

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

	if (_takingCover && owner->AI_MOVE_DONE)
	{
		// When we are done moving to cover position, stay for some time there 
		// then come out and move back to where we were standing before taking cover
		owner->AI_RUN = false;
		owner->FaceEnemy();
		owner->GetSubsystem(SubsysMovement)->ClearTasks();
		owner->GetSubsystem(SubsysMovement)->PushTask(TaskPtr(new WaitTask(_emergeDelay)));
		owner->GetSubsystem(SubsysMovement)->QueueTask(TaskPtr(new MoveToPositionTask(_positionBeforeTakingCover)));

		owner->GetSubsystem(SubsysSenses)->ClearTasks();
		owner->GetSubsystem(SubsysSenses)->QueueTask(IdleSensoryTask::CreateInstance());
		
		_takingCover = false;
	}

	if (owner->AI_DEST_UNREACHABLE && !_takingCover)
	{
		// Can't move back to position before taking cover
		owner->GetMind()->SwitchState(STATE_LOST_TRACK_OF_ENEMY);
	}
	
	if (owner->AI_MOVE_DONE && owner->GetPhysics()->GetOrigin() == _positionBeforeTakingCover && !_takingCover)
	{
		// Reached position before taking cover, look for enemy
		// Turn to last visible enemy position
		owner->TurnToward(owner->lastVisibleEnemyPos);

		// If no enemy is visible, we lost track of him
		idActor* enemy = owner->GetEnemy();
		if (!enemy || !owner->CanSeeExt(enemy, true, true))
		{
			owner->GetMind()->SwitchState(STATE_LOST_TRACK_OF_ENEMY);
		}
	}
}

void TakeCoverState::Save(idSaveGame* savefile) const
{
	State::Save(savefile);

	savefile->WriteVec3(_positionBeforeTakingCover);
	savefile->WriteInt(_emergeDelay);
	savefile->WriteBool(_takingCover);
}

void TakeCoverState::Restore(idRestoreGame* savefile)
{
	State::Restore(savefile);

	savefile->ReadVec3(_positionBeforeTakingCover);
	savefile->ReadInt(_emergeDelay);
	savefile->ReadBool(_takingCover);
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
