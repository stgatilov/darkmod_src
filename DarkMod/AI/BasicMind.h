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

	// The next state, if this is non-NULL, the current one will be terminated.
	StatePtr _nextState;

	// The structure holding all the variables
	Memory _memory;

	// This holds the id of the subsystem whose turn it is next frame
	SubsystemId _subsystemIterator;

public:
	BasicMind(idAI* owner);
	virtual ~BasicMind() {}

	virtual void Think();

	// Switches the state
	virtual void SwitchState(const idStr& stateName);

	// Switches the state if the given priority is higher than the current one
	virtual bool SwitchStateIfHigherPriority(const idStr& stateName, int statePriority);

	// Returns the reference to the current state
	ID_INLINE StatePtr& GetState() {
		return _state;
	}

	// Returns the Memory structure, which holds the various mind variables
	ID_INLINE Memory& GetMemory() {
		return _memory;
	}

	virtual void SetAlertPos();
	virtual void PerformSensoryScan(bool processNewStimuli);
	virtual bool PerformCombatCheck();
	virtual bool SetTarget();

	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);

private:
	// Test if the alert state can be decreased
	void TestAlertStateTimer();

	// Returns TRUE if the <entity> is on an opposite team or owned by it
	bool IsEnemy(idEntity* entity, idAI* self);

	void Bark(const idStr& soundname);

	// This is called each frame to perform a multiframe hiding spot search
	void PerformHidingSpotSearch(idAI* owner);

	/**
	* This method looks at the alert level and determines
	* the duration to set for the currentHidingSpotListSearchMaxDuration
	* variable that controls the length of an ensuing search.
	*
	* @return Returns the duration that was also set in the
	*	currentHidingSpotListSearchMaxDuration in the Memory.
	*/
	int DetermineSearchDuration(idAI* owner);
};

} // namespace ai

#endif /* __AI_BASICMIND_H__ */
