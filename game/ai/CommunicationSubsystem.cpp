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

#include "precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include "CommunicationSubsystem.h"
#include "Library.h"
#include "States/State.h"
#include "Tasks/CommunicationTask.h"
#include "Tasks/CommWaitTask.h"

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

	assert(communicationTask != NULL);

	CommunicationTaskPtr curCommTask = GetCurrentCommTask();

	// Check if we have a specific action to take when the new sound is conflicting
	EActionTypeOnConflict actionType = GetActionTypeForSound(communicationTask);

	switch (actionType)
	{
		case EDefault:
		{
			// Consider the priorities
			int priority = communicationTask->GetPriority();
			int currentPriority = GetCurrentPriority();

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
					return true;
				}

				// If the current bark is playing at the moment, discard the new one
				return false;
			}
			else // priority is lower than the current one
			{
				QueueTask(communicationTask);
				return true;
			}
		}
		case EOverride:	
			ClearTasks();
			PushTask(communicationTask);
			return true;
		case EQueue:
			QueueTask(communicationTask);
			return true;
		case EDiscard:	
			// Do nothing
			return false;
		case EPush:	
			PushTask(communicationTask);
			return true;
	};

	return false;
}

void CommunicationSubsystem::AddSilence(int duration)
{
	int priorityOfLastTask = 0;
	
	if (!_taskQueue.empty())
	{
		CommunicationTaskPtr lastCommTask = 
			boost::dynamic_pointer_cast<CommunicationTask>(_taskQueue.back());

		if (lastCommTask != NULL)
		{
			priorityOfLastTask = lastCommTask->GetPriority();
		}
	}

	// Instantiate a new commwaitTask
	CommWaitTaskPtr commTask(new CommWaitTask(duration, priorityOfLastTask));

	// And queue it, it will carry the same priority as the last one
	QueueTask(commTask);
}

CommunicationSubsystem::EActionTypeOnConflict 
	CommunicationSubsystem::GetActionTypeForSound(const CommunicationTaskPtr& communicationTask)
{
	// Check if we have a specific action to take when the new sound has lower prio
	const idDict* dict = gameLocal.FindEntityDefDict(BARK_PRIORITY_DEF);

	if (dict == NULL) 
	{
		gameLocal.Warning("Cannot find bark priority entitydef %s", BARK_PRIORITY_DEF);
		return EDefault;
	}

	int priorityDifference = communicationTask->GetPriority() - GetCurrentPriority();

	// Change "snd_blah" to "prio_blah"
	idStr prioName = communicationTask->GetSoundName();
	prioName.StripLeadingOnce("snd_");
	prioName = "prio_" + prioName;

	const idKeyValue* kv = dict->FindKey(
		prioName + "_" + 
		(priorityDifference < 0 ? "onlower" : (priorityDifference == 0 ? "onequal" : "onhigher"))
	);

	if (kv == NULL) return EDefault;

	const idStr& actionStr = kv->GetValue();

	if (actionStr.IsEmpty()) return EDefault;
	
	switch (actionStr[0])
	{
		case 'd': return EDiscard; 
		case 'o': return EOverride;
		case 'p': return EPush;
		case 'q': return EQueue;
	}

	return EDefault;
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
	return (_enabled) ? GetCurrentTaskName() + " (" + idStr(static_cast<unsigned>(_taskQueue.size())) + ")" : "";
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
