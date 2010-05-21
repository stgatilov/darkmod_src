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

#include "MovementSubsystem.h"
#include "Library.h"
#include "States/State.h"
#include "Tasks/ResolveMovementBlockTask.h"
#include "Tasks/AnimalPatrolTask.h"
#include "Tasks/PathCornerTask.h"
#include "Tasks/PathAnimTask.h"
#include "Tasks/PathTurnTask.h"
#include "Tasks/PathCycleAnimTask.h"
#include "Tasks/PathSitTask.h"
#include "Tasks/PathSleepTask.h"
#include "Tasks/PathWaitTask.h"
#include "Tasks/PathWaitForTriggerTask.h"
#include "Tasks/PathHideTask.h"
#include "Tasks/PathShowTask.h"
#include "Tasks/PathLookatTask.h"
#include "Tasks/PathInteractTask.h"
#include "Tasks/MoveToPositionTask.h"


namespace ai
{

#define HISTORY_SIZE 32
#define HISTORY_BOUNDS_THRESHOLD 10 // units
#define BLOCK_TIME_OUT 800 // milliseconds
#define MAX_PATH_CORNER_SEARCH_ITERATIONS 100

MovementSubsystem::MovementSubsystem(SubsystemId subsystemId, idAI* owner) :
	Subsystem(subsystemId, owner),
	_curHistoryIndex(0),
	_historyBoundsThreshold(HISTORY_BOUNDS_THRESHOLD),
	_state(ENotBlocked),
	_lastTimeNotBlocked(-1),
	_blockTimeOut(BLOCK_TIME_OUT)
{
	_patrolling = false;

	_historyBounds.Clear();

	_originHistory.SetNum(HISTORY_SIZE);
}

// Called regularly by the Mind to run the currently assigned routine.
bool MovementSubsystem::PerformTask()
{
	idAI* owner = _owner.GetEntity();

	// Watchdog to keep AI from running into things forever
	CheckBlocked(owner);

	Patrol();
	
	return Subsystem::PerformTask();
}


void MovementSubsystem::StartPatrol()
{
	if (!_patrolling)
	{
		idAI* owner = _owner.GetEntity();
		Memory& memory = owner->GetMemory();

		bool animalPatrol = owner->spawnArgs.GetBool("animal_patrol", "0");

		// Check if the owner has patrol routes set
		idPathCorner* path = memory.currentPath.GetEntity();
		idPathCorner* lastPath = memory.lastPath.GetEntity();
		
		if (path == NULL && lastPath == NULL)
		{
			// Get a new random path off the owner's targets, this is the current one
			path = idPathCorner::RandomPath(owner, NULL, owner);
			memory.currentPath = path;

			// Also, pre-select a next path to allow path predictions
			if (path != NULL)
			{
				memory.nextPath = idPathCorner::RandomPath(path, NULL, owner);
			}
		}
		else if (path != NULL)
		{
			// we already have a stored path, patrolling was already started and is resumed now
			idPathCorner* candidate = GetNextPathCorner(path, owner);

			if (candidate != NULL)
			{
				// advance to next path corner, don't resume other path tasks at the current (presumably wrong) position 
				memory.currentPath = candidate;
				memory.nextPath = idPathCorner::RandomPath(candidate, NULL, owner);
			}
			else
			{
				// We don't have a valid path_corner in our current path branch,
				// or we ended in a "dead end" or in a loop, restart the system
				RestartPatrol();
			}
		}
		else // path == NULL && last path != NULL
		{
			// patrol routine had ended before
			// restart patrolling
			RestartPatrol();
		}

		if (memory.currentPath.GetEntity() != NULL || animalPatrol)
		{
			if (animalPatrol)
			{
				// For animals, push the AnimalPatrol task anyway, they don't need paths
				PushTask(AnimalPatrolTask::CreateInstance());
			}
			else
			{
				StartPathTask();
			}
		
			_patrolling = true;
		}
	}
}

idPathCorner* MovementSubsystem::GetNextPathCorner(idPathCorner* curPath, idAI* owner)
{
	if (curPath == NULL) return NULL; // safety check

	idPathCorner* currentTestPath = curPath;
	
	for (int i = 0; i < MAX_PATH_CORNER_SEARCH_ITERATIONS; i++)
	{
		if (idStr::Cmp(currentTestPath->spawnArgs.GetString("classname"), "path_corner") == 0)
		{
			// found a path_corner
			return currentTestPath;
		}

		// get next path
		currentTestPath = idPathCorner::RandomPath(currentTestPath, NULL, owner);

		if (currentTestPath == NULL)
		{
			// dead end, return NULL
			return NULL;
		}
		else if (currentTestPath == curPath)
		{
			// loop detected
			return NULL;
		}
	}

	return NULL;
}

void MovementSubsystem::RestartPatrol()
{
	idAI* owner = _owner.GetEntity();
	Memory& memory = owner->GetMemory();

	idPathCorner* newPath = idPathCorner::RandomPath(owner, NULL, owner);
	memory.currentPath = newPath;
	memory.nextPath = idPathCorner::RandomPath(newPath, NULL, owner);

	// if the first path is a path corner, just start with that
	// otherwise, move to idle position before restarting patrol
	if (idStr::Cmp(newPath->spawnArgs.GetString("classname"), "path_corner") != 0)
	{
		float startPosTolerance = owner->spawnArgs.GetFloat("startpos_tolerance", "-1");
		owner->movementSubsystem->PushTask(
			TaskPtr(new MoveToPositionTask(memory.idlePosition, memory.idleYaw, startPosTolerance))
		);
	}
}

void MovementSubsystem::Patrol()
{
	idAI* owner = _owner.GetEntity();
	Memory& memory = owner->GetMemory();

	if (_patrolling == false)
	{
		return;
	}

	if (_taskQueue.empty())
	{
		NextPath();
		if (memory.currentPath.GetEntity() == NULL)
		{
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("No more targets.\r");
			_patrolling = false;
			return;
		}

		StartPathTask();
	}
}

void MovementSubsystem::NextPath()
{
	idAI* owner = _owner.GetEntity();
	Memory& memory = owner->GetMemory();

	idPathCorner* path = memory.currentPath.GetEntity();

	// The current path gets stored in lastPath
    memory.lastPath = path;

    // The pre-selected "next path" is now our current one
    idPathCorner* currentPath = memory.nextPath.GetEntity();

    memory.currentPath = currentPath;

    // Now pre-select a new (random) path entity for the next round
    // this information is important for the PathCornerTask to decide which action to take on exit
	idPathCorner* next(NULL);
	if (currentPath != NULL)
	{
		next = idPathCorner::RandomPath(currentPath, NULL, owner);
	}
	
    memory.nextPath = next;
}

void MovementSubsystem::StartPathTask()
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("Starting next path task.\r");

