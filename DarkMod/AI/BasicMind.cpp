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

void BasicMind::PerformSensoryScan(bool processNewStimuli)
{
	// greebo: Ported from ai_darkmod_base::subFrameTask_canSwitchState_sensoryScan() written by SZ

	idAI* owner = _owner.GetEntity(); 
	assert(owner != NULL);

	/*vector newAlertDeltaFromLastOneSearched;
	vector tempVector;
	float alertDeltaLength;
	float xComponent;
	float yComponent;
	float zComponent;*/

	// Test if alerted
	if (owner->AI_ALERTED)
	{
		// Process alert flags for combat or stimulus location (both destroy flag values)?
		/*if (AI_AlertNum >= thresh_combat)
		{
			// This reflects the alert level inside it as well
			//DEBUG_PRINT ("Initiating combat due to stim");
			subFrameTask_canSwitchState_initiateCombat();
		}
		
		// If it was not a combat level alert, or we returned here because there
		// was no target, set the alert position
		setAlertPos();*/
		
		// Are we searching out new alerts		
		if (processNewStimuli)
		{
			
			// Is this alert far enough away from the last one we reacted to to
			// consider it a new alert? Visual alerts are highly compelling and
			// are always considered new
			/*newAlertDeltaFromLastOneSearched = (m_alertPos - AI_lastAlertPosSearched);
			alertDeltaLength = sys.vecLength(newAlertDeltaFromLastOneSearched);
			
			if ((m_alertType == "v") || (alertDeltaLength > m_alertRadius))
			{
				
			
				// This is a new alert 
				// SZ Dec 30, 2006
				// Note changed this from thresh_1 to thresh_2 to match thresh designers intentions
				if (AI_AlertNum >= thresh_2)
				{
				
					if (m_alertType == "v")
					{
						// Visual stimuli are locatable enough that we should
						// search the exact stim location first
						b_stimulusLocationItselfShouldBeSearched = true;
					}
					else
					{
						// Don't bother to search direct stim location as we don't know 
						// exactly where the stim is
						b_stimulusLocationItselfShouldBeSearched = false;
					}
					
					// One more piece of evidence of something out of place
					stateOfMind_count_evidenceOfIntruders = stateOfMind_count_evidenceOfIntruders + 1.0;
				
					// Do new reaction to stimulus
					waitFrame();
					b_searchingDueToCommunication = false;
					pushTaskIfHighestPriority("task_ReactingToStimulus", PRIORITY_REACTTOSTIMULUS);
					return;
				}	
			
			}*/ // Not too close to last stimulus or is visual stimulus
		
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
