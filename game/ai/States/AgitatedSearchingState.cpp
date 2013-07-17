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

#include "AgitatedSearchingState.h"
#include "../Memory.h"
#include "../Tasks/InvestigateSpotTask.h"
#include "../Tasks/SingleBarkTask.h"
#include "../Tasks/RepeatedBarkTask.h"
#include "../Tasks/WaitTask.h"
#include "CombatState.h"
#include "../Library.h"
#include "../../AbsenceMarker.h"
#include "../../AIComm_Message.h"

namespace ai
{

// Get the name of this state
const idStr& AgitatedSearchingState::GetName() const
{
	static idStr _name(STATE_AGITATED_SEARCHING);
	return _name;
}

bool AgitatedSearchingState::CheckAlertLevel(idAI* owner)
{
	if (!owner->m_canSearch) // grayman #3069 - AI that can't search shouldn't be here
	{
		owner->SetAlertLevel(owner->thresh_3 - 0.1);
	}

	if (owner->AI_AlertIndex < EAgitatedSearching)
	{
		// Alert index is too low for this state, fall back
		owner->GetMind()->EndState();
		return false;
	}

	// grayman #3009 - can't enter this state if sitting, sleeping,
	// sitting down, lying down, or getting up from sitting or sleeping

	moveType_t moveType = owner->GetMoveType();
	if ( moveType == MOVETYPE_SIT      || 
		 moveType == MOVETYPE_SLEEP    ||
		 moveType == MOVETYPE_SIT_DOWN ||
		 moveType == MOVETYPE_LAY_DOWN )
	{
		owner->GetUp(); // it's okay to call this multiple times
		owner->GetMind()->EndState();
		return false;
	}

	if ( ( moveType == MOVETYPE_GET_UP ) ||	( moveType == MOVETYPE_GET_UP_FROM_LYING ) )
	{
		owner->GetMind()->EndState();
		return false;
	}

	if (owner->AI_AlertIndex > EAgitatedSearching)
	{
		// Alert index is too high, switch to the higher State
		owner->Event_CloseHidingSpotSearch();
		owner->GetMind()->PushState(owner->backboneStates[ECombat]);
		return false;
	}

	// Alert Index is matching, return OK
	return true;
}

void AgitatedSearchingState::CalculateAlertDecreaseRate(idAI* owner)
{
	float alertTime = owner->atime4 + owner->atime4_fuzzyness * (gameLocal.random.RandomFloat() - 0.5);
	_alertLevelDecreaseRate = (owner->thresh_5 - owner->thresh_4) / alertTime;
}

void AgitatedSearchingState::Init(idAI* owner)
{
	// Init base class first (note: we're not calling SearchingState::Init() on purpose here)
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("AgitatedSearchingState initialised.\r");
	assert(owner);

	// Ensure we are in the correct alert level
	if ( !CheckAlertLevel(owner) )
	{
		return;
	}

	// Shortcut reference
	Memory& memory = owner->GetMemory();

	CalculateAlertDecreaseRate(owner);
	
	if (owner->GetMoveType() == MOVETYPE_SIT || owner->GetMoveType() == MOVETYPE_SLEEP)
	{
		owner->GetUp();
	}

	// Setup a new hiding spot search
	StartNewHidingSpotSearch(owner);

	// kill the repeated bark task
	owner->commSubsystem->ClearTasks(); // grayman #3182

	CommMessagePtr message = CommMessagePtr(new CommMessage(
		CommMessage::DetectedSomethingSuspicious_CommType, 
		owner, NULL, // from this AI to anyone
		NULL,
		memory.alertPos,
		memory.currentSearchEventID // grayman #3438
	));

	if (owner->AlertIndexIncreased())
	{
		if ( ( memory.alertedDueToCommunication == false ) && ( ( memory.alertType == EAlertTypeSuspicious ) || ( memory.alertType == EAlertTypeEnemy ) ) )
		{
			idStr soundName = "";
			if (owner->HasSeenEvidence())
			{
				soundName = "snd_alert4";
			}
			else
			{
				soundName = "snd_alert4NoEvidence";
			}

			owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new SingleBarkTask(soundName,message)));

			if (cv_ai_debug_transition_barks.GetBool())
			{
				gameLocal.Printf("%d: %s rises to Agitated Searching state, barks '%s'\n",gameLocal.time,owner->GetName(),soundName.c_str());
			}
		}
	}

	owner->commSubsystem->AddSilence(5000 + gameLocal.random.RandomInt(3000)); // grayman #3424

	int minTime = SEC2MS(owner->spawnArgs.GetFloat("searchbark_delay_min", "10"));
	int maxTime = SEC2MS(owner->spawnArgs.GetFloat("searchbark_delay_max", "15"));

	if (owner->HasSeenEvidence())
	{
		memory.prevSawEvidence = true; // grayman #3424
		owner->commSubsystem->AddCommTask(
			CommunicationTaskPtr(new RepeatedBarkTask("snd_state4SeenEvidence", minTime, maxTime, message))
		);
	}
	else
	{
		memory.prevSawEvidence = false; // grayman #3424
		owner->commSubsystem->AddCommTask(
			CommunicationTaskPtr(new RepeatedBarkTask("snd_state4SeenNoEvidence", minTime, maxTime, message))
		);
	}
	
	// grayman #3331 - draw your ranged weapon if you have one, otherwise draw your melee weapon.
	// Note that either weapon could be drawn, but if we default to melee, AI with ranged and
	// melee weapons will draw their melee weapon, and we'll never see ranged weapons get drawn.
	// Odds are that the enemy is nowhere nearby anyway, since we're just searching.

	if ( ( owner->GetNumRangedWeapons() > 0 ) && !owner->spawnArgs.GetBool("unarmed_ranged","0") )
	{
		owner->DrawWeapon(COMBAT_RANGED);
	}
	else if ( ( owner->GetNumMeleeWeapons() > 0 ) && !owner->spawnArgs.GetBool("unarmed_melee","0") )
	{
		owner->DrawWeapon(COMBAT_MELEE);
	}

	// Let the AI update their weapons (make them solid)
	owner->UpdateAttachmentContents(true);
}

