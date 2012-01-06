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

#include "../Memory.h"
#include "PathCornerTask.h"
#include "../Library.h"

namespace ai
{

PathCornerTask::PathCornerTask() :
	PathTask(),
	_moveInitiated(false),
	_lastPosition(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY),
	_lastFrameNum(-1),
	_usePathPrediction(false)
{}

PathCornerTask::PathCornerTask(idPathCorner* path) :
	PathTask(path),
	_moveInitiated(false),
	_lastPosition(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY),
	_lastFrameNum(-1),
	_usePathPrediction(false)
{
	_path = path;
}

// Get the name of this task
const idStr& PathCornerTask::GetName() const
{
	static idStr _name(TASK_PATH_CORNER);
	return _name;
}

void PathCornerTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	PathTask::Init(owner, subsystem);

	_lastPosition = owner->GetPhysics()->GetOrigin();
	_lastFrameNum = gameLocal.framenum;

	// Check the "run" spawnarg of this path entity
	owner->AI_RUN = (_path.GetEntity()->spawnArgs.GetBool("run", "0"));

	idPathCorner* nextPath = owner->GetMemory().nextPath.GetEntity();

	// Allow path prediction only if the next path is an actual path corner and no accuracy is set on this one
	if (_accuracy == -1 && nextPath != NULL && idStr::Icmp(nextPath->spawnArgs.GetString("classname"), "path_corner") == 0)
	{
		_usePathPrediction = true;
	}
}

bool PathCornerTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("Path Corner Task performing.\r");

	idPathCorner* path = _path.GetEntity();
	idAI* owner = _owner.GetEntity();

	// This task may not be performed with empty entity pointers
	assert(path != NULL && owner != NULL);

	// grayman #2345 - if you've timed out of a door queue, go back to your
	// previous path_corner

	if (owner->m_leftQueue)
	{
		owner->m_leftQueue = false;
		Memory& memory = owner->GetMemory();
		idPathCorner* tempPath = memory.lastPath.GetEntity();
		if (tempPath != NULL)
		{
			memory.lastPath = path;
			_path = tempPath;
			path = tempPath;
			memory.currentPath = path;
			memory.nextPath = idPathCorner::RandomPath(path, NULL, owner);
			owner->StopMove(MOVE_STATUS_DONE); // lets the new pathing take over
		}
	}

	if (_moveInitiated)
	{
		const idVec3& ownerOrigin = owner->GetPhysics()->GetOrigin();

		if (owner->AI_MOVE_DONE)
		{
			if (owner->ReachedPos(path->GetPhysics()->GetOrigin(), MOVE_TO_POSITION))
			{
				// Trigger path targets, now that we've reached the corner
				owner->ActivateTargets(owner);

				// NextPath();

				// Move is done, fall back to PatrolTask
				DM_LOG(LC_AI, LT_INFO)LOGSTRING("Move is done.\r");

				// grayman #2712 - clear REUSE timeout on the last door used

				Memory& memory = owner->GetMemory();
				CFrobDoor* frobDoor = memory.lastDoorHandled.GetEntity();
				if (frobDoor != NULL)
				{
					memory.GetDoorInfo(frobDoor).lastTimeUsed = -1; // reset timeout
				}

				return true; // finish this task
			}
			else
			{
				owner->MoveToPosition(path->GetPhysics()->GetOrigin(), _accuracy);
			}
		}
		else if (_usePathPrediction) 
		{
			// Move not done yet. Try to perform a prediction whether we will hit the path corner
			// next round. This is valid, as no accuracy is set on the current path.
			
			idVec3 moveDeltaVec = ownerOrigin - _lastPosition;
			float moveDelta = moveDeltaVec.NormalizeFast();
		
			// grayman #2414 - start of new prediction code

			if (moveDelta > 0)
			{
				idVec3 toPath = path->GetPhysics()->GetOrigin() - ownerOrigin;
				toPath.z = 0; // ignore vertical component
				float distToPath = toPath.NormalizeFast();

				// The move direction and the distance vector to the path are pointing in roughly the same direction.
				// The prediction will be rather accurate.

				if (toPath * moveDeltaVec > 0.7f)
				{
					bool turnNow = false; // whether it's time to make the turn

					// will we overshoot the path_corner within the next two checks?

					if (distToPath <= PATH_PREDICTION_MOVES*moveDelta) // quick check for overshooting
					{
						turnNow = true;
					}
					else
					{
						int frameDelta = gameLocal.framenum - _lastFrameNum;
						float factor = PATH_PREDICTION_MOVES + PATH_PREDICTION_CONSTANT/static_cast<float>(frameDelta);
						if (distToPath <= factor*moveDelta) // consider the size of the AI's bounding box if we're w/in a certain range
						{
							// Virtually translate the path_corner position back to the origin and
							// see if ReachedPos's box check would succeed.

							turnNow = owner->ReachedPosAABBCheck(path->GetPhysics()->GetOrigin() - toPath*factor*moveDelta);
						}
					}

					if (turnNow)
					{
						// Trigger path targets, now that we've almost reached the corner
						owner->ActivateTargets(owner);

						// NextPath();

						// Move is done, fall back to PatrolTask
						DM_LOG(LC_AI, LT_INFO)LOGSTRING("PathCornerTask ending prematurely.\r");

						// End this task, let the next patrol/pathcorner task take up its work before
						// the AI code is actually reaching its position and issuing StopMove
						return true;
					}
				}
			}
			else 
			{
				// No movement - this can happen right at the first execution or when blocked
				// Blocks are handled by the movement subsystem, so ignore this case
			}
		}

		_lastPosition = ownerOrigin;
		_lastFrameNum = gameLocal.framenum;

		if (owner->AI_DEST_UNREACHABLE)
		{
			// Unreachable, fall back to PatrolTask
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Destination is unreachable, skipping.\r");

			// NextPath();
			return true; // finish this task
		}

		// Move...
	}
	else
	{
		// moveToEntity() not yet called, do it now
		owner->StopMove(MOVE_STATUS_DEST_NOT_FOUND);
		owner->MoveToPosition(path->GetPhysics()->GetOrigin(), _accuracy);

		_moveInitiated = true;
	}

	return false; // not finished yet
}


// Save/Restore methods
void PathCornerTask::Save(idSaveGame* savefile) const
{
	PathTask::Save(savefile);

	savefile->WriteBool(_moveInitiated);
	savefile->WriteVec3(_lastPosition);
	savefile->WriteInt(_lastFrameNum);
	savefile->WriteBool(_usePathPrediction);
}

void PathCornerTask::Restore(idRestoreGame* savefile)
{
	PathTask::Restore(savefile);

	savefile->ReadBool(_moveInitiated);
	savefile->ReadVec3(_lastPosition);
	savefile->ReadInt(_lastFrameNum);
	savefile->ReadBool(_usePathPrediction);
}

PathCornerTaskPtr PathCornerTask::CreateInstance()
{
	return PathCornerTaskPtr(new PathCornerTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar pathCornerTaskRegistrar(
	TASK_PATH_CORNER, // Task Name
	TaskLibrary::CreateInstanceFunc(&PathCornerTask::CreateInstance) // Instance creation callback
);

} // namespace ai
