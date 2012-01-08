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
#include "HandleElevatorTask.h"
#include "../../MultiStateMoverButton.h"
#include "../EAS/EAS.h"

namespace ai
{

HandleElevatorTask::HandleElevatorTask() :
	_waitEndTime(0),
	_success(false)
{}

HandleElevatorTask::HandleElevatorTask(const eas::RouteInfoPtr& routeInfo) :
	_waitEndTime(0),
	_routeInfo(*routeInfo), // copy-construct the RouteInfo, creates a clean duplicate
	_success(false)
{}

// Get the name of this task
const idStr& HandleElevatorTask::GetName() const
{
	static idStr _name(TASK_HANDLE_ELEVATOR);
	return _name;
}

void HandleElevatorTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Init the base class
	Task::Init(owner, subsystem);

	owner->PushMove(); // Save the move

	if (_routeInfo.routeNodes.size() < 2)
	{
		// no RouteNodes available?
		subsystem.FinishTask(); 
		return;
	}

	// Grab the first RouteNode
	const eas::RouteNodePtr& node = *_routeInfo.routeNodes.begin();

	// Retrieve the elevator station info from the EAS
	eas::ElevatorStationInfoPtr stationInfo = 
		owner->GetAAS()->GetEAS()->GetElevatorStationInfo(node->elevatorStation);

	//Memory& memory = owner->GetMemory();
	CMultiStateMoverPosition* pos = stationInfo->elevatorPosition.GetEntity();
	CMultiStateMover* elevator = stationInfo->elevator.GetEntity();

	// Is the elevator station reachable?
	if (!IsElevatorStationReachable(pos))
	{
		subsystem.FinishTask();
		return;
	}

	if (owner->ReachedPos(pos->GetPhysics()->GetOrigin(), MOVE_TO_POSITION))
	{
		// We are already at the elevator position, this is true if the elevator is there
		_state = EInitiateMoveToRideButton;
	}
	// Start moving towards the elevator station
	else if (owner->MoveToPosition(pos->GetPhysics()->GetOrigin()))
	{
		// If AI_MOVE_DONE is true, we are already at the target position
		_state = (owner->GetMoveStatus() == MOVE_STATUS_DONE) ? EInitiateMoveToRideButton : EMovingTowardsStation;
	}
	else
	{
		// Position entity cannot be reached, probably due to elevator not being there, use the button entity
		_state = EInitiateMoveToFetchButton;
	}

	if (_state == EInitiateMoveToRideButton)
	{
		// add ourself to the user lists of the elevator
		elevator->GetUserManager().AddUser(owner);
	}
}

