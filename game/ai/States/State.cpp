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

#include "State.h"
#include "../Memory.h"
#include "../Tasks/SingleBarkTask.h"
#include "../Tasks/GreetingBarkTask.h"
#include "../Tasks/HandleDoorTask.h"
#include "../Tasks/HandleElevatorTask.h"
#include "../../AIComm_Message.h"
#include "../../StimResponse/StimResponse.h"
#include "LostTrackOfEnemyState.h" // grayman #3431
#include "SearchingState.h"
#include "CombatState.h"
#include "BlindedState.h"
#include "SwitchOnLightState.h"
#include "FailedKnockoutState.h"
#include "ExamineRopeState.h"   // grayman #2872
#include "HitByMoveableState.h" // grayman #2816
#include "FleeState.h" // grayman #3317

#include "../../BinaryFrobMover.h"
#include "../../FrobDoor.h"
#include "../../AbsenceMarker.h"

#include "../../Grabber.h"
#include "../Tasks/PlayAnimationTask.h"

#include "ConversationState.h"		// grayman #2603
#include "../../ProjectileResult.h" // grayman #2872
#include "../../BloodMarker.h"		// grayman #3075

namespace ai
{
//----------------------------------------------------------------------------------------
// The following strings define classes of person, these are used if AIUse is AIUSE_PERSON 

// This is the key value
#define PERSONTYPE_KEY				"personType"

// And these are values in use, add to this list as needed
#define PERSONTYPE_GENERIC			"PERSONTYPE_GENERIC"
#define PERSONTYPE_NOBLE			"PERSONTYPE_NOBLE"
#define PERSONTYPE_CITYWATCH		"PERSONTYPE_CITYWATCH"
#define PERSONTYPE_MERC_PROGUARD	"PERSONTYPE_MERC_PROGUARD"
#define PERSONTYPE_BUILDER			"PERSONTYPE_BUILDER"
#define PERSONTYPE_PAGAN			"PERSONTYPE_PAGAN"
#define PERSONTYPE_THIEF			"PERSONTYPE_THIEF"
#define PERSONTYPE_PRIEST			"PERSONTYPE_PRIEST"
#define PERSONTYPE_ELITE			"PERSONTYPE_ELITE"
#define PERSONTYPE_BEGGAR			"PERSONTYPE_BEGGAR" // grayman #3323

//----------------------------------------------------------------------------------------
// The following strings define genders of person, these are used if AIUse is AIUSE_PERSON 
// I don't want to get into the politics of gender identity here, this is just because the recorded
// voices will likely be in gendered languages.  As such, I'm just including the categories
// that are involved in word gender selection in many languages.
#define PERSONGENDER_KEY		"personGender"

#define PERSONGENDER_MALE		"PERSONGENDER_MALE"
#define PERSONGENDER_FEMALE		"PERSONGENDER_FEMALE"
#define PERSONGENDER_UNKNOWN	"PERSONGENDER_UNKNOWN"

//const float CHANCE_FOR_GREETING = 0.3f;		// 30% chance for greeting
const int MIN_TIME_LIGHT_ALERT = 10000;		// ms - grayman #2603
const int REMARK_DISTANCE = 200;			// grayman #2903 - no greeting or warning if farther apart than this
const int MIN_DIST_TO_LOWLIGHT_DOOR = 300;	// grayman #2959 - AI must be closer than this to "see" a low-light door
const int PERSON_NEAR_DOOR = 150;			// grayman #2959 - AI must be closer than this to a suspicious door to be considered handling it
const int BLOOD2BLEEDER_MIN_DIST = 300;		// grayman #3075 - AI must be closer than this to blood to be considered the owner

// grayman #3317 - If an AI's body is discovered w/in this amount of time after death or KO,
// the finding AI should react differently than if the body is found after this
// amount of time has passed.
const int DISCOVERY_TIME_LIMIT = 4000;		// in ms

//----------------------------------------------------------------------------------------
// grayman #2903 - no warning if the sender is farther than this horizontally from the alert spot (one per alert type)

const int WARN_DIST_ENEMY_SEEN = 800;
const int WARN_DIST_CORPSE_FOUND = 600;
const int WARN_DIST_MISSING_ITEM = 500;
const int WARN_DIST_EVIDENCE_INTRUDERS = 400;

const int WARNING_RESPONSE_DELAY = 1000; // grayman #2920 - how long to delay before responding to a warning (ms)

// grayman #3075 - when checking whether two AI have seen the same corpse, the two corpse positions
// are allowed to be up to this far apart to be considered the same corpse
//const int WARN_DIST_BODY_CAN_SHIFT = 100; // grayman #3424 - no longer used

const int WARN_DIST_MAX_Z = 100; // no warning if the sender is farther than this vertically from the alert spot (same for each alert type)
//----------------------------------------------------------------------------------------
const int VIS_STIM_DELAY_MIN =  500; // grayman #2924 - min amount of time delay (ms) before processing certain visual stims
const int VIS_STIM_DELAY_MAX = 1000; // grayman #2924 - max amount of time delay (ms) before processing certain visual stims
//----------------------------------------------------------------------------------------
// The following defines a key that should be non-0 if the device should be closed
#define AIUSE_SHOULDBECLOSED_KEY		"shouldBeClosed"

//----------------------------------------------------------------------------------------

void State::Init(idAI* owner)
{
	_owner = owner;
	_alertLevelDecreaseRate = 0;

	// Load the value from the spawnargs to avoid looking it up each frame
	owner->GetMemory().deadTimeAfterAlertRise = owner->spawnArgs.GetInt("alert_decrease_deadtime", "300");
}

bool State::CheckAlertLevel(idAI* owner)
{
	return true; // always true by default
}

void State::SetOwner(idAI* owner)
{
	_owner = owner;
}


void State::UpdateAlertLevel()
{
	idAI* owner = _owner.GetEntity();
	int currentTime = gameLocal.time;
	int thinkDuration = currentTime - owner->m_lastThinkTime;

	Memory& memory = owner->GetMemory();
	
	// angua: alert level stays for a short time before starting to decrease
	if (currentTime >= memory.lastAlertRiseTime + memory.deadTimeAfterAlertRise && 
		owner->AI_AlertLevel > 0)
	{
		float decrease = _alertLevelDecreaseRate * MS2SEC(thinkDuration);
		float newAlertLevel = owner->AI_AlertLevel - decrease;
		owner->SetAlertLevel(newAlertLevel);
	}
}


// Save/Restore methods
void State::Save(idSaveGame* savefile) const
{
	_owner.Save(savefile);

	savefile->WriteFloat(_alertLevelDecreaseRate);
}

void State::Restore(idRestoreGame* savefile)
{
	_owner.Restore(savefile);

	savefile->ReadFloat(_alertLevelDecreaseRate);
}

void State::OnVisualAlert(idActor* enemy)
{
	assert(enemy != NULL);

	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	if (owner->AI_DEAD || owner->AI_KNOCKEDOUT)
	{
		return;
	}

	Memory& memory = owner->GetMemory();

	memory.alertClass = EAlertVisual_1;
	memory.alertType = EAlertTypeSuspicious;
	idVec3 lastAlertPosSearched = memory.alertPos; // grayman #3075
	memory.alertPos = owner->GetVisDir();
	memory.alertRadius = VISUAL_ALERT_RADIUS;
	memory.alertSearchVolume = VISUAL_SEARCH_VOLUME;
	memory.alertSearchExclusionVolume.Zero();
	memory.mandatory = false; // grayman #3331
	
	// set the flag back (greebo: Is this still necessary?)
	owner->AI_VISALERT = false;

	// Is this alert far enough away from the last one we reacted to to
	// consider it a new alert? Visual alerts are highly compelling and
	// are always considered new
	idVec3 newAlertDeltaFromLastOneSearched(memory.alertPos - lastAlertPosSearched); // grayman #3075
	float alertDeltaLengthSqr = newAlertDeltaFromLastOneSearched.LengthSqr();
	
	if ( lastAlertPosSearched.Compare(idVec3(0,0,0)) || (alertDeltaLengthSqr > memory.alertSearchVolume.LengthSqr() ) ) // grayman #3075
	{
		// This is a new alert // SZ Dec 30, 2006
		// Note changed this from thresh_2 to thresh_3 to match thresh designer's intentions
		if (owner->IsSearching()) // grayman #2603
		{
			// We are in searching mode or we are switching to it, handle this new incoming alert

			// Visual stimuli are locatable enough that we should
			// search the exact stim location first
			memory.stimulusLocationItselfShouldBeSearched = true;
			
			// greebo: TODO: Each incoming stimulus == evidence of intruders?
			// One more piece of evidence of something out of place
			memory.countEvidenceOfIntruders += EVIDENCE_COUNT_INCREASE_VIS_ALERT;
			memory.posEvidenceIntruders = owner->GetPhysics()->GetOrigin(); // grayman #2903
			memory.timeEvidenceIntruders = gameLocal.time; // grayman #2903
			memory.visualAlert = true; // grayman #2422
		
			// Do new reaction to stimulus
			memory.alertedDueToCommunication = false;

			// Restart the search, in case we're already searching
			memory.restartSearchForHidingSpots = true;
		}	
	} // Not too close to last stimulus or is visual stimulus
}

void State::OnTactileAlert(idEntity* tactEnt)
{
	assert(tactEnt != NULL); // don't take NULL entities

	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	if (owner->AI_KNOCKEDOUT || owner->AI_DEAD)
	{
		return;
	}

	// If this is a projectile, fire the corresponding event
	if (tactEnt->IsType(idProjectile::Type))
	{
		// grayman #3140 - now handled by the path through
		// idProjectile::Collide()->idAI::Damage()
		//OnProjectileHit(static_cast<idProjectile*>(tactEnt));
	}
	else 
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

		// grayman #2816 - this was assuming tactEnt is an actor,
		// when it might not be. Execute the alert if this is an actor
		// who is an enemy, or if this isn't an actor (you were hit by something
		// other than a projectile).

		bool isActor = tactEnt->IsType(idActor::Type);
		bool isEnemy = ( isActor && owner->IsEnemy(tactEnt) );
		if ( !isActor || isEnemy )
		{
			Memory& memory = owner->GetMemory();
			int eventID = -1;

			if ( isEnemy )
			{
				owner->Event_SetEnemy(tactEnt);
				memory.alertType = EAlertTypeEnemy;
				eventID = owner->LogSuspiciousEvent( E_EventTypeEnemy, owner->GetPhysics()->GetOrigin(), NULL ); // grayman #3424  
			}
			else
			{
				memory.alertType = EAlertTypeSuspicious;
			}

			// grayman #3331 - if fleeing, do none of this
			if ( owner->GetMind()->GetState()->GetName() != "Flee" )
			{
				memory.alertClass = EAlertTactile;
				memory.alertPos = owner->GetPhysics()->GetOrigin();
				memory.currentSearchEventID = eventID; // grayman #3424
				memory.alertRadius = TACTILE_ALERT_RADIUS;
				memory.alertSearchVolume = TACTILE_SEARCH_VOLUME;
				memory.alertSearchExclusionVolume.Zero();
				memory.visualAlert = false; // grayman #2422
				memory.mandatory = true;	// grayman #3331

				// grayman #2816 - turn toward what hit you, not toward your origin

				owner->TurnToward(tactEnt->GetPhysics()->GetOrigin());
				owner->AI_TACTALERT = false;
			}
		}
	}
}

/* grayman #3140 - no longer used
void State::OnProjectileHit(idProjectile* projectile)
{
	idAI* owner = _owner.GetEntity();

	if (owner->AI_DEAD || owner->AI_KNOCKEDOUT)
	{
		return;
	}

	if (owner->AI_AlertLevel <= (owner->thresh_5 - 0.1f))
	{
		// Set the alert level right below combat threshold
		owner->SetAlertLevel(owner->thresh_5 - 0.1f);

		// The owner will start to search, set up the parameters
		Memory& memory = owner->GetMemory();

		memory.alertClass = EAlertTactile;
		memory.alertType = EAlertTypeDamage;

		idVec3 projVel = projectile->GetPhysics()->GetLinearVelocity();
		projVel.NormalizeFast();

		memory.alertPos = owner->GetPhysics()->GetOrigin() - projVel * 300;
		memory.alertPos.x += 200 * gameLocal.random.RandomFloat() - 100;
		memory.alertPos.y += 200 * gameLocal.random.RandomFloat() - 100;
		memory.alertRadius = LOST_ENEMY_ALERT_RADIUS;
		memory.alertSearchVolume = LOST_ENEMY_SEARCH_VOLUME;
		memory.alertSearchExclusionVolume.Zero();
		memory.visualAlert = false;
		memory.mandatory = true; // grayman #3331
	}
}
*/

void State::OnAudioAlert()
{
	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	if (owner->AI_DEAD || owner->AI_KNOCKEDOUT)
	{
		return;
	}

	Memory& memory = owner->GetMemory();
//	memory.alertClass = EAlertAudio; // grayman #3424 - move to later

	// grayman #3424 - If alertClass is not EAlertNone,
	// don't change it to EAlertAudio. Doing so causes
	// the wrong rampdown bark when the AI comes out of a search near
	// something more important, like a dead body.

	if ( memory.alertClass == EAlertNone )
	{
		memory.alertClass = EAlertAudio;
		memory.alertType = EAlertTypeSuspicious;
	}

	memory.alertPos = owner->GetSndDir();
	memory.lastAudioAlertTime = gameLocal.time;
	memory.mandatory = false; // grayman #3331

	// Search within radius of stimulus that is 1/3 the distance from the
	// observer to the point at the time heard
	float distanceToStim = (owner->GetPhysics()->GetOrigin() - memory.alertPos).LengthFast();

	// greebo: Apply a certain fuzziness to the audio alert position
	// 200 units distance corresponds to 50 units fuzziness in X/Y direction
	idVec3 start = memory.alertPos; // grayman #2422

	memory.alertPos += idVec3(
		(gameLocal.random.RandomFloat() - 0.5f)*AUDIO_ALERT_FUZZINESS,
		(gameLocal.random.RandomFloat() - 0.5f)*AUDIO_ALERT_FUZZINESS,
		0 // no fuzziness in z-direction
	) * distanceToStim / 400.0f;

	// grayman #2422 - to avoid moving the alert point into the void, or into
	// the room next door, trace from the original point to the fuzzy point. If you encounter
	// the world, move the fuzzy point there.

	trace_t result;
	idEntity *ignore = NULL;
	while ( true )
	{
		gameLocal.clip.TracePoint( result, start, memory.alertPos, MASK_OPAQUE, ignore );
		if ( result.fraction == 1.0f )
		{
			break; // reached memory.alertPos, so no need to change it
		}

		if ( result.fraction < VECTOR_EPSILON )
		{
			// no movement on this leg of the trace, so move memory.alertPos to the struck point 
			memory.alertPos = result.endpos; // move the alert point
			break;
		}

		// End the trace if we hit the world

		idEntity* entHit = gameLocal.entities[result.c.entityNum];

		if ( entHit == gameLocal.world )
		{
			memory.alertPos = result.endpos; // move the alert point
			break;
		}

		// Continue the trace from the struck point

		start = result.endpos;
		ignore = entHit; // for the next leg, ignore the entity we struck
	}

	float searchVolModifier = distanceToStim / 600.0f;
	if (searchVolModifier < 0.4f)
	{
		searchVolModifier = 0.4f;
	}

	memory.alertRadius = AUDIO_ALERT_RADIUS;
	memory.alertSearchVolume = AUDIO_SEARCH_VOLUME * searchVolModifier;
	memory.alertSearchExclusionVolume.Zero();
	memory.visualAlert = false; // grayman #2422
	
	// Reset the flag (greebo: is this still necessary?)
	owner->AI_HEARDSOUND = false;
	
	memory.stimulusLocationItselfShouldBeSearched = true;
}

// grayman #3431 - check whether we're blinded by a blind stim

bool State::CanBeBlinded(idEntity* stimSource, bool skipVisibilityCheck)
{
	idAI* owner = _owner.GetEntity();

	// Don't react if we are already blind
	if (owner->AI_DEAD || owner->AI_KNOCKEDOUT || owner->GetAcuity("vis") == 0)
	{
		return false;
	}

	bool blinded = false;

	// greebo: We don't check for alert type weights here, flashbombs are "top priority"

	if (!skipVisibilityCheck) 
	{
		// Perform visibility check
		// Check FOV first
		if (owner->CheckFOV(stimSource->GetPhysics()->GetOrigin()))
		{
			// FOV check passed, check occlusion (skip lighting check)
			if (owner->CanSeeExt(stimSource, false, false))
			{
				// Success, AI is blinded
				blinded = true;
			}
		}
		else
		{
			// greebo: FOV check might have failed, still consider near explosions of flashbombs
			// as some AI might be so close to the player that dropping a flashbomb goes outside FOV
			idVec3 distVec = (stimSource->GetPhysics()->GetOrigin() - owner->GetPhysics()->GetOrigin());
			float dist = distVec.NormalizeFast();
			
			// Check if the distance is within 100 units and the explosion is in front of us
			if ( ( dist < 100.0f ) && ( distVec * owner->viewAxis.ToAngles().ToForward() ) > 0)
			{
				blinded = true;
			}
		}
	}
	else 
	{
		// Skip visibility check
		blinded = true;
	}

	return blinded;
}

void State::OnBlindStim(idEntity* stimSource, bool skipVisibilityCheck)
{
	idAI* owner = _owner.GetEntity();

	// Don't react if we are already blind
	if (owner->AI_DEAD || owner->AI_KNOCKEDOUT || owner->GetAcuity("vis") == 0)
	{
		return;
	}

	// greebo: We don't check for alert type weights here, flashbombs are "top priority"

	Memory& memory = owner->GetMemory();

	memory.alertClass = EAlertVisual_1;
	memory.alertedDueToCommunication = false;
	memory.alertPos = stimSource->GetPhysics()->GetOrigin();
	memory.alertRadius = LOST_ENEMY_ALERT_RADIUS; // grayman #3431
	memory.alertSearchVolume = LOST_ENEMY_SEARCH_VOLUME; // grayman #3431
	memory.alertSearchExclusionVolume.Zero(); // grayman #3431
	memory.alertType = EAlertTypeWeapon;
	memory.visualAlert = false; // grayman #2422
	memory.mandatory = true;	// grayman #3331

	if ( CanBeBlinded(stimSource, skipVisibilityCheck) )
	{
		// grayman #3431 - if the AI has an enemy, queue a "lost enemy" state so it plays
		// after the blinded state completes
		if ( owner->GetEnemy() )
		{
			owner->GetMind()->PushState(STATE_LOST_TRACK_OF_ENEMY);
		}

		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("AI blinded by flash, switching to BlindedState.\r");
		owner->GetMind()->PushState(STATE_BLINDED);
	}
}

