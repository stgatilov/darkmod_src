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

#include "../Memory.h"
#include "PatrolTask.h"
#include "PathCornerTask.h"
#include "../Library.h"

namespace ai
{

PathCornerTask::PathCornerTask() :
	PathTask(),
	_moveInitiated(false),
	_lastPosition(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY),
	_lastFrameNum(-1)
{}

PathCornerTask::PathCornerTask(idPathCorner* path) :
	PathTask(path),
	_moveInitiated(false),
	_lastPosition(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY),
	_lastFrameNum(-1)
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
}

bool PathCornerTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("Path Corner Task performing.\r");

	idPathCorner* path = _path.GetEntity();
	idAI* owner = _owner.GetEntity();

	// This task may not be performed with empty entity pointers
	assert(path != NULL && owner != NULL);

	if (_moveInitiated)
	{
		const idVec3& ownerOrigin = owner->GetPhysics()->GetOrigin();

		if (owner->AI_MOVE_DONE)
		{
			if (owner->ReachedPos(path->GetPhysics()->GetOrigin(), MOVE_TO_POSITION))
			{
				// Trigger path targets, now that we've reached the corner
				owner->ActivateTargets(owner);
				NextPath();

				// Move is done, fall back to PatrolTask
				DM_LOG(LC_AI, LT_INFO)LOGSTRING("Move is done.\r");

				return true; // finish this task
			}
			else
			{
				owner->MoveToPosition(path->GetPhysics()->GetOrigin(), _accuracy);
			}
		}
		else
		{
			// Move not done yet. Try to perform a prediction whether we will hit the path corner
			// next round.

			idVec3 moveDeltaVec = ownerOrigin - _lastPosition;
			float moveDelta = moveDeltaVec.NormalizeFast();

			if (moveDelta > 0)
			{
				idVec3 toPath = path->GetPhysics()->GetOrigin() - ownerOrigin;
				float distToPath = toPath.NormalizeFast();
				
				// The move direction and the distance vector to the path are pointing in roughly the same direction
				// The prediction will be rather accurate
				if (toPath * moveDeltaVec > 0.7f)
				{
					// Virtually translate the origin towards the path and see if the ReachedPos'
					// box check would succeed
					// Take the amount of frames into account between this and the last time this task was
					// performed. This is a non-constant value, so calculate it on the fly
					int frameDelta = (gameLocal.framenum - _lastFrameNum)*2 - 1;

					if (owner->ReachedPosAABBCheck(path->GetPhysics()->GetOrigin() - toPath * moveDelta * frameDelta))
					{
						// Trigger path targets, now that we've almost reached the corner
						owner->ActivateTargets(owner);
						NextPath();

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
			NextPath();
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
}

void PathCornerTask::Restore(idRestoreGame* savefile)
{
	PathTask::Restore(savefile);

	savefile->ReadBool(_moveInitiated);
	savefile->ReadVec3(_lastPosition);
	savefile->ReadInt(_lastFrameNum);
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
