/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef __AI_UNREACHABLE_TARGET_STATE_H__
#define __AI_UNREACHABLE_TARGET_STATE_H__

#include "State.h"

namespace ai
{

#define STATE_UNREACHABLE_TARGET "UnreachableTarget"

class UnreachableTargetState :
	public State
{
	// The AI's enemy
	idEntityPtr<idActor> _enemy;
	bool _takingCoverPossible;
	int _takeCoverTime;
	bool _moveRequired;
	int _reachEnemyCheck;

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

#endif /* __AI_UNREACHABLE_TARGET_STATE_H__ */
