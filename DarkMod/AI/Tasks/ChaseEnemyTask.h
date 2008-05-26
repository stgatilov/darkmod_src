/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef __AI_CHASE_ENEMY_TASK_H__
#define __AI_CHASE_ENEMY_TASK_H__

#include "Task.h"
#include "../../MultiStateMover.h"

namespace ai
{

// Define the name of this task
#define TASK_CHASE_ENEMY "ChaseEnemy"

class ChaseEnemyTask;
typedef boost::shared_ptr<ChaseEnemyTask> ChaseEnemyTaskPtr;

class ChaseEnemyTask :
	public Task
{
	idEntityPtr<idActor> _enemy;
	int _reachEnemyCheck;

	// Private default constructor
	ChaseEnemyTask();
public:
	ChaseEnemyTask(idActor* enemy);

	// Get the name of this task
	virtual const idStr& GetName() const;

	// Override the base Init method
	virtual void Init(idAI* owner, Subsystem& subsystem);

	virtual bool Perform(Subsystem& subsystem);

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);

	// Creates a new Instance of this task
	static ChaseEnemyTaskPtr CreateInstance();

	// Class-specific methods
	virtual void SetEnemy(idActor* enemy);

private:
	CMultiStateMoverPosition* CanFetchElevator(CMultiStateMover* mover, idAI* owner);
};

} // namespace ai

#endif /* __AI_CHASE_ENEMY_TASK_H__ */
