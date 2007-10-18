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

static bool init_version = FileVersionList("$Id: PathCornerTask.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "PatrolTask.h"
#include "PathCornerTask.h"
#include "../Library.h"

namespace ai
{

// Get the name of this task
const idStr& PathCornerTask::GetName() const
{
	static idStr _name(TASK_PATH_CORNER);
	return _name;
}

void PathCornerTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);
}

// Called each frame
void PathCornerTask::Perform()
{
	DM_LOG(LC_AI, LT_INFO).LogString("Path Corner Task performing.\r");

	idPathCorner* path = _path.GetEntity();

	// This task may not be performed with an empty entity
	assert(path);

	// TODO
}

void PathCornerTask::SetTargetEntity(idPathCorner* path) 
{
	assert(path);
	_path = path;
}

// Save/Restore methods
void PathCornerTask::Save(idSaveGame* savefile) const
{}

void PathCornerTask::Restore(idRestoreGame* savefile)
{}

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
