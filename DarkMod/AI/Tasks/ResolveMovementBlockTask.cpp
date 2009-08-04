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

#include "ResolveMovementBlockTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

ResolveMovementBlockTask::ResolveMovementBlockTask() :
	_blockingEnt(NULL),
	_preTaskContents(-1)
{}

ResolveMovementBlockTask::ResolveMovementBlockTask(idEntity* blockingEnt) :
	_blockingEnt(blockingEnt),
	_preTaskContents(-1)
{}

// Get the name of this task
const idStr& ResolveMovementBlockTask::GetName() const
{
	static idStr _name(TASK_RESOLVE_MOVEMENT_BLOCK);
	return _name;
}

void ResolveMovementBlockTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);

	if (_blockingEnt == NULL)
	{
		DM_LOG(LC_AI, LT_WARNING)LOGSTRING("AI %s cannot resolve a NULL blocking entity.", owner->name.c_str());
		subsystem.FinishTask();
	}

	// Get the direction we're pushing against
	_initialAngles = owner->viewAxis.ToAngles();

	// Set the TTL for this task
	_endTime = gameLocal.time + 20000; // 20 seconds

	// Save the movement state
	owner->PushMove();

	if (_blockingEnt->IsType(idAI::Type))
	{
		DM_LOG(LC_AI, LT_WARNING)LOGSTRING("AI %s starting to resolve blocking AI: %s", owner->name.c_str(), _blockingEnt->name.c_str());
		InitBlockingAI(owner, subsystem);
	}
	else if (_blockingEnt->IsType(idStaticEntity::Type))
	{
		DM_LOG(LC_AI, LT_WARNING)LOGSTRING("AI %s starting to resolve static blocking entity: %s", owner->name.c_str(), _blockingEnt->name.c_str());
		InitBlockingStatic(owner, subsystem);
	}
	else
	{
		// Unknown entity type, exit task
		DM_LOG(LC_AI, LT_WARNING)LOGSTRING("AI %s cannot resolve blocking entity: %s", owner->name.c_str(), _blockingEnt->name.c_str());
		subsystem.FinishTask(); 
	}
}

void ResolveMovementBlockTask::InitBlockingAI(idAI* owner, Subsystem& subsystem)
{
	idVec3 ownerRight, ownerForward;
	_initialAngles.ToVectors(&ownerForward, &ownerRight);

	// Move 10 units to the right
	owner->MoveToPosition(owner->GetPhysics()->GetOrigin() + ownerRight*10, 5);
}

void ResolveMovementBlockTask::InitBlockingStatic(idAI* owner, Subsystem& subsystem)
{
	// Get the bounds of the blocking entity and see if there is a way around it
	idPhysics* blockingPhys = _blockingEnt->GetPhysics();
	idBounds blockBounds = blockingPhys->GetAbsBounds();
	blockBounds[0].z = blockBounds[1].z = 0;

	// Check if there is space right to the obstacle
	idBounds bounds = owner->GetPhysics()->GetBounds();

	if (owner->GetAAS() == NULL) return;

	// angua: move the bottom of the bounds up a bit, to avoid finding small objects on the ground that are "in the way"
	bounds[0][2] += owner->GetAAS()->GetSettings()->maxStepHeight;

	// check if there is a way around
	idTraceModel trm(bounds);
	idClipModel clip(trm);

	idVec3 ownerRight, ownerForward;
	_initialAngles.ToVectors(&ownerForward, &ownerRight);

	// Take a point to the right
	idVec3 testPoint = blockingPhys->GetOrigin();
	testPoint.z = owner->GetPhysics()->GetOrigin().z;

	// Move one AAS bounding box size outwards from the model
	float blockBoundsWidth = blockBounds.GetRadius(blockBounds.GetCenter());

	idBounds aasBounds = owner->GetAAS()->GetSettings()->boundingBoxes[0];
	aasBounds[0].z = aasBounds[1].z = 0;

	float aasBoundsWidth = aasBounds.GetRadius();

	testPoint += ownerRight * (blockBoundsWidth + aasBoundsWidth);

	int contents = gameLocal.clip.Contents(testPoint, &clip, mat3_identity, CONTENTS_SOLID, owner);

	if (cv_ai_debug_blocked.GetBool())
	{
		idVec3 temp = blockingPhys->GetOrigin();
		temp.z = owner->GetPhysics()->GetOrigin().z;

		gameRenderWorld->DebugArrow(colorWhite, temp, temp + idVec3(0,0,10), 1, 1000);

		temp += ownerRight * (blockBoundsWidth + aasBoundsWidth + 5);
		gameRenderWorld->DebugArrow(colorLtGrey, temp, temp + idVec3(0,0,10), 1, 1000);

		gameRenderWorld->DebugBounds(contents ? colorRed : colorGreen, bounds, testPoint, 1000);
	}

	if (contents == 0)
	{
		owner->MoveToPosition(testPoint);
		return;
	}

	// Right side is blocked, look at the left
	testPoint = blockingPhys->GetOrigin();
	testPoint.z = owner->GetPhysics()->GetOrigin().z;
	testPoint -= ownerRight * (blockBoundsWidth + aasBoundsWidth + 5);

	contents = gameLocal.clip.Contents(testPoint, &clip, mat3_identity, CONTENTS_SOLID, owner);

	if (cv_ai_debug_blocked.GetBool())
	{
		gameRenderWorld->DebugBounds(contents ? colorRed : colorGreen, bounds, testPoint, 1000);
	}

	if (contents == 0)
	{
		owner->MoveToPosition(testPoint);
		return;
	}

	// Neither left nor right have free space
	owner->StopMove(MOVE_STATUS_BLOCKED_BY_OBJECT);
	owner->AI_BLOCKED = true;
	owner->AI_DEST_UNREACHABLE = true;

	subsystem.FinishTask();
}

