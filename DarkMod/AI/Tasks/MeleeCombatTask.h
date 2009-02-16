/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef __AI_MELEE_COMBAT_TASK_H__
#define __AI_MELEE_COMBAT_TASK_H__

#include "Task.h"

namespace ai
{

// Define the name of this task
#define TASK_MELEE_COMBAT "MeleeCombat"

class MeleeCombatTask;
typedef boost::shared_ptr<MeleeCombatTask> MeleeCombatTaskPtr;

class MeleeCombatTask :
	public Task
{
	idEntityPtr<idActor> _enemy;
	/** 
	* Set to true if we want to force an attack or parry at the next opportunity
	* I.e., we wait until we can do this action even if we could do the other first
	* These are cleared once the forced action is started
	**/
	bool				_bForceAttack;
	bool				_bForceParry;

public:
	// Get the name of this task
	virtual const idStr& GetName() const;

	// Override the base Init method
	virtual void Init(idAI* owner, Subsystem& subsystem);

	virtual bool Perform(Subsystem& subsystem);

	// Creates a new Instance of this task
	static MeleeCombatTaskPtr CreateInstance();

	virtual void OnFinish(idAI* owner);

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);

private:
	void PerformReady(idAI* owner);
	void PerformAttack(idAI* owner);
	void PerformParry(idAI* owner);

	/**
	* Start up an attack or parry animation
	**/
	void StartAttack(idAI* owner);
	void StartParry(idAI* owner);
};

} // namespace ai

#endif /* __AI_MELEE_COMBAT_TASK_H__ */
