/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef _COMBAT_STATE_LANTERN_BOT_H_
#define _COMBAT_STATE_LANTERN_BOT_H_

#include "../../../game/ai/ai.h"
#include "CombatState.h"

namespace ai
{

#define STATE_COMBAT_LANTERN_BOT "CombatLanternBot"

class CombatStateLanternBot :
	public CombatState
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
