/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

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

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("SwitchOnLightState initialised.\r");
	assert(owner);

	// Shortcut reference
	Memory& memory = owner->GetMemory();
	idLight* light = _light.GetEntity();

	idStr lightType = light->spawnArgs.GetString(AIUSE_LIGHTTYPE_KEY);

	_switchingOn = false;
	_waitEndTime = 0;
	_lightOn = false;

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
			StartSwitchOn(owner, light);
		}
		else
		{
			// Move a bit from the light origin towards the ai 
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
				if (owner->PathToGoal(path, areaNum, owner->GetPhysics()->GetOrigin(), targetAreaNum, targetPoint, owner))
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
		DM_LOG(LC_AI, LT_ERROR)LOGSTRING("No stim source, terminating state!\r");
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
	
	if (_lightOn && gameLocal.time >= _waitEndTime)
	{
		owner->GetMind()->EndState();
		return;
	}

	if (!_lightOn && light->GetLightLevel() > 0)
	{
		// Light is on again
		_waitEndTime = gameLocal.time + 1000;
		_lightOn = true;
	}

	if (!_lightOn && _switchingOn && gameLocal.time >= _waitEndTime)
	{
		light->CallScriptFunctionArgs("frob_ignite", true, 0, "e", light);
		_waitEndTime = gameLocal.time + 5000;
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

		if (!_switchingOn && delta <= size.x)
		{
			StartSwitchOn(owner, light);
		}
		else if (!_switchingOn && owner->AI_MOVE_DONE)
		{
			if (delta <= 1.5 * owner->GetArmReachLength())
			{
				StartSwitchOn(owner, light);
				
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

void SwitchOnLightState::StartSwitchOn(idAI* owner, idLight* light)
{
	owner->GetSubsystem(SubsysMovement)->ClearTasks();
	owner->StopMove(MOVE_STATUS_DONE);
	_waitEndTime = gameLocal.time + 500;
	_switchingOn = true;
	owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Use_righthand", 4);

	// TODO: Play anim
	
}

void SwitchOnLightState::Save(idSaveGame* savefile) const
{
	State::Save(savefile);
	_light.Save(savefile);

	savefile->WriteInt(_waitEndTime);
	savefile->WriteBool(_switchingOn);
	savefile->WriteBool(_lightOn);
}

void SwitchOnLightState::Restore(idRestoreGame* savefile)
{
	State::Restore(savefile);
	_light.Restore(savefile);

	savefile->ReadInt(_waitEndTime);
	savefile->ReadBool(_switchingOn);
	savefile->ReadBool(_lightOn);
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
