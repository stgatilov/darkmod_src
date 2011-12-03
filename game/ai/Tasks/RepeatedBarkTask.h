/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef __AI_REPEATED_BARK_TASK_H__
#define __AI_REPEATED_BARK_TASK_H__

#include "CommunicationTask.h"
#include "../../AIComm_Message.h"

namespace ai
{

// Define the name of this task
#define TASK_REPEATED_BARK "RepeatedBark"

class RepeatedBarkTask;
typedef boost::shared_ptr<RepeatedBarkTask> RepeatedBarkTaskPtr;

class RepeatedBarkTask :
	public CommunicationTask
{
private:
	// times in milliseconds:
	int _barkRepeatIntervalMin;
	int _barkRepeatIntervalMax;
	int _nextBarkTime;

	// The message which should be delivered when barking
	CommMessagePtr _message;

	// Default Constructor
	RepeatedBarkTask();

public:
	/**
	 * greebo: Pass the sound shader name plus the interval range in milliseconds.
	 * The message argument is optional and can be used to let this Task emit messages
	 * when playing the sound.
	 */
	RepeatedBarkTask(const idStr& soundName, 
					 int barkRepeatIntervalMin, int barkRepeatIntervalMax, 
					 const CommMessagePtr& message = CommMessagePtr());

	// Get the name of this task
	virtual const idStr& GetName() const;

	// Override the base Init method
	virtual void Init(idAI* owner, Subsystem& subsystem);

	virtual bool Perform(Subsystem& subsystem);

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);

	// Creates a new Instance of this task
	static RepeatedBarkTaskPtr CreateInstance();
};

} // namespace ai

#endif /* __AI_REPEATED_BARK_TASK_H__ */
