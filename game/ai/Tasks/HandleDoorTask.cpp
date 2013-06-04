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

#include "precompiled_game.h"
#pragma hdrstop

static bool versioned = RegisterVersionedFile("$Id$");

#include "../Memory.h"
#include "HandleDoorTask.h"
#include "InteractionTask.h"
#include "../AreaManager.h"

namespace ai
{

#define QUEUE_TIMEOUT 10000		// milliseconds (grayman #2345 - max time to wait in a door queue)
#define DOOR_TIMEOUT 20000		// milliseconds (grayman #2700 - max time to execute a move to mid pos or back pos)
#define QUEUE_DISTANCE 150		// grayman #2345 - distance from door where incoming AI pause
#define NEAR_DOOR_DISTANCE 72	// grayman #2345 - less than this and you're close to the door

// Get the name of this task
const idStr& HandleDoorTask::GetName() const
{
	static idStr _name(TASK_HANDLE_DOOR);
	return _name;
}

void HandleDoorTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Init the base class
	Task::Init(owner, subsystem);

	Memory& memory = owner->GetMemory();

	_retryCount = 0;
	_triedFitting = false;	// grayman #2345 - useful if you're stuck behind a door
	_leaveQueue = -1;		// grayman #2345
	_leaveDoor = -1;		// grayman #2700
	_canHandleDoor = true;	// grayman #2712

	CFrobDoor* frobDoor = memory.doorRelated.currentDoor.GetEntity();
	if (frobDoor == NULL)
	{
		subsystem.FinishTask(); // grayman #2345 - can't perform the task if there's no door
		return;
	}

	if (!owner->m_bCanOperateDoors)
	{
		_canHandleDoor = false; // grayman #2712
		if (!frobDoor->IsOpen() || !FitsThrough())
		{
			owner->StopMove(MOVE_STATUS_DEST_UNREACHABLE);
			// add AAS area number of the door to forbidden areas
			AddToForbiddenAreas(owner, frobDoor);

			// grayman #2345 - to accomodate open doors and ~CanOperateDoors spawnflag

			subsystem.FinishTask(); 
			return;
		}
	}

	if (frobDoor->spawnArgs.GetBool("ai_should_not_handle"))
	{
		_canHandleDoor = false; // grayman #2712
		// AI will ignore this door (not try to handle it) 
		if (!frobDoor->IsOpen() || !FitsThrough())
		{
			// if it is closed, add to forbidden areas so AI will not try to path find through
			idAAS*	aas = owner->GetAAS();
			if (aas != NULL)
			{
				int areaNum = frobDoor->GetAASArea(aas);
				if (areaNum > 0) // grayman #2685 - footlocker lids are doors with no area number
				{
					gameLocal.m_AreaManager.AddForbiddenArea(areaNum, owner);
					owner->PostEventMS(&AI_ReEvaluateArea, owner->doorRetryTime, areaNum);
				}
			}
			subsystem.FinishTask();
			return;
		}
		// Door is open, and the AI can fit through, so continue with door-handling
	}

	// grayman #2866 - Is this a suspicious door, and is it time to close it?

	CFrobDoor* closeMe = memory.closeMe.GetEntity();

	_doorShouldBeClosed = false;
	if ( memory.closeSuspiciousDoor )
	{
		if ( closeMe == frobDoor )
		{
			// grayman #2866 - If this door is one we're closing because it's
			// supposed to be closed, there can't be anyone already in the door queue.
			// The assumption is that whoever's already in the queue will close the door.

			if ( frobDoor->GetUserManager().GetNumUsers() > 0 )
			{
				subsystem.FinishTask(); // quit the task
				return;
			}
			_doorShouldBeClosed = true;
		}
	}

	// Let the owner save its move

	owner->m_RestoreMove = false;	// grayman #2706 - whether we should restore a saved move when finished with the door
	if (!owner->GetEnemy())			// grayman #2690 - AI run toward where they saw you last. Don't save that location when handling doors.
	{
		owner->PushMove();
		owner->m_RestoreMove = true;
	}

	owner->m_HandlingDoor = true;

	_wasLocked = false;

	if (frobDoor->IsLocked())
	{
		// check if we have already tried the door
        idAAS*  aas = owner->GetAAS();
        if (aas != NULL)
        {
			int areaNum = frobDoor->GetAASArea(aas);
            if (gameLocal.m_AreaManager.AreaIsForbidden(areaNum, owner))
			{
				subsystem.FinishTask();
				return;
			}              
		}

		_wasLocked = true;
	}

	CFrobDoor* doubleDoor = frobDoor->GetDoubleDoor();

	AddUser(owner,frobDoor); // grayman #2345 - order the queue if needed
	if (doubleDoor != NULL)
	{
		AddUser(owner,doubleDoor); // grayman #2345 - order the queue if needed
	}

	_doorInTheWay = false;

	GetDoorHandlingPositions(owner, frobDoor);

	if ( _doorShouldBeClosed ) // grayman #2866
	{
		_doorHandlingState = EStateMovingToBackPos;
	}
	else
	{
		_doorHandlingState = EStateNone;
	}
}

// grayman #2345 - adjust goal positions based on what's happening around the door.

void HandleDoorTask::PickWhere2Go(CFrobDoor* door)
{
	idAI* owner = _owner.GetEntity();
	bool useMid = true;

	// If you're the only AI on the queue, close the door behind you if you're supposed to.
	// grayman #1327 - but only if no one (other than you) is searching around the door

	int numUsers = door->GetUserManager().GetNumUsers();

	if ( _doorShouldBeClosed ) // grayman #3104 - if in this state, continue to head for _backPos
	{
		useMid = false; // use _backPos
	}
	else if (owner->AI_RUN) // grayman #2670
	{
		// run for the mid position
	}
	else if (!_canHandleDoor) // grayman #2712
	{
		// walk to the mid position
	}
	else if ( owner->AI_AlertIndex >= ESearching ) // grayman #2866 - when approaching to investigate a door, walk to mid, not to back
	{
		// walk to the mid position
	}
	else if (numUsers < 2)
	{
		if (AllowedToClose(owner) && (_doorInTheWay || owner->ShouldCloseDoor(door) || _doorShouldBeClosed )) // grayman #2866
		{
			useMid = false; // use _backPos
		}
	}

	if (useMid)
	{
		if (_doorHandlingState != EStateMovingToMidPos)
		{
			owner->MoveToPosition(_midPos,HANDLE_DOOR_ACCURACY);
			_doorHandlingState = EStateMovingToMidPos;
		}
	}
	else
	{
		owner->MoveToPosition(_backPos,HANDLE_DOOR_ACCURACY);
		_doorHandlingState = EStateMovingToBackPos;
	}
}

void HandleDoorTask::MoveToSafePosition(CFrobDoor* door)
{
	idAI* owner = _owner.GetEntity();

	const idVec3& centerPos = door->GetClosedBox().GetCenter();
	idVec3 dir2AI = owner->GetPhysics()->GetOrigin() - centerPos;
	dir2AI.z = 0;
	dir2AI.NormalizeFast();
	_safePos = centerPos + 1.5*NEAR_DOOR_DISTANCE*dir2AI;

	if (!owner->MoveToPosition(_safePos,HANDLE_DOOR_ACCURACY))
	{
		// TODO: position not reachable, need a better one
	}
	_doorHandlingState = EStateMovingToSafePos;
}


