/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 2338 $
 * $Date: 2008-05-15 18:23:41 +0200 (Do, 15 Mai 2008) $
 * $Author: greebo $
 *
 ***************************************************************************/

#ifndef __AI_PATH_CYCLE_ANIM_TASK_H__
#define __AI_PATH_CYCLE_ANIM_TASK_H__

#include "Task.h"

namespace ai
{

// Define the name of this task
#define TASK_PATH_CYCLE_ANIM "PathCycleAnim"

class PathCycleAnimTask;
typedef boost::shared_ptr<PathCycleAnimTask> PathCycleAnimTaskPtr;

class PathCycleAnimTask :
	public Task
{
	idEntityPtr<idPathCorner> _path;

	int _waitEndTime;

	// Private constructor
	PathCycleAnimTask();

public:
	PathCycleAnimTask(idPathCorner* path);

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
	static PathCycleAnimTaskPtr CreateInstance();

	// Class-specific methods
	virtual void SetTargetEntity(idPathCorner* path);
};

} // namespace ai

#endif /* __AI_PATH_CYCLE_ANIM_TASK_H__ */
