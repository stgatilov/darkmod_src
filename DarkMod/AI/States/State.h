/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/

#ifndef __AI_STATE_H__
#define __AI_STATE_H__

#include <boost/shared_ptr.hpp>

namespace ai
{

class State
{
public:
	// Get the name of this state
	virtual const idStr& GetName() const = 0;

	// Returns the priority of this state
	virtual int GetPriority() const = 0;

	// This is called when the state is about to be invoked the first time by Mind.
	virtual void Init(idAI* owner) = 0;

	/**
	 * greebo: This is called each time the Mind is thinking and gives 
	 *         the State an opportunity to monitor the Subsystems.
	 *         
	 * Note: This is basically called each frame, so don't do expensive things in here.
	 */
	virtual void Think(idAI* owner) = 0;

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const = 0;
	virtual void Restore(idRestoreGame* savefile) = 0;
};
typedef boost::shared_ptr<State> StatePtr;

} // namespace ai

#endif /* __AI_STATE_H__ */
