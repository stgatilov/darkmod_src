/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/

#ifndef __AI_INVESTIGATE_SPOT_TASK_H__
#define __AI_INVESTIGATE_SPOT_TASK_H__

#include "Task.h"

namespace ai
{

/**
* greebo: This task requires memory.currentSearchSpot to be valid.
* 
* This task is intended to be pushed into the action Subsystem and
* performs single-handedly how the given hiding spot should be handled.
*
* Note: This Task employs the Movement Subsystem when the algorithm
* judges to walk/run over to the given search spot.
**/

// Define the name of this task
#define TASK_INVESTIGATE_SPOT "InvestigateSpot"

class InvestigateSpotTask;
typedef boost::shared_ptr<InvestigateSpotTask> InvestigateSpotTaskPtr;

class InvestigateSpotTask :
	public Task
{
	// The time this task may exit
	int _exitTime;

public:
	// Get the name of this task
	virtual const idStr& GetName() const;

	// Override the base Init method
	virtual void Init(idAI* owner, Subsystem& subsystem);

	virtual bool Perform(Subsystem& subsystem);

	// Creates a new Instance of this task
	static InvestigateSpotTaskPtr CreateInstance();
};

} // namespace ai

#endif /* __AI_INVESTIGATE_SPOT_TASK_H__ */
