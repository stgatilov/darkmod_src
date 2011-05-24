/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef __AI_SWITCH_ON_LIGHT_H__
#define __AI_SWITCH_ON_LIGHT_H__

#include "State.h"

namespace ai
{

#define STATE_SWITCH_ON_LIGHT "SwitchOnLight"

class SwitchOnLightState :
	public State
{
private:
	// Default constructor
	SwitchOnLightState();

	idEntityPtr<idLight> _light;

	// time to wait after starting anim before the light is switched on
	int _waitEndTime;

	idEntity* _goalEnt; // grayman #2603 - entity to walk toward when relighting a light
	float _standOff;	// grayman #2603 - get this close to relight

	enum ERelightState	// grayman #2603
	{
		EStateStarting,
		EStateApproaching,
		EStateTurningToward,
		EStateRelight,
		EStatePause,
		EStateFinal
	} _relightState;

	bool CheckRelightPosition(idLight* light, idAI* owner, idVec3& pos); // grayman #2603

public:
	// Constructor using light source as input parameter
	SwitchOnLightState(idLight* light);

	// Get the name of this state
	virtual const idStr& GetName() const;

	virtual void Wrapup(idAI* owner, idLight* light, bool ignore); // grayman #2603

	virtual float GetMaxReach(idAI* owner, idEntity* torch, idStr lightType); // grayman #2603
	virtual bool GetSwitchGoal(idAI* owner, CBinaryFrobMover* mySwitch, idVec3 &target); // grayman #2603

	// This is called when the state is first attached to the AI's Mind.
	virtual void Init(idAI* owner);

	// Gets called each time the mind is thinking
	virtual void Think(idAI* owner);

	// Start switching on (stop move, start anim)
	void StartSwitchOn(idAI* owner, idLight* light);

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);

	static StatePtr CreateInstance();
};

} // namespace ai

#endif /* __AI_SWITCH_ON_LIGHT_H__ */
