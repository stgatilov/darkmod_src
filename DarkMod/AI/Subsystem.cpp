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
#include "Library.h"
#include "Tasks/EmptyTask.h"

namespace ai
{

Subsystem::Subsystem(idAI* owner) :
	_enabled(true)
{
	assert(owner != NULL);
	_owner = owner;

	// We start with an empty task
	InstallTask(EmptyTask::CreateInstance());
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

	// Save the task, if there is an active one
	savefile->WriteBool(_task != NULL);
	if (_task != NULL)
	{
		savefile->WriteString(_task->GetName().c_str());
		_task->Save(savefile);
	}
}

void Subsystem::Restore(idRestoreGame* savefile)
{
	_owner.Restore(savefile);

	bool hasTask;
	savefile->ReadBool(hasTask);

	if (hasTask)
	{
		idStr taskName;
		savefile->ReadString(taskName);

		_task = TaskLibrary::Instance().CreateInstance(taskName.c_str());

		assert(_task != NULL);
		_task->Restore(savefile);
	}
	else
	{
		// Assure the task pointer to be NULL.
		_task = TaskPtr();
	}
}

} // namespace ai