bool HandleElevatorTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("HandleElevatorTask performing.\r");

	idAI* owner = _owner.GetEntity();

	// grayman #2948 - leave elevator handling if KO'ed or dead

	if ( owner->AI_KNOCKEDOUT || owner->AI_DEAD )
	{
		return true;
	}
	
	//Memory& memory = owner->GetMemory();

	// Grab the first RouteNode
	const eas::RouteNodePtr& node = *_routeInfo.routeNodes.begin();

	// Retrieve the elevator station info from the EAS
	eas::ElevatorStationInfoPtr stationInfo = 
		owner->GetAAS()->GetEAS()->GetElevatorStationInfo(node->elevatorStation);

	CMultiStateMoverPosition* pos = stationInfo->elevatorPosition.GetEntity();
	CMultiStateMover* elevator = stationInfo->elevator.GetEntity();

	// check users of elevator and position
	int numElevatorUsers = elevator->GetUserManager().GetNumUsers();
	idActor* masterElevatorUser = elevator->GetUserManager().GetMasterUser();
	int numPositionUsers = pos->GetUserManager().GetNumUsers();

	// Grab the second RouteNode
	eas::RouteNodeList::const_iterator first = _routeInfo.routeNodes.begin();
	const eas::RouteNodePtr& node2 = *(++first);

	// Retrieve the elevator station info from the EAS
	eas::ElevatorStationInfoPtr stationInfo2 = 
		owner->GetAAS()->GetEAS()->GetElevatorStationInfo(node2->elevatorStation);

	CMultiStateMoverPosition* targetPos = stationInfo2->elevatorPosition.GetEntity();

	idVec3 dir;
	float dist;

	switch (_state)
	{
		case EMovingTowardsStation:
			//dist = (owner->GetPhysics()->GetOrigin() - pos->GetPhysics()->GetOrigin()).LengthFast();
			if (owner->GetMoveStatus() == MOVE_STATUS_DONE)
			{
				// Move is done, this means that we might be close enough, but it's not guaranteed
				_state = EInitiateMoveToFetchButton;
			}
			else if ((owner->GetPhysics()->GetOrigin() - pos->GetPhysics()->GetOrigin()).LengthSqr() < 500*500 &&
				      (owner->CanSeeExt(pos, true, false) || owner->CanSeeExt(elevator, true, false)))
			{
				if (elevator->IsAtPosition(pos))
				{
					// TODO: elevator is at the desired position, get onto it
					MoveToPositionEntity(owner, pos);
					_state = EMoveOntoElevator;

					// add ourself to the user lists of the elevator and the elevator position
					elevator->GetUserManager().AddUser(owner);
					pos->GetUserManager().AddUser(owner);

				}
				else
				{
					// elevator is somewhere else
					_state = EInitiateMoveToFetchButton;
				}

			}	
			break;

		case EInitiateMoveToFetchButton:
		{
			CMultiStateMoverButton* fetchButton = pos->GetFetchButton();
			if (fetchButton == NULL)
			{
				owner->StopMove(MOVE_STATUS_DEST_UNREACHABLE);
				return true;
			}
			// it's not occupied, get to the button
			MoveToButton(owner, fetchButton);
			_state = EMovingToFetchButton;
		}
		break;

		case EMovingToFetchButton:
		{
			CMultiStateMoverButton* fetchButton = pos->GetFetchButton();
			if (fetchButton == NULL)
			{
				owner->StopMove(MOVE_STATUS_DEST_UNREACHABLE);
				return true;
			}

			dir = owner->GetPhysics()->GetOrigin() - fetchButton->GetPhysics()->GetOrigin();
			dir.z = 0;
			dist = dir.LengthFast();
			if (dist < owner->GetArmReachLength() + 10)
			{
				owner->StopMove(MOVE_STATUS_DONE);

				// add ourself to the user lists of the elevator and the elevator position
				elevator->GetUserManager().AddUser(owner);
				pos->GetUserManager().AddUser(owner);

				numElevatorUsers = elevator->GetUserManager().GetNumUsers();
				masterElevatorUser = elevator->GetUserManager().GetMasterUser();
				numPositionUsers = pos->GetUserManager().GetNumUsers();

				if (numElevatorUsers > 1 && masterElevatorUser != owner)
				{
					// elevator is in use, have to wait

					// elevator is at the right position, get onto it
					if (elevator->IsAtPosition(pos))
					{
						owner->MoveToPosition(pos->GetPhysics()->GetOrigin());
						_state = EMoveOntoElevator;
					}
				}
				else
				{
					// raise hand to press button
					owner->TurnToward(fetchButton->GetPhysics()->GetOrigin());
					owner->Event_LookAtPosition(fetchButton->GetPhysics()->GetOrigin(), 1);
					owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Use_righthand", 4);
					_state = EPressFetchButton;
					_waitEndTime = gameLocal.time + 400;
				}

			}
			else if (owner->GetMoveStatus() == MOVE_STATUS_DEST_UNREACHABLE)
			{
				// Destination unreachable, help!
				return true;
			}
		}
		break;

		case EPressFetchButton:
		{
			CMultiStateMoverButton* fetchButton = pos->GetFetchButton();
			if (fetchButton == NULL)
			{
				owner->StopMove(MOVE_STATUS_DEST_UNREACHABLE);
				return true;
			}

			if (gameLocal.time >= _waitEndTime)
			{
				// Press button and wait for elevator
				fetchButton->Operate();
				owner->StopMove(MOVE_STATUS_WAITING);
				_state = EWaitForElevator;

				// this is to avoid having the AI pressing the button twice 
				// when the elevator takes too long to start moving
				_waitEndTime = gameLocal.time + 500;
			}
		}
		break;

		case EWaitForElevator:
			if (elevator->IsAtPosition(pos))
			{
				// TODO elevator is at the desired position, get onto it
				owner->MoveToPosition(pos->GetPhysics()->GetOrigin());
				_state = EMoveOntoElevator;
			}
			else if (elevator->IsAtRest() && gameLocal.time > _waitEndTime)
			{
				// Elevator has stopped moving and is not at our station, press the fetch button!
				_state = EInitiateMoveToFetchButton;
			}
			break;

		case EMoveOntoElevator:
		{
			if (owner->GetMoveStatus() == MOVE_STATUS_DONE)
			{
				// Move towards ride button
				_state = EInitiateMoveToRideButton;

				// remove ourself from users list of elevator station
				pos->GetUserManager().RemoveUser(owner);
			}
			else if (owner->OnElevator(false))
			{
				// remove ourself from users list of elevator station
				pos->GetUserManager().RemoveUser(owner);
			}
			
			if (!elevator->IsAtPosition(pos))
			{
				// elevator moved away while we attempted to move onto it
				if (owner->OnElevator(false))
				{
					// elevator is already moving
					owner->StopMove(MOVE_STATUS_DONE);
					_waitEndTime = gameLocal.time + 500;
					_state = ERideOnElevator;
				}
				else
				{

					// move towards fetch button
					owner->StopMove(MOVE_STATUS_DONE);
					_state = EInitiateMoveToFetchButton;
				}
			}

		}
		break;

		case EInitiateMoveToRideButton:
		{
			CMultiStateMoverButton* rideButton = pos->GetRideButton(targetPos);
			if (rideButton == NULL)
			{
				owner->StopMove(MOVE_STATUS_DEST_UNREACHABLE);
				return true;
			}

			if (!elevator->IsAtPosition(pos))
			{
				// elevator is already moving
				owner->StopMove(MOVE_STATUS_DONE);
				_state = ERideOnElevator;
				_waitEndTime = gameLocal.time + 500;
			}
			else
			{
				MoveToButton(owner, rideButton);
				_state = EMovingToRideButton;
			}
		}
		break;

		case EMovingToRideButton:
		{
			CMultiStateMoverButton* rideButton = pos->GetRideButton(targetPos);
			if (rideButton == NULL)
			{
				owner->StopMove(MOVE_STATUS_DEST_UNREACHABLE);
				return true;
			}

			if (!elevator->IsAtPosition(pos))
			{
				// elevator is already moving
				owner->StopMove(MOVE_STATUS_DONE);
				_state = ERideOnElevator;
				_waitEndTime = gameLocal.time + 500;
				break;
			}

			dir = owner->GetPhysics()->GetOrigin() - rideButton->GetPhysics()->GetOrigin();
			dir.z = 0;
			dist = dir.LengthFast();
			if (dist < owner->GetArmReachLength() + 20)
			{
				owner->StopMove(MOVE_STATUS_DONE);
				if (numPositionUsers < 1)
				{
					// all users at this station have moved onto the elevator
					// (removed themselves from the position user list)
					// raise hand to press ride button
					owner->TurnToward(rideButton->GetPhysics()->GetOrigin());
					owner->Event_LookAtPosition(rideButton->GetPhysics()->GetOrigin(), 1);
					owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Use_righthand", 4);
					_state = EPressRideButton;
					_waitEndTime = gameLocal.time + 400;
				}
			}
		}
		break;

		case EPressRideButton:
		{
			if (!elevator->IsAtPosition(pos))
			{
				// elevator is already moving
				owner->StopMove(MOVE_STATUS_DONE);
				_waitEndTime = gameLocal.time + 500;
				_state = ERideOnElevator;
				break;
			}

			if (gameLocal.time >= _waitEndTime)
			{
				CMultiStateMoverButton* rideButton = pos->GetRideButton(targetPos);
				if (rideButton != NULL)
				{
					// Press button and wait while elevator moves
					rideButton->Operate();
					_state = ERideOnElevator;
					_waitEndTime = gameLocal.time + 500;
				}
				else
				{
					owner->StopMove(MOVE_STATUS_DEST_UNREACHABLE);
					return true;
				}
			}
		}
		break;

		case ERideOnElevator:
			if (elevator->IsAtPosition(targetPos))
			{
				// reached target station, get off the elevator
				_success = true;
				_state = EGetOffElevator;
				// Restore the movestate we had before starting this task
				owner->PopMove();
			}
			else if (elevator->IsAtRest() && gameLocal.time > _waitEndTime)
			{
				// elevator stopped at a different position
				// restart our pathing from here
				return true;
			}
			break;
		
		case EGetOffElevator:
			if (!owner->OnElevator(false))
			{
				return true;
			}
			break;


		default:
			break;
	}


	// Optional debug output
	if (cv_ai_elevator_show.GetBool())
	{
		DebugDraw(owner);
	}

	return false; // not finished yet
}

