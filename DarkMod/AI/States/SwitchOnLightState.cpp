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
#include "../Tasks/SingleBarkTask.h"
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

	idStr lightType = light->spawnArgs.GetString(AIUSE_LIGHTTYPE_KEY);

	if (lightType == AIUSE_LIGHTTYPE_TORCH)
	{
		idVec3 lightDirection = owner->GetPhysics()->GetOrigin() - light->GetPhysics()->GetOrigin();
		
		idVec3 size(16, 16, 82);
		idAAS* aas = owner->GetAAS();
		if (aas)
		{
			size = aas->GetSettings()->boundingBoxes[0][1];
		}

		float maxHeight =  size.z + owner->GetArmReachLength();
		idVec2 projection = lightDirection.ToVec2();

		if (projection.LengthFast() < owner->GetArmReachLength() && lightDirection.z < maxHeight)
		{
			// If we are already close enough, relight the torch immediately
			owner->GetSubsystem(SubsysMovement)->ClearTasks();
			owner->GetSubsystem(SubsysAction)->ClearTasks();
			owner->StopMove(MOVE_STATUS_DONE);
			// TODO: Play anim
			light->CallScriptFunctionArgs("frob_ignite", true, 0, "e", light);
		}
		else
		{
			// Move a bit in front of the light origin towards the ai 
			// and perform a trace down to detect the ground
			lightDirection.NormalizeFast();
		
			idVec3 startPoint = light->GetPhysics()->GetOrigin() + lightDirection * size.x;
			idVec3 bottomPoint = startPoint;
			bottomPoint.z -= maxHeight;
			
			idVec3 targetPoint = startPoint;
			trace_t result;
			if (gameLocal.clip.TracePoint(result, startPoint, bottomPoint, MASK_OPAQUE, NULL))
			{
				targetPoint.z = result.endpos.z + 1;
				// gameRenderWorld->DebugArrow(colorRed, startPoint, targetPoint, 2, 5000);

				int areaNum = owner->PointReachableAreaNum(owner->GetPhysics()->GetOrigin(), 1.0f);
				int targetAreaNum = owner->PointReachableAreaNum(targetPoint, 1.0f);
				aasPath_t path;
				if (owner->PathToGoal(path, areaNum, owner->GetPhysics()->GetOrigin(), targetAreaNum, targetPoint))
				{
					targetPoint.z = result.endpos.z + 1;

					owner->GetSubsystem(SubsysAction)->ClearTasks();
					owner->GetSubsystem(SubsysMovement)->ClearTasks();
					owner->GetSubsystem(SubsysMovement)->PushTask(TaskPtr(new MoveToPositionTask(targetPoint)));

					if (gameLocal.time - memory.lastTimeVisualStimBark >= MINIMUM_SECONDS_BETWEEN_STIMULUS_BARKS)
					{
						memory.lastTimeVisualStimBark = gameLocal.time;
						owner->GetSubsystem(SubsysCommunication)->PushTask(
							TaskPtr(new SingleBarkTask("snd_yesRelightTorch"))
						);
					}

					light->ResponseIgnore(ST_VISUAL, owner);
				}
				else
				{
					// Probably can't reach light, no path to goal found
					light->ResponseIgnore(ST_VISUAL, owner);
					if (gameLocal.time - memory.lastTimeVisualStimBark >= MINIMUM_SECONDS_BETWEEN_STIMULUS_BARKS)
					{
						memory.lastTimeVisualStimBark = gameLocal.time;
						owner->GetSubsystem(SubsysCommunication)->PushTask(
							TaskPtr(new SingleBarkTask("snd_foundTorchOut"))
						);
					}
					owner->GetMind()->EndState();
					return;
				}
			}
			else
			{
				// Probably can't reach the light, too far above ground
				light->ResponseIgnore(ST_VISUAL, owner);
				if (gameLocal.time - memory.lastTimeVisualStimBark >= MINIMUM_SECONDS_BETWEEN_STIMULUS_BARKS)
				{
					memory.lastTimeVisualStimBark = gameLocal.time;
					owner->GetSubsystem(SubsysCommunication)->PushTask(
						TaskPtr(new SingleBarkTask("snd_foundTorchOut"))
					);
				}
				owner->GetMind()->EndState();
				return;
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

	// Let the AI check its senses
	owner->PerformVisualScan();
	if (owner->AI_AlertLevel >= owner->thresh_5)
	{
		light->ResponseAllow(ST_VISUAL, owner);
		owner->GetMind()->EndState();
		return;
	}
	
	if (light->GetLightLevel() > 0)
	{
		// Light is on again
		// TODO: Wait until anim is finished
		owner->GetMind()->EndState();
		return;
	}

	idStr lightType = light->spawnArgs.GetString(AIUSE_LIGHTTYPE_KEY);

	idVec3 size(16, 16, 82);
	if (idAAS* aas = owner->GetAAS())
	{
		size = aas->GetSettings()->boundingBoxes[0][1];
	}

	if (lightType == AIUSE_LIGHTTYPE_TORCH)
	{
		idVec3 lightDirection = owner->GetPhysics()->GetOrigin() - light->GetPhysics()->GetOrigin();
		lightDirection.z = 0;
		float delta = lightDirection.LengthFast();

		if (delta <= size.x)
		{
			owner->GetSubsystem(SubsysMovement)->ClearTasks();
			owner->StopMove(MOVE_STATUS_DONE);
			// TODO: Play anim
			light->CallScriptFunctionArgs("frob_ignite", true, 0, "e", light);
		}
		else if (owner->AI_MOVE_DONE)
		{
			if (delta <= 1.5 * owner->GetArmReachLength())
			{
				owner->GetSubsystem(SubsysMovement)->ClearTasks();
				owner->StopMove(MOVE_STATUS_DONE);
				// TODO: Play anim
				light->CallScriptFunctionArgs("frob_ignite", true, 0, "e", light);
			}
			else
			{
				// TODO: Try moving closer?
				owner->GetMind()->EndState();
				return;
			}
		}
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
