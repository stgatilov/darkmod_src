/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/

#ifndef __AI_MELEE_COMBAT_TASK_H__
#define __AI_MELEE_COMBAT_TASK_H__

#include <boost/shared_ptr.hpp>

namespace ai
{

// Define the name of this task
#define TASK_MELEE_COMBAT "Melee_Combat"

class MeleeCombatTask;
typedef boost::shared_ptr<MeleeCombatTask> MeleeCombatTaskPtr;

class MeleeCombatTask :
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
	static MeleeCombatTaskPtr CreateInstance();
};

} // namespace ai

#endif /* __AI_MELEE_COMBAT_TASK_H__ */
