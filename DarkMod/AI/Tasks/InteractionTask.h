/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef __AI_INTERACTION_TASK_H__
#define __AI_INTERACTION_TASK_H__

#include "Task.h"

namespace ai
{

// Define the name of this task
#define TASK_INTERACTION "Interaction"

class InteractionTask;
typedef boost::shared_ptr<InteractionTask> InteractionTaskPtr;

class InteractionTask :
	public Task
{
	idEntity* _interactEnt;

	int _waitEndTime;

	InteractionTask();

public:
	InteractionTask(idEntity* interactEnt);

	// Get the name of this task
	virtual const idStr& GetName() const;

	// Override the base Init method
	virtual void Init(idAI* owner, Subsystem& subsystem);

	virtual bool Perform(Subsystem& subsystem);

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);

	// Creates a new Instance of this task
	static InteractionTaskPtr CreateInstance();
};

} // namespace ai

#endif /* __AI_INTERACTION_TASK_H__ */
