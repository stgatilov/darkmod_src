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

bool Subsystem::IsEmpty() const
{
	return _taskQueue.empty();
}

idStr Subsystem::GetDebugInfo()
{
	return (_enabled) ? GetCurrentTaskName() + " (" + idStr(static_cast<unsigned>(_taskQueue.size())) + ")" : "";
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
		
		// Call the OnFinish event of the task, if appropriate
		const TaskPtr& task = _recycleBin.back();

		if (task->IsInitialised())
		{
			task->OnFinish(owner);
		}

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
		const TaskPtr& task = _recycleBin.back();

		if (task->IsInitialised())
		{
			task->OnFinish(_owner.GetEntity());
		}
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
		// Call the OnFinish event of the task after adding it to the bin
		_recycleBin.insert(_recycleBin.end(), _taskQueue.begin(), _taskQueue.end());

		// Remove ALL tasks from the main queue
		_taskQueue.clear();

		// Now call the OnFinish method. This might alter the original _taskQueue
		for (TaskQueue::const_iterator i = _recycleBin.begin(); i != _recycleBin.end(); ++i)
		{
			Task& task = *(*i);

			if (task.IsInitialised())
			{
				task.OnFinish(_owner.GetEntity());
			}
		}
	}

	// Disable this subsystem, but only if the task queue is actually empty
	// the Task::OnFinish() routines might have posted new tasks to the queue
	if (_taskQueue.empty())
	{
		_enabled = false;
	}
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
