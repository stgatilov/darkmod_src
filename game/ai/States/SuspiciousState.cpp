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

#include "precompiled_game.h"
#pragma hdrstop

static bool versioned = RegisterVersionedFile("$Id$");

#include "SuspiciousState.h"
#include "../Memory.h"
#include "../../AIComm_Message.h"
#include "../Tasks/RandomHeadturnTask.h"
#include "../Tasks/SingleBarkTask.h"
#include "SearchingState.h"
#include "../Library.h"

namespace ai
{

// Get the name of this state
const idStr& SuspiciousState::GetName() const
{
	static idStr _name(STATE_SUSPICIOUS);
	return _name;
}

bool SuspiciousState::CheckAlertLevel(idAI* owner)
{
	if (owner->AI_AlertIndex < ESuspicious)
	{
		// Alert index is too low for this state, fall back
		owner->GetMind()->EndState();

		// grayman #2866 - If there's a nearby door that was suspicious, close it now

		// If returning to what we were doing before takes us through
		// the door, don't close it here. It will get closed when we're on the other
		// side and close it normally.

		// If it doesn't take us through the door, then close it here.

		// To find out if our path takes us through this door,
		// compare which side of the door we were on when we first saw
		// the door with which side we're on now.

		// If, before we put ourselves on the door queue to handle it, we find that
		// others are on the queue, then we don't need to close the door.

		Memory& memory = owner->GetMemory();
		CFrobDoor* door = memory.closeMe.GetEntity();
		if ( door != NULL )
		{
			if ( door->GetUserManager().GetNumUsers() > 0 )
			{
				return false; // others are queued up to use the door, so quit
			}

			// grayman #3104 - if I'm handling a door now, I won't be able to initiate
			// handling of the suspicious door in order to close it. So I'll forget about
			// doing that, and let the suspicious door stim me again. Perhaps I'll see it
			// later.

			if ( owner->m_HandlingDoor )
			{
				CFrobDoor* currentDoor = owner->GetMemory().doorRelated.currentDoor.GetEntity();
				if ( currentDoor && ( currentDoor != door ) )
				{
					memory.closeMe = NULL;
					memory.closeSuspiciousDoor = false;
					door->SetSearching(NULL);
					door->AllowResponse(ST_VISUAL,owner); // respond to the next stim
					return false;
				}
			}

			memory.closeFromAwayPos = false; // close from the side the door swings toward
			if ( memory.susDoorSameAsCurrentDoor )
			{
				if ( memory.doorSwingsToward )
				{
					memory.closeFromAwayPos = true; // close from the side the door swings away from
				}
			}
			else
			{
				if ( !memory.doorSwingsToward )
				{
					memory.closeFromAwayPos = true; // close from the side the door swings away from
				}
			}

			// grayman #2866 - check for custom door handling positions

			idEntityPtr<idEntity> frontPos;
			idEntityPtr<idEntity> backPos;

			idList< idEntityPtr<idEntity> > list;

			if ( door->GetDoorHandlingEntities( owner, list ) ) // for doors that use door handling positions
			{
				frontPos = list[0];
				backPos = list[1];
				memory.closeFromAwayPos = false;
				if ( memory.backPos == backPos ) // on same side of door that we were when we spotted the suspicious door?
				{
					if ( memory.susDoorSameAsCurrentDoor )
					{
						memory.closeFromAwayPos = true; // grayman #2866
					}
				}
				else // on different side of door than when we spotted the suspicious door
				{
					if ( !memory.susDoorSameAsCurrentDoor )
					{
						memory.closeFromAwayPos = true;
					}
				}
			}

			memory.closeSuspiciousDoor = true; // grayman #2866
			OnFrobDoorEncounter(door); // set up the door handling task, so you can close the door
		}

		return false;
	}

	if (owner->AI_AlertIndex > ESuspicious)
	{
		// grayman #3069 - some AI don't search, so don't allow
		// them to rise into higher alert indices

		if ( owner->m_canSearch )
		{
			// Alert index is too high, switch to the higher State
			owner->GetMind()->PushState(owner->backboneStates[ESearching]);
			return false;
		}

		owner->SetAlertLevel(owner->thresh_3 - 0.1); // set alert level to just under ESearching
	}

	// Alert Index is matching, return OK
	return true;
}

void SuspiciousState::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("SuspiciousState initialised.\r");
	assert(owner);

	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner))
	{
		return;
	}

	float alertTime = owner->atime2 + owner->atime2_fuzzyness * (gameLocal.random.RandomFloat() - 0.5);
	_alertLevelDecreaseRate = (owner->thresh_3 - owner->thresh_2) / alertTime;

	// Shortcut reference
	Memory& memory = owner->GetMemory();

	owner->senseSubsystem->ClearTasks();
	owner->actionSubsystem->ClearTasks();

	// grayman #3438 - kill the repeated bark task
	owner->commSubsystem->ClearTasks();

	if (owner->GetMoveType() == MOVETYPE_SLEEP)
	{
		owner->GetUp();
	}
	
	if (gameLocal.random.RandomFloat() > 0.5f)
	{
		owner->movementSubsystem->ClearTasks();
		owner->StopMove(MOVE_STATUS_DONE);
		memory.stopRelight = true; // grayman #2603 - abort a relight in progress
		memory.stopExaminingRope = true; // grayman #2872 - stop examining rope
		memory.stopReactingToHit = true; // grayman #2816

		if ( memory.alertPos.x != idMath::INFINITY ) // grayman #3438
		{
			if ( !owner->CheckFOV(memory.alertPos) && ( owner->GetMoveType() == MOVETYPE_ANIM ) )
			{
				// Search spot is not within FOV, turn towards the position
				// don't turn while sitting
				owner->TurnToward(memory.alertPos);
			}
		}
	}

	if ( memory.alertPos.x != idMath::INFINITY ) // grayman #3438
	{
		// In any case, look at the point to investigate
		owner->Event_LookAtPosition(memory.alertPos, 2.0f);
	}

	// Play bark if alert level is ascending

	if (owner->AlertIndexIncreased())
	{
		if ( !memory.alertedDueToCommunication ) // grayman #2920
		{
			// barking
			idStr bark;

			if ((memory.alertClass == EAlertVisual_1) ||
				(memory.alertClass == EAlertVisual_2) ||
				(memory.alertClass == EAlertVisual_3)) // grayman #2603, #3424
			{
				bark = "snd_alert1s";
			}
			else if (memory.alertClass == EAlertAudio)
			{
				bark = "snd_alert1h";
			}
			else
			{
				bark = "snd_alert1";
			}
			owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new SingleBarkTask(bark)));
		}
		else
		{
			memory.alertedDueToCommunication = false; // reset
		}
	}

	// Let the AI update their weapons (make them nonsolid)
	owner->UpdateAttachmentContents(false);
}

// Gets called each time the mind is thinking
void SuspiciousState::Think(idAI* owner)
{
	UpdateAlertLevel();
	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner))
	{
		return;
	}
	
	// Let the AI check its senses
	owner->PerformVisualScan();
}


StatePtr SuspiciousState::CreateInstance()
{
	return StatePtr(new SuspiciousState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar suspiciousStateRegistrar(
	STATE_SUSPICIOUS, // Task Name
	StateLibrary::CreateInstanceFunc(&SuspiciousState::CreateInstance) // Instance creation callback
);

} // namespace ai