bool HandleDoorTask::Perform(Subsystem& subsystem)
{
	idAI* owner = _owner.GetEntity();
	Memory& memory = owner->GetMemory();
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("HandleDoorTask performing by %s\r",owner->name.c_str());

	CFrobDoor* frobDoor = memory.doorRelated.currentDoor.GetEntity();
	if (frobDoor == NULL)
	{
		return true;
	}

	// grayman #2948 - leave door handling if KO'ed or dead

	if ( owner->AI_KNOCKEDOUT || owner->AI_DEAD )
	{
		return true;
	}

	// grayman #2816 - stop door handling for various reasons

	if ( memory.stopHandlingDoor )
	{
		return true;
	}
	
	if (frobDoor->IsOpen())
	{
		// The door is open. If it's not a "should be closed" door, and I can't
		// handle it, and I don't fit through the opening, add the door's area #
		// to my list of forbidden areas.

		if ( !_doorShouldBeClosed ) // grayman #2866
		{
			if (!_canHandleDoor && !FitsThrough()) // grayman #2712
			{
				owner->StopMove(MOVE_STATUS_DEST_UNREACHABLE);
				// add AAS area number of the door to forbidden areas
				AddToForbiddenAreas(owner, frobDoor);
				return true;
			}
		}
	}
	else 
	{
		// The door is closed. If I can't deal with it, add its area #
		// to my list of forbidden areas.
		if (!_canHandleDoor) // grayman #2712
		{
			owner->StopMove(MOVE_STATUS_DEST_UNREACHABLE);
			// add AAS area number of the door to forbidden areas
			AddToForbiddenAreas(owner, frobDoor);
			return true;
		}
	}

	// grayman #2700 - we get a certain amount of time to complete a move
	// to the mid position or back position before leaving door handling

	if ((_leaveDoor > 0) && (gameLocal.time >= _leaveDoor))
	{
		_leaveDoor = -1;
		return true;
	}

	int numUsers = frobDoor->GetUserManager().GetNumUsers();
	idActor* masterUser = frobDoor->GetUserManager().GetMasterUser();
	int queuePos = frobDoor->GetUserManager().GetIndex(owner); // grayman #2345

	const idVec3& frobDoorOrg = frobDoor->GetPhysics()->GetOrigin();
	const idVec3& openPos = frobDoorOrg + frobDoor->GetOpenPos();
	const idVec3& closedPos = frobDoorOrg + frobDoor->GetClosedPos();
	const idVec3& centerPos = frobDoor->GetClosedBox().GetCenter(); // grayman #1327

	// if our current door is part of a double door, this is the other part.
	CFrobDoor* doubleDoor = frobDoor->GetDoubleDoor();

	idBounds bounds = owner->GetPhysics()->GetBounds();

	// angua: move the bottom of the bounds up a bit, to avoid finding small objects on the ground that are "in the way"

	// grayman #2691 - except for AI whose bounding box height is less than maxStepHeight, otherwise applying the bump up
	// causes the clipmodel to be "upside-down", which isn't good. In that case, give the bottom a bump up equal to half
	// of the clipmodel's height so it at least gets a small bump.

	float ht = owner->GetAAS()->GetSettings()->maxStepHeight;
	if (bounds[0].z + ht < bounds[1].z)
	{
		bounds[0].z += ht;
	}
	else
	{
		bounds[0].z += (bounds[1].z - bounds[0].z)/2.0;
	}
	// bounds[0][2] += 16; // old way
	float size = bounds[1][0];

	if (cv_ai_door_show.GetBool()) 
	{
		DrawDebugOutput(owner);
	}

	idEntity *tactileEntity = owner->GetTactileEntity();	// grayman #2692
	idVec3 ownerOrigin = owner->GetPhysics()->GetOrigin();	// grayman #2692

	// grayman #3390 - refactor door handling code

	switch (_doorHandlingState)
	{
		case EStateNone:
			if (!frobDoor->IsOpen()) // closed
			{
				if ( ( doubleDoor != NULL ) && doubleDoor->IsOpen() )
				{
					// the other part of the double door is already open
					// no need to open this one
					ResetDoor(owner, doubleDoor);
				}
				else
				{
					if (!AllowedToOpen(owner))
					{
						AddToForbiddenAreas(owner, frobDoor);
						return true;
					}

					idEntity* controller = GetRemoteControlEntityForDoor();

					if ( ( controller != NULL ) && ( masterUser == owner ) && ( controller->GetUserManager().GetNumUsers() == 0 ) )
					{
						// We have an entity to control this door, interact with it
						owner->StopMove(MOVE_STATUS_DONE);
						subsystem.PushTask(TaskPtr(new InteractionTask(controller)));
						return false;
					}
				}

				// grayman #3317 - use less position accuracy when running
				if (!owner->MoveToPosition(_frontPos,owner->AI_RUN ? HANDLE_DOOR_ACCURACY_RUNNING : HANDLE_DOOR_ACCURACY))
				{
					// TODO: position not reachable, need a better one
				}

				_doorHandlingState = EStateApproachingDoor;
			}
			else // open
			{
				if (!_canHandleDoor) // grayman #2712
				{
					_doorHandlingState = EStateApproachingDoor;
					break;
				}

				// check if we are blocking the door
				if (frobDoor->IsBlocked() || 
					(frobDoor->WasInterrupted() || 
					frobDoor->WasStoppedDueToBlock()))
				{
					if (FitsThrough())
					{
						if (owner->AI_AlertIndex >= ESearching)
						{
							return true;
						}

						// grayman #3317 - use less position accuracy when running
						if (!owner->MoveToPosition(_frontPos,owner->AI_RUN ? HANDLE_DOOR_ACCURACY_RUNNING : HANDLE_DOOR_ACCURACY))
						{
							// TODO: position not reachable, need a better one
						}

						_doorHandlingState = EStateApproachingDoor;
						break;
					}
					else if (!AllowedToOpen(owner))
					{
						AddToForbiddenAreas(owner, frobDoor);
						return true;
					}
					else
					{
						idEntity* controller = GetRemoteControlEntityForDoor();

						if (controller != NULL)
						{	 
							if (masterUser == owner && controller->GetUserManager().GetNumUsers() == 0)
							{
								// We have an entity to control this door, interact with it
								subsystem.PushTask(TaskPtr(new InteractionTask(controller)));
								return false;
							}
						}

						// grayman #3317 - use less position accuracy when running
						if (!owner->MoveToPosition(_frontPos,owner->AI_RUN ? HANDLE_DOOR_ACCURACY_RUNNING : HANDLE_DOOR_ACCURACY))
						{
							// TODO: position not reachable, need a better one
						}

						_doorHandlingState = EStateApproachingDoor;
					}
				}
				else
				{
					// door is open and possibly in the way, may need to close it

					// test the angle between the view direction of the AI and the open door
					// door can only be in the way when the view direction 
					// is approximately perpendicular to the open door

					idVec3 ownerDir = owner->viewAxis.ToAngles().ToForward();

					idVec3 testVector = openPos - frobDoorOrg;
					testVector.z = 0;
					float length = testVector.LengthFast();
					float dist = size * SQUARE_ROOT_OF_2;
					length += dist;
					testVector.NormalizeFast();

					float product = idMath::Fabs(ownerDir * testVector);

					// grayman #2453 - the above test can give a false result depending
					// on the AI's orientation to the door, so make sure we're close enough
					// to the door to make the test valid.

					idVec3 dir = centerPos - owner->GetPhysics()->GetOrigin(); // grayman #3104 - use center of closed door
					dir.z = 0;
					float dist2Door = dir.LengthFast();

					if ((product > 0.3f) || (dist2Door > QUEUE_DISTANCE))
					{
						// door is open and not (yet) in the way

						_doorHandlingState = EStateApproachingDoor; // grayman #2345 - you should pause if necessary
						return false;
					}

					// check if there is a way around
					idTraceModel trm(bounds);
					idClipModel clip(trm);
	
					// check point next to the open door
					
					idVec3 testPoint = frobDoorOrg + testVector * length;

					int contents = gameLocal.clip.Contents(testPoint, &clip, mat3_identity, CONTENTS_SOLID, owner);

					if (contents)
					{
						// door is in the way, there is not enough space next to the door to fit through
						// find a suitable position and close the door
						DoorInTheWay(owner, frobDoor);
						_doorHandlingState = EStateApproachingDoor;
					}
					else
					{
						// check a little bit in front and behind the test point, 
						// might not be enough space there to squeeze through
						idVec3 normal = testVector.Cross(idVec3(0, 0, 1));
						normal.NormalizeFast();
						idVec3 testPoint2 = testPoint + dist * normal;

						contents = gameLocal.clip.Contents(testPoint2, &clip, mat3_identity, CONTENTS_SOLID, owner);
						if (contents)
						{
							// door is in the way, there is not enough space to fit through
							// find a suitable position and close the door
							DoorInTheWay(owner, frobDoor);
							_doorHandlingState = EStateApproachingDoor;
						}
						else
						{
							idVec3 testPoint3 = testPoint - dist * normal;

							contents = gameLocal.clip.Contents(testPoint3, &clip, mat3_identity, CONTENTS_SOLID, owner);
							if (contents)
							{
								// door is in the way, there is not enough space to fit through
								// find a suitable position and close the door
								DoorInTheWay(owner, frobDoor);
								_doorHandlingState = EStateApproachingDoor;
							}
							else
							{
								// door is not in the way and open

								_doorHandlingState = EStateApproachingDoor; // grayman #2345 - you should pause if necessary
								return false;
							}
						}
					}
				}
			}
			break;

		case EStateApproachingDoor:
			if (!frobDoor->IsOpen()) // closed
			{
				idVec3 dir = centerPos - owner->GetPhysics()->GetOrigin(); // grayman #3104 - distance from center of door
				dir.z = 0;
				float dist = dir.LengthFast();
				if (masterUser == owner)
				{
					if (dist <= QUEUE_DISTANCE) // grayman #2345 - this was the next layer up
					{
						GetDoorHandlingPositions(owner, frobDoor);
						if (_doorInTheWay)
						{	
							DoorInTheWay(owner, frobDoor);
							_doorHandlingState = EStateMovingToBackPos;
						}
						else
						{
							// grayman #3317 - use less position accuracy when running
							if (!owner->MoveToPosition(_frontPos,owner->AI_RUN ? HANDLE_DOOR_ACCURACY_RUNNING : HANDLE_DOOR_ACCURACY))
							{
								// TODO: position not reachable, need a better one
							}
							_doorHandlingState = EStateMovingToFrontPos;
						}
					}
				}
				else
				{
					if (owner->GetMoveStatus() == MOVE_STATUS_WAITING)
					{
						// grayman #2345 - if you've been in queue too long, leave

						if ((_leaveQueue != -1) && (gameLocal.time >= _leaveQueue))
						{
							owner->m_leftQueue = true; // timed out of a door queue
							_leaveQueue = -1; // grayman #2345
							return true;
						}
					}
					else if (dist <= QUEUE_DISTANCE) // grayman #2345
					{
						if (dist <= NEAR_DOOR_DISTANCE) // grayman #2345 - too close to door when you're not the master?
						{
							MoveToSafePosition(frobDoor); // grayman #3390
						}
						else
						{
							owner->StopMove(MOVE_STATUS_WAITING);
							if (queuePos > 0)
							{
								_leaveQueue = gameLocal.time + (1 + gameLocal.random.RandomFloat()/2.0)*QUEUE_TIMEOUT; // set queue timeout
							}
						}
					}
				}
			}
			else // open
			{
				if (_canHandleDoor) // grayman #2712
				{
					// check if we are blocking the door
					if (frobDoor->IsBlocked() || 
						(frobDoor->WasInterrupted() || 
						frobDoor->WasStoppedDueToBlock()))
					{
						if (FitsThrough())
						{
							if (owner->AI_AlertIndex >= ESearching)
							{
								return true;
							}
						}
						else // grayman #3390
						{
							if ( masterUser == owner )
							{
								if (!owner->MoveToPosition(_frontPos,owner->AI_RUN ? HANDLE_DOOR_ACCURACY_RUNNING : HANDLE_DOOR_ACCURACY))
								{
									// TODO: position not reachable, need a better one
								}

								_doorHandlingState = EStateMovingToFrontPos;
								break;
							}
						}
					}
				}

				// grayman #2345 - if you're waiting in the queue, use a timeout
				// to leave the queue - masterUser is most likely stuck

				if (owner->GetMoveStatus() == MOVE_STATUS_WAITING)
				{
					// grayman #2345 - if you've been in queue too long, leave

					if ((_leaveQueue != -1) && (gameLocal.time >= _leaveQueue))
					{
						owner->m_leftQueue = true; // timed out of a door queue
						_leaveQueue = -1;
						return true;
					}
				}

				idVec3 dir = centerPos - owner->GetPhysics()->GetOrigin(); // grayman #3104 - use center of closed door
				dir.z = 0;
				float dist = dir.LengthFast();
				if (dist <= QUEUE_DISTANCE)
				{
					// grayman #1327 - don't move forward if someone other than me is searching near the door

					bool canMove2Door = ( masterUser == owner );
					if ( canMove2Door )
					{
						idEntity* searchingEnt = frobDoor->GetSearching();
						if ( searchingEnt )
						{
							if ( searchingEnt->IsType(idAI::Type) )
							{
								idAI* searchingAI = static_cast<idAI*>(searchingEnt);
								if ( searchingAI != owner )
								{
									idVec3 dirSearching = centerPos - searchingAI->GetPhysics()->GetOrigin(); // grayman #3104 - use center of closed door
									dirSearching.z = 0;
									float distSearching = dirSearching.LengthFast() - 32;
									if ( distSearching <= dist )
									{
										canMove2Door = false;
									}
								}
							}
						}
					}

					if ( canMove2Door )
					{
						if (!_canHandleDoor) // grayman #2712
						{
							owner->m_canResolveBlock = false;
							PickWhere2Go(frobDoor);
							break;
						}

						GetDoorHandlingPositions(owner, frobDoor);
						if (_doorInTheWay)
						{	
							DoorInTheWay(owner, frobDoor);
							_doorHandlingState = EStateMovingToBackPos;
						}
						else
						{
							PickWhere2Go(frobDoor); // grayman #3317 - go through an already-opened door, don't bother with _frontPos
//							if (!owner->MoveToPosition(_frontPos,HANDLE_DOOR_ACCURACY)) // grayman #2345 - need more accurate AI positioning
//							{
//								// TODO: position not reachable, need a better one
//							}
//							_doorHandlingState = EStateMovingToFrontPos;
						}
					}
					else if (dist <= NEAR_DOOR_DISTANCE) // grayman #1327 - too close to door when you're not the master
														 // or if you are the master but someone's searching around the door?
					{
						MoveToSafePosition(frobDoor); // grayman #3390
					}
					else if (owner->GetMoveStatus() != MOVE_STATUS_WAITING)
					{
						owner->StopMove(MOVE_STATUS_WAITING);
						if (queuePos > 0)
						{
							_leaveQueue = gameLocal.time + (1 + gameLocal.random.RandomFloat()/2.0)*QUEUE_TIMEOUT; // set queue timeout
						}
					}
				}
				else if (owner->MoveDone())
				{
					// grayman #3317 - use less position accuracy when running
					if (!owner->MoveToPosition(_frontPos,owner->AI_RUN ? HANDLE_DOOR_ACCURACY_RUNNING : HANDLE_DOOR_ACCURACY))
					{
						// TODO: position not reachable, need a better one
					}

					_doorHandlingState = EStateMovingToFrontPos; // grayman #2712
				}
			}
			break;
		case EStateMovingToSafePos:
			if (!frobDoor->IsOpen()) // closed
			{
				if ( owner->AI_MOVE_DONE || owner->ReachedPos(_safePos, MOVE_TO_POSITION) || (owner->GetTactileEntity() != NULL)) // grayman #3004 - leave this state if you're not moving
				{
					owner->StopMove(MOVE_STATUS_WAITING);
					owner->TurnToward(centerPos); // grayman #1327
					if (queuePos > 0)
					{
						_leaveQueue = gameLocal.time + (1 + gameLocal.random.RandomFloat()/2.0)*QUEUE_TIMEOUT; // set queue timeout
					}
					_doorHandlingState = EStateApproachingDoor;
				}
			}
			else // open
			{
				if (owner->AI_MOVE_DONE || owner->ReachedPos(_safePos, MOVE_TO_POSITION) || (owner->GetTactileEntity() != NULL)) // grayman #3004 - leave this state if you're not moving
				{
					owner->StopMove(MOVE_STATUS_WAITING);
					owner->TurnToward(centerPos); // grayman #1327

					// grayman #3390 - you've moved away from the door. If someone
					// else is closer than you, and you're the master, give up that role
					if ( masterUser == owner )
					{
						frobDoor->GetUserManager().ResetMaster(frobDoor); // redefine which AI is the master
						masterUser = frobDoor->GetUserManager().GetMasterUser();
						queuePos = frobDoor->GetUserManager().GetIndex(owner);
					}

					if (queuePos > 0) // not the master
					{
						_leaveQueue = gameLocal.time + (1 + gameLocal.random.RandomFloat()/2.0)*QUEUE_TIMEOUT; // set queue timeout
					}
					_doorHandlingState = EStateApproachingDoor;
				}
			}
			break;
		case EStateMovingToFrontPos:
			if (!frobDoor->IsOpen()) // closed
			{
				owner->m_canResolveBlock = false; // grayman #2345

				if (doubleDoor != NULL && doubleDoor->IsOpen())
				{
					// the other part of the double door is already open
					// no need to open this one
					ResetDoor(owner, doubleDoor);
					break;
				}
		
				if (!AllowedToOpen(owner))
				{
					AddToForbiddenAreas(owner, frobDoor);
					return true;
				}

				if (owner->ReachedPos(_frontPos, MOVE_TO_POSITION) || // grayman #2345 #2692 - are we close enough to reach around a blocking AI?
					(tactileEntity && tactileEntity->IsType(idAI::Type) && (closedPos - ownerOrigin).LengthFast() < 100))
				{
					// reached front position
					owner->StopMove(MOVE_STATUS_DONE);
					owner->TurnToward(closedPos);
					_waitEndTime = gameLocal.time + 750;
					_doorHandlingState = EStateWaitBeforeOpen;
					break;
				}
				else if (owner->AI_MOVE_DONE)
				{
					// grayman #3317 - use less position accuracy when running
					if (!owner->MoveToPosition(_frontPos,owner->AI_RUN ? HANDLE_DOOR_ACCURACY_RUNNING : HANDLE_DOOR_ACCURACY))
					{
						// TODO: position not reachable, need a better one
					}
				}
			}
			else // open
			{
				owner->m_canResolveBlock = false; // grayman #2345

				// check if the door was blocked or interrupted
				if (frobDoor->IsBlocked() || 
					frobDoor->WasInterrupted() || 
					frobDoor->WasStoppedDueToBlock())
				{
					if (FitsThrough())
					{
						// gap is large enough, go through if searching or in combat
						if (owner->AI_AlertIndex >= ESearching)
						{
							return true;
						}

						// gap is large enough, move to back position
						PickWhere2Go(frobDoor);
						break; // grayman #3390
					}

					// I can't fit through the door. Can I open it, even though I might
					// not be the master?

					if (!AllowedToOpen(owner))
					{
						AddToForbiddenAreas(owner, frobDoor);
						return true;
					}

					idVec3 currentPos = frobDoor->GetCurrentPos();
					// gameRenderWorld->DebugArrow(colorCyan, currentPos, currentPos + idVec3(0, 0, 20), 2, 1000);

					if (owner->ReachedPos(_frontPos, MOVE_TO_POSITION) ||
						((closedPos - ownerOrigin).LengthFast() < 100)) // grayman #3390 - are we close enough?
					{
						// reached front position, or close enough
						owner->StopMove(MOVE_STATUS_DONE);
						owner->TurnToward(currentPos);
						_waitEndTime = gameLocal.time + 650;
						_doorHandlingState = EStateWaitBeforeOpen;
					}
					else if (owner->AI_MOVE_DONE)
					{
						// grayman #3317 - use less position accuracy when running
						if (!owner->MoveToPosition(_frontPos,owner->AI_RUN ? HANDLE_DOOR_ACCURACY_RUNNING : HANDLE_DOOR_ACCURACY))
						{
							// TODO: position not reachable, need a better one
						}
					}
					else // can't fit through door, can't open it, see if I need to move out of the way
					{
						_doorHandlingState = EStateApproachingDoor;
					}
				}
				// door is already open, move to back position or mid position if you're the master
				else if (masterUser == owner)
				{
					// grayman #2345 - introduce use of a midpoint to help AI through doors that were found open

					PickWhere2Go(frobDoor);
				}
				else // grayman #3390 - otherwise, you shouldn't be moving to the front pos
				{
					_doorHandlingState = EStateApproachingDoor;
				}
			}
			break;
		case EStateWaitBeforeOpen:
			if (!frobDoor->IsOpen()) // closed
			{
				if (doubleDoor != NULL && doubleDoor->IsOpen())
				{
					// the other part of the double door is already open
					// no need to open this one
					ResetDoor(owner, doubleDoor);
					break;
				}

				if (!AllowedToOpen(owner))
				{
					AddToForbiddenAreas(owner, frobDoor);
					return true;
				}

				if (gameLocal.time >= _waitEndTime)
				{
					if (masterUser == owner)
					{
						owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Use_righthand", 4);

						_doorHandlingState = EStateStartOpen;
						_waitEndTime = gameLocal.time + owner->spawnArgs.GetInt("door_open_delay_on_use_anim", "500");
					}
				}
			}
			else // open
			{
				// check blocked or interrupted
				if (!FitsThrough())
				{
					if (!AllowedToOpen(owner))
					{
						if (frobDoor->IsBlocked() || 
							frobDoor->WasInterrupted() || 
							frobDoor->WasStoppedDueToBlock())
						{
							AddToForbiddenAreas(owner, frobDoor);
							return true;
						}
					}
					else if (gameLocal.time >= _waitEndTime)
					// grayman #720 - need the AI to reach for the door
					{
						// if (!OpenDoor())
						// {
						//		return true;
						// }
						owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Use_righthand", 4);
						_doorHandlingState = EStateStartOpen;
						_waitEndTime = gameLocal.time + owner->spawnArgs.GetInt("door_open_delay_on_use_anim", "500");
					}
				}
				else
				{
					// no need for waiting, door is already open, let's move
					if (owner->AI_AlertIndex >= ESearching)
					{
						return true;
					}

					PickWhere2Go(frobDoor);
				}
			}
			break;
		case EStateStartOpen:
			if (!frobDoor->IsOpen()) // closed
			{
				if (doubleDoor != NULL && doubleDoor->IsOpen())
				{
					// the other part of the double door is already open
					// no need to open this one
					ResetDoor(owner, doubleDoor);
					break;
				}
				if (!AllowedToOpen(owner))
				{
					AddToForbiddenAreas(owner, frobDoor);
					return true;
				}

				if (gameLocal.time >= _waitEndTime)
				{
					if (masterUser == owner)
					{
						frobDoor->SetWasFoundLocked(frobDoor->IsLocked()); // grayman #3104
						if (!OpenDoor())
						{
							return true;
						}
					}
				}
			}
			else // open
			{
				if (frobDoor->IsBlocked() || 
					(frobDoor->WasInterrupted() || 
					frobDoor->WasStoppedDueToBlock()))
				{
					if (!FitsThrough())
					{
						if (!AllowedToOpen(owner))
						{
							AddToForbiddenAreas(owner, frobDoor);
							return true;
						}

						if (gameLocal.time >= _waitEndTime)
						{
							if (!OpenDoor())
							{
								return true;
							}
						}
					}
				}
				else
				{
					if (owner->AI_AlertIndex >= ESearching)
					{
						return true;
					}

					// no need for waiting, door is already open, let's move

					PickWhere2Go(frobDoor);
				}
			}
			break;
		case EStateOpeningDoor:
			if (!frobDoor->IsOpen()) // closed
			{
				// we have already started opening the door, but it is closed

				// grayman #2862 - it's possible that we JUST opened the door and
				// it hasn't yet registered that it's not closed. So wait a short
				// while before deciding that it's still closed.

				if ( gameLocal.time < _waitEndTime )
				{
					break;
				}

				// the door isn't changing state, so it must truly be closed

				if (doubleDoor != NULL && doubleDoor->IsOpen())
				{
					// the other part of the double door is already open
					// no need to open this one
					ResetDoor(owner, doubleDoor);
					break;
				}

				if (!AllowedToOpen(owner))
				{
					AddToForbiddenAreas(owner, frobDoor);
					return true;
				}

				// try again
				owner->StopMove(MOVE_STATUS_DONE);
				owner->TurnToward(closedPos);
				if (masterUser == owner)
				{
					frobDoor->SetWasFoundLocked(frobDoor->IsLocked()); // grayman #3104
					if (!OpenDoor())
					{
						return true;
					}
				}
			}
			else // open
			{
				// check blocked
				if (frobDoor->IsBlocked() || 
					(frobDoor->WasInterrupted() && 
					frobDoor->WasStoppedDueToBlock()))
				{
					if ( !_triedFitting && FitsThrough() && (masterUser == owner)) // grayman #2345 - added _triedFitting
					{
						// gap is large enough, move to back position
						PickWhere2Go(frobDoor);
						_triedFitting = true; // grayman #2345
					}
					else
					{
						_triedFitting = false; // grayman #2345 - reset if needed
						if (frobDoor->GetLastBlockingEnt() == owner)
						{
							// we are blocking the door
							// check whether we should open or close it
							idVec3 forward = owner->viewAxis.ToAngles().ToForward(); // grayman #2345 - use viewaxis, not getaxis()
//							idVec3 forward = owner->GetPhysics()->GetAxis().ToAngles().ToForward(); // grayman #2345 - old way
							idVec3 doorDir = frobDoor->GetOpenDir() * frobDoor->GetPhysics()->GetAxis();

							if (forward * doorDir < 0)
							{
								// we are facing the opposite of the opening direction of the door
								// close it
								owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Use_righthand", 4);
								_doorHandlingState = EStateStartClose;
								_waitEndTime = gameLocal.time + owner->spawnArgs.GetInt("door_open_delay_on_use_anim", "500");
							}
							else
							{
								// we are facing the opening direction of the door
								// open it
								owner->StopMove(MOVE_STATUS_DONE);
								owner->TurnToward(closedPos);
								if (masterUser == owner)
								{
									if (!OpenDoor())
									{
										return true;
									}
								}
							}
						}
						else if (_retryCount > 3)
						{
							// if the door is blocked, stop after a few tries
							AddToForbiddenAreas(owner, frobDoor);
							return true;
						}
						else if (masterUser == owner)
						{
							// something else is blocking the door
							// possibly the player, another AI or an object
							// try closing the door and opening it again
							frobDoor->SetLastUsedBy(owner); // grayman #2859
							frobDoor->Close(true);
							_waitEndTime = gameLocal.time + 300;
							_doorHandlingState = EStateWaitBeforeOpen;
							_retryCount++;
						}
					}
				}
				//check interrupted
				else if (frobDoor->WasInterrupted())
				{
					if (FitsThrough() && masterUser == owner)
					{
						if (owner->AI_AlertIndex >= ESearching)
						{
							return true;
						}

						// gap is large enough, move to back position
						PickWhere2Go(frobDoor);
					}
					else if (!AllowedToOpen(owner))
					{
						AddToForbiddenAreas(owner, frobDoor);
						return true;
					}
					else if (masterUser == owner)
					{
						// can't move through already, need to open further
						if (!OpenDoor())
						{
							return true;
						}
					}
				}
				// when door is fully open, let's get moving
				else if	(!frobDoor->IsChangingState() && masterUser == owner)
				{
					// grayman #2712 - he can't stop being master until he's through the door
//					if (owner->AI_AlertIndex >= ESearching)
//					{
//						return true;
//					}

					PickWhere2Go(frobDoor); // grayman #2345 - recheck if you should continue to midPos
				}
			}
			break;
		case EStateMovingToMidPos:
			if (!frobDoor->IsOpen()) // closed
			{
				// door has closed while we were walking through it.
				// end this task (it will be initiated again if we are still in front of the door).
				return true;
			}
			else // open
			{
				// grayman #3104 - when searching, an AI can get far from the
				// door as he searches, but he still thinks he's handling a door
				// because he hasn't yet reached the mid position. If he wanders
				// too far while searching or in combat mode, quit door handling

				if ( owner->IsSearching() )
				{
					// We can assume the AI has wandered off if his distance
					// from the mid position is less than his distance to the
					// door center, and he's beyond a threshold distance.
					
					float dist2Goal = ( _midPos - ownerOrigin ).LengthFast();
					float dist2Door = ( centerPos - ownerOrigin).LengthFast();
					if ( ( dist2Door > QUEUE_DISTANCE ) && ( dist2Door > dist2Goal ) )
					{
						return true;
					}
				}
	
				if (_canHandleDoor)
				{
					// check blocked
					if (frobDoor->IsBlocked() || 
						(frobDoor->WasInterrupted() && 
						frobDoor->WasStoppedDueToBlock()))
					{
						if (frobDoor->GetLastBlockingEnt() == owner)
						{
							if (!owner->m_bCanOperateDoors) // grayman #2345
							{
								return true; // can't operate a door
							}

							// we are blocking the door
							owner->StopMove(MOVE_STATUS_DONE);
							if (masterUser == owner)
							{
								owner->TurnToward(closedPos);
								if (!OpenDoor())
								{
									return true;
								}
								break; // grayman #3390
							}
							else
							{
								// I'm NOT the master, so I can't handle the door
								MoveToSafePosition(frobDoor); // grayman #3390
							}
						}
					}
					else if (frobDoor->WasInterrupted() && !FitsThrough())
					{
						// grayman #3390 - The door is partly open, and I can't fit through
						// the opening. Since I'm the master, it's my responsibility to
						// get the door fully opened
						if (!owner->MoveToPosition(_frontPos,owner->AI_RUN ? HANDLE_DOOR_ACCURACY_RUNNING : HANDLE_DOOR_ACCURACY))
						{
							// TODO: position not reachable, need a better one
						}
						_doorHandlingState = EStateMovingToFrontPos;
						break;
					}
				}
				
				// reached mid position?
				if (owner->AI_MOVE_DONE)
				{
					if (owner->ReachedPos(_midPos, MOVE_TO_POSITION) || (owner->GetTactileEntity() != NULL)) // grayman #2345
					{
						return true;
					}

					if ( !owner->IsSearching() ) // grayman #3104 - it's ok to stand still while searching
					{
						owner->MoveToPosition(_midPos,HANDLE_DOOR_ACCURACY);
					}
				}
				else
				{
					if (_canHandleDoor) // grayman #2712
					{
						PickWhere2Go(frobDoor); // grayman #2345 - recheck if you should continue to midPos
					}
				}
			}
			break;
		case EStateMovingToBackPos:
			if (!frobDoor->IsOpen()) // closed
			{
				// door has closed while we were walking through it.
				// end this task (it will be initiated again if we are still in front of the door).
				return true;
			}
			else // open
			{
				// grayman #3104 - when searching, an AI can get far from the
				// door as he searches, but he still thinks he's handling a door
				// because he hasn't yet reached the back position. If he wanders
				// too far while searching or in combat mode, quit door handling

				if ( owner->IsSearching() )
				{
					// We can assume the AI has wandered off if his distance
					// from the back position is less than his distance to the
					// door center, and he's beyond a threshold distance.
					
					float dist2Goal = ( _backPos - ownerOrigin ).LengthFast();
					float dist2Door = ( centerPos - ownerOrigin).LengthFast();
					if ( ( dist2Door > QUEUE_DISTANCE ) && ( dist2Door > dist2Goal ) )
					{
						return true;
					}
				}

				// check blocked

				if (frobDoor->IsBlocked() || 
					(frobDoor->WasInterrupted() && 
					frobDoor->WasStoppedDueToBlock()))
				{
					if (frobDoor->GetLastBlockingEnt() == owner)
					{
						if (!owner->m_bCanOperateDoors) // grayman #2345
						{
							return true; // can't operate a door
						}

						// we are blocking the door

						if ( !_doorShouldBeClosed ) // grayman #2866
						{
							owner->StopMove(MOVE_STATUS_DONE);
							owner->TurnToward(closedPos);
							if (masterUser == owner)
							{
								if (!OpenDoor())
								{
									return true;
								}
							}
						}
						else
						{
							owner->StopMove(MOVE_STATUS_DONE);
						}
					}
				}
				else
				{
					if ( !_doorShouldBeClosed ) // grayman #2866
					{
						if (frobDoor->WasInterrupted() && !FitsThrough())
						{
							// end this task, it will be reinitialized when needed
							//return true;

							// grayman #3390 - instead of leaving the queue, stay in it
							// and move away, to a safe distance. Relinquish your master
							// position if you're the master.

							MoveToSafePosition(frobDoor); // grayman #3390
							break;
						}
					}
				}

				if (owner->AI_MOVE_DONE)
				{
					if (owner->ReachedPos(_backPos, MOVE_TO_POSITION) || // grayman #2345 #2692 - are we close enough to reach around a blocking AI?
						(tactileEntity && tactileEntity->IsType(idAI::Type) && (closedPos - ownerOrigin).LengthFast() < 100))
					{
						if (!AllowedToClose(owner) || owner->AI_RUN)  // grayman #2670
						{
							return true;
						}

						bool closeDoor = false;
						if (numUsers < 2)
						{
							if (_doorInTheWay || owner->ShouldCloseDoor(frobDoor) || _doorShouldBeClosed ) // grayman #2866
							{
								idEntity* controller = GetRemoteControlEntityForDoor();

								if (controller != NULL && controller->GetUserManager().GetNumUsers() == 0)
								{
									// We have an entity to control this door, interact with it
									subsystem.PushTask(TaskPtr(new InteractionTask(controller)));
									return false;
								}

								// close the door
								owner->StopMove(MOVE_STATUS_DONE);
								owner->TurnToward(openPos);
								_waitEndTime = gameLocal.time + 650;
								_doorHandlingState = EStateWaitBeforeClose;
								closeDoor = true;
							}
						}

						if (!closeDoor)
						{
							return true;
						}
					}
					else if ( !owner->IsSearching() ) // grayman #3104 - it's ok to stand still while searching
					{
						owner->MoveToPosition(_backPos,HANDLE_DOOR_ACCURACY); // grayman #2345 - need more accurate AI positioning
					}
				}
				else // grayman #2712
				{
					PickWhere2Go(frobDoor); // grayman #2345 - recheck if you should continue to _backPos
				}
			}
			break;
		case EStateWaitBeforeClose:
			if (!frobDoor->IsOpen()) // closed
			{
				// door has already closed before we were attempting to do it
				// no need for more waiting
				return true;
			}
			else // open
			{
				if (!AllowedToClose(owner) ||
					(!_doorInTheWay && (owner->AI_AlertIndex >= ESearching)) ||
					 owner->AI_RUN) // grayman #2670
				{
					return true;
				}

				if (gameLocal.time >= _waitEndTime && (numUsers < 2 || _doorInTheWay))
				{
					if (masterUser == owner)
					{
						owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Use_righthand", 4);
						_doorHandlingState = EStateStartClose;
						_waitEndTime = gameLocal.time + owner->spawnArgs.GetInt("door_open_delay_on_use_anim", "500");
					}
				}
				else if (numUsers > 1 && !_doorInTheWay)
				{
					return true;
				}
			}
			break;
		case EStateStartClose:
			if (!frobDoor->IsOpen()) // closed
			{
				// door has already closed before we were attempting to do it
				// no need for more waiting
				return true;
			}
			else // open
			{
				if (!AllowedToClose(owner) ||
					(!_doorInTheWay && (owner->AI_AlertIndex >= ESearching)) ||
					 owner->AI_RUN) // grayman #2670
				{
					return true;
				}

				if ( ( gameLocal.time >= _waitEndTime ) && ( ( numUsers < 2 ) || _doorInTheWay))
				{
					frobDoor->SetLastUsedBy(owner); // grayman #2859
					frobDoor->Close(true);
					_doorHandlingState = EStateClosingDoor;
				}
				else if ( ( numUsers > 1 ) && !_doorInTheWay)
				{
					return true;
				}
			}
			break;
		case EStateClosingDoor:
			if (!frobDoor->IsOpen()) // closed
			{
				// we have moved through the door and closed it

				// grayman #2862 - locking the door doesn't care how many users remain on the queue

/*				if (numUsers > 1)
				{
					return true;
				}
 */
				// If the door should ALWAYS be locked or it was locked before => lock it
				// but only if the owner is able to unlock it in the first place
				if (owner->CanUnlock(frobDoor) && AllowedToLock(owner) &&
					(_wasLocked || frobDoor->GetWasFoundLocked() || frobDoor->spawnArgs.GetBool("should_always_be_locked", "0"))) // grayman #3104
				{
					frobDoor->Lock(false); // lock the door
				}

				if (doubleDoor != NULL)
				{
					// If the other door is open, you need to close it.
					//
					// grayman #2732 - If it's closed, and needs to be locked, lock it.

					if (doubleDoor->IsOpen())
					{
						// the other part of the double door is still open
						// we want to close this one too
						ResetDoor(owner, doubleDoor);
						owner->MoveToPosition(_backPos,HANDLE_DOOR_ACCURACY); // grayman #2345 - need more accurate AI positioning
						_doorHandlingState = EStateMovingToBackPos;
						break;
					}
					else
					{
						if (owner->CanUnlock(doubleDoor) && AllowedToLock(owner) &&
							(_wasLocked || doubleDoor->GetWasFoundLocked() || doubleDoor->spawnArgs.GetBool("should_always_be_locked", "0"))) // grayman #3104
						{
							doubleDoor->Lock(false); // lock the second door
						}
					}
				}

				// continue what we were doing before.
				return true;
			}
			else // open
			{
				if (!AllowedToClose(owner) ||
					(owner->AI_AlertIndex >= ESearching) ||
					 owner->AI_RUN) // grayman #2670
				{
					return true;
				}

				// check blocked or interrupted
				if (frobDoor->IsBlocked() || 
					frobDoor->WasInterrupted() || 
					frobDoor->WasStoppedDueToBlock())
				{
					return true;
				}
			}
			break;
		default:
			break;
	}

	// grayman #2700 - set the door use timeout

	if ((_doorHandlingState == EStateMovingToMidPos) || (_doorHandlingState == EStateMovingToBackPos))
	{
		if (_leaveDoor < 0)
		{
			_leaveDoor = gameLocal.time + DOOR_TIMEOUT; // grayman #2700 - set door use timeout
		}
	}
	else
	{
		_leaveDoor = -1; // reset timeout
	}

	return false; // not finished yet
}

