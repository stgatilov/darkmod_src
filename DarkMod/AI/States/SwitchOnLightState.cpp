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
#include "../Tasks/RandomHeadturnTask.h"
#include "../Tasks/RandomTurningTask.h"

namespace ai
{

// grayman #2603 - heights for determining whether a light or switch is high/med/low off the floor

#define RELIGHT_HEIGHT_HIGH 65
#define RELIGHT_HEIGHT_LOW  30

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

// grayman #2603 - Wrap up and end state

void SwitchOnLightState::Wrapup(idAI* owner, idLight* light, bool lightOn)
{
	if (lightOn)
	{
		light->IgnoreResponse(ST_VISUAL,owner); // ignore until the light changes state again, going off

		// remove this light from the list of recently seen doused lights

		idEntityPtr<idEntity> lightPtr;
		lightPtr = light;
		owner->m_RecentDousedLightsSeen.Remove(lightPtr);
	}
	else
	{
		light->AllowResponse(ST_VISUAL,owner); // we'll need to catch it later if it goes off again
	}
	light->SetBeingRelit(false);
	owner->m_RelightingLight = false;
	owner->GetMind()->EndState();
}

void SwitchOnLightState::Init(idAI* owner)
{
	// grayman #2603 - a number of changes were made in this method

	// Shortcut reference
	Memory& memory = owner->GetMemory();

	// don't initiate a relight if something more important has happened
	if (memory.stopRelight)
	{
		return;
	}

	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("SwitchOnLightState initialised.\r");
	assert(owner);

	idLight* light = _light.GetEntity();

	// Make sure light is still off

	if (light->GetLightLevel() > 0)
	{
		Wrapup(owner,light,true);
		return;
	}

	_waitEndTime = 0;

	// First determine if this light is controlled by a switch. After all entities were spawned
	// at map start, each AIUSE_LIGHTTYPE_ELECTRIC light stored all the switches targetting it.
	// Checking for switches on torches is harmless, since they won't have any.
	//
	// GetSwitch() returns the switch closest to the AI. You can have multiple switches for a light,
	// i.e. one at each end of a hall, and the AI will go to the one nearest him at the moment he
	// decides to relight the light.
	//
	// TODO: GetSwitch() doesn't consider reachability. If the closest switch is unreachable at the
	// moment, GetSwitch() should fall back to the next-closest switch, seeking one that's reachable.
	
	idEntity* mySwitch = light->GetSwitch(owner); // Are there switches? If so, use the closest one.
	if (mySwitch)
	{
		// There's a switch, so the AI should walk to the switch and frob it. That should
		// be all that's needed to turn the light back on. The AI should do this even if
		// he doesn't have a direct LOS to the switch.

		_goalEnt = mySwitch;
	}
	else
	{
		// No switch, so approach the light.

		_goalEnt = light;
	}

	idStr lightType = light->spawnArgs.GetString(AIUSE_LIGHTTYPE_KEY);
	idVec3 goalOrigin = _goalEnt->GetPhysics()->GetOrigin();
	idVec3 goalDirection;
	if (mySwitch)
	{
		// Find the direction the switch translates, and project in the reverse
		// direction to find a spot to stand while activating it.

		if (mySwitch->IsType(CBinaryFrobMover::Type))
		{
			CBinaryFrobMover* switchMover = static_cast<CBinaryFrobMover*>(mySwitch);
			idVec3 deltaPosition;
			idAngles deltaAngles;
			switchMover->GetRemainingMovement(deltaPosition,deltaAngles);
			goalDirection = -deltaPosition;
		}
		else
		{
			goalDirection = owner->GetPhysics()->GetOrigin() - goalOrigin;
		}
	}
	else
	{
		goalDirection = owner->GetPhysics()->GetOrigin() - goalOrigin;
	}
	
	idVec3 size(16, 16, 82);
	idAAS* aas = owner->GetAAS();
	if (aas)
	{
		size = aas->GetSettings()->boundingBoxes[0][1];
	}

	float maxHeight =  size.z + owner->GetArmReachLength() - 8;

	idVec2 projection = goalDirection.ToVec2();

	// Am I carrying a torch? Useful when setting the standoff distance

	idEntity* torch = owner->GetTorch();

	// Move a bit from the goal origin towards the AI 
	// and perform a trace down to detect the ground.

	goalDirection.NormalizeFast();

	// If the goal entity has a bounding box (i.e. a model with
	// a built-in light, or a switch), use an average of its x/y sizes to add to
	// how far away the AI should stand.

	idEntity* bindMaster = _goalEnt->GetBindMaster();
	_standOff = 0;
	while (bindMaster != NULL)
	{
		idVec3 goalSize = bindMaster->GetPhysics()->GetBounds().GetSize();
		float goalDist = (goalSize.x + goalSize.y)/4;
		if (goalDist > _standOff)
		{
			_standOff = goalDist;
		}
		bindMaster = bindMaster->GetBindMaster(); // go up the hierarchy
	}

	_standOff += 2*size.x;
	float armReach = owner->GetArmReachLength();
	float standOffTemp = _standOff; // use this to establish reachability
	if (standOffTemp < armReach) // can't try to get close to candles on tables
	{
		standOffTemp = armReach;
	}

	// Use standOffTemp to find the floor near the goal. In case this is a candle sitting
	// on a table, you have to move out a reasonable distance to clear the table.

	idVec3 startPoint = goalOrigin + goalDirection * standOffTemp;
	idVec3 bottomPoint = startPoint;
	bottomPoint.z -= maxHeight;
	
	idVec3 targetPoint = startPoint;
	trace_t result;
	if (gameLocal.clip.TracePoint(result, startPoint, bottomPoint, MASK_OPAQUE, NULL))
	{
		// Found the floor.

		targetPoint.z = result.endpos.z + 1; // move the target point to the floor

		//gameRenderWorld->DebugArrow(colorRed, startPoint, targetPoint, 2, 5000);

		// Is there a path to the target point?

		int areaNum = owner->PointReachableAreaNum(owner->GetPhysics()->GetOrigin(), 1.0f);
		int targetAreaNum = owner->PointReachableAreaNum(targetPoint, 1.0f);
		aasPath_t path;
		if (owner->PathToGoal(path, areaNum, owner->GetPhysics()->GetOrigin(), targetAreaNum, targetPoint, owner))
		{
			// A path has been found. Now we adjust where we're going to stand based on
			// the relight method.

			// You can stand closer to goals that are on the floor or high (presumably on a wall). Goals
			// in the "medium" range are either switches on walls or candles on tables. You can't
			// try to get close to medium-height candles, because the presence of a table can make pathfinding
			// think you can't get to the candle. Medium-height switches on walls aren't a problem if you
			// stand a bit farther off from them.

			// Also adjust the standoff for flames based on whether you're using a torch or tinderbox.

			// Abandon standOffTemp and use the original _standOff.

			if (lightType == AIUSE_LIGHTTYPE_TORCH)
			{
				float ht = goalOrigin.z - targetPoint.z; // height of goal off the floor
				if (ht > RELIGHT_HEIGHT_HIGH) // high
				{
					if (torch)
					{
						_standOff -= 8; // get closer to goal
					}
					else // tinderbox
					{
						_standOff -= 16; // get closer to goal
					}
				}
				else if (ht < RELIGHT_HEIGHT_LOW) // low
				{
					if (torch == NULL)
					{
						_standOff -= 16; // get closer to goal
					}
					else // tinderbox
					{
					}
				}
				else // medium
				{
					if (torch)
					{
						_standOff -= 8; // get closer to goal
					}
					else // tinderbox
					{
						_standOff -= 16; // get closer to goal
					}
				}
			}
			idVec3 tp = goalOrigin + goalDirection * _standOff; // use adjusted standoff
			tp.z = targetPoint.z;

			owner->actionSubsystem->ClearTasks();
			owner->movementSubsystem->ClearTasks();
			owner->movementSubsystem->PushTask(TaskPtr(new MoveToPositionTask(tp,idMath::INFINITY,5)));

			if (gameLocal.time - memory.lastTimeVisualStimBark >= MINIMUM_SECONDS_BETWEEN_STIMULUS_BARKS)
			{
				memory.lastTimeVisualStimBark = gameLocal.time;
				owner->GetSubsystem(SubsysCommunication)->PushTask(TaskPtr(new SingleBarkTask("snd_yesRelightTorch")));
			}
			
			light->IgnoreResponse(ST_VISUAL, owner);
			_waitEndTime = gameLocal.time + 1000; // allow time for move to begin
			_relightState = EStateStarting;
			return;
		}

		// No path to goal. Success depends on the angle of approach. I.e. a candle sitting on a table
		// might only allow the AI to stand in certain places to light it. Try again later, when perhaps
		// the angle of approach provides a more favorable outcome.
		
		if (gameLocal.time - memory.lastTimeVisualStimBark >= MINIMUM_SECONDS_BETWEEN_STIMULUS_BARKS)
		{
			memory.lastTimeVisualStimBark = gameLocal.time;
			idStr bark = (lightType == AIUSE_LIGHTTYPE_TORCH) ? "snd_foundTorchOut" : "snd_foundLightsOff";
			owner->GetSubsystem(SubsysCommunication)->PushTask(TaskPtr(new SingleBarkTask(bark)));
		}
		Wrapup(owner,light,false);
		return;
	}

	// Probably can't reach the light, too far above ground. This could also be from the trace
	// hitting a table a candle is sitting on. Try again later. Also, a wall torch halfway up stairs
	// might allow for relighting when the AI is walking down the stairs, but not when he's walking up.

	// TODO: Count the number of times a light is out of reach. If it reaches a threshold, always ignore it.

	if (gameLocal.time - memory.lastTimeVisualStimBark >= MINIMUM_SECONDS_BETWEEN_STIMULUS_BARKS)
	{
		memory.lastTimeVisualStimBark = gameLocal.time;
		idStr bark = (lightType == AIUSE_LIGHTTYPE_TORCH) ? "snd_foundTorchOut" : "snd_foundLightsOff";
		owner->GetSubsystem(SubsysCommunication)->PushTask(TaskPtr(new SingleBarkTask(bark)));
	}
	
	Wrapup(owner,light,false);
}

// Gets called each time the mind is thinking
void SwitchOnLightState::Think(idAI* owner)
{
	// grayman #2603 - a number of changes were made in this method

	// It's possible that during the animation of touching a torch to an unlit flame, that
	// the torch's fire stim will light the unlit flame. You have to account for that.

	Memory& memory = owner->GetMemory();

	idLight* light = _light.GetEntity();
	if (light == NULL)
	{
		owner->m_RelightingLight = false;
		owner->GetMind()->EndState();
		return;
	}

	// check if something happened to abort the relight (i.e. dropped torch, higher alert)
	if (owner->GetMemory().stopRelight)
	{
		Wrapup(owner,light,(light->GetLightLevel() > 0));
		return;
	}

	owner->PerformVisualScan();	// Let the AI check its senses
	if (owner->AI_AlertLevel >= owner->thresh_5) // finished if alert level is too high
	{
		light->m_relightAfter = gameLocal.time + RELIGHT_DELAY; // wait awhile until you pay attention to it again
		Wrapup(owner,light,(light->GetLightLevel() > 0));
		return;
	}

	if (light->GetLightLevel() > 0) // If the light comes on before you relight it, act appropriately
	{
		switch (_relightState)
		{
			case EStateStarting:
			case EStateApproaching:
			case EStateTurningToward:
				Wrapup(owner,light,true);
				return;
			case EStateRelight:
			case EStatePause:
			case EStateFinal:
			default:
				break;
		}
	}

	if ((owner->m_HandlingDoor) || (owner->m_HandlingElevator))
	{
		return; // we're handling a door or elevator, so delay processing the rest of the relight
	}

	switch (_relightState)
	{
		case EStateStarting:
			if (owner->AI_FORWARD || (gameLocal.time >= _waitEndTime))
			{
				_relightState = EStateApproaching;
			}
			break;
		case EStateApproaching:
			{
				// Still walking toward the goal (switch or flame)

				idVec3 size(16, 16, 82);
				if (idAAS* aas = owner->GetAAS())
				{
					size = aas->GetSettings()->boundingBoxes[0][1];
				}

				idVec3 goalOrigin = _goalEnt->GetPhysics()->GetOrigin();

				idVec3 goalDirection = owner->GetPhysics()->GetOrigin() - goalOrigin;
				goalDirection.z = 0;
				float delta = goalDirection.LengthFast();

				if (delta <= _standOff)
				{
					owner->StopMove(MOVE_STATUS_DONE);
				}

				if (owner->AI_MOVE_DONE)
				{
					float lightHeight = goalOrigin.z - owner->GetPhysics()->GetOrigin().z;
					float maxHeight =  size.z + owner->GetArmReachLength() - 8;
					if ((delta <= 1.7 * owner->GetArmReachLength()) && (lightHeight <= maxHeight))
					{
						owner->TurnToward(goalOrigin);
						_relightState = EStateTurningToward;
						_waitEndTime = gameLocal.time + 750; // allow time for turn to complete
					}
					else // too far away
					{
						// TODO: Try moving closer?
						light->m_relightAfter = gameLocal.time + RELIGHT_DELAY; // wait awhile until you pay attention to it again
						Wrapup(owner,light,false);
						return;
					}
				}
				break;
			}
		case EStateTurningToward:
			if (gameLocal.time >= _waitEndTime)
			{
				StartSwitchOn(owner,light); // starts the relight animation
				owner->m_performRelight = false; // animation sets this to TRUE at the relight frame
				_relightState = EStateRelight;
				_waitEndTime = gameLocal.time + 10000; // failsafe in case something aborts the animation
			}
			break;
		case EStateRelight:
			if (owner->m_performRelight)
			{
				// Time to relight the light.
				// If you're dealing with a torch or candle, frob the light directly. If it's an electric
				// light, either _goalEnt is a switch, or it's the light itself. If a switch, activate it.
				// If it's not a switch, frob any bindMaster. If there's no bindMaster, frob the light itself.
				// Frobbing an existing bindMaster makes sure that all activities related to frobbing this
				// light are dealt with.

				idStr lightType = light->spawnArgs.GetString(AIUSE_LIGHTTYPE_KEY);
				if (lightType == AIUSE_LIGHTTYPE_TORCH)
				{
					light->CallScriptFunctionArgs("frob_ignite", true, 0, "e", light);
				}
				else // handle electric lights
				{
					if (_goalEnt == light)
					{
						idEntity* bindMaster = light->GetBindMaster();
						if (bindMaster) // light holder
						{
							bindMaster->CallScriptFunctionArgs("LightsOn", true, 0, "e", bindMaster);
						}
						else // light
						{
							light->CallScriptFunctionArgs("LightsOn", true, 0, "e", light);
						}
					}
					else // switch
					{
						_goalEnt->Activate(owner);
					}
				}
				_relightState = EStatePause;
				owner->m_performRelight = false;
				_waitEndTime = gameLocal.time + 10000; // failsafe in case something aborts the animation
			}
			else if (gameLocal.time >= _waitEndTime) // animation problem - abort
			{
				owner->m_performRelight = false;
				_relightState = EStatePause;
			}
			break;
		case EStatePause:
			if ((owner->AnimDone(ANIMCHANNEL_TORSO,4)) || (gameLocal.time >= _waitEndTime))
			{
				_waitEndTime = gameLocal.time + 3000; // pause before walking away
				_relightState = EStateFinal;

				// Slow turning because you're suspicious
				_oldTurnRate = owner->GetTurnRate();
				owner->SetTurnRate(45);
				owner->movementSubsystem->PushTask(RandomTurningTask::CreateInstance());
				owner->senseSubsystem->PushTask(RandomHeadturnTask::CreateInstance());
			}
			break;
		case EStateFinal:
			if (gameLocal.time >= _waitEndTime)
			{
				owner->SetTurnRate(_oldTurnRate); // done being suspicious after relight

				// Set up search if warranted

				if (owner->m_LatchedSearch)
				{
					if (owner->AI_AlertLevel < owner->thresh_4)
					{
						memory.alertPos = light->GetPhysics()->GetOrigin();
						memory.alertClass = EAlertVisual;
						memory.alertType = EAlertTypeLightSource;
						
						// Prepare search as if there is an enemy that has escaped
						memory.alertRadius = LOST_ENEMY_ALERT_RADIUS;
						memory.alertSearchVolume = LOST_ENEMY_SEARCH_VOLUME; 
						memory.alertSearchExclusionVolume.Zero();
							
						owner->AI_VISALERT = false;

						// Do new reaction to stimulus after relighting
						memory.stimulusLocationItselfShouldBeSearched = true;
						memory.alertedDueToCommunication = false;
					}
				}

				Wrapup(owner,light,true);
				light->AllowResponse(ST_VISUAL,owner);
				return;
			}
			break;
		default:
			break;
	}
}

void SwitchOnLightState::StartSwitchOn(idAI* owner, idLight* light)
{
	// grayman #2603 - a number of changes were made in this method

	owner->movementSubsystem->ClearTasks();
	owner->StopMove(MOVE_STATUS_DONE);
	idStr torsoAnimation = "";
	idStr legsAnimation = ""; // if empty, no need to deal with the legs
	float lightHeight = _goalEnt->GetPhysics()->GetOrigin().z - owner->GetPhysics()->GetOrigin().z;
	idStr lightType = light->spawnArgs.GetString(AIUSE_LIGHTTYPE_KEY);
	bool drawWeapon = false;

	if (lightType == AIUSE_LIGHTTYPE_TORCH)
	{
		// If I have a torch, I'll use that. If not, I'll use the tinderbox. Since
		// that animation looks iffy when I'm carrying a melee weapon, I'll put it
		// away first. Dealing with the weapon is done in the animation that
		// uses the tinderbox animation. That's why we can't use the torch's
		// replacement animation ability for these animations.

		if (owner->GetTorch() != NULL)
		{
			if (lightHeight > RELIGHT_HEIGHT_HIGH) // high?
			{
				torsoAnimation = "Torso_Relight_Torch_High";
			}
			else if (lightHeight < RELIGHT_HEIGHT_LOW) // low?
			{
				torsoAnimation = "Torso_Relight_Torch_Low";
				legsAnimation  = "Legs_Relight_Torch_Low";
			}
			else // medium
			{
				torsoAnimation = "Torso_Relight_Torch_Med";
			}
		}
		else // use tinderbox (sheathes any drawn weapon, relights, then redraws weapon)
		{
			if (lightHeight > RELIGHT_HEIGHT_HIGH) // high?
			{
				torsoAnimation = "Torso_Relight_Tinderbox_High";
			}
			else if (lightHeight < RELIGHT_HEIGHT_LOW) // low?
			{
				torsoAnimation = "Torso_Relight_Tinderbox_Low";
				legsAnimation  = "Legs_Relight_Tinderbox_Low";
			}
			else // medium
			{
				torsoAnimation = "Torso_Relight_Tinderbox_Med";
			}
		}
	}
	else // electric
	{
		if (lightHeight > RELIGHT_HEIGHT_HIGH) // high?
		{
			torsoAnimation = "Torso_Relight_Electric_High"; // reach up toward the switch or light
		}
		else if (lightHeight < RELIGHT_HEIGHT_LOW) // low?
		{
			torsoAnimation = "Torso_Relight_Electric_Low"; // reach down toward the switch or light
			legsAnimation  = "Legs_Relight_Electric_Low";
		}
		else // medium
		{
			torsoAnimation = "Torso_Relight_Electric_Med"; // reach out toward the switch or light
		}
	}

	owner->SetAnimState(ANIMCHANNEL_TORSO, torsoAnimation.c_str(), 4);
	if (legsAnimation.Length() > 0)
	{
		owner->SetAnimState(ANIMCHANNEL_LEGS, legsAnimation.c_str(), 4);
	}
}

void SwitchOnLightState::Save(idSaveGame* savefile) const
{
	State::Save(savefile);
	_light.Save(savefile);

	savefile->WriteInt(_waitEndTime);
	savefile->WriteObject(_goalEnt);	// grayman #2603
	savefile->WriteFloat(_standOff);	// grayman #2603
	savefile->WriteInt(static_cast<int>(_relightState)); // grayman #2603
	savefile->WriteFloat(_oldTurnRate);	// grayman #2603
}

void SwitchOnLightState::Restore(idRestoreGame* savefile)
{
	State::Restore(savefile);
	_light.Restore(savefile);

	savefile->ReadInt(_waitEndTime);
	savefile->ReadObject(reinterpret_cast<idClass*&>(_goalEnt)); // grayman #2603
	savefile->ReadFloat(_standOff);		// grayman #2603
	int temp;
	savefile->ReadInt(temp);
	_relightState = static_cast<ERelightState>(temp); // grayman #2603
	savefile->ReadFloat(_oldTurnRate);	// grayman #2603
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
