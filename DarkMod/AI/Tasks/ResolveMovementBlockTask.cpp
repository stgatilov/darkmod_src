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
	_masterAI(NULL),
	_preTaskContents(-1)
{}

ResolveMovementBlockTask::ResolveMovementBlockTask(idAI* masterAI) :
	_masterAI(masterAI),
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

	// Get the direction we're pushing against
	_initialAngles = owner->viewAxis.ToAngles();

	idVec3 ownerRight, ownerForward;
	_initialAngles.ToVectors(&ownerForward, &ownerRight);

	idVec3 masterForward;

	if (_masterAI != NULL) 
	{
		masterForward = _masterAI->viewAxis.ToAngles().ToForward();
	}
	else
	{
		DM_LOG(LC_AI, LT_INFO)LOGSTRING("Initialising ResolveMovementBlockTask without master AI.\r");
		// Assume the other AI is facing the opposite direction
		masterForward = -ownerForward;
	}

	// Set the TTL for this task
	_endTime = gameLocal.time + 20000; // 20 seconds

	// Save the movement state
	owner->PushMove();

	// Move 10 units to the right
	owner->MoveToPosition(owner->GetPhysics()->GetOrigin() + ownerRight*10, 5);
}

bool ResolveMovementBlockTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("ResolveMovementBlockTask performing.\r");

	idAI* owner = _owner.GetEntity();

	// This task may not be performed with empty entity pointer
	assert(owner != NULL);

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
		idVec3 dist = _masterAI->GetPhysics()->GetOrigin() - owner->GetPhysics()->GetOrigin();

		if (dist.LengthSqr() > Square(60))
		{
			// other AI has passed, end the task
			return true;
		}
	}

	if (gameLocal.time > _endTime)
	{
		// Timeout
		return true;
	}

	return false; // not finished yet
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

	savefile->WriteObject(_masterAI);
	savefile->WriteAngles(_initialAngles);
	savefile->WriteInt(_preTaskContents);
	savefile->WriteInt(_endTime);
}

void ResolveMovementBlockTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	savefile->ReadObject(reinterpret_cast<idClass*&>(_masterAI));
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
