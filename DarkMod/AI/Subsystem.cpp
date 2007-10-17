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

static bool init_version = FileVersionList("$Id: Subsystem.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "Subsystem.h"
#include "Tasks/TaskLibrary.h"
#include "Tasks/EmptyTask.h"

namespace ai
{

Subsystem::Subsystem(idAI* owner) :
	_enabled(true)
{
	assert(owner != NULL);
	_owner = owner;

	InstallTask(
		TaskLibrary::Instance().CreateTask(TASK_EMPTY)
	);
}

void Subsystem::Enable()
{
	_enabled = true;
}

void Subsystem::Disable()
{
	_enabled = false;
}

bool Subsystem::IsEnabled() const
{
	return _enabled;
}

// Called regularly by the Mind to run the currently assigned routine.
void Subsystem::PerformTask()
{
	if (_enabled)
	{
		assert(_task != NULL);

		_task->Perform();
	}
}

void Subsystem::InstallTask(const TaskPtr& newTask)
{
	// Don't accept NULL tasks, use the EmptyTask class instead
	assert(newTask != NULL);

	// Install the new task, this may trigger a shared_ptr destruction of the old task
	_task = newTask;
}

// Save/Restore methods
void Subsystem::Save(idSaveGame* savefile) const
{
	_owner.Save(savefile);
}

void Subsystem::Restore(idRestoreGame* savefile)
{
	_owner.Restore(savefile);

	// Restore the task
	// Get the name of the installed task

	// Which task to allocate TaskPtr(new ???)
}

} // namespace ai
