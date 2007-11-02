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

static bool init_version = FileVersionList("$Id: PatrolTask.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "../Memory.h"
#include "PatrolTask.h"
#include "EmptyTask.h"
#include "PathCornerTask.h"
#include "PathTurnTask.h"
#include "PathWaitTask.h"
#include "PathWaitForTriggerTask.h"
#include "PathHideTask.h"
#include "PathShowTask.h"
#include "../Library.h"

namespace ai
{

// Get the name of this task
const idStr& PatrolTask::GetName() const
{
	static idStr _name(TASK_PATROL);
	return _name;
}

void PatrolTask::Init(idAI* owner, Subsystem& subsystem)
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
}

bool PatrolTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("Patrol Task performing.\r");

	idPathCorner* path = _owner.GetEntity()->GetMind()->GetMemory().currentPath.GetEntity();

	// This task may not be performed with an empty path corner entity,
	// that case should have been caught by the Init() routine
	assert(path);

	// Get the classname, this determines the child routine we're spawning.
	idStr classname = path->spawnArgs.GetString("classname");

	if (classname == "path_corner")
	{
		// Allocate a new PathCornerTask
		PathCornerTaskPtr pathTask = PathCornerTask::CreateInstance();
		assert(pathTask != NULL); // task must be found

		// Set the target entity and push the task
		pathTask->SetTargetEntity(path);
		subsystem.PushTask(pathTask);
	}	
	else if (classname == "path_turn")
	{
		// Allocate a new PathCornerTask
		PathTurnTaskPtr pathTask = PathTurnTask::CreateInstance();
		assert(pathTask != NULL); // task must be found

		// Set the target entity and push the task
		pathTask->SetTargetEntity(path);
		subsystem.PushTask(pathTask);
	}
	else if (classname == "path_wait")
	{
		// Allocate a new PathCornerTask
		PathWaitTaskPtr pathTask = PathWaitTask::CreateInstance();
		assert(pathTask != NULL); // task must be found

		// Set the target entity and push the task
		pathTask->SetTargetEntity(path);
		subsystem.PushTask(pathTask);
	}
	else if (classname == "path_waitfortrigger")
	{
		// Allocate a new PathCornerTask
		PathWaitForTriggerTaskPtr pathTask = PathWaitForTriggerTask::CreateInstance();
		assert(pathTask != NULL); // task must be found

		// Set the target entity and push the task
		pathTask->SetTargetEntity(path);
		subsystem.PushTask(pathTask);
	}
	else if (classname == "path_hide")
	{
		// Allocate a new PathCornerTask
		PathHideTaskPtr pathTask = PathHideTask::CreateInstance();
		assert(pathTask != NULL); // task must be found

		// Set the target entity and push the task
		pathTask->SetTargetEntity(path);
		subsystem.PushTask(pathTask);
	}
	else if (classname == "path_show")
	{
		// Allocate a new PathCornerTask
		PathShowTaskPtr pathTask = PathShowTask::CreateInstance();
		assert(pathTask != NULL); // task must be found

		// Set the target entity and push the task
		pathTask->SetTargetEntity(path);
		subsystem.PushTask(pathTask);
	}
	else
	{
		// Finish this task
		return true;
	}

	return false; // not finished yet
}

PatrolTaskPtr PatrolTask::CreateInstance()
{
	return PatrolTaskPtr(new PatrolTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar patrolTaskRegistrar(
	TASK_PATROL, // Task Name
	TaskLibrary::CreateInstanceFunc(&PatrolTask::CreateInstance) // Instance creation callback
);

} // namespace ai
