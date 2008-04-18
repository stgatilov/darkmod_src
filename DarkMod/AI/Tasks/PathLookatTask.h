/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: crispy $
 *
 ***************************************************************************/

#ifndef __AI_PATH_LOOKAT_TASK_H__
#define __AI_PATH_LOOKAT_TASK_H__

#include "Task.h"

namespace ai
{

// Define the name of this task
#define TASK_PATH_LOOKAT "PathLookat"

class PathLookatTask;
typedef boost::shared_ptr<PathLookatTask> PathLookatTaskPtr;

class PathLookatTask :
	public Task
{
	idEntityPtr<idPathCorner> _path;

	PathLookatTask();

public:
	PathLookatTask(idPathCorner* path);

	// Get the name of this task
	virtual const idStr& GetName() const;

	// Override the base Init method
	virtual void Init(idAI* owner, Subsystem& subsystem);

	virtual bool Perform(Subsystem& subsystem);

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);

	// Creates a new Instance of this task
	static PathLookatTaskPtr CreateInstance();

	// Class-specific methods
	virtual void SetTargetEntity(idPathCorner* path);
};

} // namespace ai

#endif /* __AI_PATH_LOOKAT_TASK_H__ */
