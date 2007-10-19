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

static bool init_version = FileVersionList("$Id: BasicMind.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "BasicMind.h"
#include "States/IdleState.h"
#include "Library.h"
#include "../idAbsenceMarkerEntity.h"
#include "../AIComm_Message.h"

namespace ai
{

BasicMind::BasicMind(idAI* owner)
{
	// Set the idEntityPtr
	_owner = owner;
}

void BasicMind::Think()
{
	// Thinking
	DM_LOG(LC_AI, LT_INFO).LogString("Mind is thinking...\r");

	if (_state == NULL)
	{
		// We start with the idle state
		ChangeState(STATE_IDLE);
	}

	// greebo: We do not check for NULL pointers in the owner at this point, 
	// as this method is called by the owner itself.

	idAI* owner = _owner.GetEntity();

	switch (gameLocal.framenum % 4) {
		case 0:
			owner->GetSubsystem(SubsysSenses)->PerformTask();
			break;
		case 1:
			owner->GetSubsystem(SubsysMovement)->PerformTask();
			break;
		case 2:
			owner->GetSubsystem(SubsysCommunication)->PerformTask();
			break;
		case 3:
			owner->GetSubsystem(SubsysAction)->PerformTask();
			break;
	};

	// Check if we can decrease the alert level
	TestAlertStateTimer();
}

// Changes the state
void BasicMind::ChangeState(const idStr& stateName)
{
	StatePtr newState = StateLibrary::Instance().CreateInstance(stateName.c_str());

	if (newState != NULL)
	{
		// Change the state, the pointer is ok
		_state = newState;

		// Initialise the new state
		_state->Init(_owner.GetEntity());
	}
	else
	{
		gameLocal.Error("BasicMind: Could not change state to %s", stateName.c_str());
	}
}

void BasicMind::TestAlertStateTimer()
{
	// greebo: This has been ported from ai_darkmod_base::subFrameTask_testAlertStateTimer() by SZ
	idAI* owner = _owner.GetEntity();
	assert(owner);

	float newAlertLevel(0);
	float curTime = gameLocal.time;

	// restart the de-alert timer if we get another alert
	if (owner->AI_ALERTED)
	{
		//DEBUG_PRINT ("Restarting alert state timer");
		owner->AI_currentAlertLevelStartTime = curTime;
		return;
	}
	
	if (owner->AI_currentAlertLevelDuration <= 0)
	{
		return;
	}
	
	if (MS2SEC(curTime - owner->AI_currentAlertLevelStartTime) >= owner->AI_currentAlertLevelDuration)
	{
		// This alert level has expired, drop the alert level down halfway up the
		// next lower category (to reflect nervousness in next lower category)
		if (owner->AI_AlertNum > owner->thresh_3)
		{
			//DEBUG_PRINT ("Dropping to alert level 2.5 after alert duration expired, duration " + AI_currentAlertLevelDuration);
			newAlertLevel = owner->thresh_2 + ((owner->thresh_3 - owner->thresh_2) / 2.0);
		}
		else if (owner->AI_AlertNum > owner->thresh_2)
		{
			//DEBUG_PRINT ("Dropping to alert level 1.5 after alert duration expired, duration " + AI_currentAlertLevelDuration);
			newAlertLevel = owner->thresh_1 + ((owner->thresh_2 - owner->thresh_1) / 2.0);
		}
		else if (owner->AI_AlertNum > (owner->thresh_1 / 2.0))
		{
			//DEBUG_PRINT ("Dropping to alert level 0.5 after alert duration expired, duration " + AI_currentAlertLevelDuration);
			newAlertLevel = (owner->thresh_1/ 2.0) - 0.01; // To prevent floating point comparison error

			// Alert level is changing
			owner->Event_SetAlertLevel(newAlertLevel);
			
			// Go Idle
			return;
		}
		else
		{
			// Alert level is not changing if already < halfway between thresh_1 and thresh_2
			//DEBUG_PRINT ("Alert level already at 1.5 " + AI_currentAlertLevelDuration);
			return;
		}

		// Alert timer has expired		
		owner->AI_currentAlertLevelDuration = -1;
		
		// Alert level is changing
		owner->Event_SetAlertLevel(newAlertLevel);
	}
}

void BasicMind::SetAlertPos()
{
	// greebo: This has been ported from ai_darkmod_base::setAlertPos() written by SZ
	idAI* owner = _owner.GetEntity();
	assert(owner);

	// Create a shortcut reference
	Memory& memory = owner->GetMind()->GetMemory();

	// NOTE: If several alerts happen in the same frame,
	// the priority is tactile, visual, then sound

	// Stim bark type
	// 1 is saw
	// 2 is heard
	int stimBarkType = 0;
	
	if (owner->AI_TACTALERT)
	{
		/**
		* FIX: They should not try to MOVE to the tactile alert position
		* because if it's above their head, they can't get to it and will
		* just run in circles.  
		*
		* Instead, turn to this position manually, then set the search target
		* to the AI's own origin, to execute a search starting from where it's standing
		*
		* TODO later: Predict where the thrown object might have come from, and search
		* in that direction (requires a "directed search" algorithm)
		*/
		idEntity* target = owner->GetTactEnt();

		if (IsEnemy(target, owner)) // also checks for NULL pointers
		{
			owner->Event_SetEnemy(target);

			memory.alertType = EAlertTactile;
			memory.alertPos = owner->GetPhysics()->GetOrigin();
			memory.alertRadius = TACTILE_ALERT_RADIUS;
			memory.alertSearchVolume = TACTILE_SEARCH_VOLUME;
			memory.alertSearchExclusionVolume.Zero();

			// execute the turn manually here
			owner->TurnToward(memory.alertPos);
			
			owner->AI_TACTALERT = false;
		}
		else
		{
			DM_LOG(LC_AI, LT_INFO).LogString("No tactile alert entity was set");
		}
	}
	else if( owner->AI_VISALERT )
	{
		memory.alertType = EAlertVisual;
		memory.alertPos = owner->GetVisDir();
		memory.alertRadius = VISUAL_ALERT_RADIUS;
		memory.alertSearchVolume = VISUAL_SEARCH_VOLUME;
		memory.alertSearchExclusionVolume.Zero();
		
		owner->AI_VISALERT = false;
		stimBarkType = 1;
		
		//DEBUG_PRINT ("Visual alert pos " + m_alertPos);
	}	
	else if( owner->AI_HEARDSOUND )
	{
		memory.alertType = EAlertAudio;
		memory.alertPos = owner->GetSndDir();
		
		// Search within radius of stimulus that is 1/3 the distance from the
		// observer to the point at the time heard
		float distanceToStim = (owner->GetPhysics()->GetOrigin() - memory.alertPos).LengthFast();
		float searchVolModifier = distanceToStim / 600.0f;
		if (searchVolModifier < 0.01f)
		{
			searchVolModifier = 0.01f;
		}

		memory.alertRadius = AUDIO_ALERT_RADIUS;
		memory.alertSearchVolume = AUDIO_SEARCH_VOLUME * searchVolModifier;
		memory.alertSearchExclusionVolume.Zero();
				
		owner->AI_HEARDSOUND = false;
		stimBarkType = 2;
	}

	/*
	// Handle stimulus "barks"
	if 
	(
		(stimBarkType >= 1.0) && 
		( (sys.getTime() - AI_timeOfLastStimulusBark) >= MINIMUM_SECONDS_BETWEEN_STIMULUS_BARKS)
	)
	{
		AI_timeOfLastStimulusBark = sys.getTime();

		// Check for any friends nearby we might want to talk to
		boolean b_friendNearby = false;
		if ( (sys.getTime() - stateOfMind_lastTimeFriendlyAISeen) <= (MAX_FRIEND_SIGHTING_SECONDS_FOR_ACCOMPANIED_ALERT_BARK) )
		{
			DEBUG_PRINT ("Time since friend is " + (sys.getTime() - stateOfMind_lastTimeFriendlyAISeen));
			b_friendNearby = true;
		}

	
		if ((stimBarkType >= 1.9) && (stimBarkType <= 2.1))
		{
			// Play speech: heard something 
			if (!b_friendNearby)
			{
				if (AI_AlertNum >= thresh_2)
				{
					bark( "snd_alert2h" );
				}
				else if (AI_AlertNum >= thresh_1)
				{
					bark( "snd_alert1h" );
				}
			}
			else
			{
				if (AI_AlertNum >= thresh_2)
				{
					bark( "snd_alert2ch" );
				}
				else if (AI_AlertNum >= thresh_1)
				{
					bark( "snd_alert1ch" );
				}
			}
		}
		else if ((stimBarkType >= 0.9) && (stimBarkType <= 1.1))
		{
			// Play speech: saw something
			if (!b_friendNearby)
			{
				if (AI_AlertNum >= thresh_3)
				{
					bark ("snd_alert3s" );
				}
				if (AI_AlertNum >= thresh_2)
				{
					bark( "snd_alert2s" );
				}
				else if (AI_AlertNum >= thresh_1)
				{
					bark( "snd_alert1s" );
				}
			}
			else
			{
				if (AI_AlertNum >= thresh_2)
				{
					bark( "snd_alert2cs" );
				}
				else if (AI_AlertNum >= thresh_1)
				{
					bark( "snd_alert1cs" );
				}
			}
	
		}
	}

	// Done stimulus barks	*/
}

bool BasicMind::IsEnemy(idEntity* entity, idAI* self)
{
	if (entity == NULL)
	{
		// The NULL pointer is not your enemy! As long as you remember to check for it to avoid crashes.
		return false;
	}
	else if (entity->IsType(idAbsenceMarkerEntity::Type))
	{
		idAbsenceMarkerEntity* marker = static_cast<idAbsenceMarkerEntity*>(entity);
		return gameLocal.m_RelationsManager->IsEnemy(self->team, marker->ownerTeam);
	}
	else
	{
		// Ordinary entity, pass the call to the idAI::IsEnemy method
		return self->IsEnemy(entity);
	}
}

bool BasicMind::SetTarget()
{
	// greebo: Ported from ai_darkmod_base::setTarget() written by SZ
	idAI* owner = _owner.GetEntity();
	assert(owner);

	idActor* target = NULL;

	// NOTE: To work properly, the priority here must be: check tactile first, then sight.
	
	// Done if we already have a target
	if (owner->GetEnemy() != NULL)
	{
		//DEBUG_PRINT ("Target already assigned, using that one");
		return true;
	}

	// If the AI touched you, you're a target
	if (owner->AI_TACTALERT)
	{
		idEntity* tactEnt = owner->GetTactEnt();

		if (!target->IsType(idActor::Type)) 
		{
			// Invalid enemy type, todo?
			DM_LOG(LC_AI, LT_ERROR).LogString("Tactile entity is of wrong type: %s\r", target->name.c_str());
			return false;
		}

		target = static_cast<idActor*>(tactEnt);
		
		/** 
		* If the entity that bumped the AI is an inanimate object, isEnemy will return 0,
		* so the AI will not try to attack an inanimate object.
		**/
		if (IsEnemy(target, owner))
		{
			owner->SetEnemy(target);
			
			DM_LOG(LC_AI, LT_INFO).LogString("Set tactile alert enemy to entity %s\r", target->name.c_str());

			// set the bool back
			owner->AI_TACTALERT = false;
			return true;
		}
		else
		{
			// They bumped into a non entity, so they should ignore it and not set an
			// alert from it.
			// set the bool back (man, this is annoying)
			owner->AI_TACTALERT = false;
			return false;
		}
	}
	// If the AI saw you, you're a target
	else if (owner->AI_VISALERT)
	{	
		target = owner->FindEnemy(false);

		if (target != NULL)
		{
			owner->SetEnemy(target);
			return true;
		}
		else
		{
			target = owner->FindEnemyAI(false);

			if (target != NULL)
			{
				owner->SetEnemy(target);
				return true;
			}
		}
		
		DM_LOG(LC_AI, LT_INFO).LogString("No target\r");
		
		return false;
	}
	/*
	* Sound is the only thing that does not guarantee combat
	* The AI will just stay in the highest alert state and
	* run at the sound location until it bumps into something
	* (No cheating here!)
	*/
	else if (owner->AI_HEARDSOUND)
	{
		// do not set HEARDSOUND to false here because we still want to use it after exit
		return false;
	}

	// something weird must have happened
	return false;
}

void BasicMind::PerformCombatCheck()
{
	idAI* owner = _owner.GetEntity();
	assert(owner);

	Memory& memory = owner->GetMind()->GetMemory();

	// Check for an enemy, if this returns TRUE, we have an enemy
	bool targetFound = SetTarget();
	
	if (targetFound)
	{
		DM_LOG(LC_AI, LT_INFO).LogString("COMBAT NOW!\r");
		
		// Spotted an enemy
		memory.enemiesHaveBeenSeen = true;
		
		idActor* enemy = owner->GetEnemy();

		memory.lastEnemyPos = enemy->GetPhysics()->GetOrigin();
		
		// TODO: Switch to combat state.
		// in Combat::Init: Issue communication, check for fleeing

		// Issue a communication stim
		owner->IssueCommunication_Internal(
			static_cast<float>(CAIComm_Message::DetectedEnemy_CommType), 
			YELL_STIM_RADIUS, 
			NULL,
			enemy,
			memory.lastEnemyPos
		);

		// greebo: Check for weapons and flee if we are unarmed.
		if (owner->GetNumMeleeWeapons() == 0 && owner->GetNumRangedWeapons() == 0)
		{
			DM_LOG(LC_AI, LT_INFO).LogString("I'm unarmed, I'm afraid!\r");
			// TODO pushTaskIfHighestPriority("task_Flee", PRIORITY_FLEE);
			return;
		}

		// greebo: Check for civilian AI, which will always flee in face of a combat (this is a temporary query)
		if (owner->spawnArgs.GetBool("is_civilian", "0"))
		{
			DM_LOG(LC_AI, LT_INFO).LogString("I'm civilian. I'm afraid.\r");
			// TODO pushTaskIfHighestPriority("task_Flee", PRIORITY_FLEE);
			return;
		}
	
		// TODO: Implement move to enemy task and attach to movement subsystem

		// Try to set up movement path to enemy
		owner->MoveToEnemy();
		
		/*if( !AI_DEST_UNREACHABLE && canReachEnemy() )
		{	
			pushTaskIfHighestPriority("task_Combat", PRIORITY_COMBAT);
		}
		else
 		{
 			// TODO: find alternate path, etc

 			// Do we have a ranged weapon?
 			if (m_HasRangedWeapon)
 			{
 				// Just use ranged weapon
 				pushTaskIfHighestPriority("task_Combat", PRIORITY_COMBAT);
 			}
 			else
 			{
				// Can't reach the target
				pushTaskIfHighestPriority("task_TargetCannotBeReached", PRIORITY_CANNOTREACHTARGET);
			}
		}
	
		return;	*/
	}

	// If we got here there is no target
	DM_LOG(LC_AI, LT_INFO).LogString("No Target to justify combat alert level, lowering to agitated search\r");
		
	// Lower alert level from combat to agitated search
	owner->Event_SetAlertLevel(owner->thresh_combat - 0.01);
}

void BasicMind::PerformSensoryScan(bool processNewStimuli)
{
	// greebo: Ported from ai_darkmod_base::subFrameTask_canSwitchState_sensoryScan() written by SZ

	idAI* owner = _owner.GetEntity(); 
	assert(owner != NULL);

	// Create a shortcut reference
	Memory& memory = owner->GetMind()->GetMemory();

	// Test if alerted
	if (owner->AI_ALERTED)
	{
		// Process alert flags for combat or stimulus location (both destroy flag values)?
		if (owner->AI_AlertNum >= owner->thresh_combat)
		{
			// This reflects the alert level inside it as well
			//DEBUG_PRINT ("Initiating combat due to stim");
			PerformCombatCheck();
		}
		
		// If it was not a combat level alert, or we returned here because there
		// was no target, set the alert position
		// TODO setAlertPos();
		
		// Are we searching out new alerts		
		if (processNewStimuli)
		{
			// Is this alert far enough away from the last one we reacted to to
			// consider it a new alert? Visual alerts are highly compelling and
			// are always considered new
			idVec3 newAlertDeltaFromLastOneSearched(memory.alertPos - memory.lastAlertPosSearched);
			float alertDeltaLength = newAlertDeltaFromLastOneSearched.LengthFast();
			
			if (memory.alertType == EAlertVisual || alertDeltaLength > memory.alertRadius)
			{
				// This is a new alert
				// SZ Dec 30, 2006
				// Note changed this from thresh_1 to thresh_2 to match thresh designers intentions
				if (owner->AI_AlertNum >= owner->thresh_2)
				{
					if (memory.alertType == EAlertVisual)
					{
						// Visual stimuli are locatable enough that we should
						// search the exact stim location first
						memory.stimulusLocationItselfShouldBeSearched = true;
					}
					else
					{
						// Don't bother to search direct stim location as we don't know 
						// exactly where the stim is
						memory.stimulusLocationItselfShouldBeSearched = false;
					}
					
					// One more piece of evidence of something out of place
					memory.countEvidenceOfIntruders++;
				
					// Do new reaction to stimulus
					memory.searchingDueToCommunication = false;

					// TODO
					//pushTaskIfHighestPriority("task_ReactingToStimulus", PRIORITY_REACTTOSTIMULUS);
					return;
				}	
			} // Not too close to last stimulus or is visual stimulus
		} // Not ignoring new stimuli
	}
}

void BasicMind::Save(idSaveGame* savefile) const 
{
	_owner.Save(savefile);
	
	// Save the task, if there is an active one
	savefile->WriteBool(_state != NULL);
	if (_state != NULL)
	{
		savefile->WriteString(_state->GetName().c_str());
		_state->Save(savefile);
	}

	_memory.Save(savefile);
}

void BasicMind::Restore(idRestoreGame* savefile) 
{
	_owner.Restore(savefile);

	bool hasState;
	savefile->ReadBool(hasState);

	if (hasState)
	{
		idStr stateName;
		savefile->ReadString(stateName);

		_state = StateLibrary::Instance().CreateInstance(stateName.c_str());

		assert(_state != NULL);
		_state->Restore(savefile);
	}
	else
	{
		// Assure the state pointer to be NULL.
		_state = StatePtr();
	}

	_memory.Restore(savefile);
}

} // namespace ai
