/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef __AI_COMBAT_TASK_H__
#define __AI_COMBAT_TASK_H__

#include "Task.h"

namespace ai
{

class CombatTask :
	public Task
{
protected:
	idEntityPtr<idActor> _enemy;

	int _lastCombatBarkTime;

	CombatTask();

public:
	// Override the base Init method
	virtual void Init(idAI* owner, Subsystem& subsystem);

	//  This task lacks a Perform() method, this is to be implemented by subclasses

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);

protected:

	// Emits a combat bark plus an AI message to be delivered by soundprop
	// about the enemy's position
	void EmitCombatBark(idAI* owner, const idStr& sndName);
};
typedef boost::shared_ptr<CombatTask> CombatTaskPtr;

} // namespace ai

#endif /* __AI_RANGED_COMBAT_TASK_H__ */
