/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2008-04-29 18:53:28 +0200 (Di, 29 Apr 2008) $
 * $Author: greebo $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: ScriptTask.cpp 1435 2008-04-11 16:53:28Z greebo $", init_version);

#include "ScriptTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

ScriptTask::ScriptTask() :
	_thread(NULL)
{}

ScriptTask::ScriptTask(const idStr& functionName) :
	_functionName(functionName),
	_thread(NULL)
{}

// Get the name of this task
const idStr& ScriptTask::GetName() const
{
	static idStr _name(TASK_SCRIPT);
	return _name;
}

void ScriptTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Init the base class
	Task::Init(owner, subsystem);

	const function_t* scriptFunction = owner->scriptObject.GetFunction(_functionName);
	if (scriptFunction == NULL)
	{
		// Local function not found, check in global namespace
		scriptFunction = gameLocal.program.FindFunction(_functionName);
	}

	if (scriptFunction != NULL)
	{
		_thread = new idThread(scriptFunction);
		_thread->CallFunctionArgs(scriptFunction, true, "e", this);
		_thread->DelayedStart(0);
	}
	else
	{
		// script function not found!
		DM_LOG(LC_AI, LT_ERROR).LogString("ScriptTask could not find task %s.\r", _functionName.c_str());
		subsystem.FinishTask();
		return;
	}
}

bool ScriptTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("ScriptTask performing.\r");
	assert(_thread != NULL);

	if (_thread->IsDoneProcessing() || _thread->IsDying())
	{
		// thread is done, return TRUE to terminate this task
		_thread = NULL;
		return true;
	}

	return false; // not finished yet
}

void ScriptTask::OnFinish(idAI* owner)
{
	if (_thread != NULL)
	{
		// We've got a non-NULL thread, this means it's still alive, end it now
		_thread->End();
	}
}

void ScriptTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	savefile->WriteString(_functionName);
	savefile->WriteObject(_thread);
}

void ScriptTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	savefile->ReadString(_functionName);
	savefile->ReadObject(reinterpret_cast<idClass*&>(_thread));
}

ScriptTaskPtr ScriptTask::CreateInstance()
{
	return ScriptTaskPtr(new ScriptTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar scriptTaskRegistrar(
	TASK_SCRIPT, // Task Name
	TaskLibrary::CreateInstanceFunc(&ScriptTask::CreateInstance) // Instance creation callback
);

} // namespace ai
