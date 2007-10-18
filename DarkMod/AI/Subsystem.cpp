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
	_enabled(true),
	_finishCurrentTask(false)
{
	assert(owner != NULL);
	_owner = owner;

	// We start with an empty task
	_taskQueue.push_front(EmptyTask::CreateInstance());
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
		// Check if we don't have a task or the current one should be deleted
		if (_finishCurrentTask || _task == NULL)
		{
			// Reset the flag in any case
			_finishCurrentTask = false;

			// Clear out the current task
			_task = TaskPtr();

			if (_taskQueue.size() > 0)
			{
				// Pop the foremost element from the list
				_task = *_taskQueue.begin();
				_taskQueue.pop_front();

				// Initialise the new task with a reference to the owner and <self>
				_task->Init(_owner.GetEntity(), *this);

				_enabled = true;
			}
			else
			{
				// No more tasks, disable this Subsystem
				DM_LOG(LC_AI, LT_INFO).LogString("No more tasks, disabling subsystem.\r");
				_enabled = false;
				return;
			}
		}

		// No NULL pointer past this point
		assert(_task != NULL);

		// If the task returns TRUE, it will be removed next round
		if (_task->Perform(*this))
		{
			FinishCurrentTask();
		}
	}
}

void Subsystem::QueueTask(const TaskPtr& nextTask)
{
	assert(nextTask != NULL);
	// Take this pointer and add it to the stack
	_taskQueue.push_back(nextTask);
	_enabled = true;
}

void Subsystem::FinishCurrentTask()
{
	// Set the flag to true, this ends the current task in the next PerformTask() call.
	_finishCurrentTask = true;
}

void Subsystem::ClearTasks()
{
	_taskQueue.clear();
	_task = TaskPtr();
	_enabled = false;
}

// Save/Restore methods
void Subsystem::Save(idSaveGame* savefile) const
{
	_owner.Save(savefile);

	// TODO
	/*// Save the task, if there is an active one
	savefile->WriteBool(_task != NULL);
	if (_task != NULL)
	{
		savefile->WriteString(_task->GetName().c_str());
		_task->Save(savefile);
	}*/
}

void Subsystem::Restore(idRestoreGame* savefile)
{
	_owner.Restore(savefile);

	// TODO
	/*bool hasTask;
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
	}*/
}

} // namespace ai
