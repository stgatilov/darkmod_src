/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 3184 $
 * $Date: 2009-01-19 13:49:11 +0100 (Mo, 19 Jän 2009) $
 * $Author: angua $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: PathTask.cpp 3184 2009-01-19 12:49:11Z angua $", init_version);

#include "../Memory.h"
#include "PatrolTask.h"
#include "PathTask.h"
#include "../Library.h"

namespace ai
{

PathTask::PathTask()
{}

PathTask::PathTask(idPathCorner* path)
{
	_path = path;
}

void PathTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);

	if (_path.GetEntity() == NULL)
	{
		gameLocal.Error("Path Entity not set before Init()");
	}
}


void PathTask::SetTargetEntity(idPathCorner* path) 
{
	assert(path);
	_path = path;
}

void PathTask::NextPath()
{
	idPathCorner* path = _path.GetEntity();
	idAI* owner = _owner.GetEntity();

	// Store the new path entity into the AI's mind
	idPathCorner* next = idPathCorner::RandomPath(path, NULL, owner);
	owner->GetMind()->GetMemory().lastPath = path;
	owner->GetMind()->GetMemory().currentPath = next;
}	


// Save/Restore methods
void PathTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	_path.Save(savefile);
}

void PathTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	_path.Restore(savefile);
}


} // namespace ai
