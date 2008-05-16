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

HandleElevatorTask::HandleElevatorTask()
{}

HandleElevatorTask::HandleElevatorTask(const eas::RouteInfoPtr& routeInfo) :
	_routeInfo(*routeInfo) // copy-construct the RouteInfo, creates a clean duplicate
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

	// Start moving towards the elevator station
	if (owner->MoveToPosition(pos->GetPhysics()->GetOrigin()))
	{
		// If AI_MOVE_DONE is true, we are already at the target position
		_state = (owner->AI_MOVE_DONE) ? EStateMoveOntoElevator : EMovingTowardsStation;
	}
	else
	{
		// Position entity cannot be reached, probably due to elevator not being there, use the button entity
		CMultiStateMoverButton* button = pos->GetFetchButton();
		if (button != NULL)
		{
			MoveToButton(owner, button);
			_state = EStateMovingToFetchButton;
		}
		else
		{
			subsystem.FinishTask();
		}
	}
}

bool HandleElevatorTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("HandleElevatorTask performing.\r");

	idAI* owner = _owner.GetEntity();
	Memory& memory = owner->GetMemory();

	// Grab the first RouteNode
	const eas::RouteNodePtr& node = *_routeInfo.routeNodes.begin();

	// Retrieve the elevator station info from the EAS
	eas::ElevatorStationInfoPtr stationInfo = 
		owner->GetAAS()->GetEAS()->GetElevatorStationInfo(node->elevatorStation);

	CMultiStateMoverPosition* pos = stationInfo->elevatorPosition.GetEntity();
	CMultiStateMover* elevator = stationInfo->elevator.GetEntity();
	CMultiStateMoverButton* fetchButton = pos->GetFetchButton();
	if (fetchButton == NULL)
	{
		owner->AI_DEST_UNREACHABLE = true;
		return true;
	}

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
			dist = (owner->GetPhysics()->GetOrigin() - pos->GetPhysics()->GetOrigin()).LengthFast();
			if (dist < 500
				&&	(owner->CanSeeExt(pos, true, false) 
					|| owner->CanSeeExt(elevator, true, false)))
			{

				if (elevator->IsAtPosition(pos))
				{
					// TODO: elevator is at the desired position, get onto it
					MoveToPositionEntity(owner, pos);
					_state = EStateMoveOntoElevator;
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
						// it's not occupied, get to the button
						MoveToButton(owner, fetchButton);
						_state = EStateMovingToFetchButton;
//					}

				}

			}	
			break;

		case EStateMovingToFetchButton:
			dir = owner->GetPhysics()->GetOrigin() - fetchButton->GetPhysics()->GetOrigin();
			dir.z = 0;
			dist = dir.LengthFast();
			if (dist < owner->GetArmReachLength()+20)
			{
				owner->StopMove(MOVE_STATUS_DONE);
				owner->TurnToward(fetchButton->GetPhysics()->GetOrigin());
				owner->Event_LookAtPosition(fetchButton->GetPhysics()->GetOrigin(), 1);
				owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Use_righthand", 4);
				_state = EStatePressFetchButton;
				_waitEndTime = gameLocal.time + 400;
			}
			// TODO: set elevator user
			break;

		case EStatePressFetchButton:
			if (gameLocal.time >= _waitEndTime)
			{
				// Press button and wait for elevator
				fetchButton->Operate();
				_state = EStateWaitForElevator;
			}
			break;

		case EStateWaitForElevator:
			if (elevator->IsAtPosition(pos))
			{
				// TODO elevator is at the desired position, get onto it
				owner->MoveToPosition(pos->GetPhysics()->GetOrigin());
				_state = EStateMoveOntoElevator;
			}
			// TODO: check if the elevator is moving towards our station
			break;

		case EStateMoveOntoElevator:
		{
			CMultiStateMoverButton* rideButton = pos->GetRideButton(targetPos);
			if (rideButton == NULL)
			{
				owner->AI_DEST_UNREACHABLE = true;
				return true;
			}

			// TODO: we're done moving onto it
			if (owner->AI_MOVE_DONE)
			{
				MoveToButton(owner, rideButton);
				_state = EStateMovingToRideButton;
			}
			else if (!elevator->IsAtPosition(pos))
			{
				// elevator moved away while we attempted to move onto it
				owner->StopMove(MOVE_STATUS_DONE);
				// TODO: need to check whether we are already on the elevator
				if (0)
				{
					_state = EStateWaitOnElevator;
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
						MoveToButton(owner, fetchButton);
						_state = EStateMovingToFetchButton;
//					}
	

				}

			}

		}
		break;

		case EStateMovingToRideButton:
		{
			CMultiStateMoverButton* rideButton = pos->GetRideButton(targetPos);
			if (rideButton == NULL)
			{
				owner->AI_DEST_UNREACHABLE = true;
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
				_state = EStatePressRideButton;
				_waitEndTime = gameLocal.time + 400;
			}
		}
		break;

		case EStatePressRideButton:
		{
			if (gameLocal.time >= _waitEndTime)
			{
				CMultiStateMoverButton* rideButton = pos->GetRideButton(targetPos);
				if (rideButton != NULL)
				{
					// Press button and wait for elevator
					rideButton->Operate();
					_state = EStateWaitOnElevator;
				}
				else
				{
					owner->AI_DEST_UNREACHABLE = true;
					return true;
				}
			}
		}
		break;

		case EStateWaitOnElevator:
			if (elevator->IsAtPosition(targetPos))
			{
				// reached target station, get off the elevator
				return true;

			}
			else if (elevator->IsAtRest())
			{
				// elevator stopped at a different position
				// restart our pathing from here
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
	float size = bounds[0][1];

	idVec3 trans = button->spawnArgs.GetVector("translation", "0 2 0");
	trans.z = 0;
	if (trans.NormalizeFast() == 0)
	{
		trans = idVec3(1, 0, 0);
	}
	idVec3 target = button->GetPhysics()->GetOrigin() - size * 1.2f * trans;

	if (!owner->MoveToPosition(target))
	{
		trans = trans.Cross(gameLocal.GetGravity());
		idVec3 target = button->GetPhysics()->GetOrigin() - size * 1.2f * trans;
		if (!owner->MoveToPosition(target))
		{
			trans *= -1;
			idVec3 target = button->GetPhysics()->GetOrigin() - size * 1.2f * trans;
			if (!owner->MoveToPosition(target))
			{
				trans = trans.Cross(gameLocal.GetGravity());
				idVec3 target = button->GetPhysics()->GetOrigin() - size * 1.2f * trans;
				if (!owner->MoveToPosition(target))
				{
					owner->AI_DEST_UNREACHABLE = true;
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
			str = "EMovingTowardsStation";
			break;
		case EStateMovingToFetchButton:
			str = "EStateMovingToButton";
			break;
		case EStatePressFetchButton:
			str = "EStatePressButton";
			break;
		case EStateWaitForElevator:
			str = "EStateWaitForElevator";
			break;
		case EStateMoveOntoElevator:
			str = "EStateMoveOntoElevator";
			break;
		case EStateMovingToRideButton:
			str = "EStateMovingToRideButton";
			break;
		case EStatePressRideButton:
			str = "EStatePressRideButton";
			break;
		case EStateWaitOnElevator:
			str = "EStateWaitOnElevator";
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
}

void HandleElevatorTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);
	_routeInfo.Restore(savefile);

	int temp;
	savefile->ReadInt(temp);
	_state = static_cast<State>(temp);
	savefile->ReadInt(_waitEndTime);

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
