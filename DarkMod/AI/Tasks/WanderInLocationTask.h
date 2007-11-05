/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/

#ifndef __AI_WANDER_IN_LOCATION_TASK_H__
#define __AI_WANDER_IN_LOCATION_TASK_H__

#include "Task.h"

namespace ai
{

// Define the name of this task
#define TASK_WANDER_IN_LOCATION "WanderInLocation"

class WanderInLocationTask;
typedef boost::shared_ptr<WanderInLocationTask> WanderInLocationTaskPtr;

class WanderInLocationTask :
	public Task
{
	idVec3 _location;

	// Private default constructor
	WanderInLocationTask();
public:
	// Constructor taking the target location as argument
	WanderInLocationTask(const idVec3& location);

	// Get the name of this task
	virtual const idStr& GetName() const;

	// Override the base Init method
	virtual void Init(idAI* owner, Subsystem& subsystem);

	virtual bool Perform(Subsystem& subsystem);

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);

	// Creates a new Instance of this task
	static WanderInLocationTaskPtr CreateInstance();
};

} // namespace ai

#endif /* __AI_WANDER_IN_LOCATION_TASK_H__ */
