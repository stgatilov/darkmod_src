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

#include "CombatState.h"
#include "../Memory.h"
#include "../../AIComm_Message.h"
#include "../Tasks/RandomHeadturnTask.h"
#include "../Tasks/ChaseEnemyTask.h"
#include "../Tasks/SingleBarkTask.h"
#include "../Tasks/MeleeCombatTask.h"
#include "../Tasks/RangedCombatTask.h"
#include "../Tasks/ChaseEnemyRangedTask.h"
#include "LostTrackOfEnemyState.h"
#include "AgitatedSearchingState.h"
#include "FleeState.h"
#include "../Library.h"

#define REACTION_TIME_MIN      100	// grayman #3063
#define REACTION_TIME_MAX     2000	// grayman #3063
#define ENEMY_DEAD_BARK_DELAY 1500	// grayman #2816

namespace ai
{

const float s_DOOM_TO_METERS = 0.0254f; // grayman #3063

// Get the name of this state
const idStr& CombatState::GetName() const
{
	static idStr _name(STATE_COMBAT);
	return _name;
}

bool CombatState::CheckAlertLevel(idAI* owner)
{
	if (!owner->m_canSearch) // grayman #3069 - AI that can't search shouldn't be here
	{
		owner->SetAlertLevel(owner->thresh_3 - 0.1);
	}

	if (owner->AI_AlertIndex < ECombat)
	{
		// Alert index is too low for this state, fall back
		owner->GetMind()->EndState();
		return false;
	}

	// Alert Index is matching, return OK
	return true;
}

void CombatState::OnTactileAlert(idEntity* tactEnt)
{
	// do nothing as of now, we are already in combat mode
}

void CombatState::OnVisualAlert(idActor* enemy)
{
	// do nothing as of now, we are already in combat mode
}

void CombatState::OnAudioAlert()
{
	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	Memory& memory = owner->GetMemory();

	memory.alertClass = EAlertAudio;
	memory.alertPos = owner->GetSndDir();

	if (!owner->AI_ENEMY_VISIBLE)
	{
		if (owner->m_ignorePlayer) // grayman #3063
		{
			memory.lastTimeEnemySeen = gameLocal.time;
		}
		owner->lastReachableEnemyPos = memory.alertPos;
		// gameRenderWorld->DebugArrow(colorRed, owner->GetEyePosition(), memory.alertPos, 2, 1000);
	}
}

void CombatState::OnFailedKnockoutBlow(idEntity* attacker, const idVec3& direction, bool hitHead)
{
	// Ignore failed knockout attempts in combat mode
}

void CombatState::OnActorEncounter(idEntity* stimSource, idAI* owner)
{
	if (!stimSource->IsType(idActor::Type))
	{
		return; // No Actor, quit
	}

	if (owner->IsFriend(stimSource))
	{
		// Remember last time a friendly AI was seen
		owner->GetMemory().lastTimeFriendlyAISeen = gameLocal.time;
	}
	// angua: ignore other people during combat
}

void CombatState::Post_OnDeadActorEncounter(idActor* person, idAI* owner) // grayman #3317
{
	// don't react to a dead person
}

void CombatState::Post_OnUnconsciousActorEncounter(idActor* person, idAI* owner) // grayman #3317
{
	// don't react to an unconscious person
}

void CombatState::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	// Set end time to something invalid
	_endTime = -1;

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("CombatState initialised.\r");
	assert(owner);

	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner))
	{
		return;
	}

	if (!owner->GetMind()->PerformCombatCheck())
	{
		return;
	}

	if ( ( owner->GetMoveType() == MOVETYPE_SIT ) || ( owner->GetMoveType() == MOVETYPE_SLEEP) )
	{
		owner->GetUp();
	}

	// grayman #3075 - if we're kneeling, doing close inspection of
	// a spot, stop the animation. Otherwise, the kneeling animation gets
	// restarted a few moments later.

	idStr torsoString = "Torso_KneelDown";
	idStr legsString = "Legs_KneelDown";
	bool torsoKneelingAnim = (torsoString.Cmp(owner->GetAnimState(ANIMCHANNEL_TORSO)) == 0);
	bool legsKneelingAnim = (legsString.Cmp(owner->GetAnimState(ANIMCHANNEL_LEGS)) == 0);

	if ( torsoKneelingAnim || legsKneelingAnim )
	{
		// Reset anims
		owner->StopAnim(ANIMCHANNEL_TORSO, 0);
		owner->StopAnim(ANIMCHANNEL_LEGS, 0);
	}

	// say something along the lines of "huh?"

	// The communication system plays reaction bark
	CommMessagePtr message;
	owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new SingleBarkTask("snd_alert1s", message)));

	// All remaining init code is moved into Think() and done in the EStateInit substate,
	// because the things it does need to occur after the initial reaction delay.

	// grayman #3063
	// Add a delay before you process the remainder of Init().
	// The length of the delay depends on the distance to the enemy.

	// We have an enemy, store the enemy entity locally
	_enemy = owner->GetEnemy();
	idActor* enemy = _enemy.GetEntity();

	// grayman #3331 - clear combat state
	_combatType = COMBAT_NONE;
	
	// get melee possibilities

	_meleePossible  = ( owner->GetNumMeleeWeapons()  > 0 );
	_rangedPossible = ( owner->GetNumRangedWeapons() > 0 );

	// grayman #3331 - save combat possibilities
	_unarmedMelee = owner->spawnArgs.GetBool("unarmed_melee","0");
	_unarmedRanged = owner->spawnArgs.GetBool("unarmed_ranged","0");
	_armedMelee = _meleePossible && !_unarmedMelee;
	_armedRanged = _rangedPossible && !_unarmedRanged;

	// grayman #3331 - do we need an initial delay at weapon drawing?
	_needInitialDrawDelay = !( owner->GetAttackFlag(COMBAT_MELEE) || owner->GetAttackFlag(COMBAT_RANGED) ); // not if we have a weapon raised

	idVec3 vec2Enemy = enemy->GetPhysics()->GetOrigin() - owner->GetPhysics()->GetOrigin();
	vec2Enemy.z = 0; // ignore vertical component
	float dist2Enemy = vec2Enemy.LengthFast();
	int reactionTime =  REACTION_TIME_MIN + (dist2Enemy*(REACTION_TIME_MAX - REACTION_TIME_MIN))/(cv_ai_sight_combat_cutoff.GetFloat()/s_DOOM_TO_METERS);
	if ( reactionTime > REACTION_TIME_MAX )
	{
		reactionTime = REACTION_TIME_MAX;
	}

	// grayman #3331 - add a bit of variability so multiple AI spotting the enemy in the same frame aren't in sync

	reactionTime += gameLocal.random.RandomInt(REACTION_TIME_MAX/2);

	_combatSubState = EStateReaction;
	_reactionEndTime = gameLocal.time + reactionTime;
}

