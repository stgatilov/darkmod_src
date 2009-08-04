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
	// The "masterAI" is the one who wants to pass
	idAI* _masterAI;

	// The angles we had when starting this task
	idAngles _initialAngles;

	int _preTaskContents;

	int _endTime;

	// Default constructor
	ResolveMovementBlockTask();

public:
	// The "masterAI" is the one who wants to pass
	ResolveMovementBlockTask(idAI* masterAI);

	// Get the name of this task
	virtual const idStr& GetName() const;

	// Override the base Init method
	virtual void Init(idAI* owner, Subsystem& subsystem);

	virtual bool Perform(Subsystem& subsystem);

	virtual void OnFinish(idAI* owner);

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);

	// Creates a new Instance of this task
	static ResolveMovementBlockTaskPtr CreateInstance();
};

} // namespace ai

#endif /* __AI_RESOLVE_MOVEMENT_BLOCK_TASK_H__ */
