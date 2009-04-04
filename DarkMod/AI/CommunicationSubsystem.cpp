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



	return true;
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