void HandleDoorTask::ResetDoor(idAI* owner, CFrobDoor* newDoor)
{
	Memory& memory = owner->GetMemory();

	// reset the active door to this door					
	memory.doorRelated.currentDoor = newDoor;
	// recalculate standing positions
	const idVec3& frobDoorOrg = newDoor->GetPhysics()->GetOrigin();
	const idVec3& openDir = newDoor->GetOpenDir();
	idVec3 awayPos = GetAwayPos(owner, newDoor);
	idVec3 towardPos = GetTowardPos(owner, newDoor);

	if (_doorHandlingState == EStateWaitBeforeClose
		|| _doorHandlingState == EStateStartClose
		|| _doorHandlingState == EStateClosingDoor)
	{
		// we have already walked through, so we are standing on the side of the backpos
		if (openDir * (owner->GetPhysics()->GetOrigin() - frobDoorOrg) > 0)
		{
			// Door swings away from us
			_frontPos = awayPos;
			_backPos = towardPos;
			_midPos = GetMidPos(owner, newDoor, false); // grayman #2345/#2712
		}
		else
		{
			// Door swings toward us
			_frontPos = towardPos;
			_backPos = awayPos;
			_midPos = GetMidPos(owner, newDoor, true); // grayman #2345/#2712
		}
	}
	else
	{
		if (openDir * (owner->GetPhysics()->GetOrigin() - frobDoorOrg) > 0)
		{
			_frontPos = towardPos;
			_backPos = awayPos;
		}
		else
		{
			_frontPos = awayPos;
			_backPos = towardPos;
		}
	}

	// check for custom door handling positions
	for (const idKeyValue* kv = newDoor->spawnArgs.MatchPrefix("door_handle_position"); kv != NULL; kv = newDoor->spawnArgs.MatchPrefix("door_handle_position", kv))
	{
		idStr posStr = kv->GetValue();
		idEntity* doorHandlingPosition = gameLocal.FindEntity(posStr);

		if (doorHandlingPosition)
		{
			idVec3 posOrg = doorHandlingPosition->GetPhysics()->GetOrigin();
			idVec3 posDir = posOrg - frobDoorOrg;

			if (_doorHandlingState == EStateWaitBeforeClose
				|| _doorHandlingState == EStateStartClose
				|| _doorHandlingState == EStateClosingDoor)
			{
				// we have already walked through, so we are standing on the side of the backpos
				if (openDir * (owner->GetPhysics()->GetOrigin() - frobDoorOrg) > 0)
				{
					// found door handling position behind the door
					_backPos = posOrg;
					_midPos = posOrg; // grayman #2345
					_backPosEnt = doorHandlingPosition;
				}
				else
				{
					// found door handling position in front of the door
					_frontPos = posOrg;
					_frontPosEnt = doorHandlingPosition;

				}
			}
			else
			{
				if (posDir * (owner->GetPhysics()->GetOrigin() - frobDoorOrg) > 0)
				{
					// found door handling position in front of the door
					_frontPos = posOrg;
					_frontPosEnt = doorHandlingPosition;
				}
				else
				{
					// found door handling position behind the door
					_backPos = posOrg;
					_midPos = posOrg; // grayman #2345
					_backPosEnt = doorHandlingPosition;
				}
			}
		}
	}
}

