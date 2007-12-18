/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: angua $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: SwitchOnLightState.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "SwitchOnLightState.h"
#include "../Memory.h"
#include "../Tasks/MoveToPositionTask.h"
#include "../../StimResponse/StimResponse.h"

namespace ai
{

SwitchOnLightState::SwitchOnLightState()
{}

SwitchOnLightState::SwitchOnLightState(idLight* light)
{
	_light = light;
}

// Get the name of this state
const idStr& SwitchOnLightState::GetName() const
{
	static idStr _name(STATE_SWITCH_ON_LIGHT);
	return _name;
}

void SwitchOnLightState::Init(idAI* owner)
{
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO).LogString("SwitchOnLightState initialised.\r");
	assert(owner);

	// Shortcut reference
	Memory& memory = owner->GetMemory();
	idLight* light = _light.GetEntity();

	// Fill the subsystems with their tasks

	owner->GetSubsystem(SubsysMovement)->ClearTasks();
	owner->GetSubsystem(SubsysAction)->ClearTasks();

	idStr lightType = light->spawnArgs.GetString(AIUSE_LIGHTTYPE_KEY);

	if (lightType == AIUSE_LIGHTTYPE_TORCH)
	{
		idVec3 lightDirection = owner->GetPhysics()->GetOrigin() - light->GetPhysics()->GetOrigin();
		lightDirection.z = 0;
		lightDirection.NormalizeFast();
//		float angle = (_reachEnemyCheck * 90) % 360;
//		float sinAngle = idMath::Sin(angle);
//		float cosAngle = idMath::Cos(angle);
//		idVec3 targetDirection = enemyDirection;
//		targetDirection.x = enemyDirection.x * cosAngle + enemyDirection.y * sinAngle;
//		targetDirection.y = enemyDirection.y * cosAngle + enemyDirection.x * sinAngle;
	
		idVec3 size = owner->GetAAS()->GetSettings()->boundingBoxes[0][1];
		idVec3 targetPoint = light->GetPhysics()->GetOrigin() + lightDirection * size.x * 0.5;
		idVec3 bottomPoint = targetPoint;
		bottomPoint.z -= size.z + owner->GetArmReachLength();
		
		trace_t result;
		if (gameLocal.clip.TracePoint(result, targetPoint, bottomPoint, MASK_OPAQUE, NULL))
		{
			targetPoint.z = result.endpos.z + 1;

			int areaNum = owner->PointReachableAreaNum(owner->GetPhysics()->GetOrigin(), 1.0f);
			int targetAreaNum = owner->PointReachableAreaNum(targetPoint, 1.0f);
			aasPath_t path;
			if (owner->PathToGoal(path, areaNum, owner->GetPhysics()->GetOrigin(), targetAreaNum, targetPoint))
			{
				targetPoint.z = result.endpos.z + 1;
				owner->GetSubsystem(SubsysMovement)->PushTask(TaskPtr(new MoveToPositionTask(targetPoint)));
				light->ResponseIgnore(ST_VISUAL, owner);
			}
		}
	}
}

// Gets called each time the mind is thinking
void SwitchOnLightState::Think(idAI* owner)
{
	// Shortcut reference
	Memory& memory = owner->GetMemory();
	idLight* light = _light.GetEntity();

	if (light == NULL)
	{
		DM_LOG(LC_AI, LT_ERROR).LogString("No stim source, terminating state!\r");
		owner->GetMind()->EndState();
		return;
	}

	if (owner->AI_MOVE_DONE)
	{
		idVec3 size = owner->GetAAS()->GetSettings()->boundingBoxes[0][1];
		float reachLength = size.z + owner->GetArmReachLength();
		if ((owner->GetEyePosition() - light->GetPhysics()->GetOrigin()).LengthFast() < reachLength)
		{
			// TODO: play animation

			light->CallScriptFunctionArgs("frob_ignite", true, 0, "e", light);
		}
	}

	if (light->GetLightLevel() > 0)
	{
		owner->GetMind()->EndState();
		return;
	}

	// Let the mind check its senses (TRUE = process new stimuli)
	owner->GetMind()->PerformSensoryScan(true);

	if (owner->AI_AlertNum >= owner->thresh_combat)
	{
		light->ResponseAllow(ST_VISUAL, owner);
		owner->GetMind()->EndState();
		return;
	}

}

void SwitchOnLightState::Save(idSaveGame* savefile) const
{
	State::Save(savefile);

	_light.Save(savefile);
}

void SwitchOnLightState::Restore(idRestoreGame* savefile)
{
	State::Restore(savefile);

	_light.Restore(savefile);
}

StatePtr SwitchOnLightState::CreateInstance()
{
	return StatePtr(new SwitchOnLightState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar switchOnLightStateRegistrar(
	STATE_SWITCH_ON_LIGHT, // Task Name
	StateLibrary::CreateInstanceFunc(&SwitchOnLightState::CreateInstance) // Instance creation callback
);

} // namespace ai
