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

#include <boost/shared_ptr.hpp>

namespace ai
{

class EmptyTask :
	public Task
{
	idStr _name;

public:
	// Get the name of this task
	virtual const idStr& GetName() const
	{
		return _name;
	}

	// Performs the task, whatever this may be
	virtual void Perform()
	{
		// Do nothing
	}

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const
	{
		
	}

	virtual void Restore(idRestoreGame* savefile)
	{

	}

private:
	// Creates a new Instance of this task
	static TaskPtr CreateInstance();
};
typedef boost::shared_ptr<Task> TaskPtr;

} // namespace ai

#endif /* __AI_EMPTY_TASK_H__ */