idVec3 HandleDoorTask::GetAwayPos(idAI* owner, CFrobDoor* frobDoor)
{
	const idVec3& frobDoorOrg = frobDoor->GetPhysics()->GetOrigin();
	const idVec3& openDir = frobDoor->GetOpenDir();
	const idVec3& closedPos = frobDoorOrg + frobDoor->GetClosedPos();

	idBounds frobDoorBounds = frobDoor->GetPhysics()->GetAbsBounds();

	idBounds bounds = owner->GetPhysics()->GetBounds();

	float size = bounds[1][0];

	idVec3 dir = closedPos - frobDoorOrg;
	dir.z = 0;
	idVec3 dirNorm = dir;
	dirNorm.NormalizeFast();
	//float dist = dir.LengthFast();

	idVec3 openDirNorm = openDir;
	openDirNorm.z = 0;
	openDirNorm.NormalizeFast();

	idVec3 parallelAwayOffset = dirNorm;
	parallelAwayOffset *= size * 1.4f;

	idVec3 normalAwayOffset = openDirNorm;
	normalAwayOffset *= size * 2.5;

	idVec3 awayPos = closedPos - parallelAwayOffset - normalAwayOffset;
	awayPos.z = frobDoorBounds[0].z + 5;

	return awayPos;
}

