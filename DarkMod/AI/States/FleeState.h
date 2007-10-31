/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: angua $
 *
 ***************************************************************************/

#ifndef __AI_FLEE_H__
#define __AI_FLEE_H__

#include "State.h"

namespace ai
{

#define STATE_FLEE "Flee"
#define PRIORITY_FLEE 60000

class FleeState :
	public State
{
public:
	// Get the name of this state
	virtual const idStr& GetName() const;

	// Get/set the priority of this state
	virtual int GetPriority() const {
		return PRIORITY_FLEE;
	}

	// This is called when the state is first attached to the AI's Mind.
	virtual void Init(idAI* owner);

	// Gets called each time the mind is thinking
	virtual void Think(idAI* owner);

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const
	{} // nothing yet

	virtual void Restore(idRestoreGame* savefile)
	{} // nothing yet

	static StatePtr CreateInstance();
};

} // namespace ai

#endif /* __AI_FLEE_H__ */
