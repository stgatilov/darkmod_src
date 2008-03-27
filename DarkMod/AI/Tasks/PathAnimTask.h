/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2008-03-27 18:53:28 +0200 (Do, 27 Mar 2008) $
 * $Author: greebo $
 *
 ***************************************************************************/

#ifndef __AI_PATH_ANIM_TASK_H__
#define __AI_PATH_ANIM_TASK_H__

#include "Task.h"

namespace ai
{

// Define the name of this task
#define TASK_PATH_ANIM "PathAnim"

class PathAnimTask;
typedef boost::shared_ptr<PathAnimTask> PathAnimTaskPtr;

class PathAnimTask :
	public Task
{
	idEntityPtr<idPathCorner> _path;

	// Private constructor
	PathAnimTask();

public:
	PathAnimTask(idPathCorner* path);

	// Get the name of this task
	virtual const idStr& GetName() const;

	// Override the base Init method
	virtual void Init(idAI* owner, Subsystem& subsystem);

	virtual bool Perform(Subsystem& subsystem);

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);

	// Creates a new Instance of this task
	static PathAnimTaskPtr CreateInstance();

	// Class-specific methods
	virtual void SetTargetEntity(idPathCorner* path);
};

} // namespace ai

#endif /* __AI_PATH_ANIM_TASK_H__ */
