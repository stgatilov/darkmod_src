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
protected:
	// The owning entity
	idEntityPtr<idAI> _owner;
	float _alertLevelDecreaseRate;

public:
	// Get the name of this state
	virtual const idStr& GetName() const = 0;

	// This is called when the state is about to be invoked the first time by Mind.
	virtual void Init(idAI* owner);

	/**
	 * greebo: This is called each time the Mind is thinking and gives 
	 *         the State an opportunity to monitor the Subsystems.
	 *         
	 * Note: This is basically called each frame, so don't do expensive things in here.
	 */
	virtual void Think(idAI* owner) = 0;

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);

	// Incoming events issued by the Subsystems
	virtual void OnSubsystemTaskFinished(idAI* owner, SubsystemId subSystem)
	{} // empty default implementation

	// Handles incoming messages from other AI
	virtual void OnAICommMessage(CAIComm_Message* message);

	// Handles incoming visual stims coming from the given entity
	virtual void OnVisualStim(idEntity* stimSource);

	// greebo: These get called by the above OnVisualStim() method. 
	// The passed <stimSource> pointer is guaranteed to be non-NULL.
	virtual void OnVisualStimWeapon(idEntity* stimSource, idAI* owner);
	virtual void OnVisualStimPerson(idEntity* stimSource, idAI* owner);
	virtual void OnVisualStimBlood(idEntity* stimSource, idAI* owner);
	virtual void OnVisualStimLightSource(idEntity* stimSource, idAI* owner);
	virtual void OnVisualStimMissingItem(idEntity* stimSource, idAI* owner);
	virtual void OnVisualStimOpenDoor(idEntity* stimSource, idAI* owner);

	// greebo: Gets called by OnVisualStimPerson on finding a dead body
	// returns TRUE when the stim should be ignored from now on, FALSE otherwise
	virtual bool OnVisualStimDeadPerson(idActor* person, idAI* owner);
	virtual bool OnVisualStimUnconsciousPerson(idActor* person, idAI* owner);

protected:
	/**
	 * greebo: Method implemented by the States to check
	 *         for the correct alert index before continuing.
	 *
	 * returns TRUE by default. Backbone States override this method.
	 */
	virtual bool CheckAlertLevel(idAI* owner);

	/**
	 * Convenience method used by all the higher-level Backbone States.
	 * Basically checks whether the AI is in the given alert index.
	 *
	 * If the AlertIndex is smaller, EndState() is invoked,
	 * when it is higher, the Mind is switched to the State 
	 * with the given name <higherStateName>.
	 *
	 * @returns: TRUE if the alert level is ok, FALSE otherwise (State is about to End/Switch).
	 *           When FALSE is returned, the calling State be returned.
	 */
	bool SwitchOnMismatchingAlertIndex(int reqAlertIndex, const idStr& higherStateName);

	virtual void UpdateAlertLevel();

private:
	void OnMessageDetectedSomethingSuspicious(CAIComm_Message* message);
};
typedef boost::shared_ptr<State> StatePtr;

} // namespace ai

#endif /* __AI_STATE_H__ */
