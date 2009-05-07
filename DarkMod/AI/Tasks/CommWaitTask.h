/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef __AI_COMM_WAIT_TASK_H__
#define __AI_COMM_WAIT_TASK_H__

#include "CommunicationTask.h"

namespace ai
{

// Define the name of this task
#define TASK_COMM_WAIT "CommWaitTask"

class CommWaitTask;
typedef boost::shared_ptr<CommWaitTask> CommWaitTaskPtr;

// A simple silent task, causes the AI to shut up for a while
class CommWaitTask :
	public CommunicationTask
{
	int _endTime;

	// Default constructor
	CommWaitTask();

public:
	// Constructor: pass the duration of the silence
	CommWaitTask(int duration);

	// Get the name of this task
	virtual const idStr& GetName() const;

	virtual bool Perform(Subsystem& subsystem);

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);

	// Creates a new Instance of this task
	static CommWaitTaskPtr CreateInstance();
};

} // namespace ai

#endif /* __AI_COMM_WAIT_TASK_H__ */
