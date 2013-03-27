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

#include "precompiled_game.h"
#pragma hdrstop

static bool versioned = RegisterVersionedFile("$Id$");

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
	
	if (_soundName.IsEmpty())
	{
		DM_LOG(LC_AI, LT_ERROR)LOGSTRING("SingleBarkTask has empty soundname, ending task.\r");
		return true;
	}

	// This task may not be performed with empty entity pointers
	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	// No end time set yet, emit our bark

	// grayman #2169 - no barks while underwater

	_barkLength = 0;
	if (!owner->MouthIsUnderwater())
	{
		int msgTag = 0; // grayman #3355
		// Push the message and play the sound
		if (_message != NULL)
		{
			// Setup the message to be propagated, if we have one
			msgTag = gameLocal.GetNextMessageTag(); // grayman #3355
			owner->AddMessage(_message,msgTag);
		}

		_barkLength = owner->PlayAndLipSync(_soundName, "talk1", msgTag); // grayman #3355
		
		// Sanity check the returned length
		if (_barkLength == 0)
		{
			DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Received 0 sound length when playing %s.\r", _soundName.c_str());
		}
	}

	_barkStartTime = gameLocal.time;
	_endTime = _barkStartTime + _barkLength;

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
