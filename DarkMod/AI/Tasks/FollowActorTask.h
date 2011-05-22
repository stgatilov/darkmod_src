/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef __AI_FOLLOW_ACTOR_TASK_H__
#define __AI_FOLLOW_ACTOR_TASK_H__

#include "Task.h"

namespace ai
{

// Define the name of this task
#define TASK_FOLLOW_ACTOR "FollowActor"

class FollowActorTask;
typedef boost::shared_ptr<FollowActorTask> FollowActorTaskPtr;

class FollowActorTask :
	public Task
{
private:
	idEntityPtr<idActor> _actor;

	FollowActorTask();

public:
	FollowActorTask(idActor* actorToFollow);

	// Get the name of this task
	virtual const idStr& GetName() const;

	// Override the base Init method
	virtual void Init(idAI* owner, Subsystem& subsystem);

	virtual bool Perform(Subsystem& subsystem);

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);

	// Creates a new Instance of this task
	static FollowActorTaskPtr CreateInstance();
};

} // namespace ai

#endif /* __AI_FOLLOW_ACTOR_TASK_H__ */
