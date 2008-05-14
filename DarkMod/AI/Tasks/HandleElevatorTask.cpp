/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2008-05-13 18:53:28 +0200 (Di, 13 May 2008) $
 * $Author: greebo $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: HandleElevatorTask.cpp 1435 2008-05-13 16:53:28Z greebo $", init_version);

#include "../Memory.h"
#include "HandleElevatorTask.h"
#include "../../FrobButton.h"


namespace ai
{

HandleElevatorTask::HandleElevatorTask()
{}

HandleElevatorTask::HandleElevatorTask(CMultiStateMoverPosition* pos)
{
	_pos = pos;
}


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

	Memory& memory = owner->GetMemory();
	CMultiStateMoverPosition* pos = _pos.GetEntity();

	owner->PushMove(); // Save the move

	// Is the elevator station reachable?
	if (!IsElevatorStationReachable(pos))
	{
		subsystem.FinishTask();
		return;
	}

	// Start moving towards the elevator station
	owner->MoveToPosition(pos->GetPhysics()->GetOrigin());
	_state = EMovingTowardsStation;
}

bool HandleElevatorTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("HandleElevatorTask performing.\r");

	idAI* owner = _owner.GetEntity();
	Memory& memory = owner->GetMemory();

	CMultiStateMoverPosition* pos = _pos.GetEntity();
	CMultiStateMover* elevator; // = pos->GetElevator();
	CFrobButton* button; // = pos->GetButton();

	idVec3 dir;
	float dist;


	switch (_state)
	{
		case EMovingTowardsStation:
			dist = (owner->GetPhysics()->GetOrigin() - pos->GetPhysics()->GetOrigin()).LengthFast();
			if (dist < 500)
//				&&	(owner->CanSeeExt(pos, true, false) 
//					|| owner->CanSeeExt(elevator, true, false)))
			{
/*
				if (elevator->IsAtPosition(pos))
				{
					// elevator is at the desired position, get onto it
					// TODO: set elevator user
					// owner->MoveToPosition();
					_state = EStateMoveOntoElevator;
				}
				else
				{
					// elevator is somewhere else
					// check if the elevator is already in use
					idActor* user = elevator->GetUser();
					if (user != NULL)
					{
						// elevator is in use

					}
					else
					{
						// it's not occupied, get to the button
						idVec3 buttonPos = pos->GetButtonPos();
						owner->MoveToPosition(buttonPos);
						_state = EStateMovingToButton;
					}
				}
*/
			}	
			break;

		case EStateMovingToButton:
			dir = owner->GetPhysics()->GetOrigin() - button->GetPhysics()->GetOrigin();
			dir.z = 0;
			dist = dir.LengthFast();
			if (dist < owner->GetArmReachLength())
			{
				owner->StopMove(MOVE_STATUS_DONE);
				owner->TurnToward(button->GetPhysics()->GetOrigin());
				owner->Event_LookAtPosition(button->GetPhysics()->GetOrigin(), 1);
				owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Use_righthand", 4);
				_state = EStatePressButton;
				_waitEndTime = gameLocal.time + 400;
			}
			// TODO: set elevator user
			break;

		case EStatePressButton:
			if (gameLocal.time >= _waitEndTime)
			{
				// Press button and wait for elevator
				button->Operate();
				_state = EStateWaitForElevator;
			}
			break;

		case EStateWaitForElevator:
/*
			if (elevator->IsAtPosition(pos))
			{
				// elevator is at the desired position, get onto it
				owner->MoveToPosition();
				_state = EStateMoveOntoElevator;
			}
*/
			break;

		case EStateMoveOntoElevator:

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

	// Restore the movestate we had before starting this task
	owner->PopMove();
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
		case EStateMovingToButton:
			str = "EStateMovingToButton";
			break;
		case EStatePressButton:
			str = "EStatePressButton";
			break;
		case EStateWaitForElevator:
			str = "EStateWaitForElevator";
			break;
		case EStateMoveOntoElevator:
			str = "EStateMoveOntoElevator";
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
	_pos.Save(savefile);

	savefile->WriteInt(static_cast<int>(_state));
	savefile->WriteInt(_waitEndTime);
}

void HandleElevatorTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);
	_pos.Restore(savefile);

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
