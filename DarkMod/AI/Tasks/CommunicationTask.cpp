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
	const idDict* dict = gameLocal.FindEntityDefDict(BARK_PRIORITY_DEF);

	if (dict != NULL)
	{
		// Change "snd_blah" to "prio_blah"
		idStr prioName(soundName);
		prioName.StripLeadingOnce("snd_");
		prioName = "prio_" + prioName;

		if (!dict->GetInt(prioName, "-1", _priority))
		{
			gameLocal.Warning("Could not find bark priority for %s", soundName.c_str());
		}
	}
	else
	{
		gameLocal.Warning("Cannot find bark priority entitydef %s", BARK_PRIORITY_DEF);
		_priority = -1;
	}
}

void CommunicationTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);

	_barkStartTime = gameLocal.time;
	_barkLength = 0;
}

int CommunicationTask::GetPriority()
{
	return _priority;
}

void CommunicationTask::SetPriority(int priority)
{
	_priority = priority;
}

bool CommunicationTask::IsBarking()
{
	return (gameLocal.time < _barkStartTime + _barkLength);
}

const idStr& CommunicationTask::GetSoundName()
{
	return _soundName;
}

// Save/Restore methods
void CommunicationTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	savefile->WriteString(_soundName);
	savefile->WriteInt(_priority);
	savefile->WriteInt(_barkStartTime);
	savefile->WriteInt(_barkLength);
}

void CommunicationTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	savefile->ReadString(_soundName);
	savefile->ReadInt(_priority);
	savefile->ReadInt(_barkStartTime);
	savefile->ReadInt(_barkLength);
}

} // namespace ai