	idPathCorner* path = _owner.GetEntity()->GetMind()->GetMemory().currentPath.GetEntity();

	// This may not be performed with an empty path corner entity,
	// that case should have been caught by the Patrol() routine
	assert(path);

	std::list<TaskPtr> tasks;
	TaskPtr task;

	// Get the classname, this determines the child routine we're spawning.
	idStr classname = path->spawnArgs.GetString("classname");

	// Depending on the classname we spawn one of the various Path*Tasks
	if (classname == "path_corner")
	{
		tasks.push_back(TaskPtr(new PathCornerTask(path)));
	}
	else if (classname == "path_anim")
	{
		if (path->spawnArgs.FindKey("angle") != NULL)
		{
			// We have an angle key set, push a PathTurnTask on top of the anim task
			tasks.push_back(TaskPtr(new PathAnimTask(path)));
			// The "task" variable will be pushed later on in this code
			tasks.push_back(TaskPtr(new PathTurnTask(path)));
		}
		else 
		{
			// No "angle" key set, just schedule the animation task
			task = PathAnimTaskPtr(new PathAnimTask(path));
		}
	}
	else if (classname == "path_cycleanim")
	{
		if (path->spawnArgs.FindKey("angle") != NULL)
		{
			// We have an angle key set, push a PathTurnTask on top of the anim task
			tasks.push_back(TaskPtr(new PathCycleAnimTask(path)));
			// The "task" variable will be pushed later on in this code
			tasks.push_back(PathTurnTaskPtr(new PathTurnTask(path)));
		}
		else 
		{
			// No "angle" key set, just schedule the animation task
			tasks.push_back(PathCycleAnimTaskPtr(new PathCycleAnimTask(path)));
		}
	}
	else if (classname == "path_sit")
	{
		if (path->spawnArgs.FindKey("angle") != NULL)
		{
			// We have an angle key set, push a PathTurnTask on top of the anim task
			tasks.push_back(TaskPtr(new PathSitTask(path)));
			// The "task" variable will be pushed later on in this code
			tasks.push_back(PathTurnTaskPtr(new PathTurnTask(path)));
		}
		else 
		{
			// No "angle" key set, just schedule the animation task
			tasks.push_back(PathSitTaskPtr(new PathSitTask(path)));
		}
	}

