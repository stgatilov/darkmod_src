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
#include "FollowActorTask.h"
#include "../Library.h"

namespace ai
{

FollowActorTask::FollowActorTask()
{}

FollowActorTask::FollowActorTask(idActor* actor)
{
	_actor = actor;
}

// Get the name of this task
const idStr& FollowActorTask::GetName() const
{
	static idStr _name(TASK_FOLLOW_ACTOR);
	return _name;
}

void FollowActorTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);

	if (_actor.GetEntity() == NULL)
	{
		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("FollowActorTask will terminate after Think() since a NULL actor was being passed in.\r");
	}
}

bool FollowActorTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("FollowActorTask performing.\r");

	idActor* actor = _actor.GetEntity();

	if (actor == NULL)
	{
		return true; // no actor (anymore, or maybe we never had one)
	}

	// TODO

	return false; // not finished yet
}


// Save/Restore methods
void FollowActorTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	_actor.Save(savefile);
}

void FollowActorTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	_actor.Restore(savefile);
}

FollowActorTaskPtr FollowActorTask::CreateInstance()
{
	return FollowActorTaskPtr(new FollowActorTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar followActorTaskRegistrar(
	TASK_FOLLOW_ACTOR, // Task Name
	TaskLibrary::CreateInstanceFunc(&FollowActorTask::CreateInstance) // Instance creation callback
);

} // namespace ai
