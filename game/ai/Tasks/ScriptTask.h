/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef __AI_SCRIPT_TASK_H__
#define __AI_SCRIPT_TASK_H__

#include "Task.h"

namespace ai
{

// Define the name of this task
#define TASK_SCRIPT "ScriptTask"

class ScriptTask;
typedef boost::shared_ptr<ScriptTask> ScriptTaskPtr;

/** 
 * greebo: A ScriptTask can be plugged into any AI subsystem and
 * has the only purpose to execute a script thread. The actual
 * script function must be specified in the constructor and
 * can bei either global or local (=defined on the AI's scriptobject).
 *
 * For local scripts, the function does not to take any arguments.
 * For global scripts, the function needs to take an entity argument (=this owner).
 *
 * The Task is finishing itself when the script thread is done executing.
 *
 * Terminating this task will of course kill the thread.
 */
class ScriptTask :
	public Task
{
	// The name of the script function to execute
	idStr _functionName;	

	// The script thread
	idThread* _thread;

	// Private constructor
	ScriptTask();

public:
	// Optional constructor taking the function name
	ScriptTask(const idStr& functionName);

	// Get the name of this task
	virtual const idStr& GetName() const;

	// Override the base Init method
	virtual void Init(idAI* owner, Subsystem& subsystem);

	virtual bool Perform(Subsystem& subsystem);

	// Gets called when this task is finished (or gets terminated)
	virtual void OnFinish(idAI* owner);

	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);

	// Creates a new Instance of this task
	static ScriptTaskPtr CreateInstance();
};

} // namespace ai

#endif /* __AI_SCRIPT_TASK_H__ */
