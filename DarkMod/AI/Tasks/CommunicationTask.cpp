/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 3184 $
 * $Date: 2009-01-19 13:49:11 +0100 (Mo, 19 Jän 2009) $
 * $Author: angua $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: CommunicationTask.cpp 3184 2009-01-19 12:49:11Z angua $", init_version);

#include "../Memory.h"
#include "PatrolTask.h"
#include "CommunicationTask.h"
#include "../Library.h"

namespace ai
{

CommunicationTask::CommunicationTask()
{}

CommunicationTask::CommunicationTask(const idStr& soundName) : 
	_soundName(soundName)
{
	// Look up priority
	
}

void CommunicationTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);
}


int CommunicationTask::GetPriority()
{
	return _priority;
}

// Save/Restore methods
void CommunicationTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	savefile->WriteString(_soundName);
	savefile->WriteInt(_priority);
}

void CommunicationTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	savefile->ReadString(_soundName);
	savefile->ReadInt(_priority);
}


} // namespace ai
