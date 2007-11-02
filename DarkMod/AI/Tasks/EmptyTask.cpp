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

static bool init_version = FileVersionList("$Id: EmptyTask.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "EmptyTask.h"
#include "../Library.h"

namespace ai
{

// Get the name of this task
const idStr& EmptyTask::GetName() const
{
	static idStr _name(TASK_EMPTY);
	return _name;
}

// Performs nothing
bool EmptyTask::Perform(Subsystem& subsystem)
{
	//DM_LOG(LC_AI, LT_INFO).LogString("Empty Task performing.\r");
	return false; // not finished yet
}

EmptyTaskPtr EmptyTask::CreateInstance()
{
	return EmptyTaskPtr(new EmptyTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar emptyTaskRegistrar(
	TASK_EMPTY, // Task Name
	TaskLibrary::CreateInstanceFunc(&EmptyTask::CreateInstance) // Instance creation callback
);

} // namespace ai
