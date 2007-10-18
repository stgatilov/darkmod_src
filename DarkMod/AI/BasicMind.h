/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/

#ifndef __AI_BASICMIND_H__
#define __AI_BASICMIND_H__

#include "States/State.h"

namespace ai
{

class BasicMind :
	public Mind
{
private:
	// The reference to the owning entity
	idEntityPtr<idAI> _owner;

	// The current alert state
	EAlertState _alertState;

	StatePtr _state;

	idEntityPtr<idPathCorner> _currentPath;

public:
	BasicMind(idAI* owner);
	virtual ~BasicMind() {}

	virtual void Think();

	// Changes the state
	virtual void ChangeState(const idStr& stateName);

	// Returns the reference to the current state
	virtual StatePtr& GetState();

	// Get the current alert state 
	virtual EAlertState GetAlertState() const;

	// Set the current alert state
	virtual void SetAlertState(EAlertState newState);

	// Gets/Sets the current path entity of this AI
	virtual void SetCurrentPath(idPathCorner* path);
	virtual idPathCorner* GetCurrentPath();

	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);
};

} // namespace ai

#endif /* __AI_BASICMIND_H__ */
