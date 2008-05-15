/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef __AI_PATH_SHOW_TASK_H__
#define __AI_PATH_SHOW_TASK_H__

#include "Task.h"

namespace ai
{

// Define the name of this task
#define TASK_PATH_SHOW "PathShow"

class PathShowTask;
typedef boost::shared_ptr<PathShowTask> PathShowTaskPtr;

class PathShowTask :
	public Task
{
	idEntityPtr<idPathCorner> _path;

	PathShowTask();
public:
	PathShowTask(idPathCorner* path);

	// Get the name of this task
	virtual const idStr& GetName() const;

	// Override the base Init method
	virtual void Init(idAI* owner, Subsystem& subsystem);

	virtual bool Perform(Subsystem& subsystem);

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);

	// Creates a new Instance of this task
	static PathShowTaskPtr CreateInstance();

	// Class-specific methods
	virtual void SetTargetEntity(idPathCorner* path);
};

} // namespace ai

#endif /* __AI_PATH_SHOW_TASK_H__ */