	else if (classname == "path_sleep")
	{
		if (path->spawnArgs.FindKey("angle") != NULL)
		{
			// We have an angle key set, push a PathTurnTask on top of the sleep task
			tasks.push_back(TaskPtr(new PathSleepTask(path)));
			// The "task" variable will be pushed later on in this code
			tasks.push_back(PathTurnTaskPtr(new PathTurnTask(path)));
		}
		else 
		{
			// No "angle" key set, just schedule the sleep task
			tasks.push_back(PathSleepTaskPtr(new PathSleepTask(path)));
		}
	}

	else if (classname == "path_turn")
	{
		tasks.push_back(PathTurnTaskPtr(new PathTurnTask(path)));
	}
	else if (classname == "path_wait")
	{
		if (path->spawnArgs.FindKey("angle") != NULL)
		{
			// We have an angle key set, push a PathTurnTask on top of the anim task
			tasks.push_back(TaskPtr(new PathWaitTask(path)));
			// The "task" variable will be pushed later on in this code
			tasks.push_back(PathTurnTaskPtr(new PathTurnTask(path)));
		}
		else 
		{
			// No "angle" key set, just schedule the wait task
			tasks.push_back(PathWaitTaskPtr(new PathWaitTask(path)));
		}
	}
	else if (classname == "path_waitfortrigger")
	{
		tasks.push_back(PathWaitForTriggerTaskPtr(new PathWaitForTriggerTask(path)));
	}
	else if (classname == "path_hide")
	{
		tasks.push_back(PathHideTaskPtr(new PathHideTask(path)));
	}
	else if (classname == "path_show")
	{
		tasks.push_back(PathShowTaskPtr(new PathShowTask(path)));
	}
	else if (classname == "path_lookat")
	{
		tasks.push_back(PathLookatTaskPtr(new PathLookatTask(path)));
	}
	else if (classname == "path_interact")
	{
		tasks.push_back(PathInteractTaskPtr(new PathInteractTask(path)));
	}
	else
	{
		// Finish this task
		gameLocal.Warning("Unknown path corner classname '%s'\n", classname.c_str());
		return;
	}
	
	// Push the (rest of the) tasks to the subsystem
	for (std::list<TaskPtr>::iterator i = tasks.begin(); i != tasks.end(); ++i)
	{
		PushTask(*i);
	}
}

void MovementSubsystem::ClearTasks()
{
	Subsystem::ClearTasks();
	_patrolling = false;
}


void MovementSubsystem::CheckBlocked(idAI* owner)
{
	// Check the owner's move type to decide whether 
	// we should watch out for possible blocking or not
	if (owner->GetMoveType() == MOVETYPE_ANIM && 
		owner->AI_FORWARD)
	{
		// Owner is supposed to be moving
		const idVec3& ownerOrigin = owner->GetPhysics()->GetOrigin();

		_originHistory[_curHistoryIndex++] = ownerOrigin;

		// Wrap the index around a the boundaries
		_curHistoryIndex %= _originHistory.Num();

		// Calculate the new bounds
		_historyBounds.FromPoints(_originHistory.Ptr(), _originHistory.Num());

		bool belowThreshold = _historyBounds.GetRadius(_historyBounds.GetCenter()) < _historyBoundsThreshold;

		switch (_state)
		{
		case ENotBlocked:
			if (belowThreshold)
			{
				// Yellow alarm, we might be blocked, or we might as well
				// just have been starting to move
				_state = EPossiblyBlocked;

				// Changed state to possibly blocked, record time
				_lastTimeNotBlocked =  gameLocal.time - gameLocal.msec;
			}
			break;
		case EPossiblyBlocked:
			if (belowThreshold)
			{
				if (gameLocal.time > _lastTimeNotBlocked + _blockTimeOut)
				{
					// Blocked for too long, raise status
					_state = EBlocked;

					// Send a signal to the current State
					owner->GetMind()->GetState()->OnMovementBlocked(owner);
				}
			}
			else
			{
				// Bounds are safe, back to green state
				_state = ENotBlocked;
			}
			break;
		case EBlocked:
			if (!belowThreshold)
			{
				// Threshold exceeded, we're unblocked again
				_state = ENotBlocked;
			}
			break;
		case EResolvingBlock:
			// nothing so far
			break;
		};
	}
	else
	{
		// Not moving, or sleeping, or something else
		_historyBounds.Clear();
	}

	DebugDraw(owner);
}

