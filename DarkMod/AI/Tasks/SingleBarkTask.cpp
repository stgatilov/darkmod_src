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

#include "SingleBarkTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

SingleBarkTask::SingleBarkTask() :
	CommunicationTask("")
{}

SingleBarkTask::SingleBarkTask(const idStr& soundName, const CommMessagePtr& message) :
	CommunicationTask(soundName),
	_message(message)
{}

// Get the name of this task
const idStr& SingleBarkTask::GetName() const
{
	static idStr _name(TASK_SINGLE_BARK);
	return _name;
}

void SingleBarkTask::Init(idAI* owner, Subsystem& subsystem)
{
	// This task may not be performed with empty entity pointers
	assert(owner != NULL);

	if (!_soundName.IsEmpty())
	{
		// Push the message and play the sound
		if (_message != NULL)
		{
			owner->AddMessage(_message);
		}

		_barkLength = owner->PlayAndLipSync(_soundName, "talk1");

		_barkStartTime = gameLocal.time;

		_endTime = _barkStartTime + _barkLength;
	}
	else
	{
		_endTime = gameLocal.time;
	}

}

bool SingleBarkTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("SingleBarkTask performing.\r");

	idAI* owner = _owner.GetEntity();

	if (gameLocal.time >= _endTime)
	{
		return true; // finished!
	}
	return false;
}

void SingleBarkTask::SetSound(const idStr& soundName)
{
	_soundName = soundName;
}

// Save/Restore methods
void SingleBarkTask::Save(idSaveGame* savefile) const
{
	CommunicationTask::Save(savefile);

	savefile->WriteInt(_endTime);

	savefile->WriteBool(_message != NULL);
	if (_message != NULL)
	{
		_message->Save(savefile);
	}
}

void SingleBarkTask::Restore(idRestoreGame* savefile)
{
	CommunicationTask::Restore(savefile);

	savefile->ReadInt(_endTime);

	bool hasMessage;
	savefile->ReadBool(hasMessage);
	if (hasMessage)
	{
		_message = CommMessagePtr(new CommMessage);
		_message->Restore(savefile);
	}
	else
	{
		_message = CommMessagePtr();
	}
}

SingleBarkTaskPtr SingleBarkTask::CreateInstance()
{
	return SingleBarkTaskPtr(new SingleBarkTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar singleBarkTaskRegistrar(
	TASK_SINGLE_BARK, // Task Name
	TaskLibrary::CreateInstanceFunc(&SingleBarkTask::CreateInstance) // Instance creation callback
);

} // namespace ai
