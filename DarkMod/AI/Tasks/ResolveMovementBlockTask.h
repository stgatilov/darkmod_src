/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef __AI_RESOLVE_MOVEMENT_BLOCK_TASK_H__
#define __AI_RESOLVE_MOVEMENT_BLOCK_TASK_H__

#include "Task.h"

namespace ai
{

// Define the name of this task
#define TASK_RESOLVE_MOVEMENT_BLOCK "ResolveMovementBlock"

class ResolveMovementBlockTask;
typedef boost::shared_ptr<ResolveMovementBlockTask> ResolveMovementBlockTaskPtr;

class ResolveMovementBlockTask :
	public Task
{
private:

	// Default constructor
	ResolveMovementBlockTask();

public:
	// Get the name of this task
	virtual const idStr& GetName() const;

	// Override the base Init method
	virtual void Init(idAI* owner, Subsystem& subsystem);

	virtual bool Perform(Subsystem& subsystem);

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);

	// Creates a new Instance of this task
	static ResolveMovementBlockTaskPtr CreateInstance();
};

} // namespace ai

#endif /* __AI_RESOLVE_MOVEMENT_BLOCK_TASK_H__ */
