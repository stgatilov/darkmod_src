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

static bool init_version = FileVersionList("$Id: StimulusSensoryTask.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "StimulusSensoryTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

// Get the name of this task
const idStr& StimulusSensoryTask::GetName() const
{
	static idStr _name(TASK_STIMULUS_SENSORY);
	return _name;
}

void StimulusSensoryTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);

	// nothing so far, remove me (FIXME)
}

bool StimulusSensoryTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("StimulusSensoryTask performing.\r");

	idAI* owner = _owner.GetEntity();
	
	// This task may not be performed with empty entity pointers
	assert(owner != NULL);

	// Let the mind check its senses (TRUE = process new stimuli)
	owner->GetMind()->PerformSensoryScan(true);

	

	return false; // not finished yet
}

StimulusSensoryTaskPtr StimulusSensoryTask::CreateInstance()
{
	return StimulusSensoryTaskPtr(new StimulusSensoryTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar stimulusSensoryTaskRegistrar(
	TASK_STIMULUS_SENSORY, // Task Name
	TaskLibrary::CreateInstanceFunc(&StimulusSensoryTask::CreateInstance) // Instance creation callback
);

} // namespace ai
