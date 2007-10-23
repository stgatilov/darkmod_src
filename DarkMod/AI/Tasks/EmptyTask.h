/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/

#ifndef __AI_EMPTY_TASK_H__
#define __AI_EMPTY_TASK_H__

#include "Task.h"

namespace ai
{

// Define the name of this task
#define TASK_EMPTY "Empty"

class EmptyTask;
typedef boost::shared_ptr<EmptyTask> EmptyTaskPtr;

class EmptyTask :
	public Task
{
public:
	// Get the name of this task
	virtual const idStr& GetName() const;

	// Empty implementation
	virtual bool Perform(Subsystem& subsystem);

	// Creates a new Instance of this task
	static EmptyTaskPtr CreateInstance();
};

} // namespace ai

#endif /* __AI_EMPTY_TASK_H__ */
