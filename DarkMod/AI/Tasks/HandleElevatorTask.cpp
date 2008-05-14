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
		/*case EStateMovingToFrontPos:
			str = "EStateMovingToFrontPos";
			break;
		case EStateWaitBeforeOpen:
			str = "EStateWaitBeforeOpen";
			break;
		case EStateStartOpen:
			str = "EStateStartOpen";
			break;*/
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
}

void HandleElevatorTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);
	_pos.Restore(savefile);

	int temp;
	savefile->ReadInt(temp);
	_state = static_cast<State>(temp);
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
