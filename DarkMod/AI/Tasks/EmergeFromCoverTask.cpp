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

static bool init_version = FileVersionList("$Id: EmergeFromCoverTask.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "../Memory.h"
#include "EmergeFromCoverTask.h"
#include "../States/LostTrackOfEnemyState.h"
#include "../Library.h"

namespace ai
{

// Get the name of this task
const idStr& EmergeFromCoverTask::GetName() const
{
	static idStr _name(TASK_EMERGE_FROM_COVER);
	return _name;
}

void EmergeFromCoverTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Init the base class
	Task::Init(owner, subsystem);

	Memory& memory = owner->GetMind()->GetMemory();

	// Remember the position before taking cover and move back to it.
	idVec3 oldPosition = memory.positionBeforeTakingCover;
	owner->AI_RUN = false;
	owner->MoveToPosition(oldPosition);
}

bool EmergeFromCoverTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("Emerge From Cover Task performing.\r");

	idAI* owner = _owner.GetEntity();

	// This task may not be performed with empty entity pointer
	assert(owner != NULL);

	if (owner->AI_DEST_UNREACHABLE)
	{
		owner->GetMind()->SwitchState(STATE_LOST_TRACK_OF_ENEMY);
		return true;
	}
	
	if (owner->AI_MOVE_DONE)
	{
		// Turn to last visible enemy position
		owner->TurnToward(owner->lastVisibleEnemyPos);

		// If no enemy is visible, we lost track of him
		idActor* enemy = owner->GetEnemy();
		if (!enemy || !owner->CanSeeExt(enemy, true, true))
		{
			owner->GetMind()->SwitchState(STATE_LOST_TRACK_OF_ENEMY);
		}
	}

	return false; // not finished yet
}

// Save/Restore methods
void EmergeFromCoverTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);
}

void EmergeFromCoverTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);
}

EmergeFromCoverTaskPtr EmergeFromCoverTask::CreateInstance()
{
	return EmergeFromCoverTaskPtr(new EmergeFromCoverTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar emergeFromCoverTaskRegistrar(
	TASK_EMERGE_FROM_COVER, // Task Name
	TaskLibrary::CreateInstanceFunc(&EmergeFromCoverTask::CreateInstance) // Instance creation callback
);

} // namespace ai
