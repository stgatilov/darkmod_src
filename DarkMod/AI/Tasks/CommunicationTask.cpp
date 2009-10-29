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

		// Emit a warning if priority not found, but only if sound name is empty
		if (!dict->GetInt(prioName, "-1", _priority) && !soundName.IsEmpty())
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
