/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef __AI_PATROL_TASK_H__
#define __AI_PATROL_TASK_H__

#include "Task.h"

namespace ai
{

// Define the name of this task
#define TASK_PATROL "Patrol"

class PatrolTask;
typedef boost::shared_ptr<PatrolTask> PatrolTaskPtr;

class PatrolTask :
	public Task
{
public:
	// Get the name of this task
	virtual const idStr& GetName() const;

	// Override the base Init method
	virtual void Init(idAI* owner, Subsystem& subsystem);

	virtual bool Perform(Subsystem& subsystem);

	// Creates a new Instance of this task
	static PatrolTaskPtr CreateInstance();
};

} // namespace ai

#endif /* __AI_PATROL_TASK_H__ */
