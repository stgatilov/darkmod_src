/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: angua $
 *
 ***************************************************************************/

#ifndef __AI_MOVE_TO_POSITION_H__
#define __AI_MOVE_TO_POSITION_H__

#include "Task.h"

namespace ai
{

// Define the name of this task
#define TASK_MOVE_TO_POSITION "Move_To_Position"

class MoveToPositionTask;
typedef boost::shared_ptr<MoveToPositionTask> MoveToPositionTaskPtr;

class MoveToPositionTask :
	public Task
{
private:

	// The target position
	idVec3 _targetPosition;

	// Default constructor
	MoveToPositionTask();

public:
	// Constructor taking the target position as input argument
	MoveToPositionTask(const idVec3 targetPosition);

	// Get the name of this task
	virtual const idStr& GetName() const;

	// Override the base Init method
	virtual void Init(idAI* owner, Subsystem& subsystem);

	virtual bool Perform(Subsystem& subsystem);

	void SetPosition(idVec3 targetPosition);

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);

	// Creates a new Instance of this task
	static MoveToPositionTaskPtr CreateInstance();
};

} // namespace ai

#endif /* __AI_MOVE_TO_POSITION_H__ */
