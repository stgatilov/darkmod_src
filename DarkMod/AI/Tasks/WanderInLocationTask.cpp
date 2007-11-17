/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: WanderInLocationTask.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "WanderInLocationTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

WanderInLocationTask::WanderInLocationTask() :
	_location(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY)
{}

WanderInLocationTask::WanderInLocationTask(const idVec3& location) :
	_location(location)
{}

// Get the name of this task
const idStr& WanderInLocationTask::GetName() const
{
	static idStr _name(TASK_WANDER_IN_LOCATION);
	return _name;
}

void WanderInLocationTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Init the base class
	Task::Init(owner, subsystem);

	owner->AI_RUN = false;

	// Turn toward the initial location
	owner->TurnToward(_location);
	
	// Move toward the initial loation
	owner->MoveToPosition(_location);
}

bool WanderInLocationTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("WanderInLocationTask performing.\r");

	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	Memory& memory = owner->GetMemory();

	// if we are approaching or past maximum distance, next wander is back toward the initial position
	if ((owner->GetPhysics()->GetOrigin() - _location).LengthFast() >= memory.alertRadius)
	{
		owner->MoveToPosition(_location);
	}
	else if (owner->AI_MOVE_DONE)
	{
		// Wander in a new direction if wander phase time expired or we got stopped by something
		owner->WanderAround();
	}

	return false; // not finished yet
}

void WanderInLocationTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	savefile->WriteVec3(_location);
}

void WanderInLocationTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	savefile->ReadVec3(_location);
}

WanderInLocationTaskPtr WanderInLocationTask::CreateInstance()
{
	return WanderInLocationTaskPtr(new WanderInLocationTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar wanderInLocationTaskRegistrar(
	TASK_WANDER_IN_LOCATION, // Task Name
	TaskLibrary::CreateInstanceFunc(&WanderInLocationTask::CreateInstance) // Instance creation callback
);

} // namespace ai