void State::OnVisualStim(idEntity* stimSource)
{
	if (cv_ai_opt_novisualstim.GetBool()) 
	{
		return;
	}

	idAI* owner = _owner.GetEntity();
	if (owner == NULL)
	{
		// Owner might not be initialised, serviceEvents is called after Mind::Think()
		return;
	}

	// gameLocal.Printf("Visual stim for %s, visual acuity %0.2f\n", owner->GetName(), owner->GetAcuity("vis") );

	// Don't respond to NULL entities or when dead/knocked out/blind
	if ((stimSource == NULL) || 
		owner->AI_KNOCKEDOUT || owner->AI_DEAD || (owner->GetAcuity("vis") == 0))
	{
		return;
	}

	// grayman #2416 - If I'm in the middle of certain animations, do nothing

	moveType_t moveType = owner->GetMoveType();
	if ( moveType == MOVETYPE_SIT_DOWN			|| // standing->sitting
		 moveType == MOVETYPE_GET_UP			|| // sitting->standing
		 moveType == MOVETYPE_GET_UP_FROM_LYING	|| // sleeping->standing
		 moveType == MOVETYPE_SLEEP				|| // sleeping
		 moveType == MOVETYPE_LAY_DOWN )		   // standing->lying down
	{
		return;
	}

	// Get AI use of the stim
	idStr aiUse = stimSource->spawnArgs.GetString("AIUse");

	// grayman #2603

	// First check the chance of seeing a particular AIUSE type. For AI that have zero chance
	// of responding to particular visual stims, don't spend time determining if they can
	// see it or need to respond to it.
	//
	// Also, it lets us quickly mark the stim 'ignore in the future'. In all AIUSE types,
	// we don't mark stims 'ignore' until we get down into the response code. If we don't
	// execute that code because the AI will never respond to the stim, we never get a
	// chance to ignore future stims.

	StimMarker aiUseType = EAIuse_Default; // marker for aiUse type
	float chanceToNotice(0);

	// These are ordered by most likely chance of encountering a stim type

	if (aiUse == AIUSE_LIGHTSOURCE)
	{
		aiUseType = EAIuse_Lightsource;
		chanceToNotice = owner->spawnArgs.GetFloat("chanceNoticeLight");
	}
	else if (aiUse == AIUSE_PERSON)
	{
		aiUseType = EAIuse_Person;
		chanceToNotice = owner->spawnArgs.GetFloat("chanceNoticePerson");
	}
	else if (aiUse == AIUSE_WEAPON)
	{
		aiUseType = EAIuse_Weapon;
		chanceToNotice = owner->spawnArgs.GetFloat("chanceNoticeWeapon");
	}
	else if (aiUse == AIUSE_DOOR)
	{
		aiUseType = EAIuse_Door;
		chanceToNotice = owner->spawnArgs.GetFloat("chanceNoticeDoor");
	}
	else if (aiUse == AIUSE_SUSPICIOUS) // grayman #1327
	{
		aiUseType = EAIuse_Suspicious;
		chanceToNotice = owner->spawnArgs.GetFloat("chanceNoticeSuspiciousItem");
	}
	else if (aiUse == AIUSE_ROPE) // grayman #2872
	{
		aiUseType = EAIuse_Rope;
		chanceToNotice = owner->spawnArgs.GetFloat("chanceNoticeRope","0.0");
	}
	else if (aiUse == AIUSE_BLOOD_EVIDENCE)
	{
		aiUseType = EAIuse_Blood_Evidence;
		chanceToNotice = owner->spawnArgs.GetFloat("chanceNoticeBlood");
	}
	else if (aiUse == AIUSE_MISSING_ITEM_MARKER)
	{
		aiUseType = EAIuse_Missing_Item_Marker;
		chanceToNotice = owner->spawnArgs.GetFloat("chanceNoticeMissingItem");
	}
	else if (aiUse == AIUSE_MONSTER) // grayman #3331
	{
		aiUseType = EAIuse_Monster;
		chanceToNotice = owner->spawnArgs.GetFloat("chanceNoticeMonster");
	}
	else if (aiUse == AIUSE_UNDEAD) // grayman #3343
	{
		aiUseType = EAIuse_Undead;
		chanceToNotice = owner->spawnArgs.GetFloat("chanceNoticeUndead");
	}
	else if (aiUse == AIUSE_BROKEN_ITEM)
	{
		aiUseType = EAIuse_Broken_Item;
		chanceToNotice = owner->spawnArgs.GetFloat("chanceNoticeBrokenItem");
	}
	else // grayman #2885 - no AIUse spawnarg, so we don't know what it is
	{
		stimSource->IgnoreResponse(ST_VISUAL, owner);
		return;
	}

	// If chanceToNotice for this stim type is zero, ignore it in the future

	if ( chanceToNotice == 0.0 )
	{
		stimSource->IgnoreResponse(ST_VISUAL, owner);
		return;
	}

	// chanceToNotice > 0, so randomly decide if there's a chance of responding

	float chance(gameLocal.random.RandomFloat());
	bool pass = true;

	// grayman #2603 - special handling for light stims

	idEntityPtr<idEntity> stimPtr;
	stimPtr = stimSource;
	if (aiUseType == EAIuse_Lightsource)
	{
		// grayman - If we ever begin to notice OFF lights that come ON in front
		// of us, this check here is the first place to catch that. It should either
		// branch off into a new way of handling OFF->ON lights, or mix handling them
		// in with the ON->OFF lights handling.

		// Ignore lights that are on.

		idLight* light = static_cast<idLight*>(stimSource);
		if ((light->GetLightLevel() > 0) && (!light->IsSmoking()))
		{
			stimSource->IgnoreResponse(ST_VISUAL,owner);
			return;
		}

		// grayman #2905 - AI should never relight or bark about lights that were spawned off
		// at map start and have a shouldBeOn value of 0.

		if ( light->GetStartedOff() )
		{
			stimSource->IgnoreResponse(ST_VISUAL,owner);
			return;
		}

		// grayman #2603 - Let's see if the AI is involved in a conversation.
		// FIXME: This might not be enough, if the AI has pushed other states on top of the conversation state
		ConversationStatePtr convState = boost::dynamic_pointer_cast<ConversationState>(owner->GetMind()->GetState());

		if (convState != NULL)
		{
			return; // we're in a conversation, so delay processing the rest of the relight
		}

		// Before we check the odds of noticing this stim, see if it belongs
		// to our doused torch. Noticing that should not be subject to
		// the probability settings.

		if (!CheckTorch(owner,light))
		{
			return;
		}

		// It might not yet be time to notice this stim. If it's on
		// our list of delayed stims, see if its delay has expired.
		// If it expired less than 20ms ago, the AI can respond to it
		// only if he's walking toward it, not away from it. This keeps
		// AI from walking past a doused light and then turning back to
		// relight it, which looks odd.

		int expired = owner->GetDelayedStimExpiration(stimPtr);
		
		if (expired > 0)
		{
			if (gameLocal.time >= expired)
			{
				if (chance > chanceToNotice)
				{
					owner->SetDelayedStimExpiration(stimPtr);
					pass = false;
				}
				else if (gameLocal.time < expired + 1000) // recently expired?
				{
					if (!owner->CanSeeExt(stimSource,true,false)) // ahead of you?
					{
						owner->SetDelayedStimExpiration(stimPtr); // behind, so try again later
						pass = false;
					}
				}
			}
			else // delay hasn't expired
			{
				pass = false;
			}
		}
		else if (chance > chanceToNotice)
		{
			owner->SetDelayedStimExpiration(stimPtr); // delay the next check of this stim
			pass = false;
		}
	}
	else if (chance > chanceToNotice) // check all other stim chances
	{
		pass = false;
	}

	if (!pass)
	{
		return; // maybe next time
	}

	// Only respond if we can actually see it

	idVec3 ropeStimSource = idVec3(idMath::INFINITY,idMath::INFINITY,idMath::INFINITY); // grayman #2872

	if (aiUseType == EAIuse_Lightsource)
	{
		// grayman #2603 -  A light sends a stim to each entity w/in its radius that's not ignoring it,
		// and it doesn't care about walls. Since AI don't care if a light is on, we can avoid the trace
		// in CanSeeExt() if the light's off.

		// We did a check above to see if the light's on. At this point, we know it's off.

		// Special case for a light. We know it's off if there's no light. Also we can notice it
		// if we are not looking right at it.

		// We can do a couple quick checks at this point:
		//
		// 1. If the AI is busy relighting a light, don't respond
		// 2. If another AI is busy relighting this light, don't respond

		// Doing these now saves us the trouble of determining if there's LOS, which involves a trace.

		// A light that's off can lead to the AI trying to turn it back on.

		// If we're already in the process of relighting a light, don't repond to this stim now.
		// We'll see it again later.
		// grayman #2872 - if we're examining a rope, don't respond to this stim now.

		idLight* light = static_cast<idLight*>(stimSource);
		if ( owner->m_RelightingLight || owner->m_ExaminingRope )
		{
			owner->SetDelayedStimExpiration(stimPtr); // delay your next check of this stim
			return;
		}

		// If this light is being relit, don't respond to this stim now.
		// Someone else is relighting it. You can't ignore it, because if that attempt is aborted, you'll
		// want to see the stim later.

		if (light->IsBeingRelit())
		{
			owner->SetDelayedStimExpiration(stimPtr); // delay your next check of this stim
			return;
		}

		// Now do the LOS check.

		if (!owner->CanSeeExt(stimSource, false, false))
		{
			return;
		}
	}
	else if (aiUseType == EAIuse_Door)
	{
		// grayman #2866 - in the interest of reducing stim processing for closed doors,
		// add a check here to see if the door is closed. Otherwise, a closed door will
		// ping every AI w/in its radius (500) but the AI won't shut it down until it
		// can see the door. A guard not patrolling will receive endless pings unless
		// we shut it down here.

		CFrobDoor* door = static_cast<CFrobDoor*>(stimSource);
		if ( !door->IsOpen() )
		{
			door->DisableStim(ST_VISUAL); // it shouldn't be pinging anyone until it's opened
			return;
		}

		// grayman #2866 - check visibility of door's center when it's closed instead of its origin

		idVec3 targetPoint = door->GetClosedBox().GetCenter();

		if ( !owner->CanSeeTargetPoint( targetPoint, stimSource, true ) ) // 'true' = consider illumination
		{
			// grayman #2959 - if owner is handling this door, he should recognize
			// that it's open and shouldn't be, regardless of illumination. He's about
			// to use it, after all. This is similar to bumping into a hanging rope
			// in low illumination.

			if ( owner->m_HandlingDoor )
			{
				CFrobDoor* frobDoor = owner->GetMemory().doorRelated.currentDoor.GetEntity();
				if ( ( frobDoor == NULL ) || ( frobDoor != stimSource ) )
				{
					return; // handling a door, but not the one that stimmed the owner
				}

				// We're on the queue of the door that stimmed us.
				// In dim light, we can't "see" the door until we're close to it.

				if ( (owner->GetPhysics()->GetOrigin() - targetPoint).LengthFast() > MIN_DIST_TO_LOWLIGHT_DOOR )
				{
					return; // handling the door that stimmed us, but we're not close enough yet
				}

				// Repeat the CanSee, but w/o the lighting check this time.
				// We need LOS before we can "see" the suspicious door.

				if ( !owner->CanSeeTargetPoint( targetPoint, stimSource, false ) ) // 'false' = don't consider illumination
				{
					return; // handling the door that stimmed us, but we have no LOS yet
				}
			}
			else
			{
				return; // not handling a door
			}
		}
	}
	else if (aiUseType == EAIuse_Broken_Item)
	{
		if (!owner->CanSeeExt(stimSource, false, false))
		{
			return;
		}
	}
	else if (aiUseType == EAIuse_Rope) // grayman #2872
	{
		// Check if the stimSource is attached to a stuck rope arrow or a broken flinder

		idEntity* bindMaster = stimSource->GetBindMaster();
		if ( bindMaster != NULL )
		{
			idEntity* rope = bindMaster->FindMatchingTeamEntity( idAFEntity_Generic::Type );
			if ( rope != NULL )
			{
				// stimSource is a stuck rope arrow, so we have to check visibility of a spot somewhere along the rope

				ropeStimSource = owner->CanSeeRope( stimSource );
				if ( ropeStimSource == idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY))
				{
					return; // can't see the rope
				}
			}
			else
			{
				aiUseType = EAIuse_Suspicious; // change to suspicious flinder
				if ( !owner->CanSee(stimSource, true) )
				{
					return; // can't see the flinder
				}
			}
		}
		else
		{
			aiUseType = EAIuse_Suspicious; // change to suspicious flinder
			if ( !owner->CanSee(stimSource, true) )
			{
				return; // can't see the flinder
			}
		}
	}
	else if (!owner->CanSee(stimSource, true))
	{
		//DEBUG_PRINT ("I can't see the " + aiUse);
		return;
	}

	// Special check to see if this is a person/monster/undead we're seeing.
		
	if ( ( aiUseType == EAIuse_Person ) || ( aiUseType == EAIuse_Monster ) || ( aiUseType == EAIuse_Undead ) ) // grayman #3343
	{
		OnActorEncounter(stimSource, owner);
		return;
	}

	// Not a person/monster/undead stim, so ignore all other stims if we have an enemy.

	if (owner->GetEnemy() != NULL)
	{
		return;
	}

	// Process non-actor stim types

	// grayman #2924 - Check whether this alert type should be
	// processed, based on what other stims/alerts the AI has experienced.

	switch(aiUseType)
	{
	case EAIuse_Weapon:
		if (!ShouldProcessAlert(EAlertTypeWeapon))
		{
			return;
		}
		break;
	case EAIuse_Suspicious: // grayman #1327
		if (!ShouldProcessAlert(EAlertTypeSuspiciousItem))
		{
			return;
		}
		break;
	case EAIuse_Rope: // grayman #2872
		if (ShouldProcessAlert(EAlertTypeRope))
		{
			OnVisualStimRope(stimSource,owner,ropeStimSource);
		}
		return;
		break;
	case EAIuse_Blood_Evidence:
		if (!ShouldProcessAlert(EAlertTypeBlood))
		{
			return;
		}
		break;
	case EAIuse_Lightsource:
		if (ShouldProcessAlert(EAlertTypeLightSource))
		{
			OnVisualStimLightSource(stimSource,owner);
		}
		return;
		break;
	case EAIuse_Missing_Item_Marker:
		if (!ShouldProcessAlert(EAlertTypeMissingItem))
		{
			return;
		}
		break;
	case EAIuse_Broken_Item:
		if (!ShouldProcessAlert(EAlertTypeBrokenItem))
		{
			return;
		}
		break;
	case EAIuse_Door:
		if (!ShouldProcessAlert(EAlertTypeDoor))
		{
			return;
		}
		break;
	case EAIuse_Default:
	default:
		return;
		break;
	}

	// grayman #2924 - We're going to process this visual stim.
	// Ignore it in the future

	stimSource->IgnoreResponse(ST_VISUAL, owner);

	// Delay the processing randomly between VIS_STIM_DELAY_MIN and VIS_STIM_DELAY_MAX ms.

	int delay = VIS_STIM_DELAY_MIN + gameLocal.random.RandomInt(VIS_STIM_DELAY_MAX - VIS_STIM_DELAY_MIN);

	// Post the event to do the processing
	owner->PostEventMS( &AI_DelayedVisualStim, delay, stimSource);

	owner->m_allowAudioAlerts = false; // grayman #3424
}

void State::DelayedVisualStim(idEntity* stimSource, idAI* owner)
{
	owner->m_allowAudioAlerts = true; // grayman #3424

	idStr aiUse = stimSource->spawnArgs.GetString("AIUse");

	if (aiUse == AIUSE_WEAPON)
	{
		OnVisualStimWeapon(stimSource,owner);
	}
	else if (aiUse == AIUSE_DOOR)
	{
		OnVisualStimDoor(stimSource,owner);
	}
	else if (aiUse == AIUSE_SUSPICIOUS)
	{
		OnVisualStimSuspicious(stimSource,owner);
	}
	else if (aiUse == AIUSE_BLOOD_EVIDENCE)
	{
		OnVisualStimBlood(stimSource,owner);
	}
	else if (aiUse == AIUSE_MISSING_ITEM_MARKER)
	{
		OnVisualStimMissingItem(stimSource,owner);
	}
	else if (aiUse == AIUSE_BROKEN_ITEM)
	{
		OnVisualStimBrokenItem(stimSource,owner);
	}
}

bool State::ShouldProcessAlert(EAlertType newAlertType)
{
	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	// Memory shortcut
	Memory& memory = owner->GetMemory();
	
	if (owner->alertTypeWeight[memory.alertType] <= owner->alertTypeWeight[newAlertType])
	{
		return true;
	}

	return false;
}

void State::OnVisualStimWeapon(idEntity* stimSource, idAI* owner)
{
	assert(stimSource != NULL && owner != NULL); // must be fulfilled

	// Memory shortcut
	Memory& memory = owner->GetMemory();

	// We've seen this object, don't respond to it again
//	stimSource->IgnoreResponse(ST_VISUAL, owner); // grayman #2924 - already done

	if (stimSource->IsType(idWeapon::Type))
	{
		// Is it a friendly weapon?  To find out we need to get its owner.
		idActor* objectOwner = static_cast<idWeapon*>(stimSource)->GetOwner();
		
		if (owner->IsFriend(objectOwner))
		{
			DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Ignoring visual stim from weapon with friendly owner\r");
			return;
		}
	}
	
	// Vocalize that see something out of place
	//gameLocal.Printf("Hmm, that isn't right! A weapon!\n");
	if (owner->AI_AlertLevel < owner->thresh_5 &&
		gameLocal.time - memory.lastTimeVisualStimBark >= MINIMUM_SECONDS_BETWEEN_STIMULUS_BARKS)
	{
		memory.lastTimeVisualStimBark = gameLocal.time;
		owner->commSubsystem->AddCommTask(
			CommunicationTaskPtr(new SingleBarkTask("snd_foundWeapon"))
		);
	}

	// TWO more piece of evidence of something out of place: A weapon is not a good thing
	memory.countEvidenceOfIntruders += EVIDENCE_COUNT_INCREASE_WEAPON;
	memory.posEvidenceIntruders = owner->GetPhysics()->GetOrigin(); // grayman #2903
	memory.timeEvidenceIntruders = gameLocal.time; // grayman #2903
	memory.stopRelight = true; // grayman #2603
	memory.stopExaminingRope = true; // grayman #2872 - stop examining rope
	memory.stopReactingToHit = true; // grayman #2816

	// Raise alert level
	if (owner->AI_AlertLevel < owner->thresh_4 - 0.1f)
	{
		owner->SetAlertLevel(owner->thresh_4 - 0.1f);
	}
	
	memory.alertPos = stimSource->GetPhysics()->GetOrigin();
	memory.alertClass = EAlertVisual_2; // grayman #2603
	memory.alertType = EAlertTypeWeapon;

	// Do search as if there is an enemy that has escaped
	memory.alertRadius = LOST_ENEMY_ALERT_RADIUS;
	memory.alertSearchVolume = LOST_ENEMY_SEARCH_VOLUME; 
	memory.alertSearchExclusionVolume.Zero();
	
	owner->AI_VISALERT = false;
	memory.visualAlert = false; // grayman #2422
	memory.mandatory = false;	// grayman #3331
	
	// Do new reaction to stimulus
	memory.stimulusLocationItselfShouldBeSearched = true;
	memory.investigateStimulusLocationClosely = true; // deep investigation
	memory.alertedDueToCommunication = false;
}

// grayman #1327 - modified copy of OnVisualStimWeapon()

void State::OnVisualStimSuspicious(idEntity* stimSource, idAI* owner)
{
	assert( ( stimSource != NULL ) && ( owner != NULL) ); // must be fulfilled

	if ( owner->AI_AlertLevel >= owner->thresh_5 ) // grayman #2423 - pay no attention if in combat
	{
		stimSource->AllowResponse(ST_VISUAL, owner); // grayman #2924
		return;
	}

	// Memory shortcut
	Memory& memory = owner->GetMemory();

	// We've seen this object, don't respond to it again
	stimSource->IgnoreResponse(ST_VISUAL, owner);

	if ( stimSource->IsType(idWeapon::Type) )
	{
		// Is it a friendly weapon?  To find out we need to get its owner.
		idActor* objectOwner = static_cast<idWeapon*>(stimSource)->GetOwner();
		
		if ( owner->IsFriend(objectOwner) )
		{
			return;
		}
	}
	else if ( stimSource->IsType(CProjectileResult::Type) )
	{
		// grayman #3075 - If this arrow is bound to a dead body,
		// ignore it. The dead body will be found separately.

		// What we have is the projectile result (stimSource) bound
		// to the arrow, which is eventually bound to the body. Go up the
		// bindMaster chain until you find the body. (Or not.)

		idEntity *bindMaster = stimSource->GetBindMaster();
		while ( bindMaster != NULL )
		{
			if ( bindMaster->IsType(idAI::Type) )
			{
				return; // ignore the arrow
			}
			bindMaster = bindMaster->GetBindMaster();
		}
	}

	// Vocalize that we see something out of place
	//gameLocal.Printf("Hmm, that's suspicious!\n");
	if ( ( owner->AI_AlertLevel < owner->thresh_5 ) &&
		 ( gameLocal.time - memory.lastTimeVisualStimBark >= MINIMUM_SECONDS_BETWEEN_STIMULUS_BARKS) )
	{
		memory.lastTimeVisualStimBark = gameLocal.time;
		owner->commSubsystem->AddCommTask(
			CommunicationTaskPtr(new SingleBarkTask("snd_foundSuspiciousItem"))
		);
	}

	// One more piece of evidence of something out of place.
	memory.countEvidenceOfIntruders += EVIDENCE_COUNT_INCREASE_SUSPICIOUS;
	memory.posEvidenceIntruders = owner->GetPhysics()->GetOrigin(); // grayman #2903
	memory.timeEvidenceIntruders = gameLocal.time; // grayman #2903
	memory.stopRelight = true;
	memory.stopExaminingRope = true; // grayman #2872 - stop examining rope
	memory.stopReactingToHit = true; // grayman #2816

	// Raise alert level
	if ( owner->AI_AlertLevel < owner->thresh_4 - 0.1f )
	{
		owner->SetAlertLevel(owner->thresh_4 - 0.1f);
	}
	
	memory.alertPos = stimSource->GetPhysics()->GetOrigin();
	memory.alertClass = EAlertVisual_2;
	memory.alertType = EAlertTypeSuspiciousItem;

	// Do search as if there is an enemy that has escaped
	memory.alertRadius = LOST_ENEMY_ALERT_RADIUS;
	memory.alertSearchVolume = LOST_ENEMY_SEARCH_VOLUME; 
	memory.alertSearchExclusionVolume.Zero();
	
	owner->AI_VISALERT = false;
	memory.visualAlert = false; // grayman #2422
	memory.mandatory = false;	// grayman #3331
	
	// Do new reaction to stimulus
	memory.stimulusLocationItselfShouldBeSearched = true;
	memory.investigateStimulusLocationClosely = true; // deep investigation
	memory.alertedDueToCommunication = false;
}

// grayman #2872 - modified copy of OnVisualStimSuspicious()

void State::OnVisualStimRope( idEntity* stimSource, idAI* owner, idVec3 ropeStimSource )
{
	assert( ( stimSource != NULL ) && ( owner != NULL ) ); // must be fulfilled

	// Memory shortcut
	Memory& memory = owner->GetMemory();

	// We've seen this object, don't respond to it again
	stimSource->IgnoreResponse(ST_VISUAL, owner);

	// Vocalize that see something out of place
	//gameLocal.Printf("Hmm, that rope shouldn't be there!\n");
	if (owner->AI_AlertLevel < owner->thresh_5 &&
		gameLocal.time - memory.lastTimeVisualStimBark >= MINIMUM_SECONDS_BETWEEN_STIMULUS_BARKS)
	{
		memory.lastTimeVisualStimBark = gameLocal.time;
		owner->commSubsystem->AddCommTask(
			CommunicationTaskPtr(new SingleBarkTask("snd_foundSuspiciousItem"))
		);
	}

	// One more piece of evidence of something out of place.
	memory.countEvidenceOfIntruders += EVIDENCE_COUNT_INCREASE_ROPE;
	memory.posEvidenceIntruders = owner->GetPhysics()->GetOrigin(); // grayman #2903
	memory.timeEvidenceIntruders = gameLocal.time; // grayman #2903
	memory.stopRelight = true;

	// Raise alert level
	if (owner->AI_AlertLevel < owner->thresh_4 - 0.1f)
	{
		owner->SetAlertLevel(owner->thresh_4 - 0.1f);
	}
	
	idEntity* bindMaster = stimSource->GetBindMaster();
	if ( bindMaster != NULL )
	{
		owner->m_ExaminingRope = true;
		owner->m_LatchedSearch = true; // set up search after rope is examined
		memory.stopExaminingRope = false;
		idEntity* rope = bindMaster->FindMatchingTeamEntity( idAFEntity_Generic::Type );
		idAFEntity_Generic* ropeAF = static_cast<idAFEntity_Generic*>(rope);
		owner->GetMind()->SwitchState(StatePtr(new ExamineRopeState(ropeAF,ropeStimSource))); // go examine the rope
	}
}

