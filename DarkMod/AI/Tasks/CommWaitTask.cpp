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

#include "CommWaitTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

CommWaitTask::CommWaitTask() :
	CommunicationTask(""),
	_endTime(-1)
{
	_priority = VERY_HIGH_BARK_PRIORITY;
}

CommWaitTask::CommWaitTask(int duration) :
	CommunicationTask(""),
	_endTime(gameLocal.time + duration)
{
	_priority = VERY_HIGH_BARK_PRIORITY;
}

// Get the name of this task
const idStr& CommWaitTask::GetName() const
{
	static idStr _name(TASK_COMM_WAIT);
	return _name;
}

bool CommWaitTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("CommWaitTask performing.\r");

	return (gameLocal.time >= _endTime);
}

// Save/Restore methods
void CommWaitTask::Save(idSaveGame* savefile) const
{
	CommunicationTask::Save(savefile);

	savefile->WriteInt(_endTime);
}

void CommWaitTask::Restore(idRestoreGame* savefile)
{
	CommunicationTask::Restore(savefile);

	savefile->ReadInt(_endTime);
}

CommWaitTaskPtr CommWaitTask::CreateInstance()
{
	return CommWaitTaskPtr(new CommWaitTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar commWaitTaskRegistrar(
	TASK_COMM_WAIT, // Task Name
	TaskLibrary::CreateInstanceFunc(&CommWaitTask::CreateInstance) // Instance creation callback
);

} // namespace ai
