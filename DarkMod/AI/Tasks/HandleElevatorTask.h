/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2008-05-13 18:53:28 +0200 (Di, 13 May 2008) $
 * $Author: greebo $
 *
 ***************************************************************************/

#ifndef __AI_HANDLE_ELEVATOR_TASK_H__
#define __AI_HANDLE_ELEVATOR_TASK_H__

#include "Task.h"

#include "../../BinaryFrobMover.h"

namespace ai
{

// Define the name of this task
#define TASK_HANDLE_ELEVATOR "HandleElevator"

class HandleElevatorTask;
typedef boost::shared_ptr<HandleElevatorTask> HandleElevatorTaskPtr;

class HandleElevatorTask :
	public Task
{
private:
	// The elevator position we're asked to use
	idEntityPtr<CMultiStateMoverPosition> _pos;

	enum State
	{
		EMovingTowardsStation,
		EStateMovingToButton,
		EStatePressButton,
		EStateWaitForElevator,
		EStateMoveOntoElevator,
		ENumStates,
	} _state;

	int _waitEndTime;
	
	// Private constructor
	HandleElevatorTask();
public:

	HandleElevatorTask(CMultiStateMoverPosition* pos);
	// Get the name of this task
	virtual const idStr& GetName() const;

	// Override the base Init method
	virtual void Init(idAI* owner, Subsystem& subsystem);

	virtual bool Perform(Subsystem& subsystem);

	virtual void OnFinish(idAI* owner);

	void Save(idSaveGame* savefile) const;
	void Restore(idRestoreGame* savefile);

	// Creates a new Instance of this task
	static HandleElevatorTaskPtr CreateInstance();

private:
	// Checks if the elevator station is reachable, returns TRUE if this is the case
	bool IsElevatorStationReachable(CMultiStateMoverPosition* pos);

	void DebugDraw(idAI* owner);
};

} // namespace ai

#endif /* __AI_HANDLE_ELEVATOR_TASK_H__ */