// grayman #2345/#2712 - new method to find the mid position

idVec3 HandleDoorTask::GetMidPos(idAI* owner, CFrobDoor* frobDoor, bool away)
{
	const idVec3& frobDoorOrg = frobDoor->GetPhysics()->GetOrigin();
	const idVec3& openDir = frobDoor->GetOpenDir();
	const idVec3& closedPos = frobDoorOrg + frobDoor->GetClosedPos();

	idBounds frobDoorBounds = frobDoor->GetPhysics()->GetAbsBounds();
	idBounds bounds = owner->GetPhysics()->GetBounds();
	float size = bounds[1][0];
	idVec3 dir = closedPos - frobDoorOrg;
	dir.z = 0;
	float doorWidth = dir.LengthFast();
	idVec3 dirNorm = dir;
	dirNorm.NormalizeFast();

	idVec3 openDirNorm = openDir;
	openDirNorm.z = 0;
	openDirNorm.NormalizeFast();

	idVec3 parallelMidOffset = dirNorm;
	parallelMidOffset *= doorWidth/2; // grayman #2712 - align with center of closed door position

	idVec3 normalMidOffset = openDirNorm;

	if (away)
	{
		normalMidOffset *= 1.25*doorWidth; // grayman #2712 - when the door swings away from you, clear it before ending the task
	}
	else
	{
		normalMidOffset *= -size * 3.0; // don't have to go so far when the door swings toward you
	}

	idVec3 midPos = closedPos - parallelMidOffset + normalMidOffset;
	midPos.z = frobDoorBounds[0].z + 5;

	return midPos;
}

