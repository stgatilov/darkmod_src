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

#include "PatrolTask.h"
#include "EmptyTask.h"
#include "PathCornerTask.h"
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

	if (owner->spawnArgs.GetBool("patrol", "1")) 
	{
		// Find the next path associated with the owning AI
		idPathCorner* path = idPathCorner::RandomPath(owner, NULL);

		if (path == NULL)
		{
			// No path corner entities found!
			DM_LOG(LC_AI, LT_INFO).LogString("Warning: No Path corner entites found for %s\r", owner->name.c_str());
			// Replace this task with an empty one
			TaskPtr emptyTask(TaskLibrary::Instance().CreateInstance(TASK_EMPTY));
			subsystem.QueueTask(emptyTask);
			return;
		}

		_currentPath = path;
	}
}

// Called each frame
void PatrolTask::Perform()
{
	DM_LOG(LC_AI, LT_INFO).LogString("Patrol Task performing.\r");

	idPathCorner* path = _currentPath.GetEntity();

	// This task may not be performed with an empty path corner entity
	assert(path);

	// Move to entity path!
	if (path->spawnArgs.GetString("classname") == "path_corner")
	{
		// Allocate a new PathCornerTask
		PathCornerTaskPtr pathTask = PathCornerTask::CreateInstance();

		assert(pathTask != NULL);

		pathTask->SetTargetEntity(path);
	}
}

// Save/Restore methods
void PatrolTask::Save(idSaveGame* savefile) const
{}

void PatrolTask::Restore(idRestoreGame* savefile)
{}

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
