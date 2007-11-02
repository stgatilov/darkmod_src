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
#include "States/ReactingToStimulusState.h"
#include "Tasks/SingleBarkTask.h"
#include "States/CombatState.h"
#include "Library.h"
#include "../idAbsenceMarkerEntity.h"
#include "../AIComm_Message.h"

namespace ai
{

// This is the default state
#define STATE_DEFAULT STATE_IDLE

BasicMind::BasicMind(idAI* owner) :
	_subsystemIterator(SubsystemCount),
	_switchState(false)
{
	// Set the idEntityPtr
	_owner = owner;
}

void BasicMind::Think()
{

	// Clear the recyclebin, it might hold a finished state from the last frame
	_recycleBin = StatePtr();

	if (_stateQueue.empty())
	{
		// We start with the idle state
		PushState(STATE_DEFAULT);
	}

	// At this point, we MUST have a State
	assert(_stateQueue.size() > 0);

	const StatePtr& state = _stateQueue.front();

	// greebo: We do not check for NULL pointers in the owner at this point, 
	// as this method is called by the owner itself, it _has_ to exist.
	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	// Thinking
	DM_LOG(LC_AI, LT_INFO).LogString("Mind is thinking... %s\r", owner->name.c_str());


	// Should we switch states (i.e. initialise a new one)?
	if (_switchState)
	{
		// Clear the flag
		_switchState = false;

		// Initialise the state, this will put the Subsystem Tasks in-place
		state->Init(owner);
	}

	if (!_switchState)
	{
		// Let the State do its monitoring task
		state->Think(owner);
	}

	// Try to perform the subsystem tasks, skipping inactive subsystems
	// Maximum number of tries is SubsystemCount.
	for (int i = 0; i < static_cast<int>(SubsystemCount); i++)
	{
		// Increase the iterator and wrap around, if necessary
		_subsystemIterator = static_cast<SubsystemId>(
			(static_cast<int>(_subsystemIterator) + 1) % static_cast<int>(SubsystemCount)
		);

		// Subsystems return TRUE when their task was executed
		if (owner->GetSubsystem(_subsystemIterator)->PerformTask())
		{
			// Task performed, break, iterator will be increased next round
			break;
		}
	}

	// Check if we can decrease the alert level
	TestAlertStateTimer();
}

void BasicMind::PushState(const idStr& stateName)
{
	// Get a new state with the given name
	StatePtr newState = StateLibrary::Instance().CreateInstance(stateName.c_str());

	if (newState != NULL)
	{
		// Push the state to the front of the queue
		_stateQueue.push_front(newState);

		// Trigger a stateswitch next round
		_switchState = true;
	}
	else
	{
		gameLocal.Error("BasicMind: Could not push state %s", stateName.c_str());
	}
}

bool BasicMind::PushStateIfHigherPriority(const idStr& stateName, int priority)
{
	if (_stateQueue.size() > 0) 
	{
		const StatePtr& curState = _stateQueue.front();

		if (curState->GetPriority() < priority)
		{
			// Priority of the current task is lower, take the new one
			SwitchState(stateName);
			return true;
		}
	}
	else
	{
		// No tasks in the queue, take this one immediately
		PushState(stateName);
		return true;
	}

	return false;
}

bool BasicMind::EndState()
{
	if (_stateQueue.empty())
	{
		// No states to end, add the default state at least
		PushState(STATE_DEFAULT);
		return true;
	}

	// Don't destroy the State object this round
	_recycleBin = _stateQueue.front();

	// Remove the current state from the queue
	_stateQueue.pop_front();

	// Trigger a stateswitch next round
	_switchState = true;

	// Return TRUE if there are additional states left
	return !_stateQueue.empty();
}

void BasicMind::SwitchState(const idStr& stateName)
{
	// greebo: Switch the state without destroying the current State object immediately
	if (_stateQueue.size() > 0)
	{
		// Store the shared_ptr in the temporary container
		_recycleBin = _stateQueue.front();
		// Remove the first element from the queue
		_stateQueue.pop_front();
	}

	// Add the new task
	PushState(stateName);
}

bool BasicMind::SwitchStateIfHigherPriority(const idStr& stateName, int priority)
{
	// greebo: Switch the state without destroying the State object immediately

	if (_stateQueue.size() > 0)
	{
		// Store the shared_ptr in the temporary container
		_recycleBin = _stateQueue.front();
		// Remove the first element from the queue
		_stateQueue.pop_front();
	}

	// Switch to the new State (conditionally)
	bool stateInstalled = PushStateIfHigherPriority(stateName, priority);

	if (!stateInstalled && _recycleBin != NULL)
	{
		// State could not be pushed, revert the queue
		_stateQueue.push_front(_recycleBin);
		
		// Prevent re-initialisation of the old state
		_switchState = false;
	}

	return stateInstalled;
}

void BasicMind::QueueState(const idStr& stateName)
{
	// Get a new state with the given name
	StatePtr newState = StateLibrary::Instance().CreateInstance(stateName.c_str());

	if (newState != NULL)
	{
		if (_stateQueue.empty())
		{
			// This is the only task, let's switch states
			_switchState = true;
		}

		// Append the state at the end of the queue
		_stateQueue.push_back(newState);
	}
	else
	{
		gameLocal.Error("BasicMind: Could not enqueue state %s", stateName.c_str());
	}
}

void BasicMind::ClearStates()
{
	_switchState = true;
	_stateQueue.clear();
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
	Memory& memory = owner->GetMemory();

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

	// Handle stimulus "barks"
	if 
	(
		(stimBarkType >= 1) && 
		( (MS2SEC(gameLocal.time) - owner->AI_timeOfLastStimulusBark) >= MINIMUM_SECONDS_BETWEEN_STIMULUS_BARKS)
	)
	{
		owner->AI_timeOfLastStimulusBark = MS2SEC(gameLocal.time);

		// Check for any friends nearby we might want to talk to
		bool b_friendNearby = false;
		if ( (MS2SEC(gameLocal.time) - memory.lastTimeFriendlyAISeen) <= MAX_FRIEND_SIGHTING_SECONDS_FOR_ACCOMPANIED_ALERT_BARK )
		{
			DM_LOG(LC_AI,LT_INFO).LogString("Time since friend is %d\r", (MS2SEC(gameLocal.time) - memory.lastTimeFriendlyAISeen));
			b_friendNearby = true;
		}

	
		if (stimBarkType == 2) 
		{
			// Play speech: heard something 
			if (!b_friendNearby)
			{
				if (owner->AI_AlertNum >= owner->thresh_2)
				{
					Bark( "snd_alert2h" );
				}
				else if (owner->AI_AlertNum >= owner->thresh_1)
				{
					Bark( "snd_alert1h" );
				}
			}
			else
			{
				if (owner->AI_AlertNum >= owner->thresh_2)
				{
					Bark( "snd_alert2ch" );
				}
				else if (owner->AI_AlertNum >= owner->thresh_1)
				{
					Bark( "snd_alert1ch" );
				}
			}
		}
		else if (stimBarkType == 1) 
		{
			// Play speech: saw something
			if (!b_friendNearby)
			{
				if (owner->AI_AlertNum >= owner->thresh_3)
				{
					Bark ("snd_alert3s" );
				}
				if (owner->AI_AlertNum >= owner->thresh_2)
				{
					Bark( "snd_alert2s" );
				}
				else if (owner->AI_AlertNum >= owner->thresh_1)
				{
					Bark( "snd_alert1s" );
				}
			}
			else
			{
				if (owner->AI_AlertNum >= owner->thresh_2)
				{
					Bark( "snd_alert2cs" );
				}
				else if (owner->AI_AlertNum >= owner->thresh_1)
				{
					Bark( "snd_alert1cs" );
				}
			}
	
		}
	}

	// Done stimulus barks	
}

void BasicMind::Bark(const idStr& soundname)
{
	idAI* owner = _owner.GetEntity();

	// Clear out any previous tasks in the commsystem
	owner->GetSubsystem(SubsysCommunication)->ClearTasks();

	// Allocate a singlebarktask, set the sound and enqueue it
	owner->GetSubsystem(SubsysCommunication)->PushTask(
		TaskPtr(new SingleBarkTask(soundname))
	);
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

		if (!tactEnt->IsType(idActor::Type)) 
		{
			// Invalid enemy type, todo?
			DM_LOG(LC_AI, LT_ERROR).LogString("Tactile entity is of wrong type: %s\r", tactEnt->name.c_str());
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

bool BasicMind::PerformCombatCheck()
{
	idAI* owner = _owner.GetEntity();
	assert(owner);

	Memory& memory = owner->GetMemory();

	// Check for an enemy, if this returns TRUE, we have an enemy
	bool targetFound = SetTarget();
	
	if (targetFound)
	{
		DM_LOG(LC_AI, LT_INFO).LogString("COMBAT NOW!\r");
		
		// Spotted an enemy
		memory.enemiesHaveBeenSeen = true;
		
		idActor* enemy = owner->GetEnemy();

		memory.lastEnemyPos = enemy->GetPhysics()->GetOrigin();
		
		StatePtr combatState = CombatState::CreateInstance();
		SwitchState(STATE_COMBAT);

		return true; // entered combat mode
	}

	// If we got here there is no target
	DM_LOG(LC_AI, LT_INFO).LogString("No Target to justify combat alert level, lowering to agitated search\r");
		
	// Lower alert level from combat to agitated search
	owner->Event_SetAlertLevel(owner->thresh_combat - 0.01);

	return false; // combat mode not entered
}

void BasicMind::PerformSensoryScan(bool processNewStimuli)
{
	// greebo: Ported from ai_darkmod_base::subFrameTask_canSwitchState_sensoryScan() written by SZ

	idAI* owner = _owner.GetEntity(); 
	assert(owner != NULL);

	// Create a shortcut reference
	Memory& memory = owner->GetMemory();

	// Test if alerted
	if (owner->AI_ALERTED)
	{
		// Process alert flags for combat or stimulus location (both destroy flag values)?
		if (owner->AI_AlertNum >= owner->thresh_combat)
		{
			// This reflects the alert level inside it as well
			//DEBUG_PRINT ("Initiating combat due to stim");
			if (PerformCombatCheck())
			{
				// Combat state entered, quit processing
				return;
			}
		}
		
		// If it was not a combat level alert, or we returned here because there
		// was no target, set the alert position
		SetAlertPos();
		
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

					// Switch to reacting stimulus task
					SwitchStateIfHigherPriority(STATE_REACTING_TO_STIMULUS, PRIORITY_REACTING_TO_STIMULUS);
					return;
				}	
			} // Not too close to last stimulus or is visual stimulus
		} // Not ignoring new stimuli
	}
}

void BasicMind::Save(idSaveGame* savefile) const 
{
	_owner.Save(savefile);
	_stateQueue.Save(savefile);
	_memory.Save(savefile);
}

void BasicMind::Restore(idRestoreGame* savefile) 
{
	_owner.Restore(savefile);
	_stateQueue.Restore(savefile);
	_memory.Restore(savefile);
}

} // namespace ai