// Gets called each time the mind is thinking
void CombatState::Think(idAI* owner)
{
	// Do we have an expiry date?
	if (_endTime > 0)
	{
		if (gameLocal.time >= _endTime)
		{
			owner->GetMind()->EndState();
		}

		return;
	}

	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner))
	{
		owner->GetMind()->EndState();
		return;
	}
	
	// grayman #3331 - make sure you're still fighting the same enemy. If not, switch sometimes.

	idActor* enemy = _enemy.GetEntity();
	idActor* newEnemy = owner->GetEnemy();

	if ( enemy )
	{
		if ( newEnemy && ( newEnemy != enemy ) )
		{
			if ( gameLocal.random.RandomFloat() < 0.25 ) // small chance of switching
			{
				owner->GetMind()->EndState();
				return; // state has ended
			}
		}
	}
	else
	{
		enemy = newEnemy;
	}

	if (!CheckEnemyStatus(enemy, owner))
	{
		owner->GetMind()->EndState();
		return; // state has ended
	}

	// angua: look at enemy
	owner->Event_LookAtPosition(enemy->GetEyePosition(), gameLocal.msec);

	Memory& memory = owner->GetMemory();

	idVec3 vec2Enemy = enemy->GetPhysics()->GetOrigin() - owner->GetPhysics()->GetOrigin();
	float dist2Enemy = vec2Enemy.LengthFast();

	// grayman #3331 - need to take vertical separation into account. It's possible to have the origins
	// close enough to be w/in the melee zone, but still be unable to hit the enemy.

	bool inMeleeRange = ( dist2Enemy <= ( 3 * owner->GetMeleeRange() ) );

	ECombatType newCombatType;
	
	if ( inMeleeRange && _meleePossible )
	{
		newCombatType = COMBAT_MELEE;
	}
	else if ( !inMeleeRange && _rangedPossible )
	{
		newCombatType = COMBAT_RANGED;
	}
	else if ( !inMeleeRange && !_rangedPossible && _meleePossible )
	{
		newCombatType = COMBAT_MELEE;
	}
	else // will flee when reaction time is over
	{
		newCombatType = COMBAT_NONE;
	}

	// Check for situation where you're in the melee zone, yet you're unable to hit
	// the enemy. This can happen if the enemy is above or below you and you can't
	// reach them.

	switch(_combatSubState)
	{
	case EStateReaction:
		{
		if ( gameLocal.time < _reactionEndTime )
		{
			return; // stay in this state until the reaction time expires
		}

		// Check to see if the enemy is still visible.
		// grayman #2816 - Visibility doesn't matter if you're in combat because
		// you bumped into your enemy.

		idEntity* tactEnt = owner->GetTactEnt();
		if ( ( tactEnt == NULL ) || !tactEnt->IsType(idActor::Type) || ( tactEnt != enemy ) || !owner->AI_TACTALERT ) 
		{
			if ( !owner->CanSee(enemy, true) )
			{
				owner->ClearEnemy();
				owner->SetAlertLevel(owner->thresh_5 - 0.1); // reset alert level just under Combat
				owner->GetMind()->EndState();
				return;
			}
		}

		owner->m_ignorePlayer = false; // grayman #3063 - clear flag that prevents mission statistics on player sightings

		// The AI has processed his reaction, and needs to move into combat, or flee.

		_criticalHealth = owner->spawnArgs.GetInt("health_critical", "0");

		// greebo: Check for weapons and flee if we are unarmed.
		if (!_meleePossible && !_rangedPossible)
		{
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("I'm unarmed, I'm afraid!\r");
			owner->GetMind()->SwitchState(STATE_FLEE);
			return;
		}

		// greebo: Check for civilian AI, which will always flee in face of a combat (this is a temporary query)
		if (owner->spawnArgs.GetBool("is_civilian", "0"))
		{
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("I'm a civilian. I'm afraid.\r");
			owner->GetMind()->SwitchState(STATE_FLEE);
			return;
		}

		memory.stopRelight = true; // grayman #2603 - abort a relight in progress
		memory.stopExaminingRope = true; // grayman #2872 - stop examining a rope
		memory.stopReactingToHit = true; // grayman #2816

		_combatSubState = EStateDoOnce;
		break;
		}

	case EStateDoOnce:
		{
		// Check for sitting or sleeping

		if (   owner->GetMoveType() == MOVETYPE_SIT 
			|| owner->GetMoveType() == MOVETYPE_SLEEP
			|| owner->GetMoveType() == MOVETYPE_SIT_DOWN
			|| owner->GetMoveType() == MOVETYPE_LAY_DOWN )
		{
			owner->GetUp(); // okay if called multiple times
			return;
		}

		// not sitting or sleeping at this point

		// Stop what you're doing
		owner->StopMove(MOVE_STATUS_DONE);
		owner->movementSubsystem->ClearTasks();
		owner->senseSubsystem->ClearTasks();
		owner->actionSubsystem->ClearTasks();

		// Bark

		// This will hold the message to be delivered with the bark, if appropriate
		CommMessagePtr message;
	
		// Only alert the bystanders if we didn't receive the alert by message ourselves
		if (!memory.alertedDueToCommunication)
		{
			message = CommMessagePtr(new CommMessage(
				CommMessage::DetectedEnemy_CommType, 
				owner, NULL, // from this AI to anyone 
				enemy,
				memory.lastEnemyPos
			));
		}

		// The communication system plays starting bark

		// grayman #3343 - accommodate different barks for human and non-human enemies

		idPlayer* player(NULL);
		if (enemy->IsType(idPlayer::Type))
		{
			player = static_cast<idPlayer*>(enemy);
		}

		idStr bark = "";

		if (player && player->m_bShoulderingBody)
		{
			bark = "snd_spotted_player_with_body";
		}
		else if ((MS2SEC(gameLocal.time - memory.lastTimeFriendlyAISeen)) <= MAX_FRIEND_SIGHTING_SECONDS_FOR_ACCOMPANIED_ALERT_BARK)
		{
			idStr enemyAiUse = enemy->spawnArgs.GetString("AIUse");
			if ( ( enemyAiUse == AIUSE_MONSTER ) || ( enemyAiUse == AIUSE_UNDEAD ) )
			{
				bark = "snd_to_combat_company_monster";
			}
			else
			{
				bark = "snd_to_combat_company";
			}
		}
		else
		{
			idStr enemyAiUse = enemy->spawnArgs.GetString("AIUse");
			if ( ( enemyAiUse == AIUSE_MONSTER ) || ( enemyAiUse == AIUSE_UNDEAD ) )
			{
				bark = "snd_to_combat_monster";
			}
			else
			{
				bark = "snd_to_combat";
			}
		}

		owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new SingleBarkTask(bark, message)));

		_justDrewWeapon = false;
		_combatSubState = EStateCheckWeaponState;
		break;
		}

	case EStateCheckWeaponState:
		// check which combat type we should use
		{
		// Can you continue with your current combat type, and not have to switch weapons?

		// Check for case where melee combat has stalled. You're in the melee zone, but you're
		// unable to hit the enemy. Perhaps he's higher or lower than you and you can't reach him.

		if ( !owner->AI_FORWARD && // not moving
			 ( _combatType == COMBAT_MELEE ) && // in melee combat
			 _rangedPossible &&    // ranged combat is possible
			 !owner->TestMelee() ) // I can't hit the enemy
		{
			float        orgZ = owner->GetPhysics()->GetOrigin().z;
			float      height = owner->GetPhysics()->GetBounds().GetSize().z;
			float   enemyOrgZ = enemy->GetPhysics()->GetOrigin().z;
			float enemyHeight = enemy->GetPhysics()->GetBounds().GetSize().z;
			if ( ( (orgZ + height + owner->melee_range_vert) < enemyOrgZ ) || // enemy too high
							     ( (enemyOrgZ + enemyHeight) < orgZ ) ) // enemy too low
			{
				newCombatType = COMBAT_RANGED;
			}
		}

		if ( newCombatType == _combatType )
		{
			// yes - no need to run weapon-switching animations
			_combatSubState = EStateCombatAndChecks;
			return;
		}

		// Do you need to switch a melee or ranged weapon? You might already have one
		// drawn, or you might have none drawn, or you might have to change weapons,
		// or you might be using unarmed attacks, and you don't need a drawn weapon.

		// Check for unarmed combat.

		if ( _unarmedMelee && ( newCombatType == COMBAT_MELEE ) )
		{
			// unarmed combat doesn't need attached weapons
			_combatType = COMBAT_NONE; // clear ranged combat tasks and start melee combat tasks
			_combatSubState = EStateCombatAndChecks;
			return;
		}

		if ( _unarmedRanged && ( newCombatType == COMBAT_RANGED ) )
		{
			// unarmed combat doesn't need attached weapons
			_combatType = COMBAT_NONE; // clear melee combat tasks and start ranged combat tasks
			_combatSubState = EStateCombatAndChecks;
			return;
		}

		// Do you have a drawn weapon?

		if ( owner->GetAttackFlag(COMBAT_MELEE) && ( newCombatType == COMBAT_MELEE ) )
		{
			// melee weapon is already drawn
			_combatSubState = EStateCombatAndChecks;
			return;
		}

		if ( owner->GetAttackFlag(COMBAT_RANGED) && ( newCombatType == COMBAT_RANGED ) )
		{
			// ranged weapon is already drawn
			_combatSubState = EStateCombatAndChecks;
			return;
		}

		// At this point, we know we need to draw a weapon that's not already drawn.
		// See if you need to sheathe a drawn weapon.

		if ( ( ( newCombatType == COMBAT_RANGED ) && owner->GetAttackFlag(COMBAT_MELEE)  ) ||
		     ( ( newCombatType == COMBAT_MELEE  ) && owner->GetAttackFlag(COMBAT_RANGED) ) )
		{
			// switch from one type of weapon to another
			owner->movementSubsystem->ClearTasks();
			owner->actionSubsystem->ClearTasks();
			_combatType = COMBAT_NONE;

			// sheathe melee weapon so you can draw ranged weapon
			owner->SheathWeapon();
			_waitEndTime = gameLocal.time + 2000; // safety net
			_combatSubState = EStateSheathingWeapon;
			return;
		}

		// No need to sheathe a weapon
		_combatSubState = EStateDrawWeapon;

		break;
		}

	case EStateSheathingWeapon:
		{
		// if you're sheathing a weapon, stay in this state until it's done, or until the timer expires

		const char *currentAnimState = owner->GetAnimState(ANIMCHANNEL_TORSO);
		idStr torsoString = "Torso_SheathWeapon";
		if ( torsoString.Cmp(currentAnimState) == 0 )
		{
			if ( gameLocal.time < _waitEndTime )
			{
				return;
			}
		}

		_combatSubState = EStateDrawWeapon;

		break;
		}

	case EStateDrawWeapon:
		{
		// grayman #3331 - if you don't already have the correct weapon drawn,
		// draw a ranged weapon if you're far from the enemy, and you have a 
		// ranged weapon, otherwise draw your melee weapon

		bool drawingWeapon = false;

		if ( !inMeleeRange )
		{
			// beyond melee range
			if ( !owner->GetAttackFlag(COMBAT_RANGED) && _rangedPossible )
			{
				owner->DrawWeapon(COMBAT_RANGED);
				drawingWeapon = true;
			}
			else // no ranged weapon
			{
				owner->DrawWeapon(COMBAT_MELEE);
				drawingWeapon = true;
			}
		}
		else // in melee range
		{
			if ( !owner->GetAttackFlag(COMBAT_MELEE) && _meleePossible )
			{
				owner->DrawWeapon(COMBAT_MELEE);
				drawingWeapon = true;
			}
		}

		// grayman #3331 - if this is the first weapon draw, to make sure the weapon is drawn
		// before starting combat, delay some before starting to chase the enemy.
		// The farther away the enemy is, the better the chance that you'll start chasing before your
		// weapon is drawn. If he's close, this gives you time to completely draw your weapon before
		// engaging him. The interesting distance is how far you have to travel to get w/in melee range.

		if ( _needInitialDrawDelay ) // True if this is the first time through, and you don't already have a raised weapon
		{
			int delay = 0;

			if ( drawingWeapon )
			{
				delay = (int)(2064.0f - 20.0f*(dist2Enemy - owner->GetMeleeRange()));
				if ( delay < 0 )
				{
					delay = gameLocal.random.RandomInt(2064);
				}
				_waitEndTime = gameLocal.time + delay;
				_combatSubState = EStateDrawingWeapon;
			}
			else
			{
				_combatSubState = EStateCombatAndChecks;
			}
			_needInitialDrawDelay = false; // No need to do this again
		}
		else
		{
			if ( drawingWeapon )
			{
				_waitEndTime = gameLocal.time;
				_combatSubState = EStateDrawingWeapon;
			}
			else
			{
				_combatSubState = EStateCombatAndChecks;
			}
		}

		break;
		}

	case EStateDrawingWeapon:
		{
		// grayman #3331 - stay in this state until weapon-drawing animation completes

		const char *currentAnimState = owner->GetAnimState(ANIMCHANNEL_TORSO);
		idStr torsoString1 = "Torso_DrawMeleeWeapon";
		idStr torsoString2 = "Torso_DrawRangedWeapon";
		if ( ( torsoString1.Cmp(currentAnimState) == 0 ) || ( torsoString2.Cmp(currentAnimState) == 0 ) )
		{
			return; // wait until weapon is drawn
		}

		if ( gameLocal.time < _waitEndTime )
		{
			return; // wait until timer expires
		}

		// Weapon is now drawn

		_justDrewWeapon = true;
		_combatSubState = EStateCombatAndChecks;

		break;
		}

	case EStateCombatAndChecks:
		{
		// Need to check if a weapon that was just drawn is correct for the zone you're now in, in case
		// you started drawing the correct weapon for one zone, and while it was drawing, you switched
		// to the other zone.
		
		if ( _justDrewWeapon )
		{
			if ( newCombatType == COMBAT_RANGED )
			{
				// beyond melee range
				if ( !owner->GetAttackFlag(COMBAT_RANGED) && _rangedPossible )
				{
					// wrong weapon raised - go back and get the correct one
					_justDrewWeapon = false;
					_combatSubState = EStateCheckWeaponState;
					return;
				}
			}
			else // in melee combat
			{
				if ( !owner->GetAttackFlag(COMBAT_MELEE) && _meleePossible )
				{
					// wrong weapon raised - go back and get the correct one
					_justDrewWeapon = false;
					_combatSubState = EStateCheckWeaponState;
					return;
				}
			}
		}

		if ( _combatType == COMBAT_NONE ) // Either combat hasn't been initially set up, or you're switching weapons
		{
			if ( newCombatType == COMBAT_RANGED )
			{
				// Set up ranged combat
				owner->actionSubsystem->PushTask(RangedCombatTask::CreateInstance());
				owner->movementSubsystem->PushTask(ChaseEnemyRangedTask::CreateInstance());
				_combatType = COMBAT_RANGED;
			}
			else
			{
				// Set up melee combat
				ChaseEnemyTaskPtr chaseEnemy = ChaseEnemyTask::CreateInstance();
				chaseEnemy->SetEnemy(enemy);
				owner->movementSubsystem->PushTask(chaseEnemy);

				owner->actionSubsystem->PushTask(MeleeCombatTask::CreateInstance());
				_combatType = COMBAT_MELEE;
			}

			// Let the AI update their weapons (make them nonsolid)
			owner->UpdateAttachmentContents(false);
		}

		// Check the distance to the enemy, the subsystem tasks need it.
		memory.canHitEnemy = owner->CanHitEntity(enemy, _combatType);
		// grayman #3331 - willBeAbleToHitEnemy is only relevant if canHitEnemy is FALSE
		if ( owner->m_bMeleePredictProximity && !memory.canHitEnemy )
		{
			memory.willBeAbleToHitEnemy = owner->WillBeAbleToHitEntity(enemy, _combatType);
		}

		// Check whether the enemy can hit us in the near future
		memory.canBeHitByEnemy = owner->CanBeHitByEntity(enemy, _combatType);

		if ( !owner->AI_ENEMY_VISIBLE && 
			 ( ( ( _combatType == COMBAT_MELEE )  && !memory.canHitEnemy ) || ( _combatType == COMBAT_RANGED) ) )
		{
			// The enemy is not visible, let's keep track of him for a small amount of time
			if (gameLocal.time - memory.lastTimeEnemySeen < MAX_BLIND_CHASE_TIME)
			{
				// Cheat a bit and take the last reachable position as "visible & reachable"
				owner->lastVisibleReachableEnemyPos = owner->lastReachableEnemyPos;
			}
			else if (owner->ReachedPos(owner->lastVisibleReachableEnemyPos, MOVE_TO_POSITION) || 
					( ( gameLocal.time - memory.lastTimeEnemySeen ) > 2 * MAX_BLIND_CHASE_TIME) )
			{
				// BLIND_CHASE_TIME has expired, we have lost the enemy!
				owner->GetMind()->SwitchState(STATE_LOST_TRACK_OF_ENEMY);
				return;
			}
		}

		// Flee if you're damaged and the current melee action is finished
		if ( ( owner->health < _criticalHealth ) && ( owner->m_MeleeStatus.m_ActionState == MELEEACTION_READY ) )
		{
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("I'm badly hurt, I'm afraid, and am fleeing!\r");
			owner->GetMind()->SwitchState(STATE_FLEE);
			return;
		}

		_combatSubState = EStateCheckWeaponState;

		break;
		}

	default:
		break;
	}
}