// grayman #2816

void State::OnHitByMoveable(idAI* owner, idEntity* tactEnt)
{
	// Vocalize that something hit me, but only if I'm not in combat mode, and I'm not in pain this frame
	if ( ( owner->AI_AlertLevel < owner->thresh_5 ) && !owner->AI_PAIN )
	{
		//gameLocal.Printf("Something hit me!\n");
		owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new SingleBarkTask("snd_notice_generic")));
	}

	owner->GetMemory().hitByThisMoveable = tactEnt;
	owner->GetMind()->SwitchState(StatePtr(new HitByMoveableState())); // react to getting hit
}

// grayman #2903 - determine if an AI is inside the warning volume to receive a warning

bool InsideWarningVolume( idVec3 alertPos, idVec3 aiOrigin, int maxDist )
{
	// if alertPos = (0,0,0), the position of the alert wasn't recorded, so fail the test

	idVec3 nullVec(0,0,0);
	if ( alertPos == nullVec )
	{
		return false; // invalid alert position
	}

	idVec3 vecFromAlert = aiOrigin - alertPos;
	if ( abs(vecFromAlert.z) > WARN_DIST_MAX_Z )
	{
		return false; // AI is too far above or below
	}

	if ( vecFromAlert.LengthSqr() > Square(maxDist) )
	{
		return false; // AI is too far away
	}

	return true; // AI is inside the warning volume
}

// grayman #3424

int State::ProcessWarning( idActor* owner, idActor* other, EventType type, const int warningDist )
{
	const idVec3& otherOrigin = other->GetPhysics()->GetOrigin();

	// Go through the owner's list of suspicious events and see if any
	// are this event type. If there's one I haven't told 'other' about yet, see if we're
	// close enough to the event site to tell him.

	for ( int i = 0 ; i < owner->m_suspiciousEventIDs.Num() ; i++ )
	{
		int eventID = owner->m_suspiciousEventIDs[i];
		SuspiciousEvent se = gameLocal.m_suspiciousEvents[eventID];
		if ( se.type == type ) // type of event
		{
			if ( InsideWarningVolume(se.location, otherOrigin, warningDist) ) // grayman #2903
			{
				// Found the right type, and we're inside the distance check. Have I already told my friend about this?
				if ( !other->HasBeenWarned( owner, eventID ) )
				{
					return eventID; // Didn't already warn, so tell my friend about this.
				}
			}
		}
	}

	return -1; // Either there's nothing to warn about, or I already told him
}

void State::OnActorEncounter(idEntity* stimSource, idAI* owner)
{
	assert( ( stimSource != NULL ) && ( owner != NULL ) ); // must be fulfilled

	Memory& memory = owner->GetMemory();

	bool ignoreStimulusFromNowOn = true;
	
	if ( !stimSource->IsType(idActor::Type) )
	{
		return; // No Actor, quit
	}

	// Hard-cast the stimsource onto an actor 
	idActor* other = static_cast<idActor*>(stimSource);
	idStr ownerAiUse = owner->spawnArgs.GetString("AIUse");
	idStr otherAiUse = other->spawnArgs.GetString("AIUse");

	if (other->health <= 0) // Are they dead?
	{
		// grayman #3343 - undead and monsters don't react to dead actors

		if ( ( ownerAiUse == AIUSE_MONSTER ) || ( ownerAiUse == AIUSE_UNDEAD ) )
		{
			ignoreStimulusFromNowOn = true;
		}
		// humans don't react to dead undead/monsters
		else if ( ( otherAiUse == AIUSE_MONSTER ) || ( otherAiUse == AIUSE_UNDEAD ) )
		{
			ignoreStimulusFromNowOn = true;
		}
		else if ( ShouldProcessAlert(EAlertTypeDeadPerson) )
		{
			// React to finding body
			ignoreStimulusFromNowOn = OnDeadPersonEncounter(other, owner);
			if ( ignoreStimulusFromNowOn )
			{
				owner->TactileIgnore(stimSource);
			}
		}
		else
		{
			ignoreStimulusFromNowOn = false;
		}
	}
	else if (other->IsKnockedOut()) // Are they unconscious?
	{
		// grayman #3343 - undead and monsters don't react to unconscious actors

		if ( ( ownerAiUse == AIUSE_MONSTER ) || ( ownerAiUse == AIUSE_UNDEAD ) )
		{
			ignoreStimulusFromNowOn = true;
		}
		// humans don't react to unconscious undead/monsters
		else if ( ( otherAiUse == AIUSE_MONSTER ) || ( otherAiUse == AIUSE_UNDEAD ) )
		{
			ignoreStimulusFromNowOn = true;
		}
		else if (ShouldProcessAlert(EAlertTypeUnconsciousPerson))
		{
			// React to finding unconscious person
			ignoreStimulusFromNowOn = OnUnconsciousPersonEncounter(other, owner);
			if (ignoreStimulusFromNowOn)
			{
				owner->TactileIgnore(stimSource);
			}
		}
		else
		{
			ignoreStimulusFromNowOn = false;
		}
	}
	else
	{
		// Not knocked out, not dead, deal with it
		if (owner->IsEnemy(other))
		{
			// Only do this if we don't have an enemy already
			// grayman #3355 - or we have one, but this new one is closer (and presumed more of a threat)

			bool setNewEnemy = false;

			if ( owner->GetEnemy() == NULL )
			{
				setNewEnemy = true;
			}
			else // we have an enemy
			{
				idActor* enemy = owner->GetEnemy(); // current enemy
				if ( other != enemy )
				{
					idVec3 ownerOrigin = owner->GetPhysics()->GetOrigin();
					float dist2EnemySqr = ( enemy->GetPhysics()->GetOrigin() - ownerOrigin ).LengthSqr();
					float dist2OtherSqr = ( other->GetPhysics()->GetOrigin() - ownerOrigin ).LengthSqr();
					if ( dist2OtherSqr < dist2EnemySqr )
					{
						setNewEnemy = true;
					}
				}
			}

			if (setNewEnemy)
			{
				// Living enemy
				//gameLocal.Printf("I see a living enemy!\n");
				owner->SetEnemy(other);
				owner->AI_VISALERT = true;
				
				owner->SetAlertLevel(owner->thresh_5*2);
				memory.alertClass = EAlertVisual_1;
				memory.alertType = EAlertTypeEnemy;
				owner->LogSuspiciousEvent( E_EventTypeEnemy, owner->GetPhysics()->GetOrigin(), NULL ); // grayman #3424  
			}

			// An enemy should not be ignored in the future - grayman #2423 - moved out of above test so all enemies are remembered
			ignoreStimulusFromNowOn = false;
		}
		// grayman #3338 - allow greetings and warnings to the player,
		// when he's a friend. When he's neutral to me, just give greetings.
		else if ( other->IsType(idPlayer::Type) )
		{
			idPlayer* player = static_cast<idPlayer*>(other);

			if ( owner->IsFriend(player) )
			{
				// Remember last time you saw a friendly face
				memory.lastTimeFriendlyAISeen = gameLocal.time;
			}

			// don't issue a warning or greeting if you're mute
			if ( !owner->m_isMute ) // grayman #2903
			{
				idVec3 dir = owner->GetPhysics()->GetOrigin() - player->GetPhysics()->GetOrigin();
				if ( dir.LengthFast() <= REMARK_DISTANCE )
				{
					idStr soundName = "";
					Memory::GreetingInfo& info = owner->GetMemory().GetGreetingInfo(player);

					// Allow warnings if the player is a friend
					if ( owner->IsFriend(player) )
					{
						// grayman #3424 - has enough time passed since the last warning?

						if ( gameLocal.time >= info.nextWarningTime )
						{
							if ( owner->m_suspiciousEventIDs.Num() > 0 ) // Have I seen anything suspicious worth warning about?
							{
								// Should I warn about an enemy?
								int eventID = ProcessWarning(owner,player,E_EventTypeEnemy,WARN_DIST_ENEMY_SEEN); // grayman #3424
								if (eventID >= 0)
								{
									player->AddSuspiciousEvent(eventID);    // player now knows about this event
									owner->AddWarningEvent(player,eventID); // log that a warning passed between us
									player->AddWarningEvent(owner,eventID); // log that a warning passed between us
									soundName = "snd_warnSawEnemy";
								}
							
								if ( soundName.IsEmpty() )
								{
									// Should I warn about finding a corpse?
									eventID = ProcessWarning(owner,player,E_EventTypeDeadPerson,WARN_DIST_CORPSE_FOUND); // grayman #3424
									if (eventID >= 0)
									{
										player->AddSuspiciousEvent(eventID);    // player now knows about this event
										owner->AddWarningEvent(player,eventID); // log that a warning passed between us
										player->AddWarningEvent(owner,eventID); // log that a warning passed between us
										soundName = "snd_warnFoundCorpse";
									}
								}

								if ( soundName.IsEmpty() )
								{
									// Should I warn about a missing item?
									eventID = ProcessWarning(owner,player,E_EventTypeMissingItem,WARN_DIST_MISSING_ITEM); // grayman #3424
									if (eventID >= 0)
									{
										player->AddSuspiciousEvent(eventID);    // player now knows about this event
										owner->AddWarningEvent(player,eventID); // log that a warning passed between us
										player->AddWarningEvent(owner,eventID); // log that a warning passed between us
										soundName = "snd_warnMissingItem";
									}
								}
							}
							
							if ( soundName.IsEmpty() )
							{
								// Should I give a general warning about intruder evidence?
								if ( memory.countEvidenceOfIntruders >= MIN_EVIDENCE_OF_INTRUDERS_TO_COMMUNICATE_SUSPICION )
								{
									if ( memory.timeEvidenceIntruders > player->timeEvidenceIntruders )
									{
										if ( InsideWarningVolume( memory.posEvidenceIntruders, player->GetPhysics()->GetOrigin(), WARN_DIST_EVIDENCE_INTRUDERS ) )
										{
											player->timeEvidenceIntruders = memory.timeEvidenceIntruders;
											soundName = "snd_warnSawEvidence";
										}
									}
								}
							}

							int delay;
							if ( !soundName.IsEmpty() )
							{
								delay = ( MINIMUM_TIME_BETWEEN_WARNINGS + gameLocal.random.RandomInt(VARIABLE_TIME_BETWEEN_WARNINGS))*1000;
							}
							else
							{
								// Since AI can spot the player every few frames, let's reduce
								// the frequency if no warning or greeting is passed on this sighting.
								delay = DELAY_BETWEEN_WARNING_FAILURES*1000;
							}
							info.nextWarningTime = gameLocal.time + delay;
						}
					}

					if ( soundName.IsEmpty() && owner->CanGreet() )
					{
						// See if a greeting is appropriate

						// Check the next time we can greet the player
						if ( gameLocal.time >= info.nextGreetingTime ) // grayman #3415
						{
							soundName = "snd_greeting_generic";
							int delay = ( MINIMUM_TIME_BETWEEN_GREETING_SAME_ACTOR + gameLocal.random.RandomInt(EXTRA_DELAY_BETWEEN_GREETING_SAME_ACTOR))*1000;
							info.nextGreetingTime = gameLocal.time + delay;

							if (cv_ai_debug_greetings.GetBool())
							{
								gameLocal.Printf("%s barks greeting '%s' to the player\n\n",owner->GetName(),soundName.c_str());
							}
						}
					}

					// Speak the chosen sound

					if ( !soundName.IsEmpty() )
					{
						memory.lastTimeVisualStimBark = gameLocal.time;
						CommMessagePtr message;
						owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new SingleBarkTask(soundName, message)));
						owner->Event_LookAtPosition(player->GetEyePosition(), 1.0 + gameLocal.random.RandomFloat()); // grayman #2925
					}
				}
			}

			// Don't ignore in future
			ignoreStimulusFromNowOn = false;
		}
		else if (owner->IsFriend(other))
		{
			if (!other->IsType(idAI::Type))
			{
				return; // safeguard
			}

			idAI* otherAI = static_cast<idAI*>(other);

			// Remember last time a friendly AI was seen
			memory.lastTimeFriendlyAISeen = gameLocal.time;

			if (otherAI->GetMoveType() == MOVETYPE_SLEEP) // grayman #2464 - is the other asleep?
			{
				return; // nothing else to do
			}

			// grayman #3343 - monsters and undead don't issue warnings or greetings

			if ( ( ownerAiUse != AIUSE_MONSTER ) && ( ownerAiUse != AIUSE_UNDEAD ) )
			{
				Memory& otherMemory = otherAI->GetMemory();

				// angua: if the other AI is searching due to an alert, join in

				// grayman #2603 - only join if he's searching and I haven't been searching recently
				// grayman #2866 - don't join if he's searching a suspicious door. Joining causes congestion at the door.
				// grayman #3317 - surely don't join if I'm fleeing! Feets don't fail me now!!

				bool fleeing = (owner->GetMind()->GetState()->GetName() == "Flee" );

				if ( !fleeing && otherAI->IsSearching() && !( owner->m_lastAlertLevel >= owner->thresh_3 ) && ( otherMemory.alertType != EAlertTypeDoor ) ) // grayman #3438
				{
					// grayman #1327 - warning should be specific

					idStr soundName = "";
					bool learnedViaComm = false; // grayman #3424
					int eventID = -1;
			
					// grayman #2903 - time-based warnings. My friend wants to warn me
					// about the alert that's causing him to search. This will be the
					// alert he's seen most recently, so we'll check what's making him
					// search, and he'll warn me about that one. Since I'm not currently
					// searching, his alert will be more recent than any of mine, so I'll
					// take on the position and timestamp of his alert.
				
					if ( otherMemory.alertType == EAlertTypeEnemy )
					{
						// warn about seeing an enemy
						// gameLocal.Printf("%s found a friend, who is warning about seeing an enemy\n",owner->name.c_str());
						
						// grayman #3424 - Do I already know about seeing an enemy at this location?

						eventID = gameLocal.FindSuspiciousEvent( E_EventTypeEnemy, otherMemory.posEnemySeen, NULL );
						if ( eventID >= 0 )
						{
							// eventID is the ID of the suspicious event associated with posEnemySeen. Has the other AI
							// warned me about this event in the past? If so, he won't warn me again. If he hasn't, he
							// should warn me and I should join his search.

							// grayman #3424 - if not only know about this event, but I've already searched because
							// of it, I won't join his search

							if ( !owner->HasBeenWarned(otherAI,eventID) && !owner->HasSearchedEvent(eventID) )
							{
								// Did I already know about this event?
								if ( !owner->FindSuspiciousEvent(eventID) )
								{
									learnedViaComm = true; // grayman #3424
									memory.countEvidenceOfIntruders += EVIDENCE_COUNT_INCREASE_ENEMY;
									memory.posEvidenceIntruders = gameLocal.m_suspiciousEvents[eventID].location;
									memory.timeEvidenceIntruders = gameLocal.time; // grayman #2903
									memory.enemiesHaveBeenSeen = true;

									if (cv_ai_debug_transition_barks.GetBool())
									{
										gameLocal.Printf("%s is warned by %s about an enemy, will use Alert Idle\n",owner->GetName(),otherAI->GetName());
									}

									memory.posEnemySeen = otherMemory.posEnemySeen;
									owner->AddSuspiciousEvent(eventID);
								}
								owner->AddWarningEvent(otherAI,eventID); // log that a warning passed between us
								otherAI->AddWarningEvent(owner,eventID); // log that a warning passed between us
								soundName = "snd_warnSawEnemy";
							}
						}
					}

					if ( soundName.IsEmpty() && ( otherMemory.alertType == EAlertTypeDeadPerson ) )
					{
						idEntity* corpse = otherMemory.corpseFound.GetEntity();
						
						// grayman #3424 - Do I already know about this corpse?

						eventID = gameLocal.FindSuspiciousEvent( E_EventTypeDeadPerson, idVec3(0,0,0), corpse );
						if ( eventID >= 0 )
						{
							if ( !owner->HasBeenWarned(otherAI,eventID) && !owner->HasSearchedEvent(eventID) ) // grayman #3424
							{
								// Did I already know about this event?
								if ( !owner->FindSuspiciousEvent(eventID) )
								{
									learnedViaComm = true; // grayman #3424
									memory.deadPeopleHaveBeenFound = true;

									if (cv_ai_debug_transition_barks.GetBool())
									{
										gameLocal.Printf("%s is warned by %s about a dead person, will use Alert Idle\n",owner->GetName(),otherAI->GetName());
									}

									memory.countEvidenceOfIntruders += EVIDENCE_COUNT_INCREASE_CORPSE;
									memory.posEvidenceIntruders = gameLocal.m_suspiciousEvents[eventID].location;
									memory.timeEvidenceIntruders = gameLocal.time;
									owner->AddSuspiciousEvent(eventID);
								}
								owner->AddWarningEvent(otherAI,eventID); // log that a warning passed between us
								otherAI->AddWarningEvent(owner,eventID); // log that a warning passed between us
								soundName = "snd_warnFoundCorpse";
								owner->SetAlertLevel(owner->thresh_4 + 0.1); // don't inherit a reduced alert level. dead people are bad.
							}
						}
					}

					if ( soundName.IsEmpty() && ( otherMemory.alertType == EAlertTypeMissingItem ) )
					{
						// grayman #3424 - Do I already know about this?

						eventID = gameLocal.FindSuspiciousEvent( E_EventTypeMissingItem, otherMemory.posMissingItem, NULL );
						if ( eventID >= 0 )
						{
							if ( !owner->HasBeenWarned(otherAI,eventID) && !owner->HasSearchedEvent(eventID) ) // grayman #3424
							{
								// Did I already know about this event?
								if ( !owner->FindSuspiciousEvent(eventID) )
								{
									learnedViaComm = true; // grayman #3424
									owner->AddSuspiciousEvent(eventID);
									memory.itemsHaveBeenStolen = true;

									if (cv_ai_debug_transition_barks.GetBool())
									{
										gameLocal.Printf("%s is warned by %s about a missing item, will use Alert Idle\n",owner->GetName(),otherAI->GetName());
									}

									memory.posMissingItem = otherMemory.posMissingItem;
									memory.countEvidenceOfIntruders += EVIDENCE_COUNT_INCREASE_MISSING_ITEM;
									memory.posEvidenceIntruders = gameLocal.m_suspiciousEvents[eventID].location;
									memory.timeEvidenceIntruders = gameLocal.time;
								}
								owner->AddWarningEvent(otherAI,eventID); // log that a warning passed between us
								otherAI->AddWarningEvent(owner,eventID); // log that a warning passed between us
								soundName = "snd_warnMissingItem";
								owner->SetAlertLevel(otherAI->AI_AlertLevel * 0.7f); // inherit a reduced alert level
							}
						}
					}

					if ( soundName.IsEmpty() )
					{
						if ( memory.timeEvidenceIntruders < otherMemory.timeEvidenceIntruders ) // is his evidence alert later?
						{
							// warn about intruders
							//gameLocal.Printf("%s found a friend, who is warning about evidence of intruders\n",owner->name.c_str());
							soundName = "snd_warnSawEvidence";
							owner->SetAlertLevel(otherAI->AI_AlertLevel * 0.7f); // inherit a reduced alert level

							memory.posEvidenceIntruders = otherMemory.posEvidenceIntruders;
							memory.timeEvidenceIntruders = otherMemory.timeEvidenceIntruders;

							// grayman #2603 - raise my evidence of intruders?

							// grayman #3424 - the evidence count can get way out of line
							// by simply inheriting what the other AI has, and it opens
							// itself up to double-dipping for the same event. Let's just stick
							// to a general "suspicious" increase.

							memory.countEvidenceOfIntruders += EVIDENCE_COUNT_INCREASE_SUSPICIOUS;
							learnedViaComm = true; // grayman #3424

							/* old way
							int warningAmount = otherMemory.countEvidenceOfIntruders;
							if ( memory.countEvidenceOfIntruders < warningAmount )
							{
								memory.countEvidenceOfIntruders = warningAmount;
							}
							 */
						}
					}

					if ( !soundName.IsEmpty() ) // grayman #2903
					{
						owner->StopMove(MOVE_STATUS_DONE);
						memory.stopRelight = true; // grayman #2603 - abort a relight in progress
						memory.stopExaminingRope = true; // grayman #2872 - stop examining rope
						memory.stopReactingToHit = true; // grayman #2816

						memory.alertPos = otherMemory.alertPos;
						memory.currentSearchEventID = eventID; // grayman #3424
						memory.alertClass = otherMemory.alertClass; // grayman #2603 - inherit the other's alert info
						memory.alertType = otherMemory.alertType;
						memory.visualAlert = otherMemory.visualAlert; // grayman #2422
						memory.mandatory = false; // grayman #3331
					
						memory.alertRadius = otherMemory.alertRadius;
						memory.alertSearchVolume = otherMemory.alertSearchVolume; 
						memory.alertSearchExclusionVolume.Zero();

						memory.alertedDueToCommunication = learnedViaComm; // grayman #3424

						// The other AI might bark, but only if he can see you.
						// grayman #3070 - he won't bark if he's in combat mode

						if ( otherAI->CanSee(owner, true) && ( otherAI->AI_AlertIndex < ECombat ) )
						{
							//gameLocal.Printf("Hey! Help me search!\n");
							otherMemory.lastTimeVisualStimBark = gameLocal.time;
							otherAI->commSubsystem->AddCommTask(CommunicationTaskPtr(new SingleBarkTask(soundName)));
							otherAI->Event_LookAtPosition(owner->GetEyePosition(), 1.0 + gameLocal.random.RandomFloat()); // grayman #2925
						}

						int delay = ( MINIMUM_TIME_BETWEEN_WARNINGS + gameLocal.random.RandomInt(VARIABLE_TIME_BETWEEN_WARNINGS))*1000;
						owner->GetMemory().GetGreetingInfo(otherAI).nextWarningTime = gameLocal.time + delay;
						delay = ( MINIMUM_TIME_BETWEEN_WARNINGS + gameLocal.random.RandomInt(VARIABLE_TIME_BETWEEN_WARNINGS))*1000;
						otherAI->GetMemory().GetGreetingInfo(owner).nextWarningTime = gameLocal.time + delay;
					}
				}
				// grayman #3202 - don't issue a warning or greeting if you're mute
				// grayman #3424 - or if your friend has been searching recently
				else if ( !(otherAI->m_lastAlertLevel >= owner->thresh_3)  && !owner->m_isMute ) // grayman #2903 // grayman #3438 
				{
					// grayman #1327 - apply the distance check to both warnings and greetings

					const idVec3& origin = owner->GetPhysics()->GetOrigin();
					const idVec3& otherOrigin = otherAI->GetPhysics()->GetOrigin();
					idVec3 dir = origin - otherOrigin;
					dir.z = 0;
					float distSqr = dir.LengthSqr();

					float remarkLimit = REMARK_DISTANCE;
					if ( owner->GetMind()->GetState()->GetName() == "Flee" ) // grayman #3140 - increase remark limit if fleeing
					{
						remarkLimit *= 3;
					}

					if ( distSqr <= Square(remarkLimit) )
					{
						// grayman #3424 - has enough time passed since the last warning?

						// Variables for the sound and the conveyed message
						idStr soundName = "";
						CommMessagePtr message; 

						if ( gameLocal.time >= owner->GetMemory().GetGreetingInfo(other).nextWarningTime )
						{
							// Issue a communication stim to the friend we spotted.
							// We can issue warnings, greetings, etc...

							// For AI->Player warnings, we needed to create a "warning delay time"
							// because AI can spot the player every few frames, and there's no
							// point in continuously finding there's nothing to say every few frames.
							//
							// For AI->AI warnings, however, we don't need that type of delay,
							// because they see each other using visual stims, which fire on
							// average every 2.5 seconds, which provides a built-in delay.

							if ( owner->m_suspiciousEventIDs.Num() > 0 ) // Have I seen anything suspicious worth warning about?
							{
								// Should I warn about seeing an enemy?

								int eventID = ProcessWarning( owner,other,E_EventTypeEnemy,WARN_DIST_ENEMY_SEEN );
								if ( eventID >= 0 )
								{
									// Check message queue to see if I've already sent this type of message
									// and it hasn't yet been received. Also check the other direction, from 'other' to me.
									if ( !owner->CheckOutgoingMessages( CommMessage::ConveyWarning_EnemiesHaveBeenSeen_CommType, other ) &&
										 !static_cast<idAI*>(other)->CheckOutgoingMessages( CommMessage::ConveyWarning_EnemiesHaveBeenSeen_CommType, owner ) )
									{
										message = CommMessagePtr(new CommMessage(
											CommMessage::ConveyWarning_EnemiesHaveBeenSeen_CommType, 
											owner, other, // from this AI to the other
											NULL,
											owner->GetPhysics()->GetOrigin(),
											eventID // grayman #3424
										));
										soundName = "snd_warnSawEnemy";
									}
								}

								if ( soundName.IsEmpty() )
								{
									// Should I warn about a corpse?

									eventID = ProcessWarning( owner,other,E_EventTypeDeadPerson,WARN_DIST_CORPSE_FOUND );
									if ( eventID >= 0 )
									{
										// Check message queue to see if I've already sent this type of message
										// and it hasn't yet been received. Also check the other direction, from 'other' to me.
										if ( !owner->CheckOutgoingMessages( CommMessage::ConveyWarning_CorpseHasBeenSeen_CommType, other ) &&
											 !static_cast<idAI*>(other)->CheckOutgoingMessages( CommMessage::ConveyWarning_CorpseHasBeenSeen_CommType, owner ) )
										{
											message = CommMessagePtr(new CommMessage(
												CommMessage::ConveyWarning_CorpseHasBeenSeen_CommType, 
												owner, other, // from this AI to the other
												NULL,
												owner->GetPhysics()->GetOrigin(),
												eventID // grayman #3424
											));
											soundName = "snd_warnFoundCorpse";
										}
									}
								}

								if ( soundName.IsEmpty() )
								{
									// Should I warn about a missing item?

									eventID = ProcessWarning( owner,other,E_EventTypeMissingItem,WARN_DIST_MISSING_ITEM );
									if ( eventID >= 0 )
									{
										// Check message queue to see if I've already sent this type of message
										// and it hasn't yet been received. Also check the other direction, from 'other' to me.
										if ( !owner->CheckOutgoingMessages( CommMessage::ConveyWarning_ItemsHaveBeenStolen_CommType, other ) &&
											 !static_cast<idAI*>(other)->CheckOutgoingMessages( CommMessage::ConveyWarning_ItemsHaveBeenStolen_CommType, owner ) )
										{
											message = CommMessagePtr(new CommMessage(
												CommMessage::ConveyWarning_ItemsHaveBeenStolen_CommType, 
												owner, other, // from this AI to the other
												NULL,
												owner->GetPhysics()->GetOrigin(),
												eventID // grayman #3424
											));
											soundName = "snd_warnMissingItem";
										}
									}
								}
							}

							// If I'm not warning about a specific event, should I warn
							// about intruders?

							if ( soundName.IsEmpty() )
							{
								if ( memory.countEvidenceOfIntruders >= MIN_EVIDENCE_OF_INTRUDERS_TO_COMMUNICATE_SUSPICION )
								{
									if ( ( otherMemory.countEvidenceOfIntruders < memory.countEvidenceOfIntruders ) )
									{
										if ( memory.timeEvidenceIntruders > otherMemory.timeEvidenceIntruders ) // is my evidence of intruders later than the other's?
										{
											if ( InsideWarningVolume( memory.posEvidenceIntruders, otherOrigin, WARN_DIST_EVIDENCE_INTRUDERS ) ) // grayman #2903
											{
												// Check message queue to see if I've already sent this type of message
												// and it hasn't yet been received. Also check the other direction, from 'other' to me.
												if ( !owner->CheckOutgoingMessages( CommMessage::ConveyWarning_EvidenceOfIntruders_CommType, other ) &&
													 !static_cast<idAI*>(other)->CheckOutgoingMessages( CommMessage::ConveyWarning_EvidenceOfIntruders_CommType, owner ) )
												{
													message = CommMessagePtr(new CommMessage(
														CommMessage::ConveyWarning_EvidenceOfIntruders_CommType, 
														owner, other, // from this AI to the other
														NULL,
														owner->GetPhysics()->GetOrigin(),
														0 // grayman #3424
													));
													soundName = "snd_warnSawEvidence";
												}
											}
										}
									}
								}
							}

							// Speak the chosen sound. soundName is non-empty if a warning is being issued.

							if ( !soundName.IsEmpty() ) // grayman #2603
							{
								int delay = ( MINIMUM_TIME_BETWEEN_WARNINGS + gameLocal.random.RandomInt(VARIABLE_TIME_BETWEEN_WARNINGS))*1000;
								owner->GetMemory().GetGreetingInfo(otherAI).nextWarningTime = gameLocal.time + delay;
								delay = ( MINIMUM_TIME_BETWEEN_WARNINGS + gameLocal.random.RandomInt(VARIABLE_TIME_BETWEEN_WARNINGS))*1000;
								otherAI->GetMemory().GetGreetingInfo(owner).nextWarningTime = gameLocal.time + delay;
								memory.lastTimeVisualStimBark = gameLocal.time;
								owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new SingleBarkTask(soundName, message)));
								owner->Event_LookAtPosition(other->GetEyePosition(), 1.0 + gameLocal.random.RandomFloat()); // grayman #2925
							}
						}

						// If I'm not warning, should I greet?

						if ( soundName.IsEmpty() )
						{
							if ( owner->CanGreet() && otherAI->CanGreet() )
							{
								// grayman #3415 - Check when we can greet this AI again
								if ( gameLocal.time >= owner->GetMemory().GetGreetingInfo(other).nextGreetingTime )
								{
									if ( owner->CheckFOV(otherAI->GetEyePosition()) )
									{
										// A special GreetingBarkTask is handling this

										// Get the sound and queue the task
										idStr greetSound = GetGreetingSound(owner, otherAI);
										owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new GreetingBarkTask(greetSound, otherAI, true)));
									}
								}
							}
						}
					}
				}
				// Don't ignore in future
				ignoreStimulusFromNowOn = false;
			}
			else
			{
				// Ignore in future
				ignoreStimulusFromNowOn = true;
			}
		}
		else // neutral actor
		{
			// grayman #3317 - don't ignore actors you're neutral to, otherwise you won't
			// recognize that they died or got KO'ed later

			if (!other->IsType(idAI::Type))
			{
				return; // safeguard
			}

			idAI* otherAI = static_cast<idAI*>(other);

			if (otherAI->GetMoveType() == MOVETYPE_SLEEP)
			{
				return; // nothing else to do
			}

			// grayman #3343 - monsters and undead don't issue warnings or greetings

			if ( ( ownerAiUse != AIUSE_MONSTER ) && ( ownerAiUse != AIUSE_UNDEAD ) )
			{
				// AI don't issue warnings to neutral AI. They do issue greetings, though.

				// grayman #3202 - don't issue a greeting if you're mute
				if (!owner->m_isMute)
				{
					// apply the distance check to greetings

					const idVec3& origin = owner->GetPhysics()->GetOrigin();
					const idVec3& otherOrigin = otherAI->GetPhysics()->GetOrigin();
					idVec3 dir = origin - otherOrigin;
					dir.z = 0;
					float distSqr = dir.LengthSqr();

					if ( distSqr <= Square(REMARK_DISTANCE) )
					{
						if ( owner->CanGreet() && otherAI->CanGreet() )
						{
							// grayman #3415 - Check when we can greet this AI again
							if ( gameLocal.time >= owner->GetMemory().GetGreetingInfo(otherAI).nextGreetingTime )
							{
								if ( owner->CheckFOV(otherAI->GetEyePosition()) )
								{
									// A special GreetingBarkTask is handling this

									// Get the sound and queue the task
									idStr greetSound = GetGreetingSound(owner, otherAI);
									owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new GreetingBarkTask(greetSound, otherAI, true)));
								}
							}
						}
					}
				}
					
				// Don't ignore in future
				ignoreStimulusFromNowOn = false;
			}
			else
			{
				// Ignore in future
				ignoreStimulusFromNowOn = true;
			}
		}
	}

	if (ignoreStimulusFromNowOn)
	{
		// We've seen this object, don't respond to it again
		stimSource->IgnoreResponse(ST_VISUAL, owner);
	}
}

