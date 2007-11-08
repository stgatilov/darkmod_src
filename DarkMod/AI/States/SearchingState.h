/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/

#ifndef __AI_SEARCHING_STATE_H__
#define __AI_SEARCHING_STATE_H__

#include "State.h"

/**
* greebo: A SearchingState is handling the AI's search routines.
* 
* The routine needs "memory.alertPos" to be set as prerequisite.
*
* The boolean variable "memory.stimulusLocationItselfShouldBeSearched" can be used
* to let the AI the "memory.alertPos" position as first hiding spot.
* If the boolean is not set to TRUE, the hiding spot search is based around memory.alertPos.
*
* The actual hiding spot search algorithm is called over multiple
* frames. Once finished, the AI can use its results (unless the 
* stimulusLocationItselfShouldBeSearched bool is set to TRUE, then alertPos is used as
* first hiding spot right away.
*
* For each hiding spot, an InvestigateSpotTask is invoked which takes care of the details.
*/

namespace ai
{

#define STATE_SEARCHING "Searching"

class SearchingState :
	public State
{
public:
	// Get the name of this state
	virtual const idStr& GetName() const;

	// This is called when the state is first attached to the AI's Mind.
	virtual void Init(idAI* owner);

	// Gets called each time the mind is thinking
	virtual void Think(idAI* owner);

	// Incoming events issued by the Subsystems
	virtual void OnSubsystemTaskFinished(idAI* owner, SubsystemId subSystem);

	static StatePtr CreateInstance();

protected:
	// Override base class method
	virtual bool CheckAlertLevel(idAI* owner);

	// This is a service method for this class and the subclasses.
	// It basically starts a new hidingspot search and initialises the subsystems
	void SetupSearch(idAI* owner);

	/*!
	* This method is used to start a new hiding spot search. Any existing search in progress is replaced.
	*/
	virtual void StartNewHidingSpotSearch(idAI* owner);

	// This is called each frame to complete a multiframe hiding spot search
	virtual void PerformHidingSpotSearch(idAI* owner);

	// Gets called when a new hiding spot should be acquired for searching.
	// Stores the result in the AI's Memory (hiding spot indices)
	// return TRUE when a hiding spot is available, FALSE if not.
	virtual bool ChooseNextHidingSpotToSearch(idAI* owner);
};

} // namespace ai

#endif /* __AI_SEARCHING_STATE_H__ */
