/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2008-04-11 18:53:28 +0200 (Fr, 11 Apr 2008) $
 * $Author: greebo $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: AnimalAnimalPatrolTask.cpp 1435 2008-04-11 16:53:28Z greebo $", init_version);

#include "AnimalPatrolTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

	namespace {
		const float WANDER_RADIUS = 240;
	}

AnimalPatrolTask::AnimalPatrolTask() :
	_state(stateNone)
{}

// Get the name of this task
const idStr& AnimalPatrolTask::GetName() const
{
	static idStr _name(TASK_ANIMAL_PATROL);
	return _name;
}

void AnimalPatrolTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Init the base class
	Task::Init(owner, subsystem);

	_state = stateNone;

	// Check if we are supposed to patrol and make sure that there
	// is a valid PathCorner entity set in the AI's mind

	if (owner->spawnArgs.GetBool("patrol", "1")) 
	{
		idPathCorner* path = owner->GetMemory().currentPath.GetEntity();

		// Check if we already have a path entity
		if (path == NULL)
		{
			// Path not yet initialised, get it afresh
			// Find the next path associated with the owning AI
			path = idPathCorner::RandomPath(owner, NULL);
		}

		// If the path is still NULL, there is nothing setup, quit this task
		if (path == NULL)
		{
			// No path corner entities found!
			DM_LOG(LC_AI, LT_INFO).LogString("Warning: No Path corner entites found for %s\r", owner->name.c_str());
			
			subsystem.FinishTask();
			return;
		}

		// Store the path entity back into the mind, it might have changed
		owner->GetMemory().currentPath = path;
	}
	else
	{
		subsystem.FinishTask();
		return;
	}
}

bool AnimalPatrolTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("Patrol Task performing.\r");

	idPathCorner* path = _owner.GetEntity()->GetMind()->GetMemory().currentPath.GetEntity();

	// This task may not be performed with an empty path corner entity,
	// that case should have been caught by the Init() routine
	assert(path);

	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	switch (_state) 
	{
		case stateNone: 
			//gameRenderWorld->DrawText("Choosing", owner->GetPhysics()->GetOrigin(), 0.6f, colorGreen, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 32);
			chooseNewState(owner);
			break;
		case stateMovingToNextSpot:
			//gameRenderWorld->DrawText("MovingToNextSpot", owner->GetPhysics()->GetOrigin(), 0.6f, colorGreen, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 32);
			//gameRenderWorld->DebugArrow(colorYellow, owner->GetPhysics()->GetOrigin(), owner->GetMoveDest(), 0, 64);
			movingToNextSpot(owner);
			break;
		case stateMovingToNextPathCorner: 
			//gameRenderWorld->DrawText("MovingToNextCorner", owner->GetPhysics()->GetOrigin(), 0.6f, colorGreen, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 32);
			movingToNextPathCorner(owner);
			break;
		case stateDoingSomething: 
			//gameRenderWorld->DrawText("DoingSomething", owner->GetPhysics()->GetOrigin(), 0.6f, colorGreen, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 32);
			switchToState(stateWaiting, owner);
			break;
		case stateWaiting:
			//gameRenderWorld->DrawText("Waiting", owner->GetPhysics()->GetOrigin(), 0.6f, colorGreen, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 32);
			waiting(owner);
			break;
	};

	return false; // not finished yet
}

void AnimalPatrolTask::switchToState(EState newState, idAI* owner) 
{
	switch (newState)
	{
	case stateMovingToNextSpot:
		{
			// Choose a goal position, somewhere near ourselves
			float xDelta = gameLocal.random.RandomFloat()*WANDER_RADIUS - WANDER_RADIUS*0.5f;
			float yDelta = gameLocal.random.RandomFloat()*WANDER_RADIUS - WANDER_RADIUS*0.5f;

			const idVec3& curPos = owner->GetPhysics()->GetOrigin();
			idVec3 newPos = curPos + idVec3(xDelta, yDelta, 5);

			owner->MoveToPosition(newPos);

			// Run with a 20% chance
			owner->AI_RUN = (gameLocal.random.RandomFloat() < 0.2f);
		}
		break;
	case stateWaiting:
		_waitEndTime = gameLocal.time + gameLocal.random.RandomInt(1000);
		break;
	case stateMovingToNextPathCorner:
		{
			idPathCorner* path = owner->GetMemory().currentPath.GetEntity();
			if (path != NULL)
			{
				owner->MoveToPosition(path->GetPhysics()->GetOrigin());
				owner->AI_RUN = path->spawnArgs.GetBool("run", "0");
			}
		}
		break;
	}

	_state = newState;
}

void AnimalPatrolTask::chooseNewState(idAI* owner) 
{
	// For now, choose randomly between the various possibilities
	int rand = gameLocal.random.RandomInt(static_cast<int>(stateCount-1)) + 1;
	
	// Switch and initialise the states
	switchToState(static_cast<EState>(rand), owner);
}

void AnimalPatrolTask::movingToNextSpot(idAI* owner) 
{
	if (owner->AI_MOVE_DONE) 
	{
		if (owner->AI_DEST_UNREACHABLE) 
		{
			// Destination is unreachable, switch to new state
			chooseNewState(owner);
		}

		// We've reached the destination, wait a bit
		switchToState(stateWaiting, owner);
	}
	else if (gameLocal.random.RandomFloat() < 0.1f)
	{
		// Still moving, maybe turn a bit
		owner->Event_SaveMove();

		float xDelta = gameLocal.random.RandomFloat()*20 - 10;
		float yDelta = gameLocal.random.RandomFloat()*20 - 10;

		// Try to move the goal a bit
		if (!owner->MoveToPosition(owner->GetMoveDest() + idVec3(xDelta, yDelta, 2)))
		{
			// MoveToPosition failed, restore the move state
			owner->Event_RestoreMove();
		}
	}
}

void AnimalPatrolTask::movingToNextPathCorner(idAI* owner) 
{
	if (owner->AI_MOVE_DONE) 
	{
		// Find the next path associated with the owning AI
		owner->GetMemory().currentPath = idPathCorner::RandomPath(owner->GetMemory().currentPath.GetEntity(), NULL);

		if (owner->AI_DEST_UNREACHABLE) 
		{
			// Destination is unreachable, switch to new state
			chooseNewState(owner);
		}

		// We've reached the destination, wait a bit
		switchToState(stateDoingSomething, owner);
	}
	else 
	{
		// Toggle running with a 5% chance
		if (gameLocal.random.RandomFloat() < 0.05f)
		{
			owner->AI_RUN = !owner->AI_RUN;
		}
	}
}

void AnimalPatrolTask::waiting(idAI* owner)
{
	// Switch the state if the time has come
	if (gameLocal.time > _waitEndTime) 
	{
		chooseNewState(owner);
	}
}

void AnimalPatrolTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	savefile->WriteInt(static_cast<int>(_state));
	savefile->WriteInt(_waitEndTime);
}

void AnimalPatrolTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	int temp;
	savefile->ReadInt(temp);
	_state = static_cast<EState>(temp);

	savefile->ReadInt(_waitEndTime);
}

AnimalPatrolTaskPtr AnimalPatrolTask::CreateInstance()
{
	return AnimalPatrolTaskPtr(new AnimalPatrolTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar animalPatrolTaskRegistrar(
	TASK_ANIMAL_PATROL, // Task Name
	TaskLibrary::CreateInstanceFunc(&AnimalPatrolTask::CreateInstance) // Instance creation callback
);

} // namespace ai