bool CombatState::CheckEnemyStatus(idActor* enemy, idAI* owner)
{
	if (enemy == NULL)
	{
		DM_LOG(LC_AI, LT_ERROR)LOGSTRING("No enemy, terminating task!\r");
		owner->GetMind()->EndState();
		return false;
	}

	if (enemy->AI_DEAD)
	{
		// grayman #2816 - remember the last enemy killed
		owner->SetLastKilled(enemy);

		owner->ClearEnemy();
		owner->StopMove(MOVE_STATUS_DONE);

		// Stop doing melee fighting
		owner->actionSubsystem->ClearTasks();

		// TODO: Check if more enemies are in range
		owner->SetAlertLevel(owner->thresh_2 + (owner->thresh_3 - owner->thresh_2) * 0.9);

		// grayman #2816 - need to delay the victory bark, because it's
		// being emitted too soon. Can't simply put a delay on SingleBarkTask()
		// because the AI clears his communication tasks when he drops back into
		// Suspicious mode, which wipes out the victory bark.
		// We need to post an event for later and emit the bark then.

		// new way

		// grayman #3343 - accommodate different barks for human and non-human enemies

		idStr bark = "";
		idStr enemyAiUse = enemy->spawnArgs.GetString("AIUse");
		if ( ( enemyAiUse == AIUSE_MONSTER ) || ( enemyAiUse == AIUSE_UNDEAD ) )
		{
			bark = "snd_killed_monster";
		}
		else
		{
			bark = "snd_killed_enemy";
		}
		owner->PostEventMS(&AI_Bark,ENEMY_DEAD_BARK_DELAY,bark);

/* old way
		// Emit the killed enemy bark
		owner->commSubsystem->AddCommTask(
			CommunicationTaskPtr(new SingleBarkTask("snd_killed_enemy"))
		);
 */
		// Set the expiration date of this state
		_endTime = gameLocal.time + 3000;

		return false;
	}

	if (!owner->IsEnemy(enemy))
	{
		// angua: the relation to the enemy has changed, this is not an enemy any more
		owner->StopMove(MOVE_STATUS_DONE);
		owner->SetAlertLevel(owner->thresh_2 + (owner->thresh_3 - owner->thresh_2) * 0.9);
		owner->ClearEnemy();
		owner->GetMind()->EndState();
		
		owner->movementSubsystem->ClearTasks();
		owner->senseSubsystem->ClearTasks();
		owner->actionSubsystem->ClearTasks();

		return false;
	}

	return true; // Enemy still alive and kicking
}

