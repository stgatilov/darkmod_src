/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include "../Memory.h"
#include "HandleDoorTask.h"
#include "InteractionTask.h"
#include "../AreaManager.h"

namespace ai
{

#define QUEUE_TIMEOUT 10000		// milliseconds (grayman #2345 - max time to wait in a door queue)
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
	/* grayman #2345 - it's possible to be here after:
	  
	     1) Beginning to handle this door.

	     2) Having to resolve a movement block, during which door-handling is pushed onto the task queue and ignored.

	     3) Coming back here when the the block is resolved and the door-handling task
		    is popped off the task queue.
	  
	   So we have to check if we're already handling a door. If so, leave.
	 */

	if (owner->m_HandlingDoor) // grayman #2345
	{
		return;
	}

	// Init the base class
	Task::Init(owner, subsystem);

	Memory& memory = owner->GetMemory();

	_retryCount = 0;

	_triedFitting = false; // grayman #2345 - useful if you're stuck behind a door

	_leaveQueue = -1; // grayman #2345

	CFrobDoor* frobDoor = memory.doorRelated.currentDoor.GetEntity();
	if (frobDoor == NULL)
	{
		return;
	}

	if (!owner->m_bCanOperateDoors)
	{
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
		// AI will ignore this door (not try to handle it) 
		if (!frobDoor->IsOpen() || !FitsThrough())
		{
			// if it is closed, add to forbidden areas so AI will not try to path find through
			idAAS*	aas = owner->GetAAS();
			if (aas != NULL)
			{
				int areaNum = frobDoor->GetAASArea(aas);
				gameLocal.m_AreaManager.AddForbiddenArea(areaNum, owner);
				owner->PostEventMS(&AI_ReEvaluateArea, owner->doorRetryTime, areaNum);
			}
		}
		subsystem.FinishTask();
		return;	
	}

	// Let the owner save its move
	owner->PushMove();
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

//	frobDoor->GetUserManager().AddUser(owner); // grayman #2345 - old way
	AddUser(owner,frobDoor); // grayman #2345 - order the queue if needed
	if (doubleDoor != NULL)
	{
//		doubleDoor->GetUserManager().AddUser(owner); // grayman #2345 - old way
		AddUser(owner,doubleDoor); // grayman #2345 - order the queue if needed
	}

	_doorInTheWay = false;

	GetDoorHandlingPositions(owner, frobDoor);

	_doorHandlingState = EStateNone;
}

// grayman #2345 - adjust goal positions based on what's happening around the door.

