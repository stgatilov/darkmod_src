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
#include "States/State.h"
#include "Tasks/EmptyTask.h"

namespace ai
{

Subsystem::Subsystem(SubsystemId subsystemId, idAI* owner) :
	_id(subsystemId),
	_enabled(false),
	_initTask(false)
{
	assert(owner != NULL);
	_owner = owner;
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

idStr Subsystem::GetDebugInfo()
{
	return (_enabled) ? GetCurrentTaskName() + " (" + idStr(_taskQueue.size()) + ")" : "";
}

idStr Subsystem::GetCurrentTaskName() const
{
	return (_taskQueue.empty()) ? "" : _taskQueue.front()->GetName();
}

TaskPtr Subsystem::GetCurrentTask() const
{
	return (_taskQueue.empty()) ? TaskPtr() : _taskQueue.front();
}

// Called regularly by the Mind to run the currently assigned routine.
bool Subsystem::PerformTask()
{
	// Clear any dying tasks from the previous frame, enabled or not
	_recycleBin = TaskPtr();

	if (!_enabled || _taskQueue.empty())
	{
		// No tasks or disabled, return FALSE
		return false;
	}

	// Pick the foremost task from the queue
	const TaskPtr& task = _taskQueue.front();

	// No NULL pointers allowed
	assert(task != NULL);

	if (_initTask)
	{
		// New task, let's initialise it
		_initTask = false;

		// Initialise the newcomer
		task->Init(_owner.GetEntity(), *this);

		if (!_enabled || _initTask)
		{
			// Subsystem has been disabled by the Init() call OR
			// the Task has been changed (_initTask == true), quit
			return false;
		}
	}

	// greebo: If the task returns TRUE, it will be removed next round.
	// Only execute the task if the initTask is not set, this might indicate
	// a task switch invoked by the previous Init() call.
	// An uninitialised task must not be performed.
	// Also, the subsystem must be enabled, it might have been disabled in the Init call.
	if (task->Perform(*this))
	{
		FinishTask();
	}

	// task was performed, return true
	return true; 
}

void Subsystem::PushTask(const TaskPtr& newTask)
{
	assert(newTask != NULL);

	// Add the task to the front and initialise it next round
	_taskQueue.push_front(newTask);
	_initTask = true;
	_enabled = true;
}

bool Subsystem::FinishTask()
{
	if (!_taskQueue.empty())
	{
		idAI* owner = _owner.GetEntity();

		// Move the task pointer from the queue to the recyclebin
		_recycleBin = _taskQueue.front();
		_taskQueue.pop_front();

		// Call the OnFinish event of the task
		_recycleBin->OnFinish(owner);

		// Issue the "TaskFinished" signal to the MindState
		owner->GetMind()->GetState()->OnSubsystemTaskFinished(_id);
	}

	if (_taskQueue.empty())
	{
		// No more tasks, disable this subsystem
		_enabled = false;
		return false;
	}

	// More tasks to do, initialise the new front task
	_initTask = true;

	// More tasks to do, return TRUE
	return true;
}

void Subsystem::SwitchTask(const TaskPtr& newTask)
{
	assert(newTask != NULL);

	if (!_taskQueue.empty())
	{
		// Move the previous front task to the bin
		_recycleBin = _taskQueue.front();
		_taskQueue.pop_front();

		// Call the OnFinish event of the task
		_recycleBin->OnFinish(_owner.GetEntity());
	}

	// Add the new task to the front
	_taskQueue.push_front(newTask);

	_initTask = true;
	_enabled = true;
}

void Subsystem::QueueTask(const TaskPtr& task)
{
	assert(task != NULL);

	if (_taskQueue.empty())
	{
		// Queue is empty, this will be our primary task, let's initialise it
		_initTask = true;
	}

	// Add the task at the end of the queue
	_taskQueue.push_back(task);

	// Enable this subsystem, we have tasks to do
	_enabled = true;
}

void Subsystem::ClearTasks()
{
	if (!_taskQueue.empty())
	{
		// Move the TaskPtr from the queue into the bin
		_recycleBin = _taskQueue.front();

		// Call the OnFinish event of the task
		_recycleBin->OnFinish(_owner.GetEntity());

		// Remove ALL tasks
		_taskQueue.clear();
	}

	// Disable this subsystem
	_enabled = false;
}

// Save/Restore methods
void Subsystem::Save(idSaveGame* savefile) const
{
	savefile->WriteInt(static_cast<int>(_id));

	_owner.Save(savefile);

	savefile->WriteBool(_enabled);
	savefile->WriteBool(_initTask);

	_taskQueue.Save(savefile);

	// Save the dying task too
	savefile->WriteBool(_recycleBin != NULL);
	if (_recycleBin != NULL)
	{
		savefile->WriteString(_recycleBin->GetName().c_str());
		_recycleBin->Save(savefile);
	}
}

void Subsystem::Restore(idRestoreGame* savefile)
{
	int temp;
	savefile->ReadInt(temp);
	_id = static_cast<SubsystemId>(temp);

	_owner.Restore(savefile);

	savefile->ReadBool(_enabled);
	savefile->ReadBool(_initTask);

	_taskQueue.Restore(savefile);

	bool hasTask;
	savefile->ReadBool(hasTask);

	if (hasTask)
	{
		idStr taskName;
		savefile->ReadString(taskName);

		_recycleBin = TaskLibrary::Instance().CreateInstance(taskName.c_str());

		assert(_recycleBin != NULL);
		_recycleBin->Restore(savefile);
	}
	else
	{
		// Assure the recyclebin pointer to be NULL.
		_recycleBin = TaskPtr();
	}
}

} // namespace ai
