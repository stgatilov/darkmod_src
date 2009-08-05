/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 3549 $
 * $Date: 2009-07-19 12:47:25 +0200 (So, 19 Jul 2009) $
 * $Author: greebo $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: CombatTask.cpp 3549 2009-07-19 10:47:25Z greebo $", init_version);

#include "CombatTask.h"
#include "SingleBarkTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

CombatTask::CombatTask() :
	_lastCombatBarkTime(-1)
{}

void CombatTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Init the base class
	Task::Init(owner, subsystem);
	
	_enemy = owner->GetEnemy();
}

void CombatTask::EmitCombatBark(idAI* owner, const idStr& sndName)
{
	// This will hold the message to be delivered with the bark, if appropriate
	CommMessagePtr message;
	
	// Only alert the bystanders if we didn't receive the alert by message ourselves
	message = CommMessagePtr(new CommMessage(
		CommMessage::DetectedEnemy_CommType, 
		owner, NULL, // from this AI to anyone 
		owner->GetEnemy(),
		owner->GetPhysics()->GetOrigin()
	));

	_lastCombatBarkTime = gameLocal.time;

	// The communication system 
	owner->commSubsystem->AddCommTask(
		CommunicationTaskPtr(new SingleBarkTask(sndName, message))
	);
}

void CombatTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	_enemy.Save(savefile);
	savefile->WriteInt(_lastCombatBarkTime);
}

void CombatTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	_enemy.Restore(savefile);
	savefile->ReadInt(_lastCombatBarkTime);
}

} // namespace ai
