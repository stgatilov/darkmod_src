/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/

#ifndef __AI_RANDOM_HEADTURN_TASK_H__
#define __AI_RANDOM_HEADTURN_TASK_H__

#include "Task.h"

namespace ai
{

// Define the name of this task
#define TASK_RANDOM_HEADTURN "RandomHeadturn"

class RandomHeadturnTask;
typedef boost::shared_ptr<RandomHeadturnTask> RandomHeadturnTaskPtr;

class RandomHeadturnTask :
	public Task
{
public:
	// Get the name of this task
	virtual const idStr& GetName() const;

	// Override the base Init method
	virtual void Init(idAI* owner, Subsystem& subsystem);

	virtual bool Perform(Subsystem& subsystem);

	// Creates a new Instance of this task
	static RandomHeadturnTaskPtr CreateInstance();

private:
	/**
	* SophisticatedZombie: This method handles a random chance of turning the AIs head
	* (visual gaze movement)
	*/
	void PerformRandomHeadTurnCheck();
};

} // namespace ai

#endif /* __AI_RANDOM_HEADTURN_TASK_H__ */
