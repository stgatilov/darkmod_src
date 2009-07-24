/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef __AI_GREETING_BARK_TASK_H__
#define __AI_GREETING_BARK_TASK_H__

#include "SingleBarkTask.h"
#include "../../AIComm_Message.h"

namespace ai
{

// Define the name of this task
#define TASK_GREETING_BARK "GreetingBark"

class GreetingBarkTask;
typedef boost::shared_ptr<GreetingBarkTask> GreetingBarkTaskPtr;

class GreetingBarkTask :
	public SingleBarkTask
{
protected:

	idActor* _greetingTarget;

	// Default constructor
	GreetingBarkTask();

public:
	// Constructor taking a sound name and the target actor as argument
	GreetingBarkTask(const idStr& soundName, idActor* greetingTarget);

	// Get the name of this task
	virtual const idStr& GetName() const;

	virtual void Init(idAI* owner, Subsystem& subsystem);
	virtual bool Perform(Subsystem& subsystem);

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);

	// Creates a new Instance of this task
	static GreetingBarkTaskPtr CreateInstance();
};

} // namespace ai

#endif /* __AI_GREETING_BARK_TASK_H__ */
