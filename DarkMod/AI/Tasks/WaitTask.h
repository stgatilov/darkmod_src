/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: angua $
 *
 ***************************************************************************/

#ifndef __AI_WAIT_TASK_H__
#define __AI_WAIT_TASK_H__

#include "Task.h"

namespace ai
{

// Define the name of this task
#define TASK_WAIT "Wait"

class WaitTask;
typedef boost::shared_ptr<WaitTask> WaitTaskPtr;

class WaitTask :
	public Task
{
private:

	int _waitTime;
	int _waitEndTime;

	// Default constructor
	WaitTask();

public:

	// Constructor with waittime (in ms) as input argument
	WaitTask(const int waitTime);

	// Get the name of this task
	virtual const idStr& GetName() const;

	// Override the base Init method
	virtual void Init(idAI* owner, Subsystem& subsystem);

	virtual bool Perform(Subsystem& subsystem);

	void SetTime(int waitTime);

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);

	// Creates a new Instance of this task
	static WaitTaskPtr CreateInstance();
};

} // namespace ai

#endif /* __AI_WAIT_TASK_H__ */