idStr State::GetGreetingSound(idAI* owner, idAI* otherAI)
{
	idStr soundName;

	// Get the types of the two persons
	idStr ownPersonType(owner->spawnArgs.GetString(PERSONTYPE_KEY));
	idStr otherPersonType(otherAI->spawnArgs.GetString(PERSONTYPE_KEY));

	// the other AI is a priest
	if (otherPersonType == PERSONTYPE_PRIEST)
	{
		if (ownPersonType != PERSONTYPE_PRIEST)
		{
			if (owner->spawnArgs.FindKey( "snd_greeting_cleric") != NULL)
			{
				soundName = "snd_greeting_cleric";
			}
		}

		// priests use generic greeting for other priests
		else
		{
			if (owner->spawnArgs.FindKey("snd_greeting_generic") != NULL)
			{
				soundName = "snd_greeting_generic";
			}
		}
	}

	// the other AI is a builder
	else if (otherPersonType == PERSONTYPE_BUILDER)
	{
		if (owner->spawnArgs.FindKey( "snd_greeting_builder") != NULL)
		{
			soundName = "snd_greeting_builder";
		}
	}

	// the other AI is a pagan
	else if (otherPersonType == PERSONTYPE_PAGAN)
	{
		if (owner->spawnArgs.FindKey( "snd_greeting_pagan") != NULL)
		{
			soundName = "snd_greeting_pagan";
		}
	}

	// grayman #3323 - the other AI is a beggar
	else if (otherPersonType == PERSONTYPE_BEGGAR)
	{
		if (owner->spawnArgs.FindKey( "snd_greeting_beggar") != NULL)
		{
			soundName = "snd_greeting_beggar";
		}
	}

	// this AI is a noble
	else if (ownPersonType == PERSONTYPE_NOBLE)
	{
		// nobles use generic greeting for other nobles
		if (otherPersonType == PERSONTYPE_NOBLE)
		{
			if (owner->spawnArgs.FindKey("snd_greeting_generic") != NULL)
			{
				soundName = "snd_greeting_generic";
			}
		}

		// the other AI is a guard
		else if (otherAI->spawnArgs.GetBool("is_civilian") == 0 &&
				(otherAI->GetNumMeleeWeapons() > 0 || otherAI->GetNumRangedWeapons() > 0))
		{
			if (owner->spawnArgs.FindKey( "snd_greeting_noble_to_guard") != NULL)
			{
				soundName = "snd_greeting_noble_to_guard";
			}
		}

		// the other AI is a civilian
		else
		{
			if (owner->spawnArgs.FindKey( "snd_greeting_noble_to_civilian") != NULL)
			{
				soundName = "snd_greeting_noble_to_civilian";
			}
		}
	}

	if (soundName.IsEmpty())
	{
		// this AI is a guard
		if (owner->spawnArgs.GetBool("is_civilian") == false &&
			(owner->GetNumMeleeWeapons() > 0 || owner->GetNumRangedWeapons() > 0))
		{
			// the other AI is a noble
			if (otherPersonType == PERSONTYPE_NOBLE)
			{
				idStr otherPersonGender = otherAI->spawnArgs.GetString(PERSONGENDER_KEY);
				if (otherPersonGender == PERSONGENDER_FEMALE)
				{
					if (owner->spawnArgs.FindKey( "snd_greeting_guard_to_noble_female") != NULL)
					{
						soundName = "snd_greeting_guard_to_noble_female";
					}
					else if (owner->spawnArgs.FindKey( "snd_greeting_noble_female") != NULL)
					{
						soundName = "snd_greeting_noble_female";
					}
				}
				else if (otherPersonGender == PERSONGENDER_MALE)
				{
					if (owner->spawnArgs.FindKey( "snd_greeting_guard_to_noble_male") != NULL)
					{
						soundName = "snd_greeting_guard_to_noble_male";
					}
					else if (owner->spawnArgs.FindKey( "snd_greeting_noble_male") != NULL)
					{
						soundName = "snd_greeting_noble_male";
					}
				}
			}

			// the other AI is a guard
			else if (otherAI->spawnArgs.GetBool("is_civilian") == 0 &&
				(otherAI->GetNumMeleeWeapons() > 0 || otherAI->GetNumRangedWeapons() > 0))
			{	
				if (owner->spawnArgs.FindKey( "snd_greeting_guard_to_guard") != NULL)
				{
					soundName = "snd_greeting_guard_to_guard";
				}
				else if (owner->spawnArgs.FindKey( "snd_greeting_guard") != NULL)
				{
					soundName = "snd_greeting_guard";
				}
			}

			// the other AI is a civilian
			else
			{
				// check for gender specific barks
				idStr otherPersonGender = otherAI->spawnArgs.GetString(PERSONGENDER_KEY);
				if (otherPersonGender == PERSONGENDER_FEMALE)
				{
					if (owner->spawnArgs.FindKey("snd_greeting_guard_to_female") != NULL)
					{
						soundName = "snd_greeting_guard_to_female";
					}
					else if (owner->spawnArgs.FindKey("snd_greeting_female") != NULL)
					{
						soundName = "snd_greeting_female";
					}
				}
				else if (otherPersonGender == PERSONGENDER_MALE)
				{
					if (owner->spawnArgs.FindKey("snd_greeting_guard_to_male") != NULL)
					{
						soundName = "snd_greeting_guard_to_male";
					}
					else if (owner->spawnArgs.FindKey("snd_greeting_male") != NULL)
					{
						soundName = "snd_greeting_male";
					}
				}

				// no gender specific barks, use generic greeting to civilian
				if (soundName.IsEmpty())
				{
					if (owner->spawnArgs.FindKey("snd_greeting_guard_to_civilian") != NULL)
					{
						soundName = "snd_greeting_guard_to_civilian";
					}
					else if (owner->spawnArgs.FindKey("snd_greeting_civilian") != NULL)
					{
						soundName = "snd_greeting_civilian";
					}
				}
			}
		}

		// this AI is a civilian
		else
		{
			// the other AI is a noble
			if (otherPersonType == PERSONTYPE_NOBLE)
			{
				idStr otherPersonGender = otherAI->spawnArgs.GetString(PERSONGENDER_KEY);
				if (otherPersonGender == PERSONGENDER_FEMALE)
				{
					if (owner->spawnArgs.FindKey( "snd_greeting_civilian_to_noble_female") != NULL)
					{
						soundName = "snd_greeting_civilian_to_noble_female";
					}
					else if (owner->spawnArgs.FindKey( "snd_greeting_noble_female") != NULL)
					{
						soundName = "snd_greeting_noble_female";
					}
				}
				else if (otherPersonGender == PERSONGENDER_MALE)
				{
					if (owner->spawnArgs.FindKey( "snd_greeting_civilian_to_noble_male") != NULL)
					{
						soundName = "snd_greeting_civilian_to_noble_male";
					}
					else if (owner->spawnArgs.FindKey( "snd_greeting_noble_male") != NULL)
					{
						soundName = "snd_greeting_noble_male";
					}
				}
			}

			// the other AI is a guard
			else if (otherAI->spawnArgs.GetBool("is_civilian") == false &&
				(otherAI->GetNumMeleeWeapons() > 0 || otherAI->GetNumRangedWeapons() > 0))
			{	
				if (owner->spawnArgs.FindKey("snd_greeting_civilian_to_guard") != NULL)
				{
					soundName = "snd_greeting_civilian_to_guard";
				}
				else if (owner->spawnArgs.FindKey( "snd_greeting_guard") != NULL)
				{
					soundName = "snd_greeting_guard";
				}
			}

			// the other AI is a civilian
			else
			{
				// check for gender specific barks
				idStr otherPersonGender = otherAI->spawnArgs.GetString(PERSONGENDER_KEY);
				if (otherPersonGender == PERSONGENDER_FEMALE)
				{
					if (owner->spawnArgs.FindKey("snd_greeting_civilian_to_female") != NULL)
					{
						soundName = "snd_greeting_civilian_to_female";
					}
					else if (owner->spawnArgs.FindKey("snd_greeting_female") != NULL)
					{
						soundName = "snd_greeting_female";
					}
				}
				else if (otherPersonGender == PERSONGENDER_MALE)
				{
					if (owner->spawnArgs.FindKey("snd_greeting_civilian_to_male") != NULL)
					{
						soundName = "snd_greeting_civilian_to_male";
					}
					else if (owner->spawnArgs.FindKey("snd_greeting_male") != NULL)
					{
						soundName = "snd_greeting_male";
					}
				}

				// no gender specific barks, use generic greeting to civilian
				if (soundName.IsEmpty())
				{
					if (owner->spawnArgs.FindKey("snd_greeting_civilian_to_civilian") != NULL)
					{
						soundName = "snd_greeting_civilian_to_civilian";
					}
					else if (owner->spawnArgs.FindKey("snd_greeting_civilian") != NULL)
					{
						soundName = "snd_greeting_civilian";
					}
				}
			}
		}
	}

	// no sound found yet, use generic one
	if (soundName.IsEmpty())
	{
		if (owner->spawnArgs.FindKey("snd_greeting_generic") != NULL)
		{
			soundName = "snd_greeting_generic";
		}
	}

	return soundName;
}

idStr State::GetGreetingResponseSound(idAI* owner, idAI* otherAI)
{
	// Check for rank spawnargs
	int ownerRank = owner->spawnArgs.GetInt("rank", "0");
	int otherRank = otherAI->spawnArgs.GetInt("rank", "0");

	if (ownerRank != 0 && otherRank != 0)
	{
		// Rank spawnargs valid, compare
		return (ownerRank < otherRank) ? "snd_response_positive_superior" : "snd_response_positive";
	}

	// Get the type of persons
	idStr ownPersonType(owner->spawnArgs.GetString(PERSONTYPE_KEY));
	idStr otherPersonType(otherAI->spawnArgs.GetString(PERSONTYPE_KEY));

	// Nobles, elites or priest, just use the generic ones for everybody.
	if (ownPersonType == PERSONTYPE_NOBLE || ownPersonType == PERSONTYPE_ELITE || 
		ownPersonType == PERSONTYPE_PRIEST)
	{
		// Owner is a "superior"
		return "snd_response_positive";
	}

	// Owner is not superior, check other type

	// For most characters (civilian or guard), any noble, elite or priest would be considered a superior.
	bool otherIsSuperior = (otherPersonType == PERSONTYPE_NOBLE || 
		otherPersonType == PERSONTYPE_ELITE || otherPersonType == PERSONTYPE_PRIEST);

	return (otherIsSuperior) ? "snd_response_positive_superior" : "snd_response_positive";
}