bool HandleElevatorTask::IsElevatorStationReachable(CMultiStateMoverPosition* pos)
{
	// TODO: Implement check here

	return true;
}

void HandleElevatorTask::OnFinish(idAI* owner)
{
	//Memory& memory = owner->GetMemory();

	owner->m_HandlingElevator = false;

	// Grab the first RouteNode
	const eas::RouteNodePtr& node = *_routeInfo.routeNodes.begin();

	// Retrieve the elevator station info from the EAS
	eas::ElevatorStationInfoPtr stationInfo = 
		owner->GetAAS()->GetEAS()->GetElevatorStationInfo(node->elevatorStation);

	// remove ourself from the user lists of the elevator and the elevator position
	CMultiStateMoverPosition* pos = stationInfo->elevatorPosition.GetEntity();
	pos->GetUserManager().RemoveUser(owner);

	CMultiStateMover* elevator = stationInfo->elevator.GetEntity();
	elevator->GetUserManager().RemoveUser(owner);


	if (!_success)
	{
		// This task could not finish successfully, stop the move
		owner->StopMove(MOVE_STATUS_DEST_UNREACHABLE);

		// Restore the movestate we had before starting this task
		owner->PopMove();

	}
}

bool HandleElevatorTask::MoveToPositionEntity(idAI* owner, CMultiStateMoverPosition* pos)
{
	if (!owner->MoveToPosition(pos->GetPhysics()->GetOrigin()))
	{
		// Position entity cannot be reached, probably due to elevator not being there, use the button entity
		CMultiStateMoverButton* button = pos->GetFetchButton();
		if (button != NULL)
		{
			return MoveToButton(owner, button);
		}

		return false;
	}

	return true;
}

