/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/

#ifndef __AI_TASK_H__
#define __AI_TASK_H__

#include <boost/shared_ptr.hpp>

namespace ai
{

/**
 * greebo: This is the abstract declaration of a Task.
 * 
 * Tasks are attached to a subsystem, which act as "slots". 
 * Only one (arbitrary) task can be attached to each subsystem at once.
 *
 * A task needs to have a unique name and a Perform() method.
 */
class Task
{
public:
	// Get the name of this task
	virtual const idStr& GetName() const = 0;

	// Performs the task, whatever this may be
	virtual void Perform() = 0;

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const = 0;
	virtual void Restore(idRestoreGame* savefile) = 0;
};
typedef boost::shared_ptr<Task> TaskPtr;

} // namespace ai

#endif /* __AI_TASK_H__ */
