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

static bool init_version = FileVersionList("$Id: CombatSensoryTask.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "CombatSensoryTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

// Get the name of this task
const idStr& CombatSensoryTask::GetName() const
{
	static idStr _name(TASK_COMBAT_SENSORY);
	return _name;
}

void CombatSensoryTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Init the base class
	Task::Init(owner, subsystem);

	_enemy = owner->GetEnemy();
}

bool CombatSensoryTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("CombatSensoryTask performing.\r");

	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	idActor* enemy = _enemy.GetEntity();
	if (enemy == NULL)
	{
		DM_LOG(LC_AI, LT_ERROR).LogString("No enemy, terminating task!\r");
		return true; // terminate me
	}

	// Move canhitenemy here

	return false; // not finished yet
}

void CombatSensoryTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	_enemy.Save(savefile);
}

void CombatSensoryTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	_enemy.Restore(savefile);
}

CombatSensoryTaskPtr CombatSensoryTask::CreateInstance()
{
	return CombatSensoryTaskPtr(new CombatSensoryTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar combatSensoryTaskRegistrar(
	TASK_COMBAT_SENSORY, // Task Name
	TaskLibrary::CreateInstanceFunc(&CombatSensoryTask::CreateInstance) // Instance creation callback
);

} // namespace ai
