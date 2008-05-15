/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef __AI_ALERT_IDLE_STATE_H__
#define __AI_ALERT_IDLE_STATE_H__

#include "State.h"

namespace ai
{

#define STATE_ALERT_IDLE "AlertIdle"

/**
 * angua: This is a specialisation of the IdleState. If the AI
 * has been highly alerted during its lifetime, it doesn't return
 * into the regular IdleState, but this one.
 */
class AlertIdleState :
	public IdleState
{

public:
	// Get the name of this state
	virtual const idStr& GetName() const;

	// This is called when the state is first attached to the AI's Mind.
	virtual void Init(idAI* owner);

	static StatePtr CreateInstance();

protected:
	// Returns the initial idle bark sound, depending on the alert level 
	// and the current state of mind
	virtual idStr GetInitialIdleBark(idAI* owner);
};

} // namespace ai

#endif /* __AI_ALERT_IDLE_STATE_H__ */
