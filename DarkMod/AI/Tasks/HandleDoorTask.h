/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: angua $
 *
 ***************************************************************************/

#ifndef __AI_HANDLE_DOOR_TASK_H__
#define __AI_HANDLE_DOOR_TASK_H__

#include "Task.h"

#include "../../BinaryFrobMover.h"

namespace ai
{

// Define the name of this task
#define TASK_HANDLE_DOOR "HandleDoor"

class HandleDoorTask;
typedef boost::shared_ptr<HandleDoorTask> HandleDoorTaskPtr;

class HandleDoorTask :
	public Task
{
private:
	idVec3 _frontPos;
	idVec3 _backPos;

	enum EDoorHandlingState {
		EStateNone,
		EStateMovingToFrontPos,
		EStateWaitBeforeOpen,
		EStateOpeningDoor,
		EStateMovingToBackPos,
		EStateWaitBeforeClose,
		EStateClosingDoor
	} _doorHandlingState;

	int _waitEndTime;
	bool _wasLocked;
public:
	// Get the name of this task
	virtual const idStr& GetName() const;

	// Override the base Init method
	virtual void Init(idAI* owner, Subsystem& subsystem);

	virtual bool Perform(Subsystem& subsystem);

	virtual void OnFinish(idAI* owner);

	void Save(idSaveGame* savefile) const;
	void Restore(idRestoreGame* savefile);


	// Creates a new Instance of this task
	static HandleDoorTaskPtr CreateInstance();
};

} // namespace ai

#endif /* __AI_HANDLE_DOOR_TASK_H__ */
