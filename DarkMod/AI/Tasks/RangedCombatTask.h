/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef __AI_RANGED_COMBAT_TASK_H__
#define __AI_RANGED_COMBAT_TASK_H__

#include "Task.h"

namespace ai
{

// Define the name of this task
#define TASK_RANGED_COMBAT "RangedCombat"

class RangedCombatTask;
typedef boost::shared_ptr<RangedCombatTask> RangedCombatTaskPtr;

class RangedCombatTask :
	public Task
{
	idEntityPtr<idActor> _enemy;

public:
	// Get the name of this task
	virtual const idStr& GetName() const;

	// Override the base Init method
	virtual void Init(idAI* owner, Subsystem& subsystem);

	virtual bool Perform(Subsystem& subsystem);

	// Creates a new Instance of this task
	static RangedCombatTaskPtr CreateInstance();

	virtual void OnFinish(idAI* owner);

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);
};

} // namespace ai

#endif /* __AI_RANGED_COMBAT_TASK_H__ */
