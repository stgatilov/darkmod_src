/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod (http://www.thedarkmod.com/)
 
 $Revision$ (Revision of last commit) 
 $Date$ (Date of last commit)
 $Author$ (Author of last commit)
 
******************************************************************************/

#ifndef __AI_HANDLE_DOOR_TASK_H__
#define __AI_HANDLE_DOOR_TASK_H__

#include "Task.h"

#include "../../BinaryFrobMover.h"

namespace ai
{

// Define the name of this task
#define TASK_HANDLE_DOOR "HandleDoor"

#define HANDLE_DOOR_ACCURACY -1	// grayman #2345 - More accuracy when reaching position to work w/door.
								// '8' says to use a 16x16 bounding box to see if you've reached a goal
								// position instead of the default 32x32 ('-1').
								// grayman #2706 - returned to default value of -1. Tighter than this
								// might cause problems.
#define HANDLE_DOOR_ACCURACY_RUNNING 24 // grayman #3317 - less accuracy when moving faster

class HandleDoorTask;
typedef boost::shared_ptr<HandleDoorTask> HandleDoorTaskPtr;

class HandleDoorTask :
	public Task
{
private:
	idVec3 _frontPos;
	idVec3 _backPos;
	idVec3 _midPos;		// grayman #2345
	idVec3 _safePos;	// grayman #2345

	idEntityPtr<idEntity> _frontPosEnt;
	idEntityPtr<idEntity> _backPosEnt;

	enum EDoorHandlingState {
		EStateNone,
		EStateApproachingDoor,
		EStateMovingToSafePos, // grayman #2345
		EStateMovingToFrontPos,
		EStateWaitBeforeOpen,
		EStateStartOpen,
		EStateOpeningDoor,
		EStateMovingToMidPos, // grayman #2345
		EStateMovingToBackPos,
		EStateWaitBeforeClose,
		EStateStartClose,
		EStateClosingDoor
	} _doorHandlingState;

	int _waitEndTime;
	bool _wasLocked;
	bool _doorInTheWay;
	int _retryCount;
	int _leaveQueue;		// grayman #2345
	int _leaveDoor;			// grayman #2700
	bool _triedFitting;		// grayman #2345
	bool _canHandleDoor;	// grayman #2712
	bool _doorShouldBeClosed; // grayman #2866
public:
	// Get the name of this task
	virtual const idStr& GetName() const;

	// Override the base Init method
	virtual void Init(idAI* owner, Subsystem& subsystem);

	virtual bool Perform(Subsystem& subsystem);

	virtual void OnFinish(idAI* owner);

	virtual bool CanAbort(); // grayman #2706

	idVec3 GetAwayPos(idAI* owner, CFrobDoor* frobDoor);
	idVec3 GetTowardPos(idAI* owner, CFrobDoor* frobDoor);
	idVec3 GetMidPos(idAI* owner, CFrobDoor* frobDoor, bool away); // grayman #2345/#2712

	void MoveToSafePosition(CFrobDoor* door); // grayman #3390

	void PickWhere2Go(CFrobDoor* door); // grayman #2345

	void GetDoorHandlingPositions(idAI* owner, CFrobDoor* frobDoor);

	void DoorInTheWay(idAI* owner, CFrobDoor* frobDoor);

	// this checks if the gap is large enough to fit through partially openend doors (blocked, interrupted)
	bool FitsThrough();

	// these checks whether the AI is allowed to open/close/unlock/lock the door from this side
	bool AllowedToOpen(idAI* owner);
	bool AllowedToClose(idAI* owner);
	bool AllowedToUnlock(idAI* owner);
	bool AllowedToLock(idAI* owner);

	// adds the door area to forbidden areas (will be re-evaluated after some time)
	void AddToForbiddenAreas(idAI* owner, CFrobDoor* frobDoor);

	// open door routine (checks if the door is locked and starts to open it when possible)
	bool OpenDoor();

	void ResetDoor(idAI* owner, CFrobDoor* newDoor);

	void DrawDebugOutput(idAI* owner);

	void Save(idSaveGame* savefile) const;
	void Restore(idRestoreGame* savefile);

	// Creates a new Instance of this task
	static HandleDoorTaskPtr CreateInstance();

private:
	// Finds an entity which can operate the door in question (a lever, forex)
	// If multiple controllers are available, the nearest one is chosen
	idEntity* GetRemoteControlEntityForDoor();

	void AddUser(idAI* owner, CFrobDoor* frobDoor); // grayman #2345

};

} // namespace ai

#endif /* __AI_HANDLE_DOOR_TASK_H__ */
