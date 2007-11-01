/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: angua $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: RepeatedBarkTask.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "RepeatedBarkTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

// Default constructor
RepeatedBarkTask :: RepeatedBarkTask() :
	_soundName(""), 
	_barkRepeatIntervalMin(0), 
	_barkRepeatIntervalMax(0)
{}

// Constructor with iunput variables
RepeatedBarkTask :: RepeatedBarkTask(const char* soundName, const int barkRepeatIntervalMin, const int barkRepeatIntervalMax) : 
	_soundName(soundName), 
	_barkRepeatIntervalMin(barkRepeatIntervalMin), 
	_barkRepeatIntervalMax(barkRepeatIntervalMax)
{}


// Get the name of this task
const idStr& RepeatedBarkTask::GetName() const
{
	static idStr _name(TASK_REPEATED_BARK);
	return _name;
}

void RepeatedBarkTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);

	// Initialise it to play the sound now
	_nextBarkTime = gameLocal.time;
}

bool RepeatedBarkTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("Repeated Bark Task performing.\r");

	idAI* owner = _owner.GetEntity();

	// This task may not be performed with empty entity pointers
	assert(owner != NULL);

	if (gameLocal.time >= _nextBarkTime)
	{
		// The time has come, bark now
		int duration = owner->PlayAndLipSync(_soundName, "talk1");

		// Reset the timer
		if (_barkRepeatIntervalMax > 0)
		{
			_nextBarkTime = gameLocal.time + duration + _barkRepeatIntervalMin + 
				gameLocal.random.RandomFloat() * (_barkRepeatIntervalMax - _barkRepeatIntervalMin);
		}
		else
		{
			_nextBarkTime = gameLocal.time + duration + _barkRepeatIntervalMin;
		}
	}

	return false; // not finished yet
}

// Save/Restore methods
void RepeatedBarkTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);
	savefile->WriteString(_soundName.c_str());
	savefile->WriteInt(_barkRepeatIntervalMin);
	savefile->WriteInt(_barkRepeatIntervalMax);
	savefile->WriteInt(_nextBarkTime);
}

void RepeatedBarkTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);
	savefile->ReadString(_soundName);
	savefile->ReadInt(_barkRepeatIntervalMin);
	savefile->ReadInt(_barkRepeatIntervalMax);
	savefile->ReadInt(_nextBarkTime);
}

RepeatedBarkTaskPtr RepeatedBarkTask::CreateInstance()
{
	return RepeatedBarkTaskPtr(new RepeatedBarkTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar repeatedBarkTaskRegistrar(
	TASK_REPEATED_BARK, // Task Name
	TaskLibrary::CreateInstanceFunc(&RepeatedBarkTask::CreateInstance) // Instance creation callback
);

} // namespace ai
