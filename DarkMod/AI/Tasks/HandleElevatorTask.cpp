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
#include "HandleElevatorTask.h"
#include "../../MultiStateMoverButton.h"
#include "../EAS/EAS.h"

namespace ai
{

HandleElevatorTask::HandleElevatorTask() :
	_success(false),
	_waitEndTime(0)
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

	Memory& memory = owner->GetMemory();
	CMultiStateMoverPosition* pos = stationInfo->elevatorPosition.GetEntity();

	// Is the elevator station reachable?
	if (!IsElevatorStationReachable(pos))
	{
		subsystem.FinishTask();
		return;
	}

	owner->m_HandlingElevator = true;

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
}

bool HandleElevatorTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("HandleElevatorTask performing.\r");

	idAI* owner = _owner.GetEntity();
	Memory& memory = owner->GetMemory();

	// Grab the first RouteNode
	const eas::RouteNodePtr& node = *_routeInfo.routeNodes.begin();

	// Retrieve the elevator station info from the EAS
	eas::ElevatorStationInfoPtr stationInfo = 
		owner->GetAAS()->GetEAS()->GetElevatorStationInfo(node->elevatorStation);

	CMultiStateMoverPosition* pos = stationInfo->elevatorPosition.GetEntity();
	CMultiStateMover* elevator = stationInfo->elevator.GetEntity();

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
					// TODO: set elevator user
				}
				else
				{
					// elevator is somewhere else
					// check if the elevator is already in use
/*
					idActor* user = elevator->GetUser();
					if (user != NULL)
					{
						// elevator is in use

					}
					else
					{
*/
					_state = EInitiateMoveToFetchButton;
//					}

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
			if (dist < owner->GetArmReachLength()+10)
			{
				owner->StopMove(MOVE_STATUS_DONE);
				owner->TurnToward(fetchButton->GetPhysics()->GetOrigin());
				owner->Event_LookAtPosition(fetchButton->GetPhysics()->GetOrigin(), 1);
				owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Use_righthand", 4);
				_state = EPressFetchButton;
				_waitEndTime = gameLocal.time + 400;
			}
			else if (owner->GetMoveStatus() == MOVE_STATUS_DEST_UNREACHABLE)
			{
				// Destination unreachable, help!
				return true;
			}
			// TODO: set elevator user
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
			else if (elevator->IsAtRest())
			{
				// Elevator has stopped moving and is not at our station, press the fetch button!
				_state = EInitiateMoveToFetchButton;
			}
			// TODO: check if the elevator is moving towards our station
			break;

		case EMoveOntoElevator:
		{
			if (owner->GetMoveStatus() == MOVE_STATUS_DONE)
			{
				// We're done moving onto the platform
				_state = EInitiateMoveToRideButton;
			}
			else if (!elevator->IsAtPosition(pos))
			{
				// elevator moved away while we attempted to move onto it
				owner->StopMove(MOVE_STATUS_WAITING);
				// TODO: need to check whether we are already on the elevator
				if (0)
				{
					_state = ERideOnElevator;
				}
				else
				{
/*
					idActor* user = elevator->GetUser();
					if (user != NULL)
					{
						// elevator is in use

					}
					else
					{
*/
						// elevator is not in use, press button again
						_state = EInitiateMoveToFetchButton;
//					}
	

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

			MoveToButton(owner, rideButton);
			_state = EMovingToRideButton;
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

			dir = owner->GetPhysics()->GetOrigin() - rideButton->GetPhysics()->GetOrigin();
			dir.z = 0;
			dist = dir.LengthFast();
			if (dist < owner->GetArmReachLength() + 20)
			{
				owner->StopMove(MOVE_STATUS_DONE);
				owner->TurnToward(rideButton->GetPhysics()->GetOrigin());
				owner->Event_LookAtPosition(rideButton->GetPhysics()->GetOrigin(), 1);
				owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Use_righthand", 4);
				_state = EPressRideButton;
				_waitEndTime = gameLocal.time + 400;
			}
		}
		break;

		case EPressRideButton:
		{
			if (gameLocal.time >= _waitEndTime)
			{
				CMultiStateMoverButton* rideButton = pos->GetRideButton(targetPos);
				if (rideButton != NULL)
				{
					// Press button and wait for elevator
					rideButton->Operate();
					_state = ERideOnElevator;
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
				return true;

			}
			else if (elevator->IsAtRest())
			{
				// elevator stopped at a different position
				// restart our pathing from here
				_success = true;
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
	Memory& memory = owner->GetMemory();

	owner->m_HandlingElevator = false;

	// Restore the movestate we had before starting this task
	owner->PopMove();

	if (!_success)
	{
		// This task could not finish successfully, stop the move
		owner->StopMove(MOVE_STATUS_DEST_UNREACHABLE);
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

	idVec3 trans = button->spawnArgs.GetVector("translation", "0 2 0");
	trans.z = 0;
	if (trans.NormalizeFast() == 0)
	{
		trans = idVec3(1, 0, 0);
	}
	idVec3 target = button->GetPhysics()->GetOrigin() - size * 1.2f * trans;

	const idVec3& gravity = gameLocal.GetGravity();
	const idVec3& buttonOrigin = button->GetPhysics()->GetOrigin();

	if (!owner->MoveToPosition(target))
	{
		trans = trans.Cross(gravity);
		idVec3 target = buttonOrigin - size * 1.2f * trans;
		if (!owner->MoveToPosition(target))
		{
			trans *= -1;
			idVec3 target = buttonOrigin - size * 1.2f * trans;
			if (!owner->MoveToPosition(target))
			{
				trans = trans.Cross(gravity);
				idVec3 target = buttonOrigin - size * 1.2f * trans;
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
