/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef __AI_PATH_WAIT_FOR_TRIGGER_TASK_H__
#define __AI_PATH_WAIT_FOR_TRIGGER_TASK_H__

#include "PathTask.h"

namespace ai
{

// Define the name of this task
#define TASK_PATH_WAIT_FOR_TRIGGER "PathWaitForTrigger"

class PathWaitForTriggerTask;
typedef boost::shared_ptr<PathWaitForTriggerTask> PathWaitForTriggerTaskPtr;

class PathWaitForTriggerTask :
	public PathTask
{
private:
	PathWaitForTriggerTask();
public:
	PathWaitForTriggerTask(idPathCorner* path);

	// Get the name of this task
	virtual const idStr& GetName() const;

	// Override the base Init method
	virtual void Init(idAI* owner, Subsystem& subsystem);

	virtual bool Perform(Subsystem& subsystem);

	// Creates a new Instance of this task
	static PathWaitForTriggerTaskPtr CreateInstance();
};

} // namespace ai

#endif /* __AI_PATH_WAIT_FOR_TRIGGER_TASK_H__ */
