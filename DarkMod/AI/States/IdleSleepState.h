/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 3221 $
 * $Date: 2009-03-04 05:53:35 +0100 (Mi, 04 Mär 2009) $
 * $Author: angua $
 *
 ***************************************************************************/

#ifndef __AI_IDLE_SLEEP_STATE_H__
#define __AI_IDLE_SLEEP_STATE_H__

#include "State.h"
#include "IdleState.h"

namespace ai
{

#define STATE_IDLE_SLEEP "IdleSleep"

class IdleSleepState :
	public IdleState
{
public:
	// Get the name of this state
	virtual const idStr& GetName() const;

	// This is called when the state is first attached to the AI's Mind.
	virtual void Init(idAI* owner);

	// Gets called each time the mind is thinking
	virtual void Think(idAI* owner);

	static StatePtr CreateInstance();

	/**
	* ishtvan: Called when targets are changed
	* Re-initializes to catch new path corners
	**/
	virtual void OnChangeTarget(idAI *owner);

protected:

	// Override base class method
	virtual bool CheckAlertLevel(idAI* owner);

};

} // namespace ai

#endif /* __AI_IDLE_SLEEP_STATE_H__ */
