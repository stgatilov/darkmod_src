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

static bool init_version = FileVersionList("$Id: ThrowObjectTask.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "ThrowObjectTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

// Get the name of this task
const idStr& ThrowObjectTask::GetName() const
{
	static idStr _name(TASK_THROW_OBJECT);
	return _name;
}

void ThrowObjectTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Init the base class
	Task::Init(owner, subsystem);

	_projectileDelayMin = owner->spawnArgs.GetFloat("outofreach_projectile_delay_min", "7.0");
	_projectileDelayMax = owner->spawnArgs.GetFloat("outofreach_projectile_delay_max", "10.0");

	_nextThrowObjectTime = gameLocal.time;
}

bool ThrowObjectTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("Throw Object Task performing.\r");

	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	Memory& memory = owner->GetMind()->GetMemory();

	if (owner->AI_ENEMY_VISIBLE &&
		_nextThrowObjectTime < gameLocal.time && 
		owner->spawnArgs.GetFloat("outofreach_projectile_enabled", "0") != 0)
	{
		gameLocal.Printf("throwing a random object...\n");
		//TODO	ai_darkmod_base::subFrameTask_throwRandomObject();
			
			_nextThrowObjectTime = gameLocal.time + _projectileDelayMin
								+ gameLocal.random.RandomFloat() * (_projectileDelayMax - _projectileDelayMin);
				
	}

	if (owner->MoveToPosition(owner->lastVisibleEnemyPos))
	{
		owner->GetMind()->PerformCombatCheck();
	}

	return false; // not finished yet
}

void ThrowObjectTask::SetEnemy(idActor* enemy)
{
	_enemy = enemy;
}

void ThrowObjectTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	savefile->WriteFloat(_projectileDelayMin);
	savefile->WriteFloat(_projectileDelayMax);
	savefile->WriteFloat(_nextThrowObjectTime);
	_enemy.Save(savefile);
}

void ThrowObjectTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	savefile->ReadFloat(_projectileDelayMin);
	savefile->ReadFloat(_projectileDelayMax);
	savefile->ReadFloat(_nextThrowObjectTime);
	_enemy.Restore(savefile);
}

ThrowObjectTaskPtr ThrowObjectTask::CreateInstance()
{
	return ThrowObjectTaskPtr(new ThrowObjectTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar throwObjectTaskRegistrar(
	TASK_THROW_OBJECT, // Task Name
	TaskLibrary::CreateInstanceFunc(&ThrowObjectTask::CreateInstance) // Instance creation callback
);

} // namespace ai
