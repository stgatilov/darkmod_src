/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/

#ifndef __AI_IDLE_STATE_H__
#define __AI_IDLE_STATE_H__

#include "State.h"

namespace ai
{

#define STATE_IDLE "Idle"

class IdleState :
	public State
{
	// These are finite if the guard has no patrol route
	idVec3 _idlePosition;
	float _idleYaw;

	// Private constructor
	IdleState();

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

protected:
	// Override base class method
	virtual bool CheckAlertLevel(idAI* owner);

private:
	// Returns the initial idle bark sound, depending on the alert level 
	// and the current state of mind
	idStr GetInitialIdleBark(idAI* owner);
};

} // namespace ai

#endif /* __AI_IDLE_STATE_H__ */
