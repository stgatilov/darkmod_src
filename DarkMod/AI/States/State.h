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

#include "../../AIComm_Message.h"
#include "../Subsystem.h"
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

	// greebo: Sets the owner of this State, this is usually called by the mind
	void SetOwner(idAI* owner);

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

	/**
	 * greebo: This handles an incoming tactile alert.
	 *
	 * @tactEnt: is the entity in question (actor, projectile, moveable,...)
	 */
	virtual void OnTactileAlert(idEntity* tactEnt);

	/**
	 * greebo: Gets called by idAI::PerformVisualScan when a visual alert is 
	 *         justified under the current circumstances.
	 *
	 * @enemy: The detected entity (usually the player). Must not be NULL.
	 */
	virtual void OnVisualAlert(idActor* enemy);

	/**
	 * greebo: Gets called when the AI is alerted by a suspicious sound.
	 */
	virtual void OnAudioAlert();

	// Handles incoming messages from other AI
	virtual void OnAICommMessage(CAIComm_Message* message);

	virtual void OnBlindStim(idEntity* stimSource, bool skipVisibilityCheck);

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
	 * greebo: Gets called by OnTactileAlert when the offending entity is 
	 *         an idProjectile. It does not alert the owning AI, as this is
	 *         handled in the calling OnTactileAlert method.
	 */
	virtual void OnProjectileHit(idProjectile* projectile);

	/**
	 * greebo: Method implemented by the States to check
	 *         for the correct alert index before continuing.
	 *
	 * returns TRUE by default. Backbone States override this method.
	 */
	virtual bool CheckAlertLevel(idAI* owner);

	virtual void UpdateAlertLevel();

private:
	void OnMessageDetectedSomethingSuspicious(CAIComm_Message* message);
};
typedef boost::shared_ptr<State> StatePtr;

} // namespace ai

#endif /* __AI_STATE_H__ */
