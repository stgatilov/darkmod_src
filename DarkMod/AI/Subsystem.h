/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/

#ifndef __AI_SUBSYSTEM_H__
#define __AI_SUBSYSTEM_H__

#include <boost/shared_ptr.hpp>
#include <list>

#include "Tasks/Task.h"

namespace ai
{

enum SubsystemId {
	SubsysMovement = 0,
	SubsysSenses,
	SubsysCommunication,
	SubsysAction,
	SubsystemCount,
};

class Subsystem
{
protected:
	idEntityPtr<idAI> _owner;

	// The stack of tasks, pushed tasks get added at the end
	typedef std::list<TaskPtr> TaskQueue;
	TaskQueue _taskQueue;

	bool _finishCurrentTask;

	// The currently active task
	TaskPtr _task;

	// TRUE if this subsystem is performing, default is ON
	bool _enabled;

public:
	Subsystem(idAI* owner);

	// Called regularly by the Mind to run the currently assigned routine.
	virtual void PerformTask();

	// Puts another task at the end of the queue
	virtual void QueueTask(const TaskPtr& nextTask);

	// Sets the current task to the EmptyTask
	virtual void FinishCurrentTask();

	// Clears out the current task plus the entire queue and disables the subsystem
	virtual void ClearTasks();

	// Enables/disables this subsystem
	virtual void Enable();
	virtual void Disable();

	// Returns TRUE if this subsystem is performing.
	virtual bool IsEnabled() const;

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);
};
typedef boost::shared_ptr<Subsystem> SubsystemPtr;

} // namespace ai

#endif /* __AI_SUBSYSTEM_H__ */
