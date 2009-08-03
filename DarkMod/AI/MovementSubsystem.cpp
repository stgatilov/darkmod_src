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

namespace ai
{

#define HISTORY_SIZE 32
#define HISTORY_BOUNDS_THRESHOLD 10 // units
#define BLOCK_TIME_OUT 3000 // milliseconds

MovementSubsystem::MovementSubsystem(SubsystemId subsystemId, idAI* owner) :
	Subsystem(subsystemId, owner),
	_curHistoryIndex(0),
	_historyBoundsThreshold(HISTORY_BOUNDS_THRESHOLD),
	_state(ENotBlocked),
	_lastTimeNotBlocked(-1),
	_blockTimeOut(BLOCK_TIME_OUT)
{
	_historyBounds.Clear();

	_originHistory.SetNum(HISTORY_SIZE);
}

// Called regularly by the Mind to run the currently assigned routine.
bool MovementSubsystem::PerformTask()
{
	idAI* owner = _owner.GetEntity();

	// Watchdog to keep AI from running into things forever
	CheckBlocked(owner);
	
	return Subsystem::PerformTask();
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

		bool belowThreshold = _historyBounds.GetRadius() < _historyBoundsThreshold;

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

					// TODO: Send a signal to the current State
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
		};
	}
	else
	{
		// Not moving, or sleeping, or something else
		_historyBounds.Clear();
	}

	DebugDraw(owner);
}

// Save/Restore methods
void MovementSubsystem::Save(idSaveGame* savefile) const
{
	Subsystem::Save(savefile);

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
		}
		gameRenderWorld->DrawText(str.c_str(), 
			(owner->GetEyePosition() - owner->GetPhysics()->GetGravityNormal()*60.0f), 
			0.25f, colour, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 3 * gameLocal.msec);
	}
}

} // namespace ai