idVec3 HandleDoorTask::GetTowardPos(idAI* owner, CFrobDoor* frobDoor)
{
	// calculate where to stand when the door swings towards us
	const idVec3& frobDoorOrg = frobDoor->GetPhysics()->GetOrigin();
	const idVec3& openDir = frobDoor->GetOpenDir();
	//const idVec3& openPos = frobDoorOrg + frobDoor->GetOpenPos();
	const idVec3& closedPos = frobDoorOrg + frobDoor->GetClosedPos();

	idBounds frobDoorBounds = frobDoor->GetPhysics()->GetAbsBounds();

	idBounds bounds = owner->GetPhysics()->GetBounds();
	float size = bounds[1][0];
	idTraceModel trm(bounds);
	idClipModel clip(trm);

	idVec3 dir = closedPos - frobDoorOrg;
	dir.z = 0;
	idVec3 dirNorm = dir;
	dirNorm.NormalizeFast();
	float dist = dir.LengthFast();
	
	idVec3 openDirNorm = openDir;
	openDirNorm.z = 0;
	openDirNorm.NormalizeFast();

	// next to the door
	idVec3 parallelTowardOffset = dirNorm;
	parallelTowardOffset *= dist + size * 2;

	idVec3 normalTowardOffset = openDirNorm;
	normalTowardOffset *= size * 2;

	idVec3 towardPos = frobDoorOrg + parallelTowardOffset + normalTowardOffset;
	towardPos.z = frobDoorBounds[0].z + 5;

	// check if we can stand at this position
	int contents = gameLocal.clip.Contents(towardPos, &clip, mat3_identity, CONTENTS_SOLID, owner);

	int areaNum = owner->GetAAS()->PointReachableAreaNum(towardPos, owner->GetPhysics()->GetBounds(), AREA_REACHABLE_WALK);

	if (contents || areaNum == 0 || frobDoor->GetOpenPeersNum() > 0)
	{
		if (cv_ai_door_show.GetBool())
		{
			gameRenderWorld->DebugBounds(colorRed, bounds, towardPos, 10000);
		}

		// this position is either blocked, in the void or can't be used since the door has open peers
		// try at 45° swinging angle
		parallelTowardOffset = dirNorm;

		normalTowardOffset = openDirNorm;

		towardPos = parallelTowardOffset + normalTowardOffset;
		towardPos.NormalizeFast();
		towardPos *= (dist + size * 2);
		towardPos += frobDoorOrg;
		towardPos.z = frobDoorBounds[0].z + 5;

		contents = gameLocal.clip.Contents(towardPos, &clip, mat3_identity, CONTENTS_SOLID, owner);

		areaNum = owner->GetAAS()->PointReachableAreaNum(towardPos, owner->GetPhysics()->GetBounds(), AREA_REACHABLE_WALK);

		if (contents || areaNum == 0 || frobDoor->GetOpenPeersNum() > 0)
		{
			if (cv_ai_door_show.GetBool())
			{
				gameRenderWorld->DebugBounds(colorRed, bounds, towardPos, 10000);
			}

			// not useable, try in front of the door far enough away
			parallelTowardOffset = dirNorm * size * 1.2f;

			normalTowardOffset = openDirNorm;
			normalTowardOffset *= dist + 2.5f * size;

			towardPos = frobDoorOrg + parallelTowardOffset + normalTowardOffset;
			towardPos.z = frobDoorBounds[0].z + 5;

			contents = gameLocal.clip.Contents(towardPos, &clip, mat3_identity, CONTENTS_SOLID, owner);

			areaNum = owner->GetAAS()->PointReachableAreaNum(towardPos, owner->GetPhysics()->GetBounds(), AREA_REACHABLE_WALK);

			if (contents || areaNum == 0)
			{
				// TODO: no suitable position found
				if (cv_ai_door_show.GetBool())
				{
					gameRenderWorld->DebugBounds(colorGreen, bounds, towardPos, 10000);
				}
			}
			else if (cv_ai_door_show.GetBool())
			{
				gameRenderWorld->DebugBounds(colorGreen, bounds, towardPos, 10000);
			}
		}
		else if (cv_ai_door_show.GetBool())
		{
			gameRenderWorld->DebugBounds(colorGreen, bounds, towardPos, 10000);
		}
	}
	else if (cv_ai_door_show.GetBool())
	{
		 gameRenderWorld->DebugBounds(colorGreen, bounds, towardPos, 10000);
	}
	return towardPos;
}

// grayman #720 - the previous FitsThrough() tried to fit an AI through at an angle
// which required taking wall thickness into account, but it didn't. This caused
// false positives which led to AI getting stuck trying to get through a door they
// were told they could fit through.

/*
bool HandleDoorTask::FitsThrough()
{
	// this calculates the gap (depending on the size of the door and the opening angle)
	// and checks if it is large enough for the AI to fit through it.
	idAI* owner = _owner.GetEntity();
	Memory& memory = owner->GetMemory();
	CFrobDoor* frobDoor = memory.doorRelated.currentDoor.GetEntity();

	idAngles tempAngle;
	idPhysics_Parametric* physics = frobDoor->GetMoverPhysics();
	physics->GetLocalAngles( tempAngle );

	const idVec3& closedPos = frobDoor->GetClosedPos();
	idVec3 dir = closedPos;
	dir.z = 0;
	float dist = dir.LengthFast();

	idAngles alpha = frobDoor->GetClosedAngles() - tempAngle;
	float absAlpha = idMath::Fabs(alpha.yaw);
	float sinAlpha = idMath::Sin(DEG2RAD(absAlpha * 0.5f));
	float delta = idMath::Fabs(2 * dist * sinAlpha);

	idBounds bounds = owner->GetPhysics()->GetBounds();
	float size = 2 * SQUARE_ROOT_OF_2 * bounds[1][0] + 10;

	return (delta >= size);
}
*/

// grayman #720 - this replacement FitsThrough() tries to fit the AI through
// from a head-on direction, which doesn't care about wall thickness. Doors
// need to be more open for the AI to fit through, but it no longer gives
// false positives.

bool HandleDoorTask::FitsThrough()
{
	// this calculates the gap (depending on the size of the door and the opening angle)
	// and checks if it is large enough for the AI to fit through it.
	idAI* owner = _owner.GetEntity();
	Memory& memory = owner->GetMemory();
	CFrobDoor* frobDoor = memory.doorRelated.currentDoor.GetEntity();

	idAngles tempAngle;
	idPhysics_Parametric* physics = frobDoor->GetMoverPhysics();
	physics->GetLocalAngles(tempAngle);

	const idVec3& closedPos = frobDoor->GetClosedPos();
	idVec3 dir = closedPos;
	dir.z = 0;
	float dist = dir.LengthFast();

	idAngles alpha = frobDoor->GetClosedAngles() - tempAngle;
	float absAlpha = idMath::Fabs(alpha.yaw);
	float delta = dist*(1.0 - idMath::Fabs(idMath::Cos(DEG2RAD(absAlpha))));

	idBounds bounds = owner->GetPhysics()->GetBounds();
	float size = 2*bounds[1][0] + 8;

	return (delta >= size);
}

bool HandleDoorTask::OpenDoor()
{
	idAI* owner = _owner.GetEntity();
	Memory& memory = owner->GetMemory();
	CFrobDoor* frobDoor = memory.doorRelated.currentDoor.GetEntity();

	// Update our door info structure
	DoorInfo& doorInfo = memory.GetDoorInfo(frobDoor);
	doorInfo.lastTimeSeen = gameLocal.time;
	doorInfo.lastTimeTriedToOpen = gameLocal.time;
	doorInfo.wasLocked = frobDoor->IsLocked();

	if (frobDoor->IsLocked())
	{
		if (!owner->CanUnlock(frobDoor) || !AllowedToUnlock(owner))
		{
			// Door is locked and we cannot unlock it
			// Check if we can open the other part of a double door
			CFrobDoor* doubleDoor = frobDoor->GetDoubleDoor();
			if ( ( doubleDoor != NULL ) && (!doubleDoor->IsLocked() || owner->CanUnlock(doubleDoor)))
			{
				ResetDoor(owner, doubleDoor);
				if (AllowedToUnlock(owner))
				{
					_doorHandlingState = EStateMovingToFrontPos;
					return true;
				}
				else
				{
					return false;
				}
			}
			owner->StopMove(MOVE_STATUS_DONE);
			// Rattle the door once
			frobDoor->Open(true);
				
			// add AAS area number of the door to forbidden areas
			AddToForbiddenAreas(owner, frobDoor);
			return false;
		}
		else
		{
			frobDoor->Unlock(true);
			doorInfo.wasLocked = frobDoor->IsLocked();
		}
	}

	frobDoor->SetLastUsedBy(owner); // grayman #2859
	owner->StopMove(MOVE_STATUS_DONE);
	frobDoor->Open(true);
	_doorHandlingState = EStateOpeningDoor;
	_waitEndTime = gameLocal.time + 1000; // grayman #2862 - a short wait to allow the door to begin opening

	return true;
}


