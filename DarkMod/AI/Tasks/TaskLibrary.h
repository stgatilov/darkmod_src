/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/

#ifndef __AI_TASK_LIBRARY_H__
#define __AI_TASK_LIBRARY_H__

#include <map>
#include <boost/noncopyable.hpp>
#include <boost/function.hpp>

namespace ai
{

// Forward Declaration
class Task;
typedef boost::shared_ptr<Task> TaskPtr;

// Define the function type to Create a Task Instance
typedef boost::function<TaskPtr()> CreateInstanceFunc;

/**
 * greebo: This Singleton Library class maps task names to
 *         static CreateInstance() member functions. All tasks
 *         register themselves with this library and provide a 
 *         callback to get an instance of themselves.
 *
 * Why am I doing this? During InitFromSavegame, the Subsystems will have
 * to re-allocate an instance of the task they were associated with. To avoid
 * implementing a complicated homegrown typesystem, this is Library is used.
 *
 * Use the CreateTask() method to acquire a new instance of a named task.
 */
class TaskLibrary :
	public boost::noncopyable
{
	typedef std::map<idStr, CreateInstanceFunc> TaskMap;
	TaskMap _tasks;

private:
	// Private constructor
	TaskLibrary();

public:
	
	/**
	 * greebo: Tries to lookup the task name in the TaskMap
	 *         and instantiates a Task of this type.
	 */
	TaskPtr CreateTask(const idStr& taskName);

	/**
	 * greebo: Each Task has to register itself here.
	 *         This must happen before any client may call
	 *         CreateTask, so place this call right below 
	 *         the Task declaration.
	 */
	void RegisterTask(const idStr& taskName, const CreateInstanceFunc& func);

	// Accessor method for the singleton instance
	static TaskLibrary& Instance();
};

// Helper functor which registers the given task in its constructor
class TaskRegistrar 
{
public:
	TaskRegistrar(const idStr& name, const CreateInstanceFunc& func)
	{
		// Pass the call
		TaskLibrary::Instance().RegisterTask(name, func);
	}
};

} // namespace ai

#endif /* __AI_TASK_LIBRARY_H__ */