bool State::OnDeadPersonEncounter(idActor* person, idAI* owner)
{
	assert( ( person != NULL ) && ( owner != NULL ) ); // must be fulfilled
	
	// grayman #3075 - Ignore any blood markers spilled by this body if they're nearby

	if ( person->IsType(idAI::Type) )
	{
		idAI* personAI = static_cast<idAI*>(person);
		idEntity* bloodMarker = personAI->GetBlood();
		if ( bloodMarker != NULL )
		{
			idVec3 personOrg = personAI->GetPhysics()->GetOrigin();
			float bloodDistSqr = (bloodMarker->GetPhysics()->GetOrigin() - personOrg).LengthSqr();

			// Ignore blood if it's close to the body

			if ( bloodDistSqr <= Square(BLOOD2BLEEDER_MIN_DIST) )
			{
				bloodMarker->IgnoreResponse(ST_VISUAL, owner);
			}
		}
	}

	// grayman #3075 - Ignore all suspicious weapons (arrows)
	// stuck into this body

	idList<idEntity *> children;
	person->GetTeamChildren(&children); // gets the head, other attachments, and children of all
	for ( int i = 0 ; i < children.Num() ; i++ )
	{
		idEntity *child = children[i];
		if ( child == NULL )
		{
			continue;
		}
		idStr aiUse = child->spawnArgs.GetString("AIUse");
		if ( child->IsType(CProjectileResult::Type) && ( aiUse == AIUSE_SUSPICIOUS ) )
		{
			child->IgnoreResponse(ST_VISUAL, owner);
		}
	}
	
	if ( owner->IsEnemy(person) ) // grayman #3317 - allow neutrals past this point
	{
		// ignore from now on
		return true;
	}

	// We've seen this person, don't respond to them again
	person->IgnoreResponse(ST_VISUAL, owner);

	// grayman #3317 - Rats and other non-people can get to this point.
	// In case we ever want to add specific behavior for reacting to
	// a dead person (i.e. rats gnaw on corpses) we can start adding that here.
	// For now, let's only continue this if we're a person.

	idStr aiUse = owner->spawnArgs.GetString("AIUse");

	if ( aiUse != AIUSE_PERSON )
	{
		return true;
	}

	// grayman #3424 - set these alert values here instead of after
	// the delay, to lock out lesser alert stims from being processed
	Memory& memory = owner->GetMemory();
	memory.alertClass = EAlertVisual_3; // grayman #3424
	memory.alertType = EAlertTypeDeadPerson;

	// grayman #3317 - We want a random delay at this point, so we'll
	// post an event to handle the reaction. Control will go over to AI_events.cpp
	// to handle the event, and immediately call Post_OnDeadPersonEncounter() below.

	int delay = 500 + gameLocal.random.RandomInt(1000); // ms
	owner->PostEventMS(&AI_OnDeadPersonEncounter,delay,person);

	owner->m_allowAudioAlerts = false; // grayman #3424

	return true; // Ignore from now on
}

// grayman #3317

void State::Post_OnDeadPersonEncounter(idActor* person, idAI* owner)
{
	assert( ( person != NULL ) && ( owner != NULL ) ); // must be fulfilled

	owner->m_allowAudioAlerts = true; // grayman #3424

	if ( owner->AI_DEAD || owner->AI_KNOCKEDOUT )
	{
		return; // can't react if you're dead or KO'ed
	}
		
	bool fleeing = false; // TRUE = I'm going to flee, FALSE = I'm not going to flee
	bool ISawItHappen = ( gameLocal.time < ( person->m_timeFellDown + DISCOVERY_TIME_LIMIT ) );

	// If I'm a civilan, or unarmed, I'll flee
	if ( ( ( owner->GetNumMeleeWeapons() == 0 ) && ( owner->GetNumRangedWeapons() == 0) ) || owner->spawnArgs.GetBool("is_civilian") )
	{
		fleeing = true;
	}

	// If I'm not already planning to flee, react 50% of the time if this is a neutral and I didn't see it happen.

	if ( !fleeing &&
		 owner->IsNeutral(person) &&
		 ( gameLocal.random.RandomFloat() < 0.5f ) &&
		 !ISawItHappen )
	{
		owner->FoundBody(person); // register this in case Mission Objectives cares
		return;
	}

	// grayman #3424 - it's possible we already know about this
	// dead body, having been told about it by a friend. Using the event IDs
	// from our list, see if we know about this.

	bool alreadyKnow = false;
	int eventID = gameLocal.FindSuspiciousEvent( E_EventTypeDeadPerson, idVec3(0,0,0), person );
	if ( eventID >= 0 )
	{
		alreadyKnow = owner->FindSuspiciousEvent(eventID);
	}

	// The dead person is a friend or a neutral, so this is suspicious

	//gameLocal.Printf("I see a dead person!\n");
	Memory& memory = owner->GetMemory();
	memory.deadPeopleHaveBeenFound = true;

	if (cv_ai_debug_transition_barks.GetBool())
	{
		gameLocal.Printf("%s found a dead person, will use Alert Idle\n",owner->GetName());
	}

	memory.corpseFound = person; // grayman #3424
	
	if ( !alreadyKnow )
	{
		memory.countEvidenceOfIntruders += EVIDENCE_COUNT_INCREASE_CORPSE; // Three more pieces of evidence of something out of place: A dead body is a REALLY bad thing
	}

	memory.posEvidenceIntruders = person->GetPhysics()->GetOrigin(); // grayman #2903
	memory.timeEvidenceIntruders = gameLocal.time; // grayman #2903
	memory.stopRelight = true; // grayman #2603
	memory.stopExaminingRope = true; // grayman #2872 - stop examining rope
	memory.stopReactingToHit = true; // grayman #2816

	// grayman #3424 - log this event and bark if it's new
	if ( !alreadyKnow )
	{
		eventID = owner->LogSuspiciousEvent( E_EventTypeDeadPerson, person->GetPhysics()->GetOrigin(), person ); // grayman #3424  

		// Determine what to say
		// grayman #3317 - say nothing if you're a witness ( ISawItHappen is TRUE )

		if ( !ISawItHappen )
		{
			// Speak a reaction
			if ( ( gameLocal.time - memory.lastTimeVisualStimBark ) >= MINIMUM_SECONDS_BETWEEN_STIMULUS_BARKS)
			{
				idStr soundName;
				idStr personGender = person->spawnArgs.GetString(PERSONGENDER_KEY);

				if (idStr(person->spawnArgs.GetString(PERSONTYPE_KEY)) == owner->spawnArgs.GetString(PERSONTYPE_KEY))
				{
					soundName = "snd_foundComradeBody";
				}
				else if (personGender == PERSONGENDER_FEMALE)
				{
					soundName = "snd_foundDeadFemale";
				}
				else
				{
					soundName = "snd_foundDeadMale";
				}
				memory.lastTimeVisualStimBark = gameLocal.time;
				owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new SingleBarkTask(soundName)));
			}
		}
	}

	// Raise alert level if planning to search
	// grayman #3424 - for other events, the following section might not
	// be allowed, to reduce the amount of searching that happens. For the
	// case of a dead body, however, it seems appropriate to let the
	// search and alert level rise to occur.
	if ( !alreadyKnow && ( owner->AI_AlertLevel < ( owner->thresh_5 + 0.1f ) ) )
	{
		idVec3 lastAlertPosSearched = memory.alertPos; // grayman #3075
		memory.alertPos = person->GetPhysics()->GetOrigin();
		memory.currentSearchEventID = eventID; // grayman #3424
		//memory.alertClass = EAlertVisual_3; // grayman #3424 - move before the delay
		//memory.alertType = EAlertTypeDeadPerson; // grayman #3424 - move before the delay
			
		// Do search as if there is an enemy that has escaped
		memory.alertRadius = LOST_ENEMY_ALERT_RADIUS;
		memory.alertSearchVolume = LOST_ENEMY_SEARCH_VOLUME; 
		memory.alertSearchExclusionVolume.Zero();
			
		owner->AI_VISALERT = false;
		memory.visualAlert = false; // grayman #2422
		memory.mandatory = false;	// grayman #3331

		owner->SetAlertLevel(owner->thresh_5 + 0.1);

		// grayman #3075
		// Is this alert far enough away from the last one we reacted to to
		// consider it a new alert and restart the search?

		// If lastAlertPosSearched is [0,0,0], restart
		if ( lastAlertPosSearched.Compare(idVec3(0,0,0)) )
		{
			// Restart the search, in case we're already searching
			memory.restartSearchForHidingSpots = true;
		}
		else
		{
			idVec3 newAlertDeltaFromLastOneSearched(memory.alertPos - lastAlertPosSearched);
	
			if ( newAlertDeltaFromLastOneSearched.LengthSqr() > memory.alertSearchVolume.LengthSqr() )
			{
				// Restart the search, in case we're already searching
				memory.restartSearchForHidingSpots = true;
			}
		}

		// grayman #3317 - No close search if death just happened. Otherwise, there's a chance

		bool shouldKneel = ( !ISawItHappen && ( gameLocal.random.RandomFloat() < 0.5f ) );

		memory.investigateStimulusLocationClosely = shouldKneel; // deep investigation
		memory.stimulusLocationItselfShouldBeSearched = shouldKneel;
		memory.alertedDueToCommunication = false;
	}
					
	// Callback for objectives
	owner->FoundBody(person);

	// Flee if you planned to, and you aren't already fleeing
	if ( fleeing && (owner->GetMind()->GetState()->GetName() != "Flee" ) )
	{
		owner->fleeingEvent = true; // I'm fleeing the scene of the murder, not fleeing an enemy
		owner->GetMind()->SwitchState(STATE_FLEE);
	}
}

bool State::OnUnconsciousPersonEncounter(idActor* person, idAI* owner)
{
	assert( ( person != NULL ) && ( owner != NULL ) ); // must be fulfilled

	if ( owner->IsEnemy(person) ) // grayman #3317 - allow neutrals past this point
	{
		// The unconscious person is your enemy, ignore from now on
		return true;
	}

	// We've seen this person, don't respond to them again
	person->IgnoreResponse(ST_VISUAL, owner);

	// grayman #3317 - Rats and other non-people can get to this point.
	// In case we ever want to add specific behavior for reacting to
	// an unconscious person (i.e. rats gnaw on body) we can start adding that here.
	// For now, let's only continue this if we're a person.

	idStr aiUse = owner->spawnArgs.GetString("AIUse");

	if ( aiUse != AIUSE_PERSON )
	{
		return true;
	}

	// grayman #3424 - set these alert values here instead of after
	// the delay, to lock out lesser alert stims from being processed
	Memory& memory = owner->GetMemory();
	memory.alertClass = EAlertVisual_3; // grayman #3424
	memory.alertType = EAlertTypeUnconsciousPerson;

	// grayman #3317 - We want a random delay at this point, so we'll
	// post an event to handle the reaction. Control will go over to AI_events.cpp
	// to handle the event, and immediately call Post_OnUnconsciousPersonEncounter() below.

	int delay = 500 + gameLocal.random.RandomInt(1000); // ms
	owner->PostEventMS(&AI_OnUnconsciousPersonEncounter,delay,person);

	owner->m_allowAudioAlerts = false; // grayman #3424

	return true;
}

void State::Post_OnUnconsciousPersonEncounter(idActor* person, idAI* owner)
{
	assert( ( person != NULL ) && ( owner != NULL ) ); // must be fulfilled

	if ( owner->AI_DEAD || owner->AI_KNOCKEDOUT )
	{
		return; // can't react if you're dead or KO'ed
	}
		
	owner->m_allowAudioAlerts = true; // grayman #3424

	bool fleeing = false; // TRUE = I'm going to flee, FALSE = I'm not going to flee
	bool ISawItHappen = ( gameLocal.time < ( person->m_timeFellDown + DISCOVERY_TIME_LIMIT ) );

	// If I'm a civilan, or unarmed, and I saw it happen, I'll flee
	if ( ( ( owner->GetNumMeleeWeapons() == 0 ) && ( owner->GetNumRangedWeapons() == 0) ) || owner->spawnArgs.GetBool("is_civilian") )
	{
		if ( ISawItHappen )
		{
			fleeing = true;
		}
	}

	// Only react 50% of the time if this is a neutral and I didn't witness the KO.

	if ( !fleeing &&
		 owner->IsNeutral(person) &&
		 gameLocal.random.RandomFloat() < 0.5f &&
		 !ISawItHappen )
	{
		owner->FoundBody(person);
		return;
	}

	//gameLocal.Printf("I see an unconscious person!\n");

	Memory& memory = owner->GetMemory();
	memory.unconsciousPeopleHaveBeenFound = true;

	if (cv_ai_debug_transition_barks.GetBool())
	{
		gameLocal.Printf("%s found an unconscious person, will use Alert Idle\n",owner->GetName());
	}

	memory.countEvidenceOfIntruders += EVIDENCE_COUNT_INCREASE_UNCONSCIOUS; // grayman #2603
	memory.posEvidenceIntruders = owner->GetPhysics()->GetOrigin(); // grayman #2903
	memory.timeEvidenceIntruders = gameLocal.time; // grayman #2903

	memory.stopRelight = true; // grayman #2603
	memory.stopExaminingRope = true; // grayman #2872 - stop examining rope
	memory.stopReactingToHit = true; // grayman #2816

	// Determine what to say
	// grayman #3317 - say nothing if you're a witness

	if ( !ISawItHappen )
	{
		// Speak a reaction
		if ( ( gameLocal.time - memory.lastTimeVisualStimBark ) >= MINIMUM_SECONDS_BETWEEN_STIMULUS_BARKS )
		{
			idStr soundName;
			idStr personGender = person->spawnArgs.GetString(PERSONGENDER_KEY);

			if (personGender == PERSONGENDER_FEMALE)
			{
				soundName = "snd_foundUnconsciousFemale";
			}
			else
			{
				soundName = "snd_foundUnconsciousMale";
			}

			memory.lastTimeVisualStimBark = gameLocal.time;
			owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new SingleBarkTask(soundName)));
		}
	}

	// Raise alert level
	if ( owner->AI_AlertLevel < ( owner->thresh_5 + 0.1f ) )
	{
		memory.alertPos = person->GetPhysics()->GetOrigin();
			
		// Do search as if there is an enemy that has escaped
		memory.alertRadius = LOST_ENEMY_ALERT_RADIUS;
		memory.alertSearchVolume = LOST_ENEMY_SEARCH_VOLUME; 
		memory.alertSearchExclusionVolume.Zero();
			
		owner->AI_VISALERT = false;
		memory.visualAlert = false; // grayman #2422
		memory.mandatory = false;	// grayman #3331
			
		owner->SetAlertLevel(owner->thresh_5 + 0.1f);
	}
					
	// grayman #3317 - No close search if KO just happened. Otherwise, there's a chance

	bool shouldKneel = ( !ISawItHappen && ( gameLocal.random.RandomFloat() < 0.5f ) );

	memory.investigateStimulusLocationClosely = shouldKneel; // deep investigation
	memory.stimulusLocationItselfShouldBeSearched = shouldKneel;
	memory.alertedDueToCommunication = false;
		
	// Callback for objectives
	owner->FoundBody(person);

	// Flee if you planned to, and you aren't already fleeing
	if ( fleeing && (owner->GetMind()->GetState()->GetName() != "Flee" ) )
	{
		owner->fleeingEvent = true; // I'm fleeing the scene of the KO, not fleeing an enemy
		owner->GetMind()->SwitchState(STATE_FLEE);
	}
}

void State::OnFailedKnockoutBlow(idEntity* attacker, const idVec3& direction, bool hitHead)
{
	idAI* owner = _owner.GetEntity();

	if (owner == NULL)
	{
		return;
	}

	// grayman #3025 - if we're already in the failed KO state,
	// we have to let the failed KO animation finish before
	// we allow another failed KO.

	if ( owner->GetMind()->GetState()->GetName() != "FailedKnockout" )
	{
		// Switch to failed knockout state
		owner->GetMind()->PushState(StatePtr(new FailedKnockoutState(attacker, direction, hitHead)));
	}
}

void State::OnProjectileHit(idProjectile* projectile, idEntity* attacker, int damageTaken)
{
	idAI* owner = _owner.GetEntity();
	if ( owner == NULL )
	{
		return;
	}

	if ( owner->AI_DEAD || owner->AI_KNOCKEDOUT ) // grayman #3424
	{
		return;
	}

	bool isAfraid = ( ( ( owner->GetNumMeleeWeapons() == 0 ) && ( owner->GetNumRangedWeapons() == 0 ) ) ||
						owner->spawnArgs.GetBool("is_civilian", "0") );

	// grayman #3331 - If you're a civilian, or you're unarmed, flee!
	// But only if no damage was done. When damaged, the flee is handled
	// by PainState, because we have to wait for the pain animation
	// to finish.
	if ( damageTaken == 0 )
	{
		if ( isAfraid )
		{
			// grayman #3140 - Emit the snd_taking_fire bark

			// This will hold the message to be delivered with the bark
			CommMessagePtr message;
	
			message = CommMessagePtr(new CommMessage(
				CommMessage::SearchOrder_CommType, 
				owner, NULL, // from this AI to anyone 
				NULL,
				owner->GetPhysics()->GetOrigin(),
				0
			));

			owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new SingleBarkTask("snd_taking_fire", message)));

			owner->fleeingEvent = true; // I'm fleeing because I was hit, not fleeing an enemy
			owner->GetMind()->SwitchState(STATE_FLEE);
			return;
		}
	}

	// grayman #2801 - When the projectile_result from an arrow isn't allowed
	// to stick around (leading to the AI barking about something suspicious),
	// this is where we have to alert the AI, regardless of whether he was
	// damaged or not.

	EAlertType alertType;

	// grayman #3331 - someone just hit you with a projectile. Unless you're
	// already in combat mode, you should react to this regardless of what
	// else you're doing.

	alertType = EAlertTypeHitByProjectile; // grayman #3331

	if ( !ShouldProcessAlert( alertType ) )
	{
		DM_LOG(LC_AI, LT_INFO)LOGSTRING("Ignoring projectile hit.\r");
		return;
	}

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("Alerting AI %s due to projectile.\r", owner->name.c_str());
	
	// grayman #3140 - If a civilian or not armed, you only got here because
	// damage was taken. PainState will set up fleeing, and we don't want
	// you searching, so there's nothing remaining for you to do here.

	if ( isAfraid )
	{
		owner->SetAlertLevel(owner->thresh_5 - 0.1f);

		// Treat getting hit by a projectile as proof that an enemy is present.
		Memory& memory = owner->GetMemory();
		memory.posEnemySeen = owner->GetPhysics()->GetOrigin();
		memory.currentSearchEventID = owner->LogSuspiciousEvent( E_EventTypeEnemy, owner->GetPhysics()->GetOrigin(), NULL ); // grayman #3424  
		return;
	}

	// At this point, if no damage was taken, emit a bark.
	if ( damageTaken == 0 )
	{
		// grayman #3140 - Emit the snd_taking_fire bark

		// This will hold the message to be delivered with the bark
		CommMessagePtr message;
	
		message = CommMessagePtr(new CommMessage(
			CommMessage::SearchOrder_CommType, 
			owner, NULL, // from this AI to anyone 
			NULL,
			owner->GetPhysics()->GetOrigin(),
			0
		));

		owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new SingleBarkTask("snd_taking_fire", message)));
	}

	// At this point, you're armed and not a civilian, and either damage was taken, or it wasn't.
	// Set up a search.

	if ( owner->AI_AlertLevel < owner->thresh_5 ) // grayman #3331 - ignore only if in combat
	{
		Memory& memory = owner->GetMemory();

		// greebo: Set the alert position not directly to the attacker's origin, but let the AI 
		// search in the right direction

		const idVec3& ownerOrigin = owner->GetPhysics()->GetOrigin();

		idVec3 attackerDir(0,0,0);
		float distance = 0;
		
		if (attacker != NULL)
		{
			attackerDir = attacker->GetPhysics()->GetOrigin() - ownerOrigin;
			distance = attackerDir.NormalizeFast();
		}

		// Start searching halfway between us and the attacker

		memory.alertPos = ownerOrigin + attackerDir * distance * 0.5f;

		// grayman #3331 - trace down until you hit something
		idVec3 bottomPoint = memory.alertPos;
		bottomPoint.z -= 1000;

		trace_t result;
		if ( gameLocal.clip.TracePoint(result, memory.alertPos, bottomPoint, MASK_OPAQUE, NULL) )
		{
			// Found the floor.
			memory.alertPos.z = result.endpos.z + 1; // move the target point to just above the floor
		}

		memory.alertClass = EAlertTactile;
		memory.alertType = alertType;
		memory.currentSearchEventID = owner->LogSuspiciousEvent( E_EventTypeEnemy, memory.alertPos, NULL ); // grayman #3424
		
		// Do search as if there is an enemy that has escaped
		memory.alertRadius = TACTILE_ALERT_RADIUS;
		memory.alertSearchVolume = TACTILE_SEARCH_VOLUME*2; 
		memory.alertSearchExclusionVolume.Zero();

		memory.stimulusLocationItselfShouldBeSearched = true; // grayman #3331 - start search at alertPos
		memory.investigateStimulusLocationClosely = false;
		memory.restartSearchForHidingSpots = true; // grayman #3331
		owner->AI_VISALERT = false;
		memory.visualAlert = false; // grayman #2422			
		memory.mandatory = true; // grayman #3331
		
		owner->SetAlertLevel(owner->thresh_5 - 0.1f);

		owner->TurnToward(memory.alertPos); // grayman #3331
	}
}


