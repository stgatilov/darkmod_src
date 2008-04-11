/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2008-04-11 18:53:28 +0200 (Fr, 04 Apr 2008) $
 * $Author: greebo $
 *
 ***************************************************************************/

#ifndef __AI_ANIMAL_PATROL_TASK_H__
#define __AI_ANIMAL_PATROL_TASK_H__

#include "Task.h"

namespace ai
{

// Define the name of this task
#define TASK_ANIMAL_PATROL "AnimalPatrol"

class AnimalPatrolTask;
typedef boost::shared_ptr<AnimalPatrolTask> AnimalPatrolTaskPtr;

class AnimalPatrolTask :
	public Task
{
public:
	// Get the name of this task
	virtual const idStr& GetName() const;

	// Override the base Init method
	virtual void Init(idAI* owner, Subsystem& subsystem);

	virtual bool Perform(Subsystem& subsystem);

	// Creates a new Instance of this task
	static AnimalPatrolTaskPtr CreateInstance();
};

} // namespace ai

#endif /* __AI_ANIMAL_PATROL_TASK_H__ */
