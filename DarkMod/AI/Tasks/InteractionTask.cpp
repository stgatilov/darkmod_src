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

#include "InteractionTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

InteractionTask::InteractionTask()
{}

InteractionTask::InteractionTask(idEntity* interactEnt) :
	_interactEnt(interactEnt)
{}

// Get the name of this task
const idStr& InteractionTask::GetName() const
{
	static idStr _name(TASK_INTERACTION);
	return _name;
}

void InteractionTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);

	if (_interactEnt == NULL) 
	{
		subsystem.FinishTask();
	}
	
	/*owner->StopMove(MOVE_STATUS_DONE);
	// Turn and look
	owner->TurnToward(_target->GetPhysics()->GetOrigin());
	owner->Event_LookAtEntity(_target, 1);

	// Start anim
	owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Use_righthand", 4);

	_waitEndTime = gameLocal.time + 600;*/
}

bool InteractionTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("InteractionTask performing.\r");

	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	if (gameLocal.time >= _waitEndTime)
	{
		// Trigger the frob action script
		_interactEnt->FrobAction(true);
		return true;
	}
	
	// Debug
	// gameRenderWorld->DebugArrow(colorGreen, owner->GetEyePosition(), _target->GetPhysics()->GetOrigin(), 10, 10000);
	
	return false; // finish this task
}

// Save/Restore methods
void InteractionTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	savefile->WriteObject(_interactEnt);
	savefile->WriteInt(_waitEndTime);
}

void InteractionTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	savefile->ReadObject(reinterpret_cast<idClass*&>(_interactEnt));
	savefile->ReadInt(_waitEndTime);
}

InteractionTaskPtr InteractionTask::CreateInstance()
{
	return InteractionTaskPtr(new InteractionTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar interactionTaskRegistrar(
	TASK_INTERACTION, // Task Name
	TaskLibrary::CreateInstanceFunc(&InteractionTask::CreateInstance) // Instance creation callback
);

} // namespace ai
