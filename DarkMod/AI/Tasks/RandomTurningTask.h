/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: angua $
 *
 ***************************************************************************/

#ifndef __AI_RANDOM_TURNING_TASK_H__
#define __AI_RANDOM_TURNING_TASK_H__

#include "Task.h"

namespace ai
{

// Define the name of this task
#define TASK_RANDOM_TURNING "Random_Turning"

class RandomTurningTask;
typedef boost::shared_ptr<RandomTurningTask> RandomTurningTaskPtr;

class RandomTurningTask :
	public Task
{
private:

	float _nextYaw;
	bool _turning;
	int _nextTurningTime;

public:
	// Get the name of this task
	virtual const idStr& GetName() const;

	// Override the base Init method
	virtual void Init(idAI* owner, Subsystem& subsystem);

	virtual bool Perform(Subsystem& subsystem);

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);

	// Creates a new Instance of this task
	static RandomTurningTaskPtr CreateInstance();

	// Class-specific methods
};

} // namespace ai

#endif /* __AI_RANDOM_TURNING_TASK_H__ */
