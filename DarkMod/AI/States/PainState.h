/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef __AI_PAIN_STATE_H__
#define __AI_PAIN_STATE_H__

#include "State.h"

namespace ai
{

#define STATE_PAIN "Pain"

// An intermediate state, playing the pain anim and alerting the AI afterwards
class PainState :
	public State
{
	int _stateEndTime;
	
public:

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

#endif /* __AI_PAIN_STATE_H__ */
