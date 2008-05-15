/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef __AI_HANDLE_ELEVATOR_TASK_H__
#define __AI_HANDLE_ELEVATOR_TASK_H__

#include "Task.h"
#include "../EAS/RouteInfo.h"
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
	enum State
	{
		EMovingTowardsStation,
		EStateMovingToFetchButton,
		EStatePressFetchButton,
		EStateWaitForElevator,
		EStateMoveOntoElevator,
		EStateMovingToRideButton,
		EStatePressRideButton,
		EStateWaitOnElevator,
		ENumStates,
	} _state;

	int _waitEndTime;

	// The actual route info structure
	eas::RouteInfo _routeInfo;
	
	// Private constructor
	HandleElevatorTask();
public:

	HandleElevatorTask(const eas::RouteInfoPtr& routeInfo);
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

	// Lets the AI move towards the position entity (is slightly more complicated than just idAI::MoveToPos)
	bool MoveToPositionEntity(idAI* owner, CMultiStateMoverPosition* pos);
};

} // namespace ai

#endif /* __AI_HANDLE_ELEVATOR_TASK_H__ */