void HandleDoorTask::GetDoorHandlingPositions(idAI* owner, CFrobDoor* frobDoor)
{
	const idVec3& frobDoorOrg = frobDoor->GetPhysics()->GetOrigin();
	const idVec3& openDir = frobDoor->GetOpenDir();

	// calculate where to stand when the door swings away from us
	idVec3 awayPos = GetAwayPos(owner, frobDoor);
	// calculate where to stand when the door swings towards us
	idVec3 towardPos =  GetTowardPos(owner, frobDoor);

	Memory& memory = owner->GetMemory();

	if ( _doorShouldBeClosed ) // grayman #2866 - suspicious door
	{
		if ( memory.closeFromAwayPos )
		{
			_backPos = awayPos;
		}
		else
		{
			_backPos = towardPos;
		}

		// grayman #3104 - also need mid position

		_midPos = this->GetMidPos(owner,frobDoor,true);
	}
	else // normal door
	{
		// check if the door swings towards or away from us
		if (openDir * (owner->GetPhysics()->GetOrigin() - frobDoorOrg) > 0)
		{
			// Door opens towards us

			_frontPos = towardPos;
			_backPos = awayPos;
			_midPos = GetMidPos(owner, frobDoor, false); // grayman #2345
		}
		else
		{
			// Door opens away from us

			_frontPos = awayPos;
			_backPos = towardPos;
			_midPos = GetMidPos(owner, frobDoor, true); // grayman #2345
		}
	}

	_frontPosEnt = NULL;
	_backPosEnt = NULL;

	// check for custom door handling positions

	idList< idEntityPtr<idEntity> > list;
	if ( frobDoor->GetDoorHandlingEntities( owner, list ) ) // for doors that use door handling positions
	{
		_frontPosEnt = list[0];
		_backPosEnt = list[1];
		if ( _doorShouldBeClosed ) // this is a suspicious door that needs closing
		{
			if ( _backPosEnt.GetEntity() != NULL )
			{
				if ( !memory.closeFromAwayPos )
				{
					_backPosEnt = _frontPosEnt;
				}

				_backPos = _backPosEnt.GetEntity()->GetPhysics()->GetOrigin();
			}
		}
		else // this is a normal door
		{
			if ( _frontPosEnt.GetEntity() != NULL )
			{
				_frontPos = _frontPosEnt.GetEntity()->GetPhysics()->GetOrigin();
			}

			if ( _backPosEnt.GetEntity() != NULL )
			{
				_backPos = _backPosEnt.GetEntity()->GetPhysics()->GetOrigin();
				_midPos = _backPos;
			}
		}
	}
}


void HandleDoorTask::DoorInTheWay(idAI* owner, CFrobDoor* frobDoor)
{
	_doorInTheWay = true;
	// check if the door swings towards or away from us
	const idVec3& openDir = frobDoor->GetOpenDir();
	const idVec3& frobDoorOrg = frobDoor->GetPhysics()->GetOrigin();
	const idVec3& closedPos = frobDoorOrg + frobDoor->GetClosedPos();
	const idVec3& openPos = frobDoorOrg + frobDoor->GetOpenPos();

	if (openDir * (owner->GetPhysics()->GetOrigin() - frobDoorOrg) > 0)
	{
		// Door opens towards us
		idVec3 closedDir = closedPos - frobDoorOrg;
		closedDir.z = 0;
		idVec3 org = owner->GetPhysics()->GetOrigin();
		idVec3 ownerDir = org - frobDoorOrg;
		ownerDir.z = 0;
		idVec3 frontPosDir = _frontPos - frobDoorOrg;
		frontPosDir.z = 0;

		float l1 = closedDir * ownerDir;
		float l2 = closedDir * frontPosDir;

		if (l1 * l2 < 0)
		{	
			const idBounds& bounds = owner->GetPhysics()->GetBounds();
			float size = bounds[1][0];

			// can't reach standard position
			idVec3 parallelOffset = openPos - frobDoorOrg;
			parallelOffset.z = 0;
			float len = parallelOffset.LengthFast();
			parallelOffset.NormalizeFast();
			parallelOffset *= len - 1.2f * size;

			idVec3 normalOffset = closedPos - frobDoorOrg;
			normalOffset.z = 0;
			normalOffset.NormalizeFast();
			normalOffset *= 1.5f * size;

			if ( _doorShouldBeClosed ) // grayman #2866
			{
				_backPos = frobDoorOrg + parallelOffset - normalOffset;
			}
			else
			{
				_frontPos = frobDoorOrg + parallelOffset - normalOffset;
			}
		}
		
		if ( _doorShouldBeClosed ) // grayman #2866
		{
			owner->MoveToPosition(_backPos,HANDLE_DOOR_ACCURACY); // grayman #2345 - need more accurate AI positioning
		}
		else
		{
			owner->MoveToPosition(_frontPos,HANDLE_DOOR_ACCURACY); // grayman #2345 - need more accurate AI positioning
		}
	}
	else
	{
		//Door opens away from us
		PickWhere2Go(frobDoor); // grayman #2712
	}
}

bool HandleDoorTask::AllowedToOpen(idAI* owner)
{
	idEntity* frontPosEntity = _frontPosEnt.GetEntity();

	if (frontPosEntity && frontPosEntity->spawnArgs.GetBool("ai_no_open"))
	{
		// AI is not allowed to open the door from this side
		return false;
	}
	return true;
}

bool HandleDoorTask::AllowedToClose(idAI* owner)
{
	idEntity* backPosEntity = _backPosEnt.GetEntity();

	if (backPosEntity && backPosEntity->spawnArgs.GetBool("ai_no_close"))
	{
		// AI is not allowed to close the door from this side
		return false;
	}
	return true;
}

bool HandleDoorTask::AllowedToUnlock(idAI* owner)
{
	idEntity* frontPosEntity = _frontPosEnt.GetEntity();

	if (frontPosEntity && frontPosEntity->spawnArgs.GetBool("ai_no_unlock"))
	{
		// AI is not allowed to unlock the door from this side
		return false;
	}
	return true;
}

bool HandleDoorTask::AllowedToLock(idAI* owner)
{
	idEntity* backPosEntity = _backPosEnt.GetEntity();

	if (backPosEntity && backPosEntity->spawnArgs.GetBool("ai_no_lock"))
	{
		// AI is not allowed to lock the door from this side
		return false;
	}
	return true;
}

void HandleDoorTask::AddToForbiddenAreas(idAI* owner, CFrobDoor* frobDoor)
{
	// add AAS area number of the door to forbidden areas
	idAAS*	aas = owner->GetAAS();
	if (aas != NULL)
	{
		int areaNum = frobDoor->GetAASArea(aas);
		gameLocal.m_AreaManager.AddForbiddenArea(areaNum, owner);
		owner->PostEventMS(&AI_ReEvaluateArea, owner->doorRetryTime, areaNum);
		frobDoor->RegisterAI(owner); // grayman #1145 - this AI is interested in this door

		// grayman #1327 - terminate a hiding spot search if one is going on. The AI
		// tried to get through this door to get to a spot, but since he can't reach
		// it, he should get another spot.

		Memory& memory = owner->GetMemory();
		if ( memory.hidingSpotInvestigationInProgress )
		{
			memory.hidingSpotInvestigationInProgress = false;
			memory.currentSearchSpot = idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY);
		}
	}
}

idEntity* HandleDoorTask::GetRemoteControlEntityForDoor()
{
	idAI* owner = _owner.GetEntity();
	Memory& memory = owner->GetMemory();

	CFrobDoor* frobDoor = memory.doorRelated.currentDoor.GetEntity();

	idEntity* bestController = NULL;

	for (const idKeyValue* kv = frobDoor->spawnArgs.MatchPrefix("door_controller");
		 kv != NULL; kv = frobDoor->spawnArgs.MatchPrefix("door_controller", kv))
	{
		// Find the entity with the given name
		idEntity* controller = gameLocal.FindEntity(kv->GetValue());

		if (controller == NULL) continue;

		if (bestController != NULL)
		{
			// We have a previously checked controller, check if this one is better
			float dist = (controller->GetPhysics()->GetOrigin() - owner->GetPhysics()->GetOrigin()).LengthSqr();
			float bestDist = (bestController->GetPhysics()->GetOrigin() - owner->GetPhysics()->GetOrigin()).LengthSqr();

			if (bestDist < dist)
			{
				continue; // no change, this one is a poorer choice
			}
		}
		
		bestController = controller;
	}

	return bestController;
}

// grayman - for debugging door queues

#if 0
void PrintDoorQ(CFrobDoor* frobDoor)
{
	DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("     %s's door queue ...\r", frobDoor->name.c_str());
	int n = frobDoor->GetUserManager().GetNumUsers();
	if ( n == 0 )
	{
		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("     is EMPTY\r");
		return;
	}

	idVec3 doorOrigin = frobDoor->GetPhysics()->GetOrigin();
	for ( int j = 0 ; j < n ; j++ )
	{
		idActor* u = frobDoor->GetUserManager().GetUserAtIndex(j);
		if ( u != NULL )
		{
			idVec3 dir = u->GetPhysics()->GetOrigin() - doorOrigin;
			dir.z = 0;
			float dist = dir.LengthFast();
			DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("     %s at %f\r", u->name.c_str(),dist);
		}
		else
		{
			DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("     NULL\r");
		}
	}
}
#endif

// grayman #2345 - when adding a door user, order the queue so that
// users still moving toward the door are in
// "distance from door" order. This handles the situation where a user
// who is close to the door is added after a user who is farther from
// the door. For example, an AI who is at the other end of a long hall,
// walking toward the door, and another AI comes around a corner near
// the door. The second AI should use the door first.

void HandleDoorTask::AddUser(idAI* newUser, CFrobDoor* frobDoor)
{
	int numUsers = frobDoor->GetUserManager().GetNumUsers();
	if (numUsers > 0)
	{
		frobDoor->GetUserManager().RemoveUser(newUser); // If newUser is already on the door queue, remove
														// them, because the queue is ordered by distance
														// from the door, and they need to be re-inserted
														// at the correct spot. If they're not already on
														// the queue, RemoveUser() does nothing.

		idVec3 centerPos = frobDoor->GetClosedBox().GetCenter();		// grayman #3104 - use center of closed door
		idVec3 dir = newUser->GetPhysics()->GetOrigin() - centerPos;	// grayman #3104 - use center of closed door
		dir.z = 0;
		float newUserDistanceSqr = dir.LengthSqr();
		float qSqr = Square(QUEUE_DISTANCE);
		for ( int i = 0 ; i < numUsers ; i++ )
		{
			idActor* user = frobDoor->GetUserManager().GetUserAtIndex(i);
			if ( user != NULL )
			{
				idVec3 userDir = user->GetPhysics()->GetOrigin() - centerPos; // grayman #3104 - use center of closed door
				userDir.z = 0;
				float userDistanceSqr = userDir.LengthSqr();
				if ( userDistanceSqr > qSqr ) // only cut in front of users not yet standing
				{
					if ( newUserDistanceSqr < userDistanceSqr ) // cut in front of a user farther away
					{
						frobDoor->GetUserManager().InsertUserAtIndex(newUser,i);
						
						// PrintDoorQ( frobDoor ); // grayman - for debugging door queues

						return;
					}
				}
			}
		}
	}

	frobDoor->GetUserManager().AppendUser(newUser);

	// PrintDoorQ( frobDoor ); // grayman - for debugging door queues
}

