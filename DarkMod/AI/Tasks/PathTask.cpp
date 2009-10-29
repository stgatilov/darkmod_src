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

	idPathCorner* path = _path.GetEntity();

	_accuracy = path->spawnArgs.GetFloat("move_to_position_tolerance", "-1");
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

	if (owner == NULL)
	{
		return;
	}

	// The current path gets stored in lastPath
	owner->GetMemory().lastPath = path;

	// The pre-selected "next path" is now our current one
	owner->GetMemory().currentPath = owner->GetMemory().nextPath.GetEntity();

	// Now pre-select a new (random) path entity for the next round
	// this information is important for the PathCornerTask to decide which action to take on exit
	idPathCorner* next = idPathCorner::RandomPath(path, NULL, owner);
	owner->GetMemory().nextPath = next;
}

// Save/Restore methods
void PathTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	_path.Save(savefile);
	savefile->WriteFloat(_accuracy);
}

void PathTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	_path.Restore(savefile);
	savefile->ReadFloat(_accuracy);
}


} // namespace ai
