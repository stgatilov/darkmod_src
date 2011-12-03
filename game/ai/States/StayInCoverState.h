/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef __AI_STAY_IN_COVER_STATE_H__
#define __AI_STAY_IN_COVER_STATE_H__

#include "State.h"

namespace ai
{

#define STATE_STAY_IN_COVER "StayInCover"

class StayInCoverState :
	public State
{
private:
	int _emergeDelay;
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
	// Override the base class method to catch projectile hit events
	virtual void OnProjectileHit(idProjectile* projectile);
};

} // namespace ai

#endif /* __AI_STAY_IN_COVER_STATE_H__ */
