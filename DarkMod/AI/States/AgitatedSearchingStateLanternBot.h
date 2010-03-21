/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef _AGITATED_SEARCHING_STATE_LANTERN_BOT_H_
#define _AGITATED_SEARCHING_STATE_LANTERN_BOT_H_

#include "../../../game/ai/ai.h"
#include "AgitatedSearchingState.h"

namespace ai
{

#define STATE_AGITATED_SEARCHING_LANTERN_BOT "AgitatedSearchingLanternBot"

class AgitatedSearchingStateLanternBot :
	public AgitatedSearchingState
{
public:
	// Get the name of this state
	virtual const idStr& GetName() const;

	// This is called when the state is first attached to the AI's Mind.
	virtual void Init(idAI* owner);

	// Gets called each time the mind is thinking
	virtual void Think(idAI* owner);

	static StatePtr CreateInstance();
};

} // namespace

#endif