bool ResolveMovementBlockTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("ResolveMovementBlockTask performing.\r");

	if (gameLocal.time > _endTime)
	{
		// Timeout
		return true;
	}

	idAI* owner = _owner.GetEntity();

	// This task may not be performed with empty entity pointer
	assert(owner != NULL);

	if (_blockingEnt->IsType(idAI::Type))
	{
		return PerformBlockingAI(owner);
	}
	else if (_blockingEnt->IsType(idStaticEntity::Type))
	{
		return PerformBlockingStatic(owner);
	}
	
	return false; // not finished yet
}

bool ResolveMovementBlockTask::PerformBlockingAI(idAI* owner)
{
	if (owner->AI_MOVE_DONE)
	{
		idVec3 ownerRight, ownerForward;
		_initialAngles.ToVectors(&ownerForward, &ownerRight);

		owner->StopMove(MOVE_STATUS_WAITING);
		owner->TurnToward(owner->GetPhysics()->GetOrigin() - ownerRight);

		if (owner->FacingIdeal() && _preTaskContents == -1)
		{
			_preTaskContents = owner->GetPhysics()->GetContents();
			owner->GetPhysics()->SetContents(0);

			// Set all attachments to nonsolid, temporarily
			owner->SaveAttachmentContents();
			owner->SetAttachmentContents(0);
		}
	}

	if (_preTaskContents != -1)
	{
		// We are waiting on the other AI to pass by
		idVec3 dist = _blockingEnt->GetPhysics()->GetOrigin() - owner->GetPhysics()->GetOrigin();

		if (dist.LengthSqr() > Square(60))
		{
			// other AI has passed, end the task
			return true;
		}
	}

	return false;
}

bool ResolveMovementBlockTask::PerformBlockingStatic(idAI* owner)
{
	return owner->AI_MOVE_DONE ? true : false;
}

void ResolveMovementBlockTask::OnFinish(idAI* owner)
{
	if (_preTaskContents != -1)
	{
		owner->GetPhysics()->SetContents(_preTaskContents);

		// Restore attachment contents again
		owner->RestoreAttachmentContents();
	}

	owner->PopMove();
}

// Save/Restore methods
void ResolveMovementBlockTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	savefile->WriteObject(_blockingEnt);
	savefile->WriteAngles(_initialAngles);
	savefile->WriteInt(_preTaskContents);
	savefile->WriteInt(_endTime);
}

void ResolveMovementBlockTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	savefile->ReadObject(reinterpret_cast<idClass*&>(_blockingEnt));
	savefile->ReadAngles(_initialAngles);
	savefile->ReadInt(_preTaskContents);
	savefile->ReadInt(_endTime);
}

ResolveMovementBlockTaskPtr ResolveMovementBlockTask::CreateInstance()
{
	return ResolveMovementBlockTaskPtr(new ResolveMovementBlockTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar resolveMovementBlockTaskRegistrar(
	TASK_RESOLVE_MOVEMENT_BLOCK, // Task Name
	TaskLibrary::CreateInstanceFunc(&ResolveMovementBlockTask::CreateInstance) // Instance creation callback
);

} // namespace ai
