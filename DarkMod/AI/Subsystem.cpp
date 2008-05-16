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

#include "Subsystem.h"
#include "Library.h"
#include "States/State.h"

namespace ai
{

Subsystem::Subsystem(SubsystemId subsystemId, idAI* owner) :
	_id(subsystemId),
	_initTask(false),
	_enabled(false)
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
	_recycleBin.clear();

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
		_recycleBin.push_back(_taskQueue.front());

		// Now remove the State from the queue
		_taskQueue.pop_front();
		
		// Call the OnFinish event of the task
		_recycleBin.back()->OnFinish(owner);

		// Issue the "TaskFinished" signal to the MindState
		owner->GetMind()->GetState()->OnSubsystemTaskFinished(owner, _id);
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
		_recycleBin.push_back(_taskQueue.front());
		_taskQueue.pop_front();

		// Call the OnFinish event of the task
		_recycleBin.back()->OnFinish(_owner.GetEntity());
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
		// Move all TaskPtrs from the queue into the bin (front to back)
		for (TaskQueue::iterator i = _taskQueue.begin(); i != _taskQueue.end(); i++)
		{
			// Call the OnFinish event of the task after adding it to the bin
			_recycleBin.push_back(*i);
			_recycleBin.back()->OnFinish(_owner.GetEntity());
		}
		
		// Remove ALL tasks from the main queue
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

	// Save the dying tasks too
	_recycleBin.Save(savefile);
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
	_recycleBin.Restore(savefile);
}

} // namespace ai
