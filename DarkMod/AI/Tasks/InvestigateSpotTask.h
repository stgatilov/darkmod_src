/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
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
	// The search spot to investigate
	idVec3 _searchSpot;

	// The time this task may exit
	int _exitTime;

	// Set to TRUE, if the AI should investigate the spot very closely
	// usually by playing the kneel_down animation.
	bool _investigateClosely;

	// Private default constructor
	InvestigateSpotTask();
public:
	// @param: see member _investigateClosely
	InvestigateSpotTask(bool investigateClosely);

	// Get the name of this task
	virtual const idStr& GetName() const;

	// Override the base Init method
	virtual void Init(idAI* owner, Subsystem& subsystem);

	virtual bool Perform(Subsystem& subsystem);

	/** 
	 * greebo: Sets a new goal position for this task.
	 *
	 * @newPos: The new position
	 */
	virtual void SetNewGoal(const idVec3& newPos);

	/** 
	 * greebo: Sets the "should investigate closely" flag.
	 */
	virtual void SetInvestigateClosely(bool closely);

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);

	// Creates a new Instance of this task
	static InvestigateSpotTaskPtr CreateInstance();
};

} // namespace ai

#endif /* __AI_INVESTIGATE_SPOT_TASK_H__ */