void State::OnMovementBlocked(idAI* owner)
{
	//DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("State::OnMovementBlocked: %s ...\r", owner->name.c_str());

	// Determine which type of object is blocking us

	const idVec3& ownerOrigin = owner->GetPhysics()->GetOrigin();

	// Set all attachments to nonsolid, temporarily
	owner->SaveAttachmentContents();
	owner->SetAttachmentContents(0);

	trace_t result;
	idVec3 dir = owner->viewAxis.ToAngles().ToForward()*20;
	idVec3 traceEnd;
	idBounds bnds = owner->GetPhysics()->GetBounds();
	traceEnd = ownerOrigin + dir;
	gameLocal.clip.TraceBounds(result, ownerOrigin, traceEnd, bnds, CONTENTS_SOLID|CONTENTS_CORPSE, owner);

	owner->RestoreAttachmentContents(); // Put back attachments
	
	idEntity* ent = NULL; // grayman #2345 - moved up
	if (result.fraction >= 1.0f)
	{
		idEntity* tactileEntity = owner->GetTactileEntity(); // grayman #2345
		if (tactileEntity) // grayman #2345
		{
			ent = tactileEntity;
		}
	}
	else
	{
		ent = gameLocal.entities[result.c.entityNum];
	}

	if (ent == NULL) 
	{
		// Trace didn't hit anything?
		//DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("State::OnMovementBlocked: %s can't find what's blocking\r", owner->name.c_str());
		return;
	}

	if (ent == gameLocal.world)
	{
		//DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("State::OnMovementBlocked: %s is blocked by world!\r", owner->name.c_str());
		return;
	}

	//DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("State::OnMovementBlocked: %s is blocked by entity %s\r", owner->name.c_str(), ent->name.c_str());

	if (cv_ai_debug_blocked.GetBool())
	{
		gameRenderWorld->DebugBounds(colorRed, ent->GetPhysics()->GetBounds().Rotate(ent->GetPhysics()->GetAxis()), ent->GetPhysics()->GetOrigin(), 2000);
	}

	// Do something against this blocking entity, depending on its type
	
	if (ent->IsType(idAI::Type))
	{
		// we found what we wanted, so handle it below
	}
	else if (ent->IsType(idAFAttachment::Type))
	{
		// This ought to be an AI's head, get its body
		ent = static_cast<idAFAttachment*>(ent)->GetBody();
	}
	else // grayman #2345 - take care of bumping into attached func_statics
	{
		// This might be a func_static attached to an AI (i.e. a pauldron)

		idEntity* ent2 = ent->GetBindMaster();
		if (ent2)
		{
			if (ent2->IsType(idAI::Type))
			{
				ent = ent2; // switch to AI bindMaster
			}
			else if (ent2->IsType(idAFAttachment::Type))
			{
				ent = static_cast<idAFAttachment*>(ent2)->GetBody(); // switch to the AI bindMaster's owner
			}
		}
	}

	if (ent->IsType(idAI::Type))
	{
		// Blocked by other AI
		idAI* master = owner;
		idAI* slave = static_cast<idAI*>(ent);

		// grayman #2728 - check mass; this overrides all other master/slave checks

		float masterMass = master->GetPhysics()->GetMass();
		if ((masterMass <= 5.0) || (slave->GetPhysics()->GetMass() <= 5.0))
		{
			if (masterMass <= 5.0)
			{
				// The master can't have small mass unless the slave does also
				std::swap(master, slave);
			}
		}
		else if (master->AI_FORWARD && // grayman #2422
				 !slave->AI_FORWARD &&
				 master->IsSearching() &&
				 ( master->AI_AlertIndex < ECombat ) ) // grayman #3070 - don't stop master if he's in combat
		{
			// Stop moving, the searching state will choose another spot soon
			master->StopMove(MOVE_STATUS_DONE);
			Memory& memory = master->GetMemory();
			memory.stopRelight = true; // grayman #2603 - abort a relight in progress
			memory.stopExaminingRope = true; // grayman #2872 - stop examining rope
			memory.stopReactingToHit = true; // grayman #2816
			master->TurnToward(master->GetCurrentYaw() + 180); // turn back toward where you came from
			return;
		}
		else
		{
			// grayman #2345 - account for rank when determining who should resolve the block

			if (slave->rank > master->rank)
			{
				// The master should have equal or higher rank
				std::swap(master, slave);
			}

			if (!master->AI_FORWARD && slave->AI_FORWARD)
			{
				// Master is not moving, swap
				std::swap(master, slave);
			}

			if (slave->AI_FORWARD && slave->AI_RUN && master->AI_FORWARD && !master->AI_RUN)
			{
				// One AI is running, this should be the master
				std::swap(master, slave);
			}

			if (slave->movementSubsystem->IsResolvingBlock() || !slave->m_canResolveBlock) // grayman #2345
			{
				std::swap(master, slave);
			}
		}

		// Tell the slave to get out of the way.

		slave->movementSubsystem->ResolveBlock(master);
	}
	else if (ent->IsType(idStaticEntity::Type))
	{
		// Blocked by func_static, these are generally not considered by Obstacle Avoidance code.
		// grayman #2345 - if the AI is bumping into a func_static, that's included.

		if (!owner->movementSubsystem->IsResolvingBlock()) // grayman #2345
		{
			owner->movementSubsystem->ResolveBlock(ent);
		}
	}
}

void State::OnVisualStimBlood(idEntity* stimSource, idAI* owner)
{
	assert( ( stimSource != NULL ) && ( owner != NULL ) ); // must be fulfilled

	Memory& memory = owner->GetMemory();

	// Ignore from now on
//	stimSource->IgnoreResponse(ST_VISUAL, owner); // grayman #2924 - already done

	// angua: ignore blood after dead bodies have been found
/*     grayman #3075 - no longer
	if (memory.deadPeopleHaveBeenFound)
	{
		return;
	}
 */

	// grayman #3075 - Each blood marker knows who spilled it.
	// If the body is nearby and visible, don't process this blood marker.

	CBloodMarker *marker = static_cast<CBloodMarker*>(stimSource);
	idAI *bleeder = marker->GetSpilledBy();
	if ( bleeder != NULL )
	{
		// Is the body near the blood marker?
	
		idVec3 bleederOrg = bleeder->GetPhysics()->GetOrigin();
		float bloodDistSqr = (marker->GetPhysics()->GetOrigin() - bleederOrg).LengthSqr();

		// Can we see the bleeder? Use FOV, but use lighting
		// only if beyond a min distance.

		if ( owner->CanSeeExt(bleeder,true,( bloodDistSqr > Square(BLOOD2BLEEDER_MIN_DIST) )) )
		{
			// grayman #3317 - The body will be found separately, so don't process the blood marker
			return;
		}
	}

	// Vocalize that see something out of place
	memory.lastTimeVisualStimBark = gameLocal.time;
	owner->commSubsystem->AddCommTask(
		CommunicationTaskPtr(new SingleBarkTask("snd_foundBlood"))
	);
	//gameLocal.Printf("Is that blood?\n");
	
	// One more piece of evidence of something out of place
	memory.countEvidenceOfIntruders += EVIDENCE_COUNT_INCREASE_BLOOD;
	memory.posEvidenceIntruders = owner->GetPhysics()->GetOrigin(); // grayman #2903
	memory.timeEvidenceIntruders = gameLocal.time; // grayman #2903
	memory.stopRelight = true; // grayman #2603
	memory.stopExaminingRope = true; // grayman #2872 - stop examining rope
	memory.stopReactingToHit = true; // grayman #2816

	// Raise alert level
	if (owner->AI_AlertLevel < owner->thresh_5 - 0.1f)
	{
		memory.alertPos = stimSource->GetPhysics()->GetOrigin();
		memory.alertClass = EAlertVisual_1;
		memory.alertType = EAlertTypeBlood;
		
		// Do search as if there is an enemy that has escaped
		memory.alertRadius = LOST_ENEMY_ALERT_RADIUS;
		memory.alertSearchVolume = LOST_ENEMY_SEARCH_VOLUME; 
		memory.alertSearchExclusionVolume.Zero();
		
		owner->AI_VISALERT = false;
		memory.visualAlert = false; // grayman #2422
		memory.mandatory = false;	// grayman #3331

		owner->SetAlertLevel(owner->thresh_5 - 0.1f);
	}
				
	// Do new reaction to stimulus
	memory.investigateStimulusLocationClosely = true; // deep investigation
	memory.stimulusLocationItselfShouldBeSearched = true;
	memory.alertedDueToCommunication = false;
}

/*
================
State::CheckTorch

grayman #2603 - Check if torch has gone out
================
*/
bool State::CheckTorch(idAI* owner, idLight* light)
{
	if (owner->m_DroppingTorch) // currently dropping my torch?
	{
		return false; // can't relight at the moment
	}

	// If I'm carrying a torch, and it's out, toss it.

	idEntity* torch = owner->GetTorch();

	if (torch)
	{
		idLight* torchLight = NULL;
		idList<idEntity *> children;
		torch->GetTeamChildren(&children);
		for (int i = 0 ; i < children.Num() ; i++)
		{
			if (children[i]->IsType(idLight::Type))
			{
				torchLight = static_cast<idLight*>(children[i]);
				break;
			}
		}

		// If my torch has gone out--regardless of whether or not it's what
		// gave us the current stim--then I should toss it. Otherwise I
		// might try to relight a doused torch with my doused torch.

		if (torchLight)
		{
			if ((torchLight->GetLightLevel() == 0) || (torchLight == light) || (torchLight->IsSmoking()))
			{
				// At this point, I know my torch is out

				if (owner->AI_AlertLevel < owner->thresh_5) // don't drop if in combat mode
				{
					// drop the torch (torch is detached by a frame command in the animation)

					// grayman #3075 - no longer needed
//					torchLight->spawnArgs.Set("shouldBeOn", "0");	// don't relight
//					torch->spawnArgs.Set("shouldBeOn", "0");		// insurance
//					torchLight->SetStimEnabled(ST_VISUAL,false);	// turn off visual stim; no one cares

					// use one animation if alert level is 4, another if not

					idStr animName = "drop_torch";
					if (owner->AI_AlertLevel >= owner->thresh_4)
					{
						animName = "drop_torch_armed";
					}
					owner->actionSubsystem->PushTask(TaskPtr(new PlayAnimationTask(animName,4)));
					owner->m_DroppingTorch = true;
				}

				// grayman #3077 - aborting a relight this way kills the
				// PlayAnimationTask() request, causing the AI to never drop
				// his torch

/*				// If you're in the middle of lighting a light, stop
				if (owner->m_RelightingLight)
				{
					owner->GetMemory().stopRelight = true;
				}
*/
				return false; // My torch is out, so don't start a relight
			}
		}
		else
		{
			return false; // couldn't find the light of my torch
		}
	}

	return true; // I'm not carrying a torch, or I am and it's still on
}

void State::OnVisualStimLightSource(idEntity* stimSource, idAI* owner)
{
	// grayman #2603 - a number of changes were made in this method

	assert(stimSource != NULL && owner != NULL); // must be fulfilled

	Memory& memory = owner->GetMemory();

	idLight* light = dynamic_cast<idLight*>(stimSource);

	if (light == NULL)
	{
		// not a light
		return;
	}

	// If I'm in combat mode, do nothing

	if (owner->AI_AlertLevel >= owner->thresh_5)
	{
		return;
	}

	// What type of light is it?

	idStr lightType = stimSource->spawnArgs.GetString(AIUSE_LIGHTTYPE_KEY);

	if ((lightType != AIUSE_LIGHTTYPE_TORCH) && (lightType != AIUSE_LIGHTTYPE_ELECTRIC))
	{
		// Can't handle this type of light, so exit this state and ignore the light.

		stimSource->IgnoreResponse(ST_VISUAL, owner);
		return;
	}

	// Have we reached the end of a delay for relighting this light?

	if (gameLocal.time < light->GetRelightAfter())
	{
		return; // process later
	}

	// grayman #3075 - don't process the flame of a carried torch
	if ( lightType == AIUSE_LIGHTTYPE_TORCH )
	{
		// Does this light belong to a carried torch?
		idEntity* bindMaster = light->GetBindMaster();
		while ( bindMaster != NULL )
		{
			if ( bindMaster->spawnArgs.GetBool("is_torch","0") )
			{
				light->SetStimEnabled(ST_VISUAL,false);	// turn off visual stim; no one cares
				return;
			}
			bindMaster = bindMaster->GetBindMaster(); // go up the hierarchy
		}
	}

	// Don't process a light the player is carrying

	CGrabber* grabber = gameLocal.m_Grabber;
	if (grabber)
	{
		idEntity* heldEnt = grabber->GetSelected();
		if (heldEnt)
		{
			idEntity* e = stimSource->GetBindMaster();
			while (e != NULL) // not carried when e == NULL
			{
				if (heldEnt == e)
				{
					// Don't ignore this light, in case the player puts it down somewhere.
					// But all AI should delay responding to it again.

					light->SetRelightAfter();
					return;
				}
				e = e->GetBindMaster(); // go up the hierarchy
			}
		}
	}

	// The next section is about whether we should turn the light back on.

	// if this is a flame and it's not vertical, ignore it

	if (!light->IsVertical(10)) // w/in 10 degrees of vertical?
	{
		light->SetRelightAfter();	// don't ignore it, but all AI should wait a while before
									// paying attention to it again, in case it rights itself
		return;
	}

	// Some light models (like wall torches) attach lights at
	// spawn time. The shouldBeOn spawnarg might be set on the bindMaster model (or
	// any of the parent bindMasters, in the case of candles in holders), and
	// we have to check that if it's not set on the stim. If it's set on any
	// bindMaster, it's also considered to be set on the stim (the light).

	SBO_Level shouldBeOn = static_cast<SBO_Level>(stimSource->spawnArgs.GetInt(AIUSE_SHOULDBEON_LEVEL,"0")); // grayman #2603

	// check whether any of the parent bindMasters has a higher value for 'shouldBeOn'

	if (shouldBeOn < (ENumSBOLevels - 1))
	{
		idEntity* bindMaster = stimSource->GetBindMaster();
		while (bindMaster != NULL)
		{
			SBO_Level sbo = static_cast<SBO_Level>(bindMaster->spawnArgs.GetInt(AIUSE_SHOULDBEON_LEVEL,"0"));
			if (sbo > shouldBeOn)
			{
				shouldBeOn = sbo;
			}
			bindMaster = bindMaster->GetBindMaster(); // go up the hierarchy
		}
	}

	// If I'm only barking, and not relighting, I'm done.

	if (shouldBeOn == ESBO_0)
	{
		// Vocalize that I see a light that's off. But don't bark too often.

		if (light->NegativeBark(owner))
		{
			idStr bark;
			if (gameLocal.random.RandomFloat() < 0.5)
			{
				bark = "snd_noRelightTorch";
			}
			else
			{
				bark = (lightType == AIUSE_LIGHTTYPE_TORCH) ? "snd_foundTorchOut" : "snd_foundLightsOff";
			}
			CommMessagePtr message; // no message, but the argument is needed so the start delay can be included
			owner->GetSubsystem(SubsysCommunication)->PushTask(TaskPtr(new SingleBarkTask(bark,message,2000,false))); // grayman #3182
			//gameLocal.Printf("That light should be on! But I won't relight it now.\n");
		}
		return;
	}

	// Should this raise an alert?

	// ESBO_0 lights don't raise an alert.
	// ESBO_1 lights raise an alert if the light has been off for less than a defined time.
	// ESBO_2 lights always raise an alert.

	if ((shouldBeOn == ESBO_2) || ((shouldBeOn == ESBO_1) && (gameLocal.time < (light->GetWhenTurnedOff() + MIN_TIME_LIGHT_ALERT))))
	{
		// One more piece of evidence of something out of place

		idEntityPtr<idEntity> stimSourcePtr;
		stimSourcePtr = stimSource;
		if (owner->m_dousedLightsSeen.Find(stimSourcePtr) == NULL)
		{
			// this light is NOT on my list of doused lights, so let it contribute evidence

			memory.countEvidenceOfIntruders += EVIDENCE_COUNT_INCREASE_LIGHT;
			memory.posEvidenceIntruders = owner->GetPhysics()->GetOrigin(); // grayman #2903
			memory.timeEvidenceIntruders = gameLocal.time; // grayman #2903
			owner->m_dousedLightsSeen.Append(stimSourcePtr); // add this light to the list

			// grayman #3438 - move alert level change to SwitchOnLightState
/*			// Raise alert level if we already have some evidence of intruders

			if ((owner->AI_AlertLevel < owner->thresh_3) && 
				(memory.enemiesHaveBeenSeen || (memory.countEvidenceOfIntruders >= MIN_EVIDENCE_OF_INTRUDERS_TO_SEARCH_ON_LIGHT_OFF)))
			{
				owner->SetAlertLevel(owner->thresh_3 - 0.1 + (owner->thresh_4 - owner->thresh_3) * 0.2
					* (memory.countEvidenceOfIntruders - MIN_EVIDENCE_OF_INTRUDERS_TO_SEARCH_ON_LIGHT_OFF)); // grayman #2603 - subtract a tenth

				if (owner->AI_AlertLevel >= (owner->thresh_5 + owner->thresh_4) * 0.5)
				{
					owner->SetAlertLevel((owner->thresh_5 + owner->thresh_4) * 0.45);
				}
			}
 */
			owner->m_LatchedSearch = true; // set up search after light is relit
		}
	}

	// The next section is about whether we have the ability to turn lights back on.

	bool turnLightOn = true;
	idEntity* inHand;

	if (lightType == AIUSE_LIGHTTYPE_TORCH)
	{
		if (owner->spawnArgs.GetBool("canLightTorches") &&
			(gameLocal.random.RandomFloat() < owner->spawnArgs.GetFloat("chanceLightTorches")))
		{
			if (owner->GetTorch() == NULL)
			{
				// No torch, so we drop back to the tinderbox method.
				// If the AI is carrying something other than a weapon,
				// disallow the tinderbox, because its animation can cause problems.
				// Wielding a weapon, since the tinderbox animation causes
				// the weapon to be sheathed and drawn again.

				inHand = owner->GetAttachmentByPosition("hand_l");
				if (inHand)
				{
					// Something in the left hand, so can't use tinderbox

					turnLightOn = false;
				}
				else
				{
					// Bow in left hand? Can use tinderbox. The bow is the only thing
					// attached at hand_l_bow because of the orientation involved.

					inHand = owner->GetAttachmentByPosition("hand_l_bow"); // check the bow
					if (!inHand)
					{
						// No bow. Anything in the right hand other than a melee weapon prevents using the tinderbox.

						inHand = owner->GetAttachmentByPosition("hand_r");
						if (inHand && (idStr::Cmp(inHand->spawnArgs.GetString("AIUse"), AIUSE_WEAPON) != 0))
						{
							turnLightOn = false;
						}
					}
				}
			}
		}
		else
		{
			turnLightOn = false;
		}
	}
	else // electric
	{
		if (owner->spawnArgs.GetBool("canOperateSwitchLights") &&
			(gameLocal.random.RandomFloat() < owner->spawnArgs.GetFloat("chanceOperateSwitchLights")))
		{
			// Anything in the right hand other than a melee weapon prevents an electric relight.

			inHand = owner->GetAttachmentByPosition("hand_r");
			if (inHand && (idStr::Cmp(inHand->spawnArgs.GetString("AIUse"), AIUSE_WEAPON) != 0))
			{
				turnLightOn = false;
			}
		}
		else
		{
			turnLightOn = false;
		}
	}

	// Turning the light on?

	if (turnLightOn)
	{
		owner->m_RelightingLight = true;
		memory.relightLight = light; // grayman #2603
		memory.stopRelight = false;
		light->SetBeingRelit(true); // this light is being relit
		stimSource->IgnoreResponse(ST_VISUAL,owner); // ignore this stim while turning the light back on
		owner->GetMind()->SwitchState(StatePtr(new SwitchOnLightState(light))); // set out to relight
		//gameLocal.Printf("%s - That light %s should be on! And I'm going to relight it.\n",owner->GetName(),stimSource->GetName());
	}
	else // Can't relight
	{
		idEntityPtr<idEntity> stimPtr;
		stimPtr = stimSource;
		owner->SetDelayedStimExpiration(stimPtr); // try again after a while
		if (light->NegativeBark(owner))
		{
			idStr bark;
			if (gameLocal.random.RandomFloat() < 0.5)
			{
				bark = "snd_noRelightTorch";
			}
			else
			{
				bark = (lightType == AIUSE_LIGHTTYPE_TORCH) ? "snd_foundTorchOut" : "snd_foundLightsOff";
			}
			CommMessagePtr message; // no message, but the argument is needed so the start delay can be included
			owner->GetSubsystem(SubsysCommunication)->PushTask(TaskPtr(new SingleBarkTask(bark,message,2000,false))); // grayman #3182
			//gameLocal.Printf("That light should be on! But I won't relight it now.\n");
		}
	}
}

