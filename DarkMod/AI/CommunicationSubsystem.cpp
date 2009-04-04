/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 2822 $
 * $Date: 2008-09-13 06:50:55 +0200 (Sa, 13 Sep 2008) $
 * $Author: greebo $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: Subsystem.cpp 2822 2008-09-13 04:50:55Z greebo $", init_version);

#include "CommunicationSubsystem.h"
#include "Library.h"
#include "States/State.h"


namespace ai
{

CommunicationSubsystem::CommunicationSubsystem(SubsystemId subsystemId, idAI* owner) :
	Subsystem(subsystemId, owner)
{}


bool CommunicationSubsystem::AddCommTask(const CommunicationTaskPtr& communicationTask)
{
	if (IsEmpty())
	{
		PushTask(communicationTask);
		return true;
	}
	

	int priority = communicationTask->GetPriority();
	int currentPriority = GetCurrentPriority();

	CommunicationTaskPtr curCommTask = GetCurrentCommTask();

	if (priority > currentPriority)
	{
		// The new bark has higher priority, clear all current bark tasks and start the new one
		ClearTasks();
		PushTask(communicationTask);
		return true;
	}

	else if (priority == currentPriority)
	{
		// the new bark has the same priority as the old one
		if (curCommTask != NULL && !curCommTask->IsBarking())
		{
			// If the current bark is not playing at the moment, switch to the new one
			SwitchTask(communicationTask);
		}
		// If the current bark is playing at the moment, discard the new one

		// TODO: clear all tasks with lower priority than this from the stack
	}
	else
	{
		// The new bark has lower priority than the current one, queue it after the current bark(s)
		QueueTask(communicationTask);

		//TODO: some barks might not want to be queued but discarded
	}

	return false;
}

CommunicationTaskPtr CommunicationSubsystem::GetCurrentCommTask()
{
	TaskPtr curTask = GetCurrentTask();

	return curTask ? boost::dynamic_pointer_cast<CommunicationTask>(curTask) : CommunicationTaskPtr();
}

int CommunicationSubsystem::GetCurrentPriority()
{
	CommunicationTaskPtr commTask = GetCurrentCommTask();

	return (commTask != NULL) ? commTask->GetPriority() : -1;
}

idStr CommunicationSubsystem::GetDebugInfo()
{
	return (_enabled) ? GetCurrentTaskName() + " (" + idStr(_taskQueue.size()) + ")" : "";
}

// Save/Restore methods
void CommunicationSubsystem::Save(idSaveGame* savefile) const
{
	Subsystem::Save(savefile);
}

void CommunicationSubsystem::Restore(idRestoreGame* savefile)
{
	Subsystem::Restore(savefile);
}






} // namespace ai