bool HandleElevatorTask::MoveToButton(idAI* owner, CMultiStateMoverButton* button)
{
	idBounds bounds = owner->GetPhysics()->GetBounds();
	float size = idMath::Fabs(bounds[0][1]);

	// in front of the button 
	// assuming that the button translates in when pressed
	idVec3 trans = button->spawnArgs.GetVector("translation", "0 2 0");
	trans.z = 0;
	if (trans.NormalizeFast() == 0)
	{
		trans = idVec3(1, 0, 0);
	}

	const idVec3& buttonOrigin = button->GetPhysics()->GetOrigin();

	// angua: target position is in front of the button with a distance 
	// a little bit larger than the AI's bounding box
	idVec3 target = buttonOrigin - size * 1.2f * trans;

	const idVec3& gravity = owner->GetPhysics()->GetGravityNormal();

	if (!owner->MoveToPosition(target))
	{
		// not reachable, try alternate target positions at the sides and behind the button
		trans = trans.Cross(gravity);
		target = buttonOrigin - size * 1.2f * trans;
		if (!owner->MoveToPosition(target))
		{
			trans *= -1;
			target = buttonOrigin - size * 1.2f * trans;
			if (!owner->MoveToPosition(target))
			{
				trans = trans.Cross(gravity);
				target = buttonOrigin - size * 1.2f * trans;
				if (!owner->MoveToPosition(target))
				{
					owner->StopMove(MOVE_STATUS_DEST_UNREACHABLE);
					return false;
				}
			}
		}
	}
	return true;
}

void HandleElevatorTask::DebugDraw(idAI* owner) 
{
	// Draw current state
	idMat3 viewMatrix = gameLocal.GetLocalPlayer()->viewAngles.ToMat3();

	idStr str;
	switch (_state)
	{
		case EMovingTowardsStation:
			str = "MovingTowardsStation";
			break;
		case EInitiateMoveToFetchButton:
			str = "InitiateMoveToFetchButton";
			break;
		case EMovingToFetchButton:
			str = "MovingToFetchButton";
			break;
		case EPressFetchButton:
			str = "PressButton";
			break;
		case EWaitForElevator:
			str = "WaitForElevator";
			break;
		case EMoveOntoElevator:
			str = "MoveOntoElevator";
			break;
		case EInitiateMoveToRideButton:
			str = "InitiateMoveToRideButton";
			break;
		case EMovingToRideButton:
			str = "MovingToRideButton";
			break;
		case EPressRideButton:
			str = "PressRideButton";
			break;
		case ERideOnElevator:
			str = "RideOnElevator";
			break;
		case EGetOffElevator:
			str = "EGetOffElevator";
			break;
			
		default:
			break;
	}

	gameRenderWorld->DrawText(str.c_str(), 
		(owner->GetEyePosition() - owner->GetPhysics()->GetGravityNormal()*60.0f), 
		0.25f, colorYellow, viewMatrix, 1, 4 * gameLocal.msec);
}

void HandleElevatorTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);
	_routeInfo.Save(savefile);

	savefile->WriteInt(static_cast<int>(_state));
	savefile->WriteInt(_waitEndTime);

	savefile->WriteBool(_success);
}

void HandleElevatorTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);
	_routeInfo.Restore(savefile);

	int temp;
	savefile->ReadInt(temp);
	_state = static_cast<State>(temp);
	savefile->ReadInt(_waitEndTime);

	savefile->ReadBool(_success);
}

HandleElevatorTaskPtr HandleElevatorTask::CreateInstance()
{
	return HandleElevatorTaskPtr(new HandleElevatorTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar handleElevatorTaskRegistrar(
	TASK_HANDLE_ELEVATOR, // Task Name
	TaskLibrary::CreateInstanceFunc(&HandleElevatorTask::CreateInstance) // Instance creation callback
);

} // namespace ai