void MovementSubsystem::SetBlockedState(const BlockedState newState)
{
	_state = newState;

	if (_state == ENotBlocked)
	{
		_lastTimeNotBlocked = gameLocal.time;
		_historyBounds.Clear();
	}
}

void MovementSubsystem::ResolveBlock(idEntity* blockingEnt)
{
	idAI* owner = _owner.GetEntity();
	
	if (owner->GetMemory().resolvingMovementBlock)
	{
		return; // Already resolving
	}

	// Push a resolution task
	PushTask(TaskPtr(new ResolveMovementBlockTask(blockingEnt)));

	// Remember this state
	SetBlockedState(EResolvingBlock);
}

bool MovementSubsystem::IsResolvingBlock()
{
	return _state == EResolvingBlock;
}

// Save/Restore methods
void MovementSubsystem::Save(idSaveGame* savefile) const
{
	Subsystem::Save(savefile);

	savefile->WriteBool(_patrolling);

	savefile->WriteInt(_originHistory.Num());

	for (int i = 0; i < _originHistory.Num(); ++i)
	{
		savefile->WriteVec3(_originHistory[i]);
	}

	savefile->WriteInt(_curHistoryIndex);
	savefile->WriteBounds(_historyBounds);
	savefile->WriteFloat(_historyBoundsThreshold);
	savefile->WriteInt(static_cast<int>(_state));
	savefile->WriteInt(_lastTimeNotBlocked);
	savefile->WriteInt(_blockTimeOut);
}

void MovementSubsystem::Restore(idRestoreGame* savefile)
{
	Subsystem::Restore(savefile);

	savefile->ReadBool(_patrolling);

	int num;
	savefile->ReadInt(num);

	_originHistory.SetNum(num);

	for (int i = 0; i < num; ++i)
	{
		savefile->ReadVec3(_originHistory[i]);
	}

	savefile->ReadInt(_curHistoryIndex);
	savefile->ReadBounds(_historyBounds);
	savefile->ReadFloat(_historyBoundsThreshold);

	int temp;
	savefile->ReadInt(temp);
	assert(temp >= ENotBlocked && temp <= EBlocked);
	_state = static_cast<BlockedState>(temp);

	savefile->ReadInt(_lastTimeNotBlocked);
	savefile->ReadInt(_blockTimeOut);
}

void MovementSubsystem::DebugDraw(idAI* owner)
{
	if (!cv_ai_debug_blocked.GetBool()) return;

	if (!_historyBounds.IsCleared())
	{
		gameRenderWorld->DebugBox(colorWhite, idBox(_historyBounds), 3* gameLocal.msec);

		idStr str;
		idVec4 colour;
		switch (_state)
		{
			case ENotBlocked:
				str = "ENotBlocked";
				colour = colorGreen;
				break;
			case EPossiblyBlocked:
				str = "EPossiblyBlocked";
				colour = colorYellow;
				break;
			case EBlocked:
				str = "EBlocked";
				colour = colorRed;
				break;
			case EResolvingBlock:
				str = "EResolvingBlock";
				colour = colorMagenta;
				break;
		}
		gameRenderWorld->DrawText(str.c_str(), 
			(owner->GetEyePosition() - owner->GetPhysics()->GetGravityNormal()*60.0f), 
			0.25f, colour, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 3 * gameLocal.msec);
	}
}

} // namespace ai
