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

#include "Memory.h"
#include "States/State.h"

namespace ai
{

class BasicMind :
	public Mind
{
private:
	// The reference to the owning entity
	idEntityPtr<idAI> _owner;

	// The state we're in
	StatePtr _state;

	// The structure holding all the variables
	Memory _memory;

public:
	BasicMind(idAI* owner);
	virtual ~BasicMind() {}

	virtual void Think();

	// Changes the state
	virtual void ChangeState(const idStr& stateName);

	// Returns the reference to the current state
	ID_INLINE StatePtr& GetState() {
		return _state;
	}

	// Returns the Memory structure, which holds the various mind variables
	ID_INLINE Memory& GetMemory() {
		return _memory;
	}

	virtual void PerformSensoryScan(bool processNewStimuli);

	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);

private:
	// Test if the alert state can be decreased
	virtual void TestAlertStateTimer();
};

} // namespace ai

#endif /* __AI_BASICMIND_H__ */
