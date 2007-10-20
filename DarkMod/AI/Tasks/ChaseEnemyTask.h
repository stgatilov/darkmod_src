/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/

#ifndef __AI_CHASE_ENEMY_TASK_H__
#define __AI_CHASE_ENEMY_TASK_H__

#include <boost/shared_ptr.hpp>

namespace ai
{

// Define the name of this task
#define TASK_CHASE_ENEMY "Chase_Enemy"

class ChaseEnemyTask;
typedef boost::shared_ptr<ChaseEnemyTask> ChaseEnemyTaskPtr;

class ChaseEnemyTask :
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
	static ChaseEnemyTaskPtr CreateInstance();

	// Class-specific methods
	virtual void SetEnemy(idActor* enemy);

private:
	// Chases the enemy, sets memory.chaseFinished to true when done.
	void Chase();
};

} // namespace ai

#endif /* __AI_CHASE_ENEMY_TASK_H__ */
