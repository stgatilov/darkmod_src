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
	CommunicationTask(""),
	_startDelay(0),
	_endTime(-1)
{}

SingleBarkTask::SingleBarkTask(const idStr& soundName, 
							   const CommMessagePtr& message, 
							   int startDelay)
:	CommunicationTask(soundName),
	_message(message),
	_startDelay(startDelay),
	_endTime(-1)
{}

// Get the name of this task
const idStr& SingleBarkTask::GetName() const
{
	static idStr _name(TASK_SINGLE_BARK);
	return _name;
}

void SingleBarkTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Init the base class
	CommunicationTask::Init(owner, subsystem);

	// End time is -1 until the bark has been emitted
	_endTime = -1;

	// Set up the start time
	_barkStartTime = gameLocal.time + _startDelay;
}

bool SingleBarkTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("SingleBarkTask performing.\r");

	if (gameLocal.time < _barkStartTime)
	{
		return false; // waiting for start delay to pass
	}

	// If an endtime has been set, the bark is already playing
	if (_endTime > 0)
	{
		// Finish the task when the time is over
		return (gameLocal.time >= _endTime);
	}
	
	// No end time set yet, emit our bark

	if (_soundName.IsEmpty())
	{
		DM_LOG(LC_AI, LT_ERROR)LOGSTRING("SingleBarkTask has empty soundname, ending task.\r");
		return true;
	}

	// This task may not be performed with empty entity pointers
	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	// Push the message and play the sound
	if (_message != NULL)
	{
		owner->AddMessage(_message);
	}

	_barkLength = owner->PlayAndLipSync(_soundName, "talk1");
	_barkStartTime = gameLocal.time;

	_endTime = _barkStartTime + _barkLength;

	// Sanity check the returned length
	if (_barkLength == 0)
	{
		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Received 0 sound length when playing %s.\r", _soundName.c_str());
	}
	
	// End the task as soon as we've finished playing the sound
	return !IsBarking();
}

void SingleBarkTask::SetSound(const idStr& soundName)
{
	_soundName = soundName;
}

void SingleBarkTask::SetMessage(const CommMessagePtr& message)
{
	_message = message;
}

// Save/Restore methods
void SingleBarkTask::Save(idSaveGame* savefile) const
{
	CommunicationTask::Save(savefile);

	savefile->WriteInt(_startDelay);
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

	savefile->ReadInt(_startDelay);
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