void HandleDoorTask::OnFinish(idAI* owner)
{
	Memory& memory = owner->GetMemory();
	CFrobDoor* frobDoor = memory.doorRelated.currentDoor.GetEntity();

	if (owner->m_HandlingDoor)
	{
		if (owner->m_RestoreMove) // grayman #2690/#2712 - AI run toward where they saw you last. Don't save that location when handling doors.
		{
			owner->PopMove();
		}

		owner->m_HandlingDoor = false;
	}

	_doorInTheWay = false;

	if (frobDoor != NULL) 
	{
		// Update our door info structure
		DoorInfo& doorInfo = memory.GetDoorInfo(frobDoor);
		doorInfo.lastTimeSeen = gameLocal.time;
		doorInfo.lastTimeUsed = gameLocal.time; // grayman #2345
		doorInfo.wasLocked = frobDoor->IsLocked();
		doorInfo.wasOpen = frobDoor->IsOpen();

		frobDoor->GetUserManager().RemoveUser(owner);
		frobDoor->GetUserManager().ResetMaster(frobDoor); // grayman #2345/#2706 - redefine which AI is the master

		// grayman #3104 - If you're the last one through the
		// door, set whether it's locked or unlocked. This takes
		// care of the problem of a door that's left open because
		// the last AI through was called off the door before
		// closing it. We don't want the door's locked state
		// from that use to govern what happens the next time
		// the door is used.

		if ( frobDoor->GetUserManager().GetNumUsers() == 0 )
		{
			frobDoor->SetWasFoundLocked(frobDoor->IsLocked()); // will only be true if the door is closed and locked
		}

		// PrintDoorQ( frobDoor ); // grayman - for debugging door queues

		CFrobDoor* doubleDoor = frobDoor->GetDoubleDoor();
		if (doubleDoor != NULL)
		{
			doubleDoor->GetUserManager().RemoveUser(owner);		// grayman #2345 - need to do for this what we did for a single door
			doubleDoor->GetUserManager().ResetMaster(doubleDoor);	// grayman #2345/#2706 - redefine which AI is the master

			// grayman #3104 - reset locked state for 2nd door

			if ( doubleDoor->GetUserManager().GetNumUsers() == 0 )
			{
				doubleDoor->SetWasFoundLocked(doubleDoor->IsLocked()); // will only be true if the door is closed and locked
			}
		}
		memory.lastDoorHandled = frobDoor; // grayman #2712
	}

	memory.doorRelated.currentDoor = NULL;

	if ( memory.closeSuspiciousDoor && frobDoor && ( memory.closeMe.GetEntity() == frobDoor ) ) // grayman #2866 - grayman #3104 - only forget suspicious door if it's the one I'm finishing now
	{
		memory.closeMe = NULL;
		memory.closeSuspiciousDoor = false;
		_doorShouldBeClosed = false;
		frobDoor->SetSearching(NULL);
		idAngles doorRotation = frobDoor->spawnArgs.GetAngles("rotate","0 90 0");
		float angleAdjustment = 90;
		if ( doorRotation.yaw != 0)
		{
			angleAdjustment = doorRotation.yaw;
		}
		owner->TurnToward(owner->GetCurrentYaw() - angleAdjustment); // turn away from the door
		owner->SetAlertLevel( ( owner->thresh_1 + owner->thresh_2 ) / 2.0f); // alert level is just below thresh_2 at this point, so this drops it down halfway to thresh_1
		frobDoor->AllowResponse(ST_VISUAL,owner); // grayman #3104 - respond again to this visual stim, in case the door was never closed
	}

	_leaveDoor = -1; // reset timeout for leaving the door
	_doorHandlingState = EStateNone;
	owner->m_canResolveBlock = true; // grayman #2345
	memory.stopHandlingDoor = false; // grayman #2816
}

bool HandleDoorTask::CanAbort() // grayman #2706
{
	return ( _doorHandlingState <= EStateMovingToSafePos );
}

void HandleDoorTask::DrawDebugOutput(idAI* owner)
{
	gameRenderWorld->DebugArrow(colorCyan, _frontPos, _frontPos + idVec3(0, 0, 20), 2, 1000);
	gameRenderWorld->DrawText("front", 
		(_frontPos + idVec3(0, 0, 30)), 
		0.2f, colorCyan, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 4 * gameLocal.msec);

	gameRenderWorld->DebugArrow(colorYellow, _backPos, _backPos + idVec3(0, 0, 20), 2, 1000);
	gameRenderWorld->DrawText("back", 
		(_backPos + idVec3(0, 0, 30)), 
		0.2f, colorYellow, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 4 * gameLocal.msec);

	gameRenderWorld->DebugArrow(colorPink, _midPos, _midPos + idVec3(0, 0, 20), 2, 1000);
	gameRenderWorld->DrawText("mid", 
		(_midPos + idVec3(0, 0, 30)), 
		0.2f, colorPink, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 4 * gameLocal.msec);

	/*	grayman #2345/#2706

	Show the door situation.

	Format is:
  
		<DoorHandlingState> <DoorName> - <Position in door queue>/<# users in door queue>

	For example:

		EStateMovingToFrontPos Door4 - 2/3

	means the AI is moving toward the front of the door and is currently in the second
	slot in a door queue of 3 users on door Door4.
	 */

	idStr position = "";
	if (owner->m_HandlingDoor)
	{
		CFrobDoor* frobDoor = owner->GetMemory().doorRelated.currentDoor.GetEntity();
		if (frobDoor != NULL)
		{
			idStr doorName = frobDoor->name;
			int numUsers = frobDoor->GetUserManager().GetNumUsers();
			int slot = frobDoor->GetUserManager().GetIndex(owner) + 1;
			if (slot > 0)
			{
				position = " " + doorName + " - " + slot + "/" + numUsers;
			}
		}
		else
		{
			position = " (ERROR: no door)";
		}
	}

	idStr str;
	switch (_doorHandlingState)
	{
		case EStateNone:
			str = "EStateNone";
			break;
		case EStateMovingToFrontPos:
			str = "EStateMovingToFrontPos";
			break;
		case EStateApproachingDoor:
			str = "EStateApproachingDoor";
			break;
		case EStateMovingToMidPos: // grayman #2345
			str = "EStateMovingToMidPos";
			break;
		case EStateMovingToSafePos: // grayman #2345
			str = "EStateMovingToSafePos";
			break;
		case EStateWaitBeforeOpen:
			str = "EStateWaitBeforeOpen";
			break;
		case EStateStartOpen:
			str = "EStateStartOpen";
			break;
		case EStateOpeningDoor:
			str = "EStateOpeningDoor";
			break;
		case EStateMovingToBackPos:
			str = "EStateMovingToBackPos";
			break;
		case EStateWaitBeforeClose:
			str = "EStateWaitBeforeClose";
			break;
		case EStateStartClose:
			str = "EStateStartClose";
			break;
		case EStateClosingDoor:
			str = "EStateClosingDoor";
			break;
		default: // grayman #2345
			str = "";
			break;
	}
	str += position;

	gameRenderWorld->DrawText(str.c_str(), 
		(owner->GetEyePosition() - owner->GetPhysics()->GetGravityNormal()*60.0f), 
		0.25f, colorYellow, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 4 * gameLocal.msec);

	Memory& memory = owner->GetMemory();
	CFrobDoor* frobDoor = memory.doorRelated.currentDoor.GetEntity();
	idActor* masterUser = frobDoor->GetUserManager().GetMasterUser();

	if (owner == masterUser)
	{
		gameRenderWorld->DrawText("Master", 
			(owner->GetPhysics()->GetOrigin() + idVec3(0, 0, 20)), 
			0.25f, colorOrange, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 4 * gameLocal.msec);
	}
	else
	{
		gameRenderWorld->DrawText("Slave", 
			(owner->GetPhysics()->GetOrigin() + idVec3(0, 0, 20)), 
			0.25f, colorMdGrey, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 4 * gameLocal.msec);

	}
}

void HandleDoorTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	savefile->WriteVec3(_frontPos);
	savefile->WriteVec3(_backPos);
	savefile->WriteVec3(_midPos);	// grayman #2345
	savefile->WriteVec3(_safePos);	// grayman #2345
	savefile->WriteInt(static_cast<int>(_doorHandlingState));
	savefile->WriteInt(_waitEndTime);
	savefile->WriteBool(_wasLocked);
	savefile->WriteBool(_doorInTheWay);
	savefile->WriteInt(_retryCount);
	savefile->WriteInt(_leaveQueue);		// grayman #2345
	savefile->WriteInt(_leaveDoor);			// grayman #2700
	savefile->WriteBool(_triedFitting);		// grayman #2345
	savefile->WriteBool(_canHandleDoor);	// grayman #2712
	savefile->WriteBool(_doorShouldBeClosed); // grayman #2866

	_frontPosEnt.Save(savefile);
	_backPosEnt.Save(savefile);
}

void HandleDoorTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	savefile->ReadVec3(_frontPos);
	savefile->ReadVec3(_backPos);
	savefile->ReadVec3(_midPos);	// grayman #2345
	savefile->ReadVec3(_safePos);	// grayman #2345
	int temp;
	savefile->ReadInt(temp);
	_doorHandlingState = static_cast<EDoorHandlingState>(temp);
	savefile->ReadInt(_waitEndTime);
	savefile->ReadBool(_wasLocked);
	savefile->ReadBool(_doorInTheWay);
	savefile->ReadInt(_retryCount);
	savefile->ReadInt(_leaveQueue);		// grayman #2345
	savefile->ReadInt(_leaveDoor);		// grayman #2700
	savefile->ReadBool(_triedFitting);	// grayman #2345
	savefile->ReadBool(_canHandleDoor);	// grayman #2712
	savefile->ReadBool(_doorShouldBeClosed); // grayman #2866

	_frontPosEnt.Restore(savefile);
	_backPosEnt.Restore(savefile);
}

HandleDoorTaskPtr HandleDoorTask::CreateInstance()
{
	return HandleDoorTaskPtr(new HandleDoorTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar handleDoorTaskRegistrar(
	TASK_HANDLE_DOOR, // Task Name
	TaskLibrary::CreateInstanceFunc(&HandleDoorTask::CreateInstance) // Instance creation callback
);

} // namespace ai