// Gets called each time the mind is thinking
void AgitatedSearchingState::Think(idAI* owner)
{
	SearchingState::Think(owner);

	// grayman #3424 - If we saw evidence (for the first time) between the previous think frame and now,
	// we need to change the repeated bark.

	Memory& memory = owner->GetMemory();
	if (memory.prevSawEvidence)
	{
		// no change needed
	}
	else if (owner->HasSeenEvidence())
	{
		memory.prevSawEvidence = true;

		// Switch repeated bark

		owner->commSubsystem->ClearTasks();

		CommMessagePtr message = CommMessagePtr(new CommMessage(
			CommMessage::DetectedSomethingSuspicious_CommType, 
			owner, NULL, // from this AI to anyone
			NULL,
			memory.alertPos,
			memory.currentSearchEventID // grayman #3438
		));

		int minTime = SEC2MS(owner->spawnArgs.GetFloat("searchbark_delay_min", "10"));
		int maxTime = SEC2MS(owner->spawnArgs.GetFloat("searchbark_delay_max", "15"));

		owner->commSubsystem->AddCommTask(
			CommunicationTaskPtr(new RepeatedBarkTask("snd_state4SeenEvidence", minTime, maxTime, message))
		);
	}
}

StatePtr AgitatedSearchingState::CreateInstance()
{
	return StatePtr(static_cast<State*>(new AgitatedSearchingState));
}

// Register this state with the StateLibrary
StateLibrary::Registrar agitatedSearchingStateRegistrar(
	STATE_AGITATED_SEARCHING, // Task Name
	StateLibrary::CreateInstanceFunc(&AgitatedSearchingState::CreateInstance) // Instance creation callback
);

} // namespace ai
