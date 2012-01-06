/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod (http://www.thedarkmod.com/)
 
 $Revision$ (Revision of last commit) 
 $Date$ (Date of last commit)
 $Author$ (Author of last commit)
 
******************************************************************************/

#include "precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include "FleeTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

FleeTask::FleeTask() :
	_escapeSearchLevel(3), // 3 means FIND_FRIENDLY_GUARDED
	_failureCount(0), // This is used for _escapeLevel 1 only
	_fleeStartTime(gameLocal.time),
	_distOpt(DIST_NEAREST)
{}

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

	owner->AI_MOVE_DONE = false;
	owner->AI_RUN = true;
}

bool FleeTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("Flee Task performing.\r");

	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);
	Memory& memory = owner->GetMemory();
	idActor* enemy = _enemy.GetEntity();

	// angua: bad luck, my friend, yuo've been too slow...
	// no more fleeing necessary when dead or ko'ed
	if (owner->AI_DEAD || owner->AI_KNOCKEDOUT)
	{
		return true;
	}

	// if the enemy we're fleeing from dies, enemy gets set to NULL
	if (enemy == NULL )
	{
		return true;
	}

	//gameRenderWorld->DrawText( va("%d  %d",_escapeSearchLevel, _distOpt), owner->GetPhysics()->GetAbsBounds().GetCenter(), 
	// 	1.0f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, gameLocal.msec );

	// angua: in any case stop fleeing after max time (1 min).
	// Might stay in flee task forever if pathing to destination not possible otherwise
	// TODO: should be spawn arg, make member 
	int maxFleeTime = 60000;

	if (_failureCount > 5 || 
			(owner->AI_MOVE_DONE && !owner->AI_DEST_UNREACHABLE && !owner->m_HandlingDoor) ||
			gameLocal.time > _fleeStartTime + maxFleeTime)
	{
		owner->StopMove(MOVE_STATUS_DONE);
		//Fleeing is done, check if we can see the enemy
		if (owner->AI_ENEMY_VISIBLE)
		{
			_failureCount = 0;
			if (_distOpt == DIST_NEAREST)
			{
				// Find fleepoint far away
				_distOpt = DIST_FARTHEST;
				_escapeSearchLevel = 3;
			}
			else if (_escapeSearchLevel > 1)
			{
				_escapeSearchLevel --;
			}
		}
		else
		{
			memory.fleeingDone = true;
			// Turn around to look back to where we came from
			owner->TurnToward(owner->GetCurrentYaw() + 180);
			return true;
		}
	}
	
	if (owner->GetMoveStatus() != MOVE_STATUS_MOVING)
	{
		// If the AI is not running yet, start fleeing
		owner->AI_RUN = true;
		if (_escapeSearchLevel >= 3)
		{
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Trying to find escape route - FIND_FRIENDLY_GUARDED.");
			// Flee to the nearest friendly guarded escape point
			if (!owner->Flee(enemy, FIND_FRIENDLY_GUARDED, _distOpt))
			{
				_escapeSearchLevel = 2;
			}
			_fleeStartTime = gameLocal.time;
		}
		else if (_escapeSearchLevel == 2)
		{
			// Try to find another escape route
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Trying alternate escape route - FIND_FRIENDLY.");
			// Find another escape route to ANY friendly escape point
			if (!owner->Flee(enemy, FIND_FRIENDLY, _distOpt))
			{
				_escapeSearchLevel = 1;
			}
		}
		else
		{
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Searchlevel = 1, ZOMG, Panic mode, gotta run now!\r");

			// Get the distance to the enemy
			float enemyDistance = owner->TravelDistance(owner->GetPhysics()->GetOrigin(), enemy->GetPhysics()->GetOrigin());

			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Enemy is as near as %d", enemyDistance);
			if (enemyDistance < 500)
			{
				// Increase the fleeRadius (the nearer the enemy, the more)
				// The enemy is still near, run further
				if (!owner->Flee(enemy, FIND_AAS_AREA_FAR_FROM_THREAT, 500))
				{
					// No point could be found.
					_failureCount++;
				}
			}
			else
			{
				// Fleeing is done for now
				owner->StopMove(MOVE_STATUS_DONE);
			}
		}
	}

	return false; // not finished yet
}

void FleeTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	savefile->WriteInt(_escapeSearchLevel);
	savefile->WriteInt(_failureCount);
	savefile->WriteInt(_fleeStartTime);

	savefile->WriteInt(static_cast<int>(_distOpt));

	_enemy.Save(savefile);
}

void FleeTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	savefile->ReadInt(_escapeSearchLevel);
	savefile->ReadInt(_failureCount);
	savefile->ReadInt(_fleeStartTime);

	int distOptInt;
	savefile->ReadInt(distOptInt);
	_distOpt = static_cast<EscapeDistanceOption>(distOptInt);

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
