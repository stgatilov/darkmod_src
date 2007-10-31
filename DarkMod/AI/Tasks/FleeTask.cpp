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

static bool init_version = FileVersionList("$Id: FleeTask.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "FleeTask.h"
#include "../Memory.h"
#include "../Library.h"
#include "../../EscapePointManager.h"

namespace ai
{

// Get the name of this task
const idStr& FleeTask::GetName() const
{
	static idStr _name(TASK_FLEE);
	return _name;
}

void FleeTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Init the base class
	Task::Init(owner, subsystem);

	_enemy = owner->GetEnemy();
	owner->AI_RUN = true;
	owner->AI_FORWARD = true;

	int _escapeSearchLevel = 3; // 3 means FIND_FRIENDLY_GUARDED

}

bool FleeTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("Flee Task performing.\r");

	idAI* owner = _owner.GetEntity();
	idActor* enemy = _enemy.GetEntity();

	assert(owner != NULL);
	
	Memory& memory = owner->GetMind()->GetMemory();
	

	if (_escapeSearchLevel >= 3)
	{
		DM_LOG(LC_AI, LT_INFO).LogString("Trying to find escape route - FIND_FRIENDLY_GUARDED.");
		// Flee to the nearest friendly guarded escape point
		owner->Flee(enemy, FIND_FRIENDLY_GUARDED, DIST_NEAREST);
	}
	else if (_escapeSearchLevel == 2)
	{
		// Try to find another escape route
		DM_LOG(LC_AI, LT_INFO).LogString("Trying alternate escape route - FIND_FRIENDLY.");
		// Find another escape route to ANY friendly escape point
		owner->Flee(enemy, FIND_FRIENDLY, DIST_NEAREST);
	}
	else // escapeSearchLevel is 1
	{
		DM_LOG(LC_AI, LT_INFO).LogString("Searchlevel = 1, ZOMG, Panic mode, gotta run now!");

		// Get the distance to the enemy
		float enemyDistance = owner->TravelDistance(owner->GetPhysics()->GetOrigin(), enemy->GetPhysics()->GetOrigin());

		DM_LOG(LC_AI, LT_INFO).LogString("Enemy is as near as %d", enemyDistance);
		if (enemyDistance < 500)
		{
			// Increase the fleeRadius (the nearer the enemy, the more)
			int failureCount = 0;
			// The enemy is still near, run further
			if (!owner->Flee(enemy, FIND_AAS_AREA_FAR_FROM_THREAT, 500))
			{
				// No point could be found.
				failureCount++;

				if (failureCount > 5)
				{
					 return true;
				}
			}
		}
	}
		

	if (owner->AI_DEST_UNREACHABLE)
	{
		if (_escapeSearchLevel > 1)
		{
			// Decrease the escape search level
			_escapeSearchLevel--;
		}
	}


	if (owner->AI_MOVE_DONE && !owner->AI_DEST_UNREACHABLE) 
	{
		//Fleeing is done
		return true;

	}


	return false; // not finished yet
}

void FleeTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	savefile->WriteInt(_escapeSearchLevel);
	_enemy.Save(savefile);
}

void FleeTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	savefile->ReadInt(_escapeSearchLevel);
	_enemy.Restore(savefile);
}

FleeTaskPtr FleeTask::CreateInstance()
{
	return FleeTaskPtr(new FleeTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar fleeTaskRegistrar(
	TASK_FLEE, // Task Name
	TaskLibrary::CreateInstanceFunc(&FleeTask::CreateInstance) // Instance creation callback
);

} // namespace ai
