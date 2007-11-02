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

// Forward declaration
class Subsystem;

/**
 * greebo: This is the basic declaration of a Task.
 * 
 * Tasks are attached to a subsystem, which act as "slots". 
 * Only one (arbitrary) task can be attached to each subsystem at once.
 *
 * A task needs to have a unique name and a Perform() method.
 */
class Task
{
protected:
	// Each task has an owning entity
	idEntityPtr<idAI> _owner;

public:
	// Get the name of this task
	virtual const idStr& GetName() const = 0;

	// Performs the task, whatever this may be
	// Returns TRUE if the task is finished, FALSE if it should continue.
	virtual bool Perform(Subsystem& subsystem) = 0;

	// Let the task perform some initialisation. This is called
	// right after the task is installed into the subsystem.
	virtual void Init(idAI* owner, Subsystem& subsystem)
	{
		_owner = owner;
	}

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const
	{
		_owner.Save(savefile);
	}

	virtual void Restore(idRestoreGame* savefile)
	{
		_owner.Restore(savefile);
	}
};
typedef boost::shared_ptr<Task> TaskPtr;

} // namespace ai

#endif /* __AI_TASK_H__ */
