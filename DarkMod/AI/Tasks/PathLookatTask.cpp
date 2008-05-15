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
#include "PathLookatTask.h"
#include "../Library.h"

namespace ai
{

PathLookatTask::PathLookatTask()
{}

PathLookatTask::PathLookatTask(idPathCorner* path)
{
	_path = path;
}

// Get the name of this task
const idStr& PathLookatTask::GetName() const
{
	static idStr _name(TASK_PATH_LOOKAT);
	return _name;
}

void PathLookatTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);

	idPathCorner* path = _path.GetEntity();

	if (path == NULL) {
		gameLocal.Error("PathLookatTask: Path Entity not set before Init()");
	}
}

bool PathLookatTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("Path Lookat Task performing.\r");

	idPathCorner* path = _path.GetEntity();
	idAI* owner = _owner.GetEntity();

	// This task may not be performed with empty entity pointers
	assert(path != NULL && owner != NULL);

	// The entity to look at is named by the "focus" spawnarg, if that spawnarg is set.
	idStr focusEntName(path->spawnArgs.GetString("focus"));
	idEntity* focusEnt = NULL;
	if (focusEntName.Length()) {
		focusEnt = gameLocal.FindEntity(focusEntName);
		if (!focusEnt) {
			// If it's specified, it should really exist
			gameLocal.Warning("Entity '%s' names a non-existent focus entity '%s'", path->name.c_str(), focusEntName.c_str());
		}
	}
	// If spawnarg not set or no such entity, fall back to using the path entity itself
	if (!focusEnt) focusEnt = path;
	
	// Duration is indicated by "wait" key
	float duration = path->spawnArgs.GetFloat("wait", "1");
	
	// Look
	owner->Event_LookAtEntity(focusEnt, duration);
	
	// Debug
	gameRenderWorld->DebugArrow(colorGreen, owner->GetEyePosition(), focusEnt->GetPhysics()->GetOrigin(), 10, 10000);
	
	// Trigger next path target(s)
	owner->ActivateTargets(owner);
	
	return true; // finish this task
}

void PathLookatTask::SetTargetEntity(idPathCorner* path) 
{
	assert(path);
	_path = path;
}

// Save/Restore methods
void PathLookatTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	_path.Save(savefile);
}

void PathLookatTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	_path.Restore(savefile);
}

PathLookatTaskPtr PathLookatTask::CreateInstance()
{
	return PathLookatTaskPtr(new PathLookatTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar pathLookatTaskRegistrar(
	TASK_PATH_LOOKAT, // Task Name
	TaskLibrary::CreateInstanceFunc(&PathLookatTask::CreateInstance) // Instance creation callback
);

} // namespace ai
