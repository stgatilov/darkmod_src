/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod (http://www.thedarkmod.com/)
 
 $Revision: 5363 $ (Revision of last commit) 
 $Date: 2012-04-01 14:08:35 -0400 (Sun, 01 Apr 2012) $ (Date of last commit)
 $Author: grayman $ (Author of last commit)
 
******************************************************************************/

#include "precompiled_game.h"
#pragma hdrstop

static bool versioned = RegisterVersionedFile("$Id: HitByMoveableState.cpp 5363 2012-04-01 18:08:35Z grayman $");

#include "HitByMoveableState.h"

namespace ai
{

HitByMoveableState::HitByMoveableState()
{}

// Get the name of this state
const idStr& HitByMoveableState::GetName() const
{
	static idStr _name(STATE_HIT_BY_MOVEABLE);
	return _name;
}

// Wrap up and end state

void HitByMoveableState::Wrapup(idAI* owner)
{
	owner->GetMemory().hitByThisMoveable = NULL;
	owner->m_ReactingToHit = false;
	owner->GetMemory().stopReactingToHit = false;
	owner->GetMind()->EndState();
}

void HitByMoveableState::Init(idAI* owner)
{
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("HitByMoveableState initialized.\r");
	assert(owner);

	idEntity* tactEnt = owner->GetMemory().hitByThisMoveable.GetEntity();
	if ( tactEnt == NULL )
	{
		Wrapup(owner);
		return;
	}

	idVec3 dir = tactEnt->GetPhysics()->GetOrigin() - owner->GetPhysics()->GetOrigin(); // direction to object
	dir.z = 0; // zero vertical component
	dir.NormalizeFast();

	// If the object is above the AI's eyes, assume it arrived
	// from above. Raise the point the AI will look at when he turns back.

	float vertDelta = tactEnt->GetPhysics()->GetOrigin().z - owner->GetEyePosition().z;

	if ( vertDelta > 0 )
	{
		dir.z = 0.5;
		dir.NormalizeFast();
	}

	_pos = owner->GetEyePosition() + dir*HIT_DIST; // where to look when turning back

	_responsibleActor = tactEnt->m_SetInMotionByActor;	// who threw it

	// TODO: Do we need to abort door or elevator handling here?
	// How about relighting?
	// How about examining a rope?

	owner->actionSubsystem->ClearTasks();
	owner->movementSubsystem->ClearTasks();

	owner->StopMove(MOVE_STATUS_DONE);
	owner->GetMemory().stopRelight = true;
	owner->GetMemory().stopExaminingRope = true;
	owner->GetMemory().stopReactingToHit = false;

	// if AI is sitting or sleeping, he has to stand before looking around

	if ( ( owner->GetMoveType() == MOVETYPE_SIT ) || ( owner->GetMoveType() == MOVETYPE_SLEEP ) )
	{
		owner->GetUp();
		_hitByMoveableState = EStateSittingSleeping;
	}
	else
	{
		_hitByMoveableState = EStateStarting;
	}

	_waitEndTime = gameLocal.time + HIT_DELAY; // slight pause before reacting
	owner->m_ReactingToHit = true;
}

// Gets called each time the mind is thinking
void HitByMoveableState::Think(idAI* owner)
{
	// check if something happened to abort the state
	if (owner->GetMemory().stopReactingToHit)
	{
		Wrapup(owner);
		return;
	}

	owner->PerformVisualScan();	// Let the AI check its senses
	if (owner->AI_AlertLevel >= owner->thresh_5) // finished if alert level is too high
	{
		Wrapup(owner);
		return;
	}

	idEntity* tactEnt = owner->GetMemory().hitByThisMoveable.GetEntity();
	if ( tactEnt == NULL )
	{
		Wrapup(owner);
		return;
	}

	switch (_hitByMoveableState)
	{
		case EStateSittingSleeping:
			if (gameLocal.time >= _waitEndTime)
			{
				if (owner->AI_MOVE_DONE && (owner->GetMoveType() != MOVETYPE_GET_UP)) // standing yet?
				{
					_hitByMoveableState = EStateTurnToward;
				}
			}
			break;
		case EStateStarting:
			if (gameLocal.time >= _waitEndTime)
			{
				_hitByMoveableState = EStateTurnToward;
			}
			break;
		case EStateTurnToward: // Turn toward the entity that hit you.
			{
				owner->TurnToward(tactEnt->GetPhysics()->GetOrigin());
				_hitByMoveableState = EStateLookAt;
				_waitEndTime = gameLocal.time + HIT_DELAY;
			}
			break;
		case EStateLookAt: // look at the entity
			if (gameLocal.time >= _waitEndTime)
			{
				float duration = HIT_DURATION + HIT_VARIATION*(gameLocal.random.RandomFloat() - 0.5f );
				owner->Event_LookAtEntity( tactEnt, duration/1000.0f );

				_waitEndTime = gameLocal.time + duration;
				_hitByMoveableState = EStateTurnBack;
			}
			break;
		case EStateTurnBack: // turn toward the direction the entity came from
			if (gameLocal.time >= _waitEndTime)
			{
				owner->TurnToward(_pos);
				float duration = HIT_DURATION + HIT_VARIATION*(gameLocal.random.RandomFloat() - 0.5f );
				_waitEndTime = gameLocal.time + duration;
				_hitByMoveableState = EStateLookBack;
			}
			break;
		case EStateLookBack: // look in the direction the entity came from
			if (gameLocal.time >= _waitEndTime)
			{
				float duration = HIT_DURATION + HIT_VARIATION*(gameLocal.random.RandomFloat() - 0.5f );
				owner->Event_LookAtPosition( _pos, duration/1000.0f );
				_waitEndTime = gameLocal.time + duration;
				_hitByMoveableState = EStateFinal;
			}
			break;
		case EStateFinal:
			if (gameLocal.time >= _waitEndTime)
			{
				// If we make it to here, we haven't spotted
				// an enemy, otherwise our alert level would
				// have been high enough to abort this state
				// at the beginning of Think().

				// If we see a friend or a neutral, we assume
				// this was a harmless impact, and we can return
				// to what we were doing.

				idVec3 ownerOrg = owner->GetPhysics()->GetOrigin();
				idBounds bounds( ownerOrg - idVec3( HIT_FIND_THROWER_HORZ, HIT_FIND_THROWER_HORZ, HIT_FIND_THROWER_VERT ), ownerOrg + idVec3( HIT_FIND_THROWER_HORZ, HIT_FIND_THROWER_HORZ, HIT_FIND_THROWER_VERT ) );

				idEntity* ents[MAX_GENTITIES];
				int num = gameLocal.clip.EntitiesTouchingBounds( bounds, CONTENTS_BODY, ents, MAX_GENTITIES );

				for ( int i = 0 ; i < num ; i++ )
				{
					idEntity *candidate = ents[i];

					if ( candidate == NULL ) // just in case
					{
						continue;
					}

					if ( candidate == owner ) // skip myself
					{
						continue;
					}

					if ( !candidate->IsType(idAI::Type) ) // skip non-AI
					{
						continue;
					}

					idAI *candidateAI = static_cast<idAI *>(candidate);

					// Only look for humanoids. The AIUse check allows
					// persons and undead to be considered, and leaves out
					// werebeasts, which are probably the only other AI
					// capable of putting something in motion. I think that's
					// okay. We just don't want to consider rats, spiders, etc.

					idStr aiUse = candidateAI->spawnArgs.GetString("AIUse");
					if ( ( aiUse != "AIUSE_PERSON" ) && ( aiUse != "AIUSE_UNDEAD" ) ) // skip non-persons
					{
						continue;
					}

					if ( candidateAI->IsKnockedOut() || ( candidateAI->health < 0 ) || ( candidateAI->GetMoveType() == MOVETYPE_SLEEP ) )
					{
						continue; // skip if knocked out, dead, or asleep
					}

					if ( candidateAI->fl.notarget )
					{
						continue; // skip if NOTARGET is set
					}

					if ( owner->IsFriend(candidateAI) || owner->IsNeutral(candidateAI) )
					{
						// make sure there's LOS (no walls in between)

						if ( owner->CanSeeExt( candidateAI, false, false ) ) // don't use FOV or lighting, to increase chance of success
						{
							Wrapup(owner);
							return;
						}
					}
				}

				// No one about, so let's cheat. If a friend or neutral threw
				// the object, we return to what we were doing.

				idActor* responsible = _responsibleActor.GetEntity();

				if ( ( responsible == NULL ) || owner->IsFriend(responsible) || owner->IsNeutral(responsible) )
				{
					Wrapup(owner);
					return;
				}

				// No one about. If an enemy threw the object, start searching.

				if ( owner->IsEnemy(responsible) )
				{
					Memory& memory = owner->GetMemory();
					memory.alertType = EAlertTypeSuspicious;
					memory.alertClass = EAlertTactile;
					memory.alertPos = ownerOrg;
					memory.alertRadius = TACTILE_ALERT_RADIUS;
					memory.alertSearchVolume = TACTILE_SEARCH_VOLUME;
					memory.alertSearchExclusionVolume.Zero();
					memory.visualAlert = false;

					// If we got this far, we give the alert
					// Set the alert amount to the according tactile alert value
					float amount = cv_ai_tactalert.GetFloat();

					// NOTE: Latest tactile alert always overrides other alerts
					owner->m_TactAlertEnt = tactEnt;
					owner->m_AlertedByActor = responsible;

					amount *= owner->GetAcuity("tact");
					owner->AlertAI("tact", amount);

					// Set last visual contact location to this location as that is used in case
					// the target gets away.
					owner->m_LastSight = ownerOrg;

					// If no enemy set so far, set the last visible enemy position.
					if ( owner->GetEnemy() == NULL )
					{
						owner->lastVisibleEnemyPos = ownerOrg;
					}

					owner->AI_TACTALERT = true;
				}

				Wrapup(owner);
				return;
			}
			break;
		default:
			break;
	}
}

void HitByMoveableState::Save(idSaveGame* savefile) const
{
	State::Save(savefile);
	_responsibleActor.Save(savefile);

	savefile->WriteInt(_waitEndTime);
	savefile->WriteInt(static_cast<int>(_hitByMoveableState));
	savefile->WriteVec3(_pos);
}

void HitByMoveableState::Restore(idRestoreGame* savefile)
{
	State::Restore(savefile);
	_responsibleActor.Restore(savefile);

	savefile->ReadInt(_waitEndTime);
	int temp;
	savefile->ReadInt(temp);
	_hitByMoveableState = static_cast<EHitByMoveableState>(temp);
	savefile->ReadVec3(_pos);
}

StatePtr HitByMoveableState::CreateInstance()
{
	return StatePtr(new HitByMoveableState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar hitByMoveableStateRegistrar(
	STATE_HIT_BY_MOVEABLE, // Task Name
	StateLibrary::CreateInstanceFunc(&HitByMoveableState::CreateInstance) // Instance creation callback
);

} // namespace ai