void State::OnVisualStimMissingItem(idEntity* stimSource, idAI* owner)
{
	assert(stimSource != NULL && owner != NULL); // must be fulfilled

	Memory& memory = owner->GetMemory();

	// We've seen this object, don't respond to it again
//	stimSource->IgnoreResponse(ST_VISUAL, owner); // grayman #2924 - already done
	
	// Can we notice missing items
	if (owner->spawnArgs.GetFloat("chanceNoticeMissingItem") <= 0.0)
	{
		return;
	}

	// Does it belong to a friendly team
	if (stimSource->team != -1 && !owner->IsFriend(stimSource))
	{
		// Its not something we know about
		//gameLocal.Printf("The missing item wasn't on my team\n");
		return;
	}

	float alert = owner->AI_AlertLevel; // grayman #3424 - initialize

	if (stimSource->IsType(CAbsenceMarker::Type))
	{
		CAbsenceMarker* absenceMarker = static_cast<CAbsenceMarker*>(stimSource);
		const idDict& refSpawnargs = absenceMarker->GetRefSpawnargs();

		float chance(gameLocal.random.RandomFloat());
		if (chance >= refSpawnargs.GetFloat("absence_noticeability", "1"))
		{
			float recheckInterval = SEC2MS(refSpawnargs.GetFloat("absence_noticeability_recheck_interval", "60"));
			if (recheckInterval > 0.0f)
			{
				stimSource->PostEventMS( &EV_ResponseAllow, recheckInterval, ST_VISUAL, owner);
			}
			return;
		}
		if (refSpawnargs.GetFloat("absence_alert", "0") > 0)
		{
			alert = owner->AI_AlertLevel + refSpawnargs.GetFloat("absence_alert", "0");
		}
	}

	// grayman #3424 - it's possible we already know about this
	// missing item, having been told about it by a friend. Using the event IDs
	// from our list, see if we know about this.

	bool alreadyKnow = false;
	int eventID = gameLocal.FindSuspiciousEvent( E_EventTypeMissingItem, stimSource->GetPhysics()->GetOrigin(), NULL );
	if ( eventID >= 0 )
	{
		alreadyKnow = owner->FindSuspiciousEvent(eventID);
	}

	if (!alreadyKnow)
	{
		//gameLocal.Printf("Something is missing from over there!\n");
	}

	if ( alert < ( owner->thresh_4 + 0.1f ) )
	{
		alert = owner->thresh_4 + 0.1f; // grayman #2903 - put the AI into agitated searching so he draws his weapon
	}

	if ( !alreadyKnow ) // grayman #3424
	{
		// Speak a reaction
		memory.lastTimeVisualStimBark = gameLocal.time;
		owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new SingleBarkTask("snd_foundMissingItem")));
	}

	// One more piece of evidence of something out of place
	memory.itemsHaveBeenStolen = true;

	if (cv_ai_debug_transition_barks.GetBool()) 
	{
		gameLocal.Printf("%s sees that something is missing, will use Alert Idle\n",owner->GetName());
	}

	if ( !alreadyKnow )
	{
		memory.countEvidenceOfIntruders += EVIDENCE_COUNT_INCREASE_MISSING_ITEM;
	}
	memory.posEvidenceIntruders = stimSource->GetPhysics()->GetOrigin(); // grayman #2903
	memory.timeEvidenceIntruders = gameLocal.time; // grayman #2903
	memory.posMissingItem = stimSource->GetPhysics()->GetOrigin(); // grayman #2903

	memory.stopRelight = true; // grayman #2603
	memory.stopExaminingRope = true; // grayman #2872 - stop examining rope
	memory.stopReactingToHit = true; // grayman #2816

	// grayman #3424 - log this event if it's new
	if ( !alreadyKnow )
	{
		eventID = owner->LogSuspiciousEvent( E_EventTypeMissingItem, stimSource->GetPhysics()->GetOrigin(), NULL ); // grayman #3424  
	}

	// Raise alert level if you didn't already know about this
	if ( !alreadyKnow && ( owner->AI_AlertLevel < alert ) )
	{
		memory.alertPos = stimSource->GetPhysics()->GetOrigin();
		memory.alertClass = EAlertVisual_1;
		memory.alertType = EAlertTypeMissingItem;
		memory.currentSearchEventID = eventID; // grayman #3424
		
		// Prepare search as if there is an enemy that has escaped
		memory.alertRadius = LOST_ENEMY_ALERT_RADIUS;
		memory.alertSearchVolume = LOST_ENEMY_SEARCH_VOLUME; 
		memory.alertSearchExclusionVolume.Zero();
		
		owner->AI_VISALERT = false;
		memory.visualAlert = false; // grayman #2422
		memory.mandatory = false;	// grayman #3331
		
		owner->SetAlertLevel(alert);
	}
}

void State::OnVisualStimBrokenItem(idEntity* stimSource, idAI* owner)
{
	assert(stimSource != NULL && owner != NULL); // must be fulfilled

	Memory& memory = owner->GetMemory();

	// We've seen this object, don't respond to it again
//	stimSource->IgnoreResponse(ST_VISUAL, owner); // grayman #2924 - already done

	//gameLocal.Printf("Something is broken over there!\n");

	owner->StopMove(MOVE_STATUS_DONE);
	owner->TurnToward(stimSource->GetPhysics()->GetOrigin());
	owner->Event_LookAtEntity(stimSource, 1);
	memory.stopRelight = true; // grayman #2603 - abort a relight in progress
	memory.stopExaminingRope = true; // grayman #2872 - stop examining rope
	memory.stopReactingToHit = true; // grayman #2816

	// Speak a reaction
	memory.lastTimeVisualStimBark = gameLocal.time;
	owner->commSubsystem->AddCommTask(
		CommunicationTaskPtr(new SingleBarkTask("snd_foundBrokenItem"))
	);

	owner->AI_RUN = true;

	// One more piece of evidence of something out of place
	memory.itemsHaveBeenBroken = true;

	if (cv_ai_debug_transition_barks.GetBool())
	{
		gameLocal.Printf("%s sees something broken, will use Alert Idle\n",owner->GetName());
	}

	memory.countEvidenceOfIntruders += EVIDENCE_COUNT_INCREASE_BROKEN_ITEM;
	memory.posEvidenceIntruders = owner->GetPhysics()->GetOrigin(); // grayman #2903
	memory.timeEvidenceIntruders = gameLocal.time; // grayman #2903

	// Raise alert level
	if (owner->AI_AlertLevel < owner->thresh_4 - 0.1f)
	{
		memory.alertPos = stimSource->GetPhysics()->GetOrigin();
		memory.alertClass = EAlertVisual_2; // grayman #2603
		memory.alertType = EAlertTypeBrokenItem;
		
		// Prepare search as if there is an enemy that has escaped
		memory.alertRadius = LOST_ENEMY_ALERT_RADIUS;
		memory.alertSearchVolume = LOST_ENEMY_SEARCH_VOLUME; 
		memory.alertSearchExclusionVolume.Zero();
		
		owner->AI_VISALERT = false;
		memory.visualAlert = false; // grayman #2422
		memory.mandatory = false;	// grayman #3331
		
		owner->SetAlertLevel(owner->thresh_5 - 0.1);
	}
}

// grayman #3104

bool State::SomeoneNearDoor(idAI* owner, CFrobDoor* door)
{
	int num;
	idEntity* candidate;
	idClipModel* cm;
	idClipModel* clipModels[ MAX_GENTITIES ];

	idVec3 doorCenter = door->GetClosedBox().GetCenter();
	idBounds doorBounds(idVec3(doorCenter.x - PERSON_NEAR_DOOR, doorCenter.y - PERSON_NEAR_DOOR, doorCenter.z - 50),
						idVec3(doorCenter.x + PERSON_NEAR_DOOR, doorCenter.y + PERSON_NEAR_DOOR, doorCenter.z + 50));
	
	num = gameLocal.clip.ClipModelsTouchingBounds( doorBounds, MASK_MONSTERSOLID, clipModels, MAX_GENTITIES );
	for ( int i = 0 ; i < num ; i++ )
	{
		cm = clipModels[i];
		candidate = cm->GetEntity();
		if ( candidate == owner )
		{
			continue; // skip observer
		}

		if ( !candidate->IsType(idAI::Type) )
		{
			continue; // skip non-AIs
		}

		idAI* candidateAI = static_cast<idAI*>(candidate);

		if ( !candidateAI->m_bCanOperateDoors )
		{
			continue; // skip AIs who can't handle doors
		}

		// check visibility

		if ( owner->CanSeeExt( candidateAI, false, false ) )
		{
			return true;
		}
	}

	return false; // no visible door-handling AI near the door
}

void State::OnVisualStimDoor(idEntity* stimSource, idAI* owner)
{
	assert( ( stimSource != NULL ) && ( owner != NULL ) ); // must be fulfilled

	Memory& memory = owner->GetMemory();
	CFrobDoor* door = static_cast<CFrobDoor*>(stimSource);

	// grayman #2924 - enable the response
	stimSource->AllowResponse(ST_VISUAL, owner);

	// Update the info structure for this door
	DoorInfo& doorInfo = memory.GetDoorInfo(door);

	doorInfo.lastTimeSeen = gameLocal.time;
	doorInfo.wasOpen = door->IsOpen();

	// greebo: If the door is open, remove the corresponding area from the "forbidden" list
	if (door->IsOpen()) 
	{
		// Also, reset the "locked" property, open doors can't be locked
		doorInfo.wasLocked = false;

		// Enable the area for pathfinding again now that the door is open
		gameLocal.m_AreaManager.RemoveForbiddenArea(doorInfo.areaNum, owner);
	}

	// Is it supposed to be closed?
	if (!stimSource->spawnArgs.GetBool(AIUSE_SHOULDBECLOSED_KEY))
	{
		// door is not supposed to be closed, ignore
		stimSource->IgnoreResponse(ST_VISUAL, owner); // grayman #2866
		return;
	}

	// grayman #2866 - Delay dealing with this door until my alert level comes down.

	if ( owner->AI_AlertIndex >= ESuspicious )
	{
		return;
	}

	// grayman #2859 - Check who last used the door.

	// grayman #2959 - the door is suspicious if:
	// - lastUsedBy is NULL (no one's ever used it, or the player used it last)
	// - lastUsedBy is friendly but not in sight
	
	idEntity* lastUsedBy = door->GetLastUsedBy();
	if ( lastUsedBy != NULL )
	{
		// grayman #1327 - Was the door last used by someone we can see,
		// or someone we can't see but who is near the door?
		// If so, we assume they're the one who opened the door, so it's
		// not suspicious. CanSeeExt( lastUsedBy, false, false )
		// doesn't care about FOV (lastUsedBy can be behind owner) and we
		// don't care how bright it is.

		if ( owner->CanSeeExt( lastUsedBy, false, false ) )
		{
			// I can still see who last used this door, so
			// he's probably handling the door now, since stims arrive when
			// the door is opened. Do nothing.

			stimSource->IgnoreResponse(ST_VISUAL, owner);
			return; // someone I can see opened the door, so all is well
		}

		// can't see him, but is he near the door?

		idVec3 personOrigin = lastUsedBy->GetPhysics()->GetOrigin();
		idVec3 doorOrigin = door->GetPhysics()->GetOrigin();
		if ( (personOrigin - doorOrigin).LengthSqr() <= Square(PERSON_NEAR_DOOR) )
		{
			stimSource->IgnoreResponse(ST_VISUAL, owner);
			return; // someone I can't see opened the door, so all is well
		}
	}

	// grayman #3104 - is anyone near the door who might have opened it?

	if ( SomeoneNearDoor(owner, door) )
	{
		stimSource->IgnoreResponse(ST_VISUAL, owner);
		return; // I see someone near the door who might have opened it, so all is well
	}

	// The open door is now suspicious.

	// grayman #2866 - Is someone else dealing with this suspicious door?

	if ( door->GetSearching() )
	{
		// The door has already alerted someone, who is probably now searching.
		// Don't handle the door yourself; the searching AI will close it when done.
		// Since someone's already searching around the door, don't get involved.

//		stimSource->IgnoreResponse(ST_VISUAL, owner); // grayman #3104 - keep responding to the door in case the other AI leaves it open
		return;
	}

	// grayman #2866 - Delay dealing with this door if I'm already dealing with another suspicious door

	CFrobDoor* closeMe = memory.closeMe.GetEntity();
	if ( closeMe != NULL )
	{
		return;
	}

	// grayman #2866 - Delay dealing with this door if I'm too far into handling a non-suspicious door.
	
	memory.susDoorSameAsCurrentDoor = false;
	const SubsystemPtr& subsys = owner->movementSubsystem;
	TaskPtr task = subsys->GetCurrentTask();
	if ( boost::dynamic_pointer_cast<HandleDoorTask>(task) != NULL )
	{
		CFrobDoor* currentDoor = memory.doorRelated.currentDoor.GetEntity();
		if ( !task->CanAbort() )
		{
			return;
		}

		// Abort the current door so I can handle the suspicious door.

		if ( currentDoor != NULL ) // shouldn't be NULL
		{
			memory.susDoorSameAsCurrentDoor = ( currentDoor == door );
			subsys->FinishTask();
		}
	}

	// grayman #2866 - NOW I can ignore future stims and process this one.
	
	stimSource->IgnoreResponse(ST_VISUAL, owner);

	// Vocalize that I see something out of place

	memory.lastTimeVisualStimBark = gameLocal.time;
	owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new SingleBarkTask("snd_foundOpenDoor")));
	//gameLocal.Printf("%s: That door (%s) isn't supposed to be open!\n",owner->name.c_str(),door->name.c_str());
	
	// This is a door that's supposed to be closed.
	// Search for a while. Remember the door so you can close it later. 

	memory.closeMe = door;
	memory.closeSuspiciousDoor = false; // becomes TRUE when it's time to close the door

	// grayman #2866 - Check if the door swings toward or away from us. We'll use this
	// to determine whether we have to walk through the door before finally
	// closing it.

	memory.doorSwingsToward = (door->GetOpenDir() * (owner->GetPhysics()->GetOrigin() - door->GetPhysics()->GetOrigin()) > 0);

	// grayman #2866 - Handle sliding doors.

	idList< idEntityPtr<idEntity> > list;
	if ( door->GetDoorHandlingEntities( owner, list ) ) // for doors that use door handling positions
	{
		memory.frontPos = list[0];
		memory.backPos = list[1];
	}

	door->SetSearching(owner); // keeps other AI from joining in the search

	// Raise alert level

	// One more piece of evidence of something out of place
	memory.countEvidenceOfIntruders += EVIDENCE_COUNT_INCREASE_DOOR;
	memory.posEvidenceIntruders = owner->GetPhysics()->GetOrigin(); // grayman #2903
	memory.timeEvidenceIntruders = gameLocal.time; // grayman #2903

	if ( owner->AI_AlertLevel < ( owner->thresh_4 - 0.1f ) )
	{
		memory.alertPos = door->GetClosedBox().GetCenter(); // grayman #2866 - search at center of door; needed to correctly search sliding doors
//		memory.alertPos = stimSource->GetPhysics()->GetOrigin(); // grayman #2866 - old way, which doesn't work well with sliding doors
		memory.alertClass = EAlertVisual_2; // grayman #2603
		memory.alertType = EAlertTypeDoor;
		
		// Do search as if there is an enemy that has escaped
		memory.alertRadius = LOST_ENEMY_ALERT_RADIUS;
		memory.alertSearchVolume = LOST_ENEMY_SEARCH_VOLUME; 
		memory.alertSearchExclusionVolume.Zero();
		
		owner->AI_VISALERT = false;
		memory.visualAlert = false; // grayman #2422
		memory.mandatory = false;	// grayman #3331

		owner->SetAlertLevel(owner->thresh_4 - 0.1f);
	}

	// Do new reaction to stimulus

	memory.stimulusLocationItselfShouldBeSearched = true;
	memory.alertedDueToCommunication = false;
}

