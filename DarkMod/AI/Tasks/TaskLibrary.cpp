/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: TaskLibrary.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "TaskLibrary.h"

namespace ai
{

// The private constructor
TaskLibrary::TaskLibrary()
{}

TaskPtr TaskLibrary::CreateTask(const std::string& taskName)
{
	TaskPtr returnValue; // NULL by default

	// Try to lookup the task in the map
	TaskMap::iterator i = _tasks.find(taskName);

	if (i != _tasks.end())
	{
		CreateInstanceFunc& createInstance(i->second);

		// Invoke the boost::function to gather the TaskPtr
		returnValue = createInstance();
	}

	// Check if the task could be found
	if (returnValue == NULL)
	{
		gameLocal.Error("Cannot allocate Task instance for name %s", taskName.c_str());
	}

	return returnValue;
}

void TaskLibrary::RegisterTask(const std::string& taskName, const CreateInstanceFunc& func)
{
	// Insert this task into the map
	_tasks.insert(
		TaskMap::value_type(taskName, func)
	);
}

// Accessor method for this singleton
TaskLibrary& TaskLibrary::Instance()
{
	static TaskLibrary _instance;
	return _instance;
}

} // namespace ai
