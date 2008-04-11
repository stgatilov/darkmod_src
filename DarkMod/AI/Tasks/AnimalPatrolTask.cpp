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

	TaskPtr task;

	return false;

	// Advance to the next path entity pointer
	idPathCorner* next = idPathCorner::RandomPath(path, NULL);

	if (next == NULL)
	{
		DM_LOG(LC_AI, LT_INFO).LogString("Cannot advance path pointer, no more targets.\r");
		subsystem.SwitchTask(task);
		// finish patrolling after this path
	}
	else
	{
		// Store the new path entity into the AI's mind
		_owner.GetEntity()->GetMind()->GetMemory().currentPath = next;
		subsystem.PushTask(task);
	}

	return false; // not finished yet
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