void HandleDoorTask::PickWhere2Go(CFrobDoor* door)
{
	// If you're using special door-handling entities, head for the back position.

	idAI* owner = _owner.GetEntity();
	if (_backPosEnt.IsValid())
	{
		if (_doorHandlingState != EStateMovingToBackPos)
		{
			owner->MoveToPosition(_backPos,HANDLE_DOOR_ACCURACY);
			_doorHandlingState = EStateMovingToBackPos;
		}
		return;
	}

	bool useMid = true;

	// If you're the only AI on the queue, close the door behind you.

	int numUsers = door->GetUserManager().GetNumUsers();
	if (numUsers < 2)
	{
		if (AllowedToClose(owner) && (_doorInTheWay || owner->ShouldCloseDoor(door)))
		{
			useMid = false; // use _backPos
		}
	}
	else // you're not alone
	{
		// Are there AI on the door queue who are waiting in
		// front of you? If so, head for the back position and not the mid position,
		// so there's less bumping when you exit the door.

		idVec3 myOrg = owner->GetPhysics()->GetOrigin();
		idVec3 myDir = owner->viewAxis.ToAngles().ToForward();

		for (int i = 1 ; i < numUsers ; i++) // ignore 0, which is yourself
		{
			idActor* actor = door->GetUserManager().GetUserAtIndex(i);
			if (actor->IsType(idAI::Type))
			{
				idAI* actorAI = static_cast<idAI*>(actor);
				if (actorAI->GetMoveStatus() == MOVE_STATUS_WAITING)
				{
					idVec3 actorDir = actorAI->viewAxis.ToAngles().ToForward();
					if ((actorDir * myDir) < 0)
					{
						// you and the actor are facing in different directions

						useMid = false; // use _backPos
						break;
					}
				}
			}
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

	int numUsers = frobDoor->GetUserManager().GetNumUsers();
	idActor* masterUser = frobDoor->GetUserManager().GetMasterUser();
	int queuePos = frobDoor->GetUserManager().GetIndex(owner); // grayman #2345

	const idVec3& frobDoorOrg = frobDoor->GetPhysics()->GetOrigin();
	const idVec3& openPos = frobDoorOrg + frobDoor->GetOpenPos();
	const idVec3& closedPos = frobDoorOrg + frobDoor->GetClosedPos();

	// if our current door is part of a double door, this is the other part.
	CFrobDoor* doubleDoor = frobDoor->GetDoubleDoor();

	idBounds bounds = owner->GetPhysics()->GetBounds();
	// angua: move the bottom of the bounds up a bit, to avoid finding small objects on the ground that are "in the way"
	bounds[0][2] += 16;
	float size = bounds[1][0];

	idEntity* frontPosEntity = _frontPosEnt.GetEntity();
	idEntity* backPosEntity = _backPosEnt.GetEntity();

	if (cv_ai_door_show.GetBool()) 
	{
		DrawDebugOutput(owner);
	}

	// Door is closed
	if (!frobDoor->IsOpen())
	{
		switch (_doorHandlingState)
		{
			case EStateNone:
				if (doubleDoor != NULL && doubleDoor->IsOpen())
				{
					// the other part of the double door is already open
					// no need to open this one
					ResetDoor(owner, doubleDoor);

					if (!owner->MoveToPosition(_frontPos,HANDLE_DOOR_ACCURACY)) // grayman #2345 - need more accurate AI positioning)
					{
						// TODO: position not reachable, need a better one
					}
					_doorHandlingState = EStateApproachingDoor;
					
					break;
				}
				else
				{
					if (!AllowedToOpen(owner))
					{
						AddToForbiddenAreas(owner, frobDoor);
						return true;
					}

					idEntity* controller = GetRemoteControlEntityForDoor();

					if (controller != NULL && masterUser == owner && controller->GetUserManager().GetNumUsers() == 0)
					{
						// We have an entity to control this door, interact with it
						owner->StopMove(MOVE_STATUS_DONE);
						subsystem.PushTask(TaskPtr(new InteractionTask(controller)));
						return false;
					}

					if (!owner->MoveToPosition(_frontPos,HANDLE_DOOR_ACCURACY)) // grayman #2345 - need more accurate AI positioning
					{
						// TODO: position not reachable, need a better one
					}
					_doorHandlingState = EStateApproachingDoor;
				}
				break;

			case EStateApproachingDoor:
			{
				idVec3 dir = frobDoorOrg - owner->GetPhysics()->GetOrigin();
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
							if (!owner->MoveToPosition(_frontPos,HANDLE_DOOR_ACCURACY)) // grayman #2345 - need more accurate AI positioning
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
							idVec3 dir2AI = owner->GetPhysics()->GetOrigin() - closedPos;
							dir2AI.z = 0;
							dir2AI.NormalizeFast();
							_safePos = closedPos + NEAR_DOOR_DISTANCE*dir2AI; // move to a safe spot

							if (!owner->MoveToPosition(_safePos,HANDLE_DOOR_ACCURACY))
							{
								// TODO: position not reachable, need a better one
							}
							_doorHandlingState = EStateMovingToSafePos;
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
				break;
			}

			case EStateMovingToSafePos: // grayman #2345
			{
				if (owner->ReachedPos(_safePos, MOVE_TO_POSITION))
				{
					owner->StopMove(MOVE_STATUS_WAITING);
					owner->TurnToward(closedPos);
					if (queuePos > 0)
					{
						_leaveQueue = gameLocal.time + (1 + gameLocal.random.RandomFloat()/2.0)*QUEUE_TIMEOUT; // set queue timeout
					}
					_doorHandlingState = EStateApproachingDoor;
				}
				break;
			}

			case EStateMovingToFrontPos:
			{
				owner->m_canResolveBlock = false; // grayman #2345
				owner->actionSubsystem->ClearTasks(); // grayman #2345 - stop playing an idle animation

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

				if (owner->ReachedPos(_frontPos, MOVE_TO_POSITION))
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
					owner->MoveToPosition(_frontPos,HANDLE_DOOR_ACCURACY); // grayman #2345 - need more accurate AI positioning
				}
				break;
			}

			case EStateWaitBeforeOpen:
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
				break;

			case EStateStartOpen:
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
						if (!OpenDoor())
						{
							return true;
						}
					}
				}
				break;

			case EStateOpeningDoor:
				// we have already started opening the door, but it is closed
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
					if (!OpenDoor())
					{
						return true;
					}
				}
				break;

			case EStateMovingToBackPos:
				// door has closed while we were walking through it.
				// end this task (it will be initiated again if we are still in front of the door).
				return true;
				break;
				
			case EStateMovingToMidPos: // grayman #2345
				// door has closed while we were walking through it.
				// end this task (it will be initiated again if we are still in front of the door).
				return true;
				break;
				
			case EStateWaitBeforeClose:
				// door has already closed before we were attempting to do it
				// no need for more waiting
				return true;
				break;


			case EStateStartClose:
				// door has already closed before we were attempting to do it
				// no need for more waiting
				return true;
				break;


			case EStateClosingDoor:
				// we have moved through the door and closed it
				if (numUsers < 2)
				{
					// If the door should ALWAYS be locked or it was locked before => lock it
					// but only if the owner is able to unlock it in the first place
					if (owner->CanUnlock(frobDoor) && AllowedToLock(owner) &&
						(_wasLocked || frobDoor->spawnArgs.GetBool("should_always_be_locked", "0")))
					{
						// if the door was locked before, lock it again
						frobDoor->Lock(false);
					}

					if (doubleDoor != NULL && doubleDoor->IsOpen())
					{
						// the other part of the double door is still open
						// we want to close this one too
						ResetDoor(owner, doubleDoor);
						owner->MoveToPosition(_backPos,HANDLE_DOOR_ACCURACY); // grayman #2345 - need more accurate AI positioning
						_doorHandlingState = EStateMovingToBackPos;
						break;
					}
				}
				// continue what we were doing before.
				return true;
				break;

			default:
				break;
		}
	}
	// Door is open
	else
	{
		switch (_doorHandlingState)
		{
			case EStateNone:

				// check if we are blocking the door
				if (frobDoor->IsBlocked() || 
					(frobDoor->WasInterrupted() || 
					frobDoor->WasStoppedDueToBlock()))
				{
					if (FitsThrough())
					{
						if (owner->AI_AlertIndex >= EInvestigating)
						{
							return true;
						}
						owner->MoveToPosition(_frontPos,HANDLE_DOOR_ACCURACY); // grayman #2345 - need more accurate AI positioning
						_doorHandlingState = EStateApproachingDoor;

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

						owner->MoveToPosition(_frontPos,HANDLE_DOOR_ACCURACY); // grayman #2345 - need more accurate AI positioning
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

					idVec3 dir = frobDoorOrg - owner->GetPhysics()->GetOrigin();
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

				break;

			case EStateApproachingDoor:
			{
				// check if we are blocking the door
				if (frobDoor->IsBlocked() || 
					(frobDoor->WasInterrupted() || 
					frobDoor->WasStoppedDueToBlock()))
				{
					if (FitsThrough())
					{
						if (owner->AI_AlertIndex >= EInvestigating)
						{
							return true;
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

				idVec3 dir = frobDoorOrg - owner->GetPhysics()->GetOrigin();
				dir.z = 0;
				float dist = dir.LengthFast();
				if (dist <= QUEUE_DISTANCE)
				{
					if (masterUser == owner)
					{
						GetDoorHandlingPositions(owner, frobDoor);
						if (_doorInTheWay)
						{	
							DoorInTheWay(owner, frobDoor);
							_doorHandlingState = EStateMovingToBackPos;

						}
						else
						{
							if (!owner->MoveToPosition(_frontPos,HANDLE_DOOR_ACCURACY)) // grayman #2345 - need more accurate AI positioning
							{
								// TODO: position not reachable, need a better one
							}
							_doorHandlingState = EStateMovingToFrontPos;

						}
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
					owner->MoveToPosition(_frontPos,HANDLE_DOOR_ACCURACY); // grayman #2345 - need more accurate AI positioning
				}
			
				break;
			}

			
			case EStateMovingToSafePos: // grayman #2345
			{
				if (owner->ReachedPos(_safePos, MOVE_TO_POSITION))
				{
					owner->StopMove(MOVE_STATUS_WAITING);
					owner->TurnToward(closedPos);
					if (queuePos > 0)
					{
						_leaveQueue = gameLocal.time + (1 + gameLocal.random.RandomFloat()/2.0)*QUEUE_TIMEOUT; // set queue timeout
					}
					_doorHandlingState = EStateApproachingDoor;
				}
				break;
			}
			
			case EStateMovingToFrontPos:
				owner->m_canResolveBlock = false;		// grayman #2345
				owner->actionSubsystem->ClearTasks();	// grayman #2345 - stop playing an idle animation

				// check if the door was blocked or interrupted
				if (frobDoor->IsBlocked() || 
					frobDoor->WasInterrupted() || 
					frobDoor->WasStoppedDueToBlock())
				{
					if (FitsThrough())
					{
						// gap is large enough, move to back position
						if (owner->AI_AlertIndex >= EInvestigating)
						{
							return true;
						}
						owner->MoveToPosition(_backPos,HANDLE_DOOR_ACCURACY); // grayman #2345 - need more accurate AI positioning
						_doorHandlingState = EStateMovingToBackPos;
					}
					else if (!AllowedToOpen(owner))
					{
						AddToForbiddenAreas(owner, frobDoor);
						return true;
					}

					idVec3 currentPos = frobDoor->GetCurrentPos();
					// gameRenderWorld->DebugArrow(colorCyan, currentPos, currentPos + idVec3(0, 0, 20), 2, 1000);

					if (owner->ReachedPos(_frontPos, MOVE_TO_POSITION))
					{
						// reached front position
						owner->StopMove(MOVE_STATUS_DONE);
						owner->TurnToward(currentPos);
						_waitEndTime = gameLocal.time + 650;
						_doorHandlingState = EStateWaitBeforeOpen;
					}
					else if (owner->AI_MOVE_DONE)
					{
						owner->MoveToPosition(_frontPos,HANDLE_DOOR_ACCURACY); // grayman #2345 - need more accurate AI positioning
					}
				}
				// door is already open, move to back position or mid position
				else if (masterUser == owner)
				{
					// grayman #2345 - introduce use of a midpoint to help AI through doors that were found open

					PickWhere2Go(frobDoor);
				}
				break;


			case EStateWaitBeforeOpen:
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
					{
						if (!OpenDoor())
						{
							return true;
						}
					}
						
				}
				else
				{
					// no need for waiting, door is already open, let's move
					if (owner->AI_AlertIndex >= EInvestigating)
					{
						return true;
					}

					owner->MoveToPosition(_backPos,HANDLE_DOOR_ACCURACY); // grayman #2345 - need more accurate AI positioning
					_doorHandlingState = EStateMovingToBackPos;
				}
				break;

			case EStateStartOpen:
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
					if (owner->AI_AlertIndex >= EInvestigating)
					{
						return true;
					}

					// no need for waiting, door is already open, let's move
					owner->MoveToPosition(_backPos,HANDLE_DOOR_ACCURACY); // grayman #2345 - need more accurate AI positioning
					_doorHandlingState = EStateMovingToBackPos;
				}
				break;


			case EStateOpeningDoor:
				// check blocked
				if (frobDoor->IsBlocked() || 
					(frobDoor->WasInterrupted() && 
					frobDoor->WasStoppedDueToBlock()))
				{
					if ( !_triedFitting && FitsThrough() && (masterUser == owner)) // grayman #2345 - added _triedFitting
					{
						// gap is large enough, move to back position
						owner->MoveToPosition(_backPos,HANDLE_DOOR_ACCURACY); // grayman #2345 - need more accurate AI positioning
						_doorHandlingState = EStateMovingToBackPos;
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
							frobDoor->Close(true);
							_waitEndTime = gameLocal.time + 300;
							_doorHandlingState = EStateWaitBeforeOpen;
							_retryCount ++;
							
						}
					}
				}
				//check interrupted
				else if (frobDoor->WasInterrupted())
				{
					if (FitsThrough() && masterUser == owner)
					{
						if (owner->AI_AlertIndex >= EInvestigating)
						{
							return true;
						}

						// gap is large enough, move to back position
						owner->MoveToPosition(_backPos,HANDLE_DOOR_ACCURACY); // grayman #2345 - need more accurate AI positioning
						_doorHandlingState = EStateMovingToBackPos;
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
				// door is fully open, let's get moving
				else if	(!frobDoor->IsChangingState() && masterUser == owner)
				{
					if (owner->AI_AlertIndex >= EInvestigating)
					{
						return true;
					}

					owner->MoveToPosition(_backPos,HANDLE_DOOR_ACCURACY); // grayman #2345 - need more accurate AI positioning
					_doorHandlingState = EStateMovingToBackPos;
				}
				break;

				// grayman #2345 - new state

			case EStateMovingToMidPos:
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
						owner->TurnToward(closedPos);
						if (masterUser == owner)
						{
							_doorHandlingState = EStateOpeningDoor;
							if (!OpenDoor())
							{
								return true;
							}
						}
					}
				}
				else if (frobDoor->WasInterrupted() && !FitsThrough())
				{
					// end this, task, it will be reinitialized when needed
					return true;
				}
				
				// reached mid position
				if (owner->AI_MOVE_DONE)
				{
					if (owner->ReachedPos(_midPos, MOVE_TO_POSITION))
					{
						return true;
					}
					else
					{
						owner->MoveToPosition(_midPos,HANDLE_DOOR_ACCURACY);
					}
				}
				else
				{
					PickWhere2Go(frobDoor); // grayman #2345 - recheck if you should continue to midPos
				}
				break;

			case EStateMovingToBackPos:
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
						owner->TurnToward(closedPos);
						if (masterUser == owner)
						{
							_doorHandlingState = EStateOpeningDoor;
							if (!OpenDoor())
							{
								return true;
							}
						}
					}
				}
				else if (frobDoor->WasInterrupted() && !FitsThrough())
				{
					// end this task, it will be reinitialized when needed
					return true;
				}

				if (owner->AI_MOVE_DONE)
				{
					if (owner->ReachedPos(_backPos, MOVE_TO_POSITION))
					{
						if (!AllowedToClose(owner))
						{
							return true;
						}
						bool closeDoor = false;
						if (numUsers < 2)
						{
							if (_doorInTheWay || owner->ShouldCloseDoor(frobDoor))
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
					else
					{
						owner->MoveToPosition(_backPos,HANDLE_DOOR_ACCURACY); // grayman #2345 - need more accurate AI positioning
					}
				}
				break;

				
			case EStateWaitBeforeClose:
				if (!AllowedToClose(owner))
				{
					return true;
				}

				if (!_doorInTheWay && owner->AI_AlertIndex >= EInvestigating)
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
				
				break;

			case EStateStartClose:
				if (!AllowedToClose(owner))
				{
					return true;
				}

				if (!_doorInTheWay && owner->AI_AlertIndex >= EInvestigating)
				{
					return true;
				}

				if (gameLocal.time >= _waitEndTime && (numUsers < 2 || _doorInTheWay))
				{
					frobDoor->Close(true);
					_doorHandlingState = EStateClosingDoor;
				}
				else if (numUsers > 1 && !_doorInTheWay)
				{
					return true;
				}
				break;


			case EStateClosingDoor:
				if (!AllowedToClose(owner))
				{
					return true;
				}
				if (owner->AI_AlertIndex >= EInvestigating)
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
				break;

			default:
				break;
		}
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
	idVec3 midPos = GetMidPos(owner, newDoor); // grayman #2345

	if (_doorHandlingState == EStateWaitBeforeClose
		|| _doorHandlingState == EStateStartClose
		|| _doorHandlingState == EStateClosingDoor)
	{
		// we have already walked through, so we are standing on the side of the backpos
		if (openDir * (owner->GetPhysics()->GetOrigin() - frobDoorOrg) > 0)
		{
			_frontPos = awayPos;
			_backPos = towardPos;
			_midPos = midPos; // grayman #2345
		}
		else
		{
			_frontPos = towardPos;
			_backPos = awayPos;
			_midPos = awayPos; // grayman #2345
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
	float dist = dir.LengthFast();

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

// grayman #2345 - new method to find the mid position

idVec3 HandleDoorTask::GetMidPos(idAI* owner, CFrobDoor* frobDoor)
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
	float dist = dir.LengthFast();

	idVec3 openDirNorm = openDir;
	openDirNorm.z = 0;
	openDirNorm.NormalizeFast();

	idVec3 parallelMidOffset = dirNorm;
	parallelMidOffset *= size * 1.4f;

	idVec3 normalMidOffset = openDirNorm;
	normalMidOffset *= size * 2.5;

	idVec3 midPos = closedPos - parallelMidOffset + normalMidOffset;
	midPos.z = frobDoorBounds[0].z + 5;

	return midPos;
}

idVec3 HandleDoorTask::GetTowardPos(idAI* owner, CFrobDoor* frobDoor)
{
	// calculate where to stand when the door swings towards us
	const idVec3& frobDoorOrg = frobDoor->GetPhysics()->GetOrigin();
	const idVec3& openDir = frobDoor->GetOpenDir();
	const idVec3& openPos = frobDoorOrg + frobDoor->GetOpenPos();
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
			if (doubleDoor != NULL && (!doubleDoor->IsLocked() || owner->CanUnlock(doubleDoor)))
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

	owner->StopMove(MOVE_STATUS_DONE);
	frobDoor->Open(true);
	_doorHandlingState = EStateOpeningDoor;

	return true;
}


void HandleDoorTask::GetDoorHandlingPositions(idAI* owner, CFrobDoor* frobDoor)
{
	const idVec3& frobDoorOrg = frobDoor->GetPhysics()->GetOrigin();
	const idVec3& openDir = frobDoor->GetOpenDir();

	//calculate where to stand when the door swings away from us
	idVec3 awayPos = GetAwayPos(owner, frobDoor);
	// calculate where to stand when the door swings towards us
	idVec3 towardPos =  GetTowardPos(owner, frobDoor);
	// grayman #2345 - calculate where to head when walking through an open door
	idVec3 midPos = GetMidPos(owner, frobDoor);

	// check if the door swings towards or away from us
	if (openDir * (owner->GetPhysics()->GetOrigin() - frobDoorOrg) > 0)
	{
		// Door opens towards us
		_frontPos = towardPos;
		_backPos = awayPos;
		_midPos = awayPos; // grayman #2345
	}
	else
	{
		// Door opens away from us
		_frontPos = awayPos;
		_backPos = towardPos;
		_midPos = midPos; // grayman #2345
	}

	_frontPosEnt = NULL;
	_backPosEnt = NULL;

	// check for custom door handling positions

	for (const idKeyValue* kv = frobDoor->spawnArgs.MatchPrefix("door_handle_position"); kv != NULL; kv = frobDoor->spawnArgs.MatchPrefix("door_handle_position", kv))
	{
		idStr posStr = kv->GetValue();
		idEntity* doorHandlingPosition = gameLocal.FindEntity(posStr);

		if (doorHandlingPosition)
		{
			const idVec3& closedPos = frobDoorOrg + frobDoor->GetClosedPos();

			idVec3 dir = closedPos - frobDoorOrg;
			dir.z = 0;

			idVec3 doorNormal = dir.Cross(gameLocal.GetGravity());

			idVec3 posOrg = doorHandlingPosition->GetPhysics()->GetOrigin();
			idVec3 posDir = posOrg - frobDoorOrg;
			posDir.z = 0;

			float posTest = doorNormal * posDir;

			idVec3 ownerDir = owner->GetPhysics()->GetOrigin() - frobDoorOrg;
			ownerDir.z = 0;

			float ownerTest = doorNormal * ownerDir;

			if (posTest * ownerTest > 0)
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

			_frontPos = frobDoorOrg + parallelOffset - normalOffset;
		}
		
		owner->MoveToPosition(_frontPos,HANDLE_DOOR_ACCURACY); // grayman #2345 - need more accurate AI positioning
	}
	else
	{
		//Door opens away from us
		owner->MoveToPosition(_backPos,HANDLE_DOOR_ACCURACY); // grayman #2345 - need more accurate AI positioning
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

// grayman #2345 - whoever's closest to the door now gets to be master,
// so he has to be moved to the top of the user list. This cuts down on
// confusion around doors.

void HandleDoorTask::ResetMaster(CFrobDoor* frobDoor)
{
	int numUsers = frobDoor->GetUserManager().GetNumUsers();
	if (numUsers > 1)
	{
		idVec3 doorOrigin = frobDoor->GetPhysics()->GetOrigin();
		idActor* closestUser = NULL;	// the user closest to the door
		int masterIndex = 0;			// index of closest user
		float minDistance = 100000;		// minimum distance of all users
		for (int i = 0 ; i < numUsers ; i++)
		{
			idActor* user = frobDoor->GetUserManager().GetUserAtIndex(i);
			if (user != NULL)
			{
				float distance = (user->GetPhysics()->GetOrigin() - doorOrigin).LengthFast();
				if (distance < minDistance)
				{
					masterIndex = i;
					closestUser = user;
					minDistance = distance;
				}
			}
		}

		if (masterIndex > 0) // only rearrange the queue if someone other than the current master is closer
		{
			frobDoor->GetUserManager().RemoveUser(closestUser);				// remove AI from current spot
			frobDoor->GetUserManager().InsertUserAtIndex(closestUser,0);	// and put him at the top
		}
	}
}

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
		// Is newUser already on the door queue?

		if (frobDoor->GetUserManager().GetIndex(newUser) >= 0)
		{
			return;
		}

		idVec3 doorOrigin = frobDoor->GetPhysics()->GetOrigin();
		float newUserDistance = (newUser->GetPhysics()->GetOrigin() - doorOrigin).LengthFast();
		for (int i = 0 ; i < numUsers ; i++)
		{
			idActor* user = frobDoor->GetUserManager().GetUserAtIndex(i);
			if (user != NULL)
			{
				float userDistance = (user->GetPhysics()->GetOrigin() - doorOrigin).LengthFast();
				if (userDistance > QUEUE_DISTANCE) // only cut in front of users not yet standing
				{
					if (newUserDistance < userDistance) // cut in front of a user farther away
					{
						frobDoor->GetUserManager().InsertUserAtIndex(newUser,i);
						return;
					}
				}
			}
		}
	}

	frobDoor->GetUserManager().AddUser(newUser);
}

void HandleDoorTask::OnFinish(idAI* owner)
{
	Memory& memory = owner->GetMemory();
	CFrobDoor* frobDoor = memory.doorRelated.currentDoor.GetEntity();

	if (owner->m_HandlingDoor)
	{
		owner->PopMove();
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

		ResetMaster(frobDoor); // grayman #2345 - redefine which AI is the master

		CFrobDoor* doubleDoor = frobDoor->GetDoubleDoor();
		if (doubleDoor != NULL)
		{
			doubleDoor->GetUserManager().RemoveUser(owner); // grayman #2345 - need to do for this what we did for a single door
			ResetMaster(doubleDoor); // grayman #2345 - redefine which AI is the master
		}
	}

	memory.doorRelated.currentDoor = NULL;
	_doorHandlingState = EStateNone;
	owner->m_canResolveBlock = true; // grayman #2345
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
			str = "EStateMovingToFrontPos";
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
	}
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
	savefile->WriteInt(_leaveQueue);	// grayman #2345
	savefile->WriteBool(_triedFitting);	// grayman #2345
	
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
	savefile->ReadBool(_triedFitting);	// grayman #2345

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
