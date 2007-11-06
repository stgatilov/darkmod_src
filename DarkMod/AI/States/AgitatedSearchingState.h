/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: angua $
 *
 ***************************************************************************/

#ifndef __AI_AGITATED_SEARCHING_STATE_H__
#define __AI_AGITATED_SEARCHING_STATE_H__

#include "State.h"
#include "SearchingState.h"

/**
* greebo: AgitatedSearchingState is one alert index above SearchingState.
*
* Apart from a few minor things this is similar to the base class SearchingState.
* 
* See the base class for documentation.
*/

namespace ai
{

#define STATE_AGITATED_SEARCHING "AgitatedSearching"
#define PRIORITY_AGITATED_SEARCHING 25000

class AgitatedSearchingState :
	protected SearchingState
{
public:
	// Get the name of this state
	virtual const idStr& GetName() const;

	// Get/set the priority of this state
	virtual int GetPriority() const {
		return PRIORITY_AGITATED_SEARCHING;
	}

	// This is called when the state is first attached to the AI's Mind.
	virtual void Init(idAI* owner);

	// Gets called each time the mind is thinking
	virtual void Think(idAI* owner);

	static StatePtr CreateInstance();
};

} // namespace ai

#endif /* __AI_AGITATED_SEARCHING_STATE_H__ */
