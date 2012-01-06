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

#include "precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include "IdleState.h"
#include "AlertIdleState.h"
#include "../Memory.h"
#include "../Tasks/RandomHeadturnTask.h"
#include "../Tasks/AnimalPatrolTask.h"
#include "../Tasks/SingleBarkTask.h"
#include "../Tasks/MoveToPositionTask.h"
#include "../Tasks/IdleAnimationTask.h"
#include "../Tasks/RepeatedBarkTask.h"
#include "ObservantState.h"
#include "../Library.h"

namespace ai
{

// Get the name of this state
const idStr& AlertIdleState::GetName() const
{
	static idStr _name(STATE_ALERT_IDLE);
	return _name;
}

void AlertIdleState::Init(idAI* owner)
{
	// Init state class first
	// Note: we do not call IdleState::Init
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("AlertIdleState initialised.\r");
	assert(owner);

	// grayman #2603 - clear recent alerts, which allows us to see new, lower-weighted, alerts
	Memory& memory = owner->GetMemory();
//	memory.alertClass = EAlertNone; // grayman #2603 - moved further down, otherwise we don't hear the correct rampdown bark
//	memory.alertType = EAlertTypeNone;

	_alertLevelDecreaseRate = 0.005f;

	_startSleeping = owner->spawnArgs.GetBool("sleeping", "0");
	_startSitting = owner->spawnArgs.GetBool("sitting", "0");

	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner))
	{
		return;
	}

	InitialiseMovement(owner);
	InitialiseCommunication(owner);
	memory.alertClass = EAlertNone;
	memory.alertType = EAlertTypeNone;

	int idleBarkIntervalMin = SEC2MS(owner->spawnArgs.GetInt("alert_idle_bark_interval_min", "40"));
	int idleBarkIntervalMax = SEC2MS(owner->spawnArgs.GetInt("alert_idle_bark_interval_max", "120"));

	owner->commSubsystem->AddCommTask(
			CommunicationTaskPtr(new RepeatedBarkTask("snd_alert_idle", idleBarkIntervalMin, idleBarkIntervalMax))
		);

	// Initialise the animation state
	owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", 0);
	owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Idle", 0);

	// The sensory system does its Idle tasks
	owner->senseSubsystem->ClearTasks();
	owner->senseSubsystem->PushTask(RandomHeadturnTask::CreateInstance());

	if (!owner->GetAttackFlag(COMBAT_MELEE) && !owner->GetAttackFlag(COMBAT_RANGED))
	{
		owner->DrawWeapon();
	}

	// Let the AI update their weapons (make them nonsolid)
	owner->UpdateAttachmentContents(false);
}

idStr AlertIdleState::GetInitialIdleBark(idAI* owner)
{
	Memory& memory = owner->GetMemory();

	// Decide what sound it is appropriate to play
	idStr soundName("");

	if (!owner->m_RelightingLight &&	// grayman #2603 - No rampdown bark if relighting a light.
		!owner->m_ExaminingRope &&		// grayman #2872 - No rampdown bark if examining a rope.
		(owner->m_maxAlertLevel >= owner->thresh_1) && // grayman #2603 - m_lastAlertLevel can be uninitialized here, so it's not a good thing to check
		(owner->m_maxAlertLevel < owner->thresh_4))
//		(owner->m_lastAlertLevel >= owner->thresh_1) &&
//		(owner->m_lastAlertLevel < owner->thresh_3))
	{
		if (memory.alertClass == EAlertVisual_2) // grayman #2603
		{
			soundName = "snd_alertdown0sus";
		}
		else if ((memory.alertClass == EAlertVisual_1) && (memory.alertType != EAlertTypeMissingItem))
		{
			soundName = "snd_alertdown0s";
		}
		else if (memory.alertClass == EAlertAudio)
		{
			soundName = "snd_alertdown0h";
		}
		else
		{
			soundName = "snd_alertdown0";
		}
	}

	return soundName;
}

StatePtr AlertIdleState::CreateInstance()
{
	return StatePtr(new AlertIdleState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar alertIdleStateRegistrar(
	STATE_ALERT_IDLE, // Task Name
	StateLibrary::CreateInstanceFunc(&AlertIdleState::CreateInstance) // Instance creation callback
);

} // namespace ai
