/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef __AI_FAILED_KNOCK_OUT_STATE_H__
#define __AI_FAILED_KNOCK_OUT_STATE_H__

#include "State.h"

namespace ai
{

#define STATE_FAILED_KNOCKED_OUT "FailedKnockout"

class FailedKnockoutState :
	public State
{
	int _stateEndTime;
	int _allowEndTime;
	idEntity* _attacker;
	idVec3 _attackDirection;
	bool _hitHead;

	FailedKnockoutState();

public:
	FailedKnockoutState(idEntity* attacker, const idVec3& attackDirection, bool hitHead);

	// Get the name of this state
	virtual const idStr& GetName() const;

	// This is called when the state is first attached to the AI's Mind.
	virtual void Init(idAI* owner);

	// Gets called each time the mind is thinking
	virtual void Think(idAI* owner);

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);

	static StatePtr CreateInstance();
};

} // namespace ai

#endif /* __AI_FAILED_KNOCK_OUT_STATE_H__ */