void CombatState::Save(idSaveGame* savefile) const
{
	State::Save(savefile);

	savefile->WriteInt(_criticalHealth);
	savefile->WriteBool(_meleePossible);
	savefile->WriteBool(_rangedPossible);
	savefile->WriteInt(static_cast<int>(_combatType));
	_enemy.Save(savefile);
	savefile->WriteInt(_endTime);
	savefile->WriteInt(static_cast<int>(_combatSubState)); // grayman #3063
	savefile->WriteInt(_reactionEndTime); // grayman #3063
	savefile->WriteInt(_waitEndTime);	  // grayman #3331
	savefile->WriteBool(_needInitialDrawDelay); // grayman #3331
	savefile->WriteBool(_unarmedMelee);		// grayman #3331
	savefile->WriteBool(_unarmedRanged);	// grayman #3331
	savefile->WriteBool(_armedMelee);		// grayman #3331
	savefile->WriteBool(_armedRanged);		// grayman #3331
	savefile->WriteBool(_justDrewWeapon);	// grayman #3331
}

void CombatState::Restore(idRestoreGame* savefile)
{
	State::Restore(savefile);

	savefile->ReadInt(_criticalHealth);
	savefile->ReadBool(_meleePossible);
	savefile->ReadBool(_rangedPossible);

	int temp;
	savefile->ReadInt(temp);
	_combatType = static_cast<ECombatType>(temp);

	_enemy.Restore(savefile);
	savefile->ReadInt(_endTime);

	// grayman #3063
	savefile->ReadInt(temp);
	_combatSubState = static_cast<ECombatSubState>(temp);

	savefile->ReadInt(_reactionEndTime); // grayman #3063
	savefile->ReadInt(_waitEndTime);	 // grayman #3331
	savefile->ReadBool(_needInitialDrawDelay); // grayman #3331
	savefile->ReadBool(_unarmedMelee);	 // grayman #3331
	savefile->ReadBool(_unarmedRanged);	 // grayman #3331
	savefile->ReadBool(_armedMelee);	 // grayman #3331
	savefile->ReadBool(_armedRanged);	 // grayman #3331
	savefile->ReadBool(_justDrewWeapon); // grayman #3331
}

StatePtr CombatState::CreateInstance()
{
	return StatePtr(new CombatState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar combatStateRegistrar(
	STATE_COMBAT, // Task Name
	StateLibrary::CreateInstanceFunc(&CombatState::CreateInstance) // Instance creation callback
);

} // namespace ai