void State::OnAICommMessage(CommMessage& message, float psychLoud)
{
	idAI* owner = _owner.GetEntity();
	// greebo: changed the IF back to an assertion, the owner should never be NULL
	assert(owner != NULL);

	if (owner->AI_DEAD || owner->AI_KNOCKEDOUT)
	{
		return;
	}

	// Get the message parameters
	CommMessage::TCommType commType = message.m_commType;
	
	idEntity* issuingEntity = message.m_p_issuingEntity.GetEntity();
	idEntity* recipientEntity = message.m_p_recipientEntity.GetEntity();

	// grayman #2924 - was this message was meant for me? If recipientEntity
	// is me or is NULL, I'll listen to it. Otherwise, I'll ignore it.
	if ( recipientEntity && ( recipientEntity != owner ) )
	{
		return;
	}

	idEntity* directObjectEntity = message.m_p_directObjectEntity.GetEntity();
	const idVec3& directObjectLocation = message.m_directObjectLocation;

	if (issuingEntity != NULL)
	{
		DM_LOG(LC_AI, LT_INFO)LOGSTRING("%s Got incoming message from %s\r", owner->name.c_str(),issuingEntity->name.c_str());
	}

	Memory& memory = owner->GetMemory();

	// greebo: Update the last AI seen timer on incoming messages from friendly AI
	if ((owner->GetPhysics()->GetOrigin() - directObjectLocation).LengthSqr() < Square(300) && 
		owner->IsFriend(issuingEntity))
	{
		memory.lastTimeFriendlyAISeen = gameLocal.time;
	}

	switch (commType)
	{
		case CommMessage::Greeting_CommType:
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Message Type: Greeting_CommType\r");
			// Have seen a friend
			memory.lastTimeFriendlyAISeen = gameLocal.time;

			if ( issuingEntity->IsType(idAI::Type) )
			{
				idAI* otherAI = static_cast<idAI*>(issuingEntity);

				// grayman #3202 - if mute, you can't reply
				if ( owner->CanGreet() && !owner->m_isMute )
				{
					// Get the sound and queue the task
					idStr greetSound = GetGreetingResponseSound(owner, otherAI);
					owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new GreetingBarkTask(greetSound, otherAI, false))); // grayman #3415
				}
				else // grayman #3202 - reset greetingState so we can receive greetings in the future
				{
					if ( owner->greetingState != ECannotGreet )
					{
						owner->greetingState = ENotGreetingAnybody;
					}
				}
			}

			break;
		case CommMessage::FriendlyJoke_CommType:
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Message Type: FriendlyJoke_CommType\r");
			// Have seen a friend
			memory.lastTimeFriendlyAISeen = gameLocal.time;

			if (directObjectEntity == owner)
			{
				gameLocal.Printf("Hah, yer no better!\n");
			}
			else
			{
				gameLocal.Printf("Ha, yer right, they be an ass\n");
			}
			break;
		case CommMessage::Insult_CommType:
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Message Type: Insult_CommType\r");
			if (directObjectEntity == owner)
			{
				gameLocal.Printf("Same to you, buddy\n");
			}
			else if (owner->IsEnemy(directObjectEntity))
			{
				gameLocal.Printf("Hah!\n");
			}
			else
			{
				gameLocal.Printf("I'm not gettin' involved\n");
			}
			break;
		case CommMessage::RequestForHelp_CommType:
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Message Type: RequestForHelp_CommType\r");
			if (owner->IsFriend(issuingEntity))
			{
				// Do we already have a target we are dealing with?
				if (owner->GetEnemy() != NULL)
				{
					//gameLocal.Printf("I'm too busy, I have a target!\n");
					break;
				}

				// grayman #3317 - Can't help if I'm fleeing
				idStr myState = owner->GetMind()->GetState()->GetName();
				if ( ( myState == "Flee" ) || ( myState == "FleeDone" ) )
				{
					//gameLocal.Printf("I'm fleeing, so I can't help!\n");
					break;
				}

				if (directObjectEntity && directObjectEntity->IsType(idActor::Type))
				{
					// Bark
					owner->GetSubsystem(SubsysCommunication)->PushTask(
						SingleBarkTaskPtr(new SingleBarkTask("snd_assistFriend")));

					//gameLocal.Printf("Ok, I'm helping you.\n");

					owner->SetEnemy(static_cast<idActor*>(directObjectEntity));
					owner->GetMind()->PerformCombatCheck();
				}

				if (owner->GetEnemy() == NULL)
				{
					// no enemy set or enemy not found yet
					// set up search
					// grayman #3009 - pass the alert position so the AI can look at it
					owner->PreAlertAI("aud", psychLoud, directObjectLocation); // grayman #3356

					memory.alertPos = directObjectLocation;
					memory.alertRadius = LOST_ENEMY_ALERT_RADIUS;
					memory.alertSearchVolume = LOST_ENEMY_SEARCH_VOLUME;
					memory.alertSearchExclusionVolume.Zero();
					memory.visualAlert = false; // grayman #2422
					memory.mandatory = false;	// grayman #3331

					memory.alertedDueToCommunication = true;
					memory.stimulusLocationItselfShouldBeSearched = true;
				}
			}
			else if (owner->AI_AlertLevel < owner->thresh_1 + (owner->thresh_2 - owner->thresh_1) * 0.5f)
			{
				owner->SetAlertLevel(owner->thresh_1 + (owner->thresh_2 - owner->thresh_1) * 0.5f);
			}
			break;
		case CommMessage::RequestForMissileHelp_CommType:
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Message Type: RequestForMissileHelp_CommType\r");
			// Respond if they are a friend and we have a ranged weapon
			if (owner->IsFriend(issuingEntity) && owner->GetNumRangedWeapons() > 0)
			{
				// Do we already have a target we are dealing with?
				if (owner->GetEnemy() != NULL)
				{
					//gameLocal.Printf("I'm too busy, I have a target!\n");
					break;
				}

				if (directObjectEntity->IsType(idActor::Type))
				{
					//gameLocal.Printf("I'll attack it with my ranged weapon!\n");

					// Bark
					owner->GetSubsystem(SubsysCommunication)->PushTask(
						SingleBarkTaskPtr(new SingleBarkTask("snd_assistFriend")));
					
					owner->SetEnemy(static_cast<idActor*>(directObjectEntity));
					owner->GetMind()->PerformCombatCheck();
				}
			}
			else 
			{
				//gameLocal.Printf("I don't have a ranged weapon or I am not getting involved.\n");
				if (owner->AI_AlertLevel < owner->thresh_2*0.5f)
				{
					owner->SetAlertLevel(owner->thresh_2*0.5f);
				}
			}
			break;
		case CommMessage::RequestForMeleeHelp_CommType:
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Message Type: RequestForMeleeHelp_CommType\r");
			// Respond if they are a friend and we have a melee weapon
			if (owner->IsFriend(issuingEntity) && owner->GetNumMeleeWeapons() > 0)
			{
				// Do we already have a target we are dealing with?
				if (owner->GetEnemy() != NULL)
				{
					//gameLocal.Printf("I'm too busy, I have a target!\n");
					break;
				}

				if (directObjectEntity->IsType(idActor::Type))
				{
					//gameLocal.Printf("I'll attack it with my melee weapon!\n");

					// Bark
					owner->GetSubsystem(SubsysCommunication)->PushTask(
						SingleBarkTaskPtr(new SingleBarkTask("snd_assistFriend")));
					
					owner->SetEnemy(static_cast<idActor*>(directObjectEntity));
					owner->GetMind()->PerformCombatCheck();
				}
			}
			else 
			{
				//gameLocal.Printf("I don't have a melee weapon or I am not getting involved.\n");
				if (owner->AI_AlertLevel < owner->thresh_2*0.5f)
				{
					owner->SetAlertLevel(owner->thresh_2*0.5f);
				}
			}
			break;
		case CommMessage::RequestForLight_CommType:
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Message Type: RequestForLight_CommType\r");
			//gameLocal.Printf("I don't know how to bring light!\n");
			break;
		case CommMessage::DetectedSomethingSuspicious_CommType:
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Message Type: DetectedSomethingSuspicious_CommType\r");
			OnMessageDetectedSomethingSuspicious(message);
			break;
		case CommMessage::DetectedEnemy_CommType:
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Message Type: DetectedEnemy_CommType\r");
			//gameLocal.Printf("Somebody spotted an enemy... (%s)\n", directObjectEntity->name.c_str());
	
			if (owner->GetEnemy() != NULL)
			{
				//gameLocal.Printf("I'm too busy with my own target!\n");
				return;
			}

			{
				float newAlertLevel = (owner->thresh_4 + owner->thresh_5) * 0.5f;

				// greebo: Only set the alert level if it is greater than our own
				if (owner->AI_AlertLevel < newAlertLevel && 
					owner->IsFriend(issuingEntity) && 
					owner->IsEnemy(directObjectEntity))
				{
					// Set the alert level between 4 and 5.
					owner->SetAlertLevel((owner->thresh_4 + owner->thresh_5)*0.5f);
					
					// We got alerted by a communication message
					memory.alertedDueToCommunication = true;
						
					//gameLocal.Printf("They're my friend, I'll attack it too!\n");
					memory.alertPos = directObjectLocation;
				}
			}
			break;
		case CommMessage::FollowOrder_CommType:
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Message Type: FollowOrder_CommType\r");
			if (recipientEntity == owner && owner->IsFriend(issuingEntity))
			{
				gameLocal.Printf("But I don't know how to follow somebody!\n");
			}
			break;
		case CommMessage::GuardLocationOrder_CommType:
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Message Type: GuardLocationOrder_CommType\r");
			if (recipientEntity == owner && owner->IsFriend(issuingEntity))
			{
				gameLocal.Printf("But I don't know how to guard a location!\n");
			}
			break;
		case CommMessage::GuardEntityOrder_CommType:
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Message Type: GuardEntityOrder_CommType\r");
			if (recipientEntity == owner && owner->IsFriend(issuingEntity))
			{
				gameLocal.Printf("But I don't know how to guard an entity!\n");
			}
			break;
		case CommMessage::PatrolOrder_CommType:
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Message Type: PatrolOrder_CommType\r");
			if (recipientEntity == owner && owner->IsFriend(issuingEntity))
			{
				gameLocal.Printf("But I don't know how to switch my patrol route!\n");
			}
			break;
		case CommMessage::SearchOrder_CommType:
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Message Type: SearchOrder_CommType\r");
			if (recipientEntity == owner && owner->IsFriend(issuingEntity))
			{
				// Set alert pos to the position we were ordered to search
				memory.alertPos = directObjectLocation;
				memory.chosenHidingSpot = directObjectLocation;
				owner->SetAlertLevel((owner->thresh_3 + owner->thresh_4)*0.5f);
				memory.visualAlert = false; // grayman #2422
				memory.mandatory = false;	// grayman #3331
			}
			break;
		case CommMessage::AttackOrder_CommType:
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Message Type: AttackOrder_CommType\r");
			// Set this as our enemy and enter combat
			if (recipientEntity == owner && owner->IsFriend(issuingEntity))
			{
				//gameLocal.Printf("Yes sir! Attacking your specified target!\n");

				if (directObjectEntity->IsType(idActor::Type))
				{
					owner->SetEnemy(static_cast<idActor*>(directObjectEntity));
					owner->SetAlertLevel(owner->thresh_5*2);
				}
			}
			else if (owner->AI_AlertLevel < owner->thresh_2*0.5f)
			{
				owner->SetAlertLevel(owner->thresh_2*0.5f);
			}
			break;
		case CommMessage::GetOutOfTheWayOrder_CommType:
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Message Type: GetOutOfTheWayOrder_CommType\r");
			break;
		case CommMessage::ConveyWarning_EvidenceOfIntruders_CommType:
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Message Type: ConveyWarning_EvidenceOfIntruders_CommType\r");
			if (issuingEntity->IsType(idAI::Type))
			{
				idAI* issuer = static_cast<idAI*>(issuingEntity);
				// Note: We deliberately don't care if the issuer is a friend or not
				Memory& issuerMemory = issuer->GetMind()->GetMemory();
				int warningAmount = issuerMemory.countEvidenceOfIntruders;
				
				if ( memory.countEvidenceOfIntruders < warningAmount )
				{
					// grayman #3424 - the evidence count can get way out of line
					// by simply inheriting what the other AI has, and it opens
					// itself up to double-dipping for the same event. Let's just stick
					// to a general "suspicious" increase.

					memory.countEvidenceOfIntruders += EVIDENCE_COUNT_INCREASE_SUSPICIOUS;
					// memory.countEvidenceOfIntruders = warningAmount; // old way
				
					// grayman #2903 - register where the issuing entity saw the evidence

					memory.posEvidenceIntruders = issuerMemory.posEvidenceIntruders;
					memory.timeEvidenceIntruders = issuerMemory.timeEvidenceIntruders;
					memory.alertedDueToCommunication = true; // grayman #2920

					if (owner->AI_AlertLevel < owner->thresh_2*0.5f)
					{
						owner->SetAlertLevel(owner->thresh_2*0.5f);
					}
				}

				// grayman #2920 - issue a delayed warning response
				owner->Bark("snd_warn_response");
//				owner->PostEventMS(&AI_Bark,WARNING_RESPONSE_DELAY,"snd_warn_response");
			}
			break;
		case CommMessage::ConveyWarning_ItemsHaveBeenStolen_CommType:
			{
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Message Type: ConveyWarning_ItemsHaveBeenStolen_CommType\r");
			// Note: We deliberately don't care if the issuer is a friend or not
			if ( !owner->FindSuspiciousEvent(message.m_eventID) )
			{
				memory.itemsHaveBeenStolen = true;

				if (cv_ai_debug_transition_barks.GetBool())
				{
					gameLocal.Printf("%s is warned by %s about a missing item, will use Alert Idle\n",owner->GetName(),issuingEntity->GetName());
				}

				owner->AddSuspiciousEvent(message.m_eventID); // grayman #3424 - I now know about this suspicious event
				memory.countEvidenceOfIntruders += EVIDENCE_COUNT_INCREASE_MISSING_ITEM; // grayman #2903
				memory.posEvidenceIntruders = gameLocal.m_suspiciousEvents[message.m_eventID].location; // grayman #2903
				memory.timeEvidenceIntruders = gameLocal.time;
			}
			if (issuingEntity->IsType(idActor::Type))
			{
				idActor* issuer = static_cast<idActor*>(issuingEntity);
				owner->AddWarningEvent(issuer,message.m_eventID); // log that a warning passed between us
				issuer->AddWarningEvent(owner,message.m_eventID); // log that a warning passed between us
			}
			memory.alertedDueToCommunication = true; // grayman #2920

			if (owner->AI_AlertLevel < owner->thresh_2*0.5f )
			{
				owner->SetAlertLevel( owner->thresh_2*0.5f );
			}

			// grayman #2920 - issue a delayed warning response
			owner->Bark("snd_warn_response");
//			owner->PostEventMS(&AI_Bark,WARNING_RESPONSE_DELAY,"snd_warn_response");
			break;
			}
		case CommMessage::ConveyWarning_CorpseHasBeenSeen_CommType: // grayman #1327
			{
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Message Type: ConveyWarning_CorpseHasBeenSeen_CommType\r");
			// Note: We deliberately don't care if the issuer is a friend or not
			// Did I already know about this event?
			if ( !owner->FindSuspiciousEvent(message.m_eventID) )
			{
				memory.deadPeopleHaveBeenFound = true;

				if (cv_ai_debug_transition_barks.GetBool())
				{
					gameLocal.Printf("%s is warned by %s about a dead person, will use Alert Idle\n",owner->GetName(),issuingEntity->GetName());
				}

				owner->AddSuspiciousEvent(message.m_eventID); // grayman #3424 - I now know about this suspicious event
				memory.countEvidenceOfIntruders += EVIDENCE_COUNT_INCREASE_CORPSE; // grayman #2903
				memory.posEvidenceIntruders = gameLocal.m_suspiciousEvents[message.m_eventID].location;
				memory.timeEvidenceIntruders = gameLocal.time;
			}
			if (issuingEntity->IsType(idActor::Type))
			{
				idActor* issuer = static_cast<idActor*>(issuingEntity);
				owner->AddWarningEvent(issuer,message.m_eventID); // log that a warning passed between us
				issuer->AddWarningEvent(owner,message.m_eventID); // log that a warning passed between us
			}
			memory.alertedDueToCommunication = true; // grayman #2920

			if (owner->AI_AlertLevel < owner->thresh_2*0.5f)
			{
				owner->SetAlertLevel(owner->thresh_2*0.5f);
			}

			// grayman #2920 - issue a delayed warning response
			owner->Bark("snd_warn_response");

			//owner->PostEventMS(&AI_Bark,WARNING_RESPONSE_DELAY,"snd_warn_response");
			break;
			}
		case CommMessage::ConveyWarning_EnemiesHaveBeenSeen_CommType:
			{
			// Note: We deliberately don't care if the issuer is a friend or not
			if ( !owner->FindSuspiciousEvent(message.m_eventID) )
			{
				memory.enemiesHaveBeenSeen = true;

				if (cv_ai_debug_transition_barks.GetBool())
				{
					gameLocal.Printf("%s is warned by %s about an enemy, will use Alert Idle\n",owner->GetName(),issuingEntity->GetName());
				}

				memory.countEvidenceOfIntruders += EVIDENCE_COUNT_INCREASE_ENEMY;
				memory.posEvidenceIntruders = gameLocal.m_suspiciousEvents[message.m_eventID].location;
				memory.timeEvidenceIntruders = gameLocal.time;
				owner->AddSuspiciousEvent(message.m_eventID); // grayman #3424 - I now know about this suspicious event
			}
			if (issuingEntity->IsType(idActor::Type))
			{
				idActor* issuer = static_cast<idActor*>(issuingEntity);
				owner->AddWarningEvent(issuer,message.m_eventID); // log that a warning passed between us
				issuer->AddWarningEvent(owner,message.m_eventID); // log that a warning passed between us
			}
			memory.alertedDueToCommunication = true; // grayman #2920

			if (owner->AI_AlertLevel < owner->thresh_2*0.5f)
			{
				owner->SetAlertLevel(owner->thresh_2*0.5f);
			}

			// grayman #2920 - issue a delayed warning response
			owner->Bark("snd_warn_response");
			//owner->PostEventMS(&AI_Bark,WARNING_RESPONSE_DELAY,"snd_warn_response");
			break;
			}
	} // switch
}

void State::OnMessageDetectedSomethingSuspicious(CommMessage& message)
{
	idEntity* issuingEntity = message.m_p_issuingEntity.GetEntity();
	//idEntity* recipientEntity = message.m_p_recipientEntity.GetEntity();
	//idEntity* directObjectEntity = message.m_p_directObjectEntity.GetEntity();
	idVec3 directObjectLocation = message.m_directObjectLocation;

	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	if (owner->AI_DEAD || owner->AI_KNOCKEDOUT)
	{
		return;
	}

	Memory& memory = owner->GetMemory();

	//gameLocal.Printf("Somebody else noticed something suspicious...\n");

	if (owner->GetEnemy() != NULL)
	{
		//gameLocal.Printf ("I'm too busy with my own target!");
		return;
	}

	// grayman #3438 - don't respond if I just finished searching
	if ( !owner->IsSearching() && ( owner->m_lastAlertLevel >= owner->thresh_3 ) )
	{
		return;
	}

	if (owner->IsFriend(issuingEntity))
	{
		idAI* issuingAI = static_cast<idAI*>(issuingEntity);
		Memory& issuerMemory = issuingAI->GetMemory();
		EAlertType ieAlertType = issuerMemory.alertType;

		// grayman #3424 - compare the alert type weights,
		// as is done when new alerts arrive
		if ( !ShouldProcessAlert(ieAlertType))
		{
			return;
		}

		// grayman #3438 - If I already searched this event, I won't search it again.
		if ( ( message.m_eventID >= 0 ) && owner->HasSearchedEvent(message.m_eventID) )
		{
			return;
		}

		// If AI that called out has a higher alert level, raise ours
		// to match theirs due to urgency in their voice
		// Inherit the alert level of the other AI, but attenuate it a bit
		float otherAlertLevel = issuingAI->AI_AlertLevel * 0.7f;

		//gameLocal.Printf("The AI who noticed something has an alert num of %f\n", otherAlertLevel);
		// grayman #3424 - Only enter the sender's search if 'otherAlertLevel' is higher than mine,
		// and it's high enough to put me into search mode, which is SearchingState (thresh_3).
		// grayman #3424 - if we're to process this request, we shouldn't compare our alert level to what we're inheriting
		if ( /*( otherAlertLevel > owner->AI_AlertLevel ) && */( otherAlertLevel >= owner->thresh_3 ) )
		{
			// Get some search points from them.
			int numSpots = owner->GetSomeOfOtherEntitiesHidingSpotList(issuingEntity);

			if (numSpots > 0)
			{
				// What is the distance to the friend?  If it is greater than a certain amount, shout intention
				// to come help
				float distanceToIssuer = (issuingEntity->GetPhysics()->GetOrigin() - owner->GetPhysics()->GetOrigin()).LengthFast();
				if (distanceToIssuer > MIN_DISTANCE_TO_ISSUER_TO_SHOUT_COMING_TO_ASSISTANCE)
				{
					// Bark
					// angua: this one was getting quite annoying if you hear it all the time
					// grayman #3424 - reinstating it, since there are a few new checks to keep
					// AI from joining so frequently; - return comments if still annoying
					owner->GetSubsystem(SubsysCommunication)->PushTask(
						SingleBarkTaskPtr(new SingleBarkTask("snd_assistFriend")));
				}
 			
				// grayman #3438 - only reset my alert level if I'm currently not searching
				if ( owner->AI_AlertLevel < owner->thresh_3 )
				{
					owner->SetAlertLevel(otherAlertLevel);
				}

				owner->StopMove(MOVE_STATUS_DONE);
				memory.stopRelight = true; // grayman #2603 - abort a relight in progress
				memory.stopExaminingRope = true; // grayman #2872 - stop examining rope
				memory.stopReactingToHit = true; // grayman #2816

				memory.alertPos = issuerMemory.alertPos;
				memory.alertClass = EAlertNone;
				memory.alertType = EAlertTypeSuspicious;
				
				memory.alertRadius = issuerMemory.alertRadius;
				memory.alertSearchVolume = issuerMemory.alertSearchVolume; 
				memory.alertSearchExclusionVolume.Zero();

				memory.alertedDueToCommunication = true;
				memory.visualAlert = false; // grayman #2422
				memory.mandatory = false;	// grayman #3331

				// grayman #3438
				if ( message.m_eventID >= 0 )
				{
					if ( !owner->FindSuspiciousEvent(message.m_eventID) )
					{
						owner->AddSuspiciousEvent(message.m_eventID);
					}
					owner->AddWarningEvent(issuingAI,message.m_eventID); // log that a warning passed between us
					issuingAI->AddWarningEvent(owner,message.m_eventID); // log that a warning passed between us
					memory.currentSearchEventID = message.m_eventID; // grayman #3424
				}
			}
			else
			{
				//gameLocal.Printf("Hmpfh, no spots to help them with\n");
			}
			
			return;
		}
	}
	else if (owner->AI_AlertLevel < owner->thresh_2*0.5f)
	{
		owner->SetAlertLevel(owner->thresh_2*0.5f);
	}
}

void State::OnFrobDoorEncounter(CFrobDoor* frobDoor)
{
	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	// grayman #2706 - can't handle doors if you're resolving a block

	if (owner->movementSubsystem->IsResolvingBlock() || owner->movementSubsystem->IsWaiting())
	{
		return;
	}

	// grayman #3029 - can handle doors if you're approaching an elevator

	if ( owner->m_HandlingElevator && !owner->m_CanSetupDoor )
	{
		return;
	}

	// grayman #2650 - can we handle doors?

	if (!owner->m_bCanOperateDoors)
	{
		return;
	}

	// grayman #2716 - if the door is too high above or too far below, ignore it

	idBounds frobDoorBounds = frobDoor->GetPhysics()->GetAbsBounds();
	float ownerZ = owner->GetPhysics()->GetOrigin().z;
	if ((frobDoorBounds[0].z > (ownerZ + 70)) || (frobDoorBounds[1].z < (ownerZ - 30)))
	{
		return;
	}

	// grayman #2695 - don't set up to handle a door you can't see.
	// grayman #3104 - unless you're closing a suspicious door

	Memory& memory = owner->GetMemory();

	if ( !memory.closeSuspiciousDoor )
	{
		idVec3 doorCenter = frobDoor->GetClosedBox().GetCenter(); // use center of closed door, regardless of whether it's open or closed
		if ( !owner->CanSeeTargetPoint( doorCenter, frobDoor, false ) ) // 'false' = don't consider illumination
		{
			return; // we have no LOS yet
		}
	}

	if (cv_ai_door_show.GetBool()) 
	{
		gameRenderWorld->DebugArrow(colorRed, owner->GetEyePosition(), frobDoor->GetPhysics()->GetOrigin(), 1, 16);
	}

	// check if we already have a door to handle
	// don't start a DoorHandleTask if it is the same door or the other part of a double door
	CFrobDoor* currentDoor = memory.doorRelated.currentDoor.GetEntity();
	if (currentDoor == NULL)
	{
		// grayman #2691 - if we don't fit through this new door, don't use it

		if (!owner->CanPassThroughDoor(frobDoor))
		{
			return;
		}

		// grayman #2345 - don't handle this door if we just finished handling it, unless alerted.

		int lastTimeUsed = owner->GetMemory().GetDoorInfo(frobDoor).lastTimeUsed;
		if ((lastTimeUsed > -1) && (gameLocal.time < lastTimeUsed + 3000)) // grayman #2712 - delay time should match REUSE_DOOR_DELAY
		{
			return; // ignore this door
		}

		// grayman #3029 - all clear to handle the door, but if you're handling
		// an elevator, you have to terminate that first.

		if (owner->m_HandlingElevator)
		{
			const SubsystemPtr& subsys = owner->movementSubsystem;
			TaskPtr task = subsys->GetCurrentTask();

			if (boost::dynamic_pointer_cast<HandleElevatorTask>(task) != NULL)
			{
				subsys->FinishTask();
			}
		}

		memory.doorRelated.currentDoor = frobDoor;
		owner->movementSubsystem->PushTask(HandleDoorTask::CreateInstance());
	}
	else // currentDoor exists
	{
		if ( ( frobDoor != currentDoor ) && ( frobDoor != currentDoor->GetDoubleDoor() ) )
		{
			// this is a new door

			// if there is already a door handling task active, 
			// terminate that one so we can start a new one next time

			const SubsystemPtr& subsys = owner->movementSubsystem;
			TaskPtr task = subsys->GetCurrentTask();

			if (boost::dynamic_pointer_cast<HandleDoorTask>(task) != NULL)
			{
				// grayman #2706 - only quit this door if you're in the approaching states.
				// otherwise, finish with this door before you move to another one.
				if (task->CanAbort())
				{
					subsys->FinishTask();
				}
			}
			else
			{
				// angua: current door is set but no door handling task active
				// door handling task was probably terminated before initialisation
				// clear current door
				memory.doorRelated.currentDoor = NULL;
			}
		}
		else // this is our current door
		{
			// grayman #3104 - there's a problem when the current door task
			// has just completed its Perform() step and is about to start
			// its OnFinish() step. At this point, HandleDoorTask is NOT the
			// current task. Testing showed it was PathCornerTask, so that snuck
			// in somehow before the OnFinish() ran for HandleDoorTask. NULLing
			// currentDoor below doesn't let OnFinish() do everything it needs to.

/*			const SubsystemPtr& subsys = owner->movementSubsystem;
			TaskPtr task = subsys->GetCurrentTask();

			if (boost::dynamic_pointer_cast<HandleDoorTask>(task) == NULL)
			{
				// angua: current door is set but no door handling task active
				// door handling task was probably terminated before initialisation
				// clear current door
				memory.doorRelated.currentDoor = NULL;
			}
*/
		}
	}
}

void State::NeedToUseElevator(const eas::RouteInfoPtr& routeInfo)
{
	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	// grayman #3050 - can't handle a new elevator if you're resolving a block

	if (owner->movementSubsystem->IsResolvingBlock() || owner->movementSubsystem->IsWaiting())
	{
		return;
	}

	// grayman #3050 - can't handle a new elevator if you're currently using a door or an elevator or you can't use elevators

	if ( owner->m_HandlingDoor || owner->m_HandlingElevator || !owner->CanUseElevators())
	{
		return;
	}

	// Prevent more ElevatorTasks from being pushed
//	owner->m_HandlingElevator = true; // grayman #3029 - this is too early; moved to Init of task
	owner->m_CanSetupDoor = true; // grayman #3029
	owner->movementSubsystem->PushTask(TaskPtr(new HandleElevatorTask(routeInfo)));
}

} // namespace ai
