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

#include "ExamineRopeState.h"
//#include "../Memory.h"
#include "../Tasks/MoveToPositionTask.h"
//#include "../Tasks/SingleBarkTask.h"
//#include "../../StimResponse/StimResponse.h"
//#include "../Tasks/RandomHeadturnTask.h"
//#include "../Tasks/RandomTurningTask.h"

namespace ai
{

ExamineRopeState::ExamineRopeState()
{}

ExamineRopeState::ExamineRopeState(idAFEntity_Generic* rope, idVec3 point)
{
	_rope = rope;
	_point = point;
}

// Get the name of this state
const idStr& ExamineRopeState::GetName() const
{
	static idStr _name(STATE_EXAMINE_ROPE);
	return _name;
}

// Wrap up and end state

void ExamineRopeState::Wrapup(idAI* owner)
{
	owner->m_ExaminingRope = false;
	owner->GetMemory().stopExaminingRope = false;
	owner->GetMind()->EndState();
}

void ExamineRopeState::Init(idAI* owner)
{
	//Memory& memory = owner->GetMemory();

	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("ExamineRopeState initialized.\r");
	assert(owner);

	idAFEntity_Generic* rope = _rope.GetEntity();
	if ( rope == NULL ) // this could happen if the player frobs the rope arrow
	{
		Wrapup(owner);
		return;
	}

	idVec3 goalDirection;
	idVec3 ownerOrg = owner->GetPhysics()->GetOrigin();
	int areaNum = owner->PointReachableAreaNum(ownerOrg, 1.0f);

	idEntity* bindMaster = rope->GetBindMaster();
	goalDirection = ownerOrg - _point; // fallback - use a direction from the point to the AI

	if ( bindMaster != NULL ) // the bindMaster is the stuck rope arrow
	{
		goalDirection = rope->GetPhysics()->GetOrigin() - bindMaster->GetPhysics()->GetOrigin(); // preferred direction
	}
	goalDirection.z = 0; // ignore vertical component
	goalDirection.NormalizeFast();

	idVec3 size(16, 16, 82);
	idAAS* aas = owner->GetAAS();
	if (aas)
	{
		size = aas->GetSettings()->boundingBoxes[0][1];
	}

	// Move away from _point and perform a trace down to detect the ground.

	float standOff = 4*size.x; // any closer than this makes the "look up" animation look a bit strained (ouch!) 
	idVec3 startPoint = _point + goalDirection * standOff;
	idVec3 bottomPoint = startPoint;
	bottomPoint.z -= 1000;

	idVec3 tp1 = startPoint;
	trace_t result;
	bool tp1Found = false;
	if ( gameLocal.clip.TracePoint(result, startPoint, bottomPoint, MASK_OPAQUE, NULL) )
	{
		// Found the floor.

		tp1.z = result.endpos.z + 1; // move the target point to just above the floor
		tp1Found = true;
	}

	// Reverse direction and see if the floor is closer in that direction

	startPoint = _point - goalDirection * standOff;
	idVec3 tp2 = startPoint;
	bool tp2Found = false;
	bottomPoint = startPoint;
	bottomPoint.z -= 1000;

	if (gameLocal.clip.TracePoint(result, startPoint, bottomPoint, MASK_OPAQUE, NULL))
	{
		// Found the floor.

		tp2.z = result.endpos.z + 1; // move the target point to just above the floor
		tp2Found = true;
	}

	bool use1 = false;

	if ( tp1Found )
	{
		if ( tp2Found )
		{
			float tp1ZDelta = abs(ownerOrg.z - tp1.z);
			float tp2ZDelta = abs(ownerOrg.z - tp2.z);
			use1 = ( tp1ZDelta <= tp2ZDelta );
		}
		else
		{
			use1 = true;
		}
	}

	_examineSpot = _point; // if no spots are found for the AI to stage an examination, search around _point
	int targetAreaNum;
	aasPath_t path;
	bool pathToGoal = false; // whether there's a path to the target point near the rope
	if ( use1 )
	{
		// Is there a path to tp1?

		targetAreaNum = owner->PointReachableAreaNum(tp1, 1.0f);
		if ( owner->PathToGoal(path, areaNum, ownerOrg, targetAreaNum, tp1, owner) )
		{
			pathToGoal = true;
			_examineSpot = tp1;
		}
	}

	if ( !pathToGoal )
	{
		if ( tp2Found )
		{
			// Is there a path to tp2?

			targetAreaNum = owner->PointReachableAreaNum(tp2, 1.0f);
			if ( owner->PathToGoal(path, areaNum, ownerOrg, targetAreaNum, tp2, owner) )
			{
				pathToGoal = true;
				_examineSpot = tp2;
			}
		}
	}

	if ( pathToGoal )
	{
		owner->actionSubsystem->ClearTasks();
		owner->movementSubsystem->ClearTasks();

		// if AI is sitting, he has to stand before sending him on his way

		owner->movementSubsystem->PushTask(TaskPtr(new MoveToPositionTask(_examineSpot,idMath::INFINITY,5)));
		if (owner->GetMoveType() == MOVETYPE_SIT)
		{
			_examineRopeState = EStateSitting;
		}
		else
		{
			_examineRopeState = EStateStarting;
		}

		_waitEndTime = gameLocal.time + 1000; // allow time for move to begin
		return;
	}

	// There's no path to the goal. Go straight to searching.

	_waitEndTime = 0;
	_examineRopeState = EStateFinal;
}

// Gets called each time the mind is thinking
void ExamineRopeState::Think(idAI* owner)
{
	// It's possible that during the examination of a rope, the player
	// frobs back the rope arrow. You have to account for that.

	Memory& memory = owner->GetMemory();

	idAFEntity_Generic* rope = _rope.GetEntity();

	if ( rope == NULL ) // this could happen if the player frobs the rope arrow
	{
		Wrapup(owner);
		return;
	}

	// check if something happened to abort the examination
	if (owner->GetMemory().stopExaminingRope)
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

	if ((owner->m_HandlingDoor) || (owner->m_HandlingElevator))
	{
		return; // we're handling a door or elevator, so delay the examination
	}

	switch (_examineRopeState)
	{
		case EStateSitting:
			if (gameLocal.time >= _waitEndTime)
			{
				if (owner->AI_MOVE_DONE && (owner->GetMoveType() != MOVETYPE_GET_UP)) // standing yet?
				{
					owner->movementSubsystem->PushTask(TaskPtr(new MoveToPositionTask(_examineSpot,idMath::INFINITY,5)));
					_examineRopeState = EStateStarting;
					_waitEndTime = gameLocal.time + 1000; // allow time for move to begin
				}
			}
			break;
		case EStateStarting:
			if (owner->AI_FORWARD || (gameLocal.time >= _waitEndTime))
			{
				_examineRopeState = EStateApproaching;
			}
			break;
		case EStateApproaching: // Walking toward the rope
			if (owner->AI_MOVE_DONE)
			{
				owner->TurnToward(_point);
				_examineRopeState = EStateTurningToward;
				_waitEndTime = gameLocal.time + 750; // allow time for turn to complete
			}
			break;
		case EStateTurningToward:
			if (gameLocal.time >= _waitEndTime)
			{
				StartExaminingTop(owner); // AI looks at top of rope
				_examineRopeState = EStateExamineTop;
				_waitEndTime = gameLocal.time + 3000;
			}
			break;
		case EStateExamineTop:
			if (gameLocal.time >= _waitEndTime)
			{
				StartExaminingBottom(owner); // AI looks at bottom of rope
				_waitEndTime = gameLocal.time + 3000;
				_examineRopeState = EStateExamineBottom;
			}
			break;
		case EStateExamineBottom:
			if (gameLocal.time >= _waitEndTime)
			{
				_waitEndTime = gameLocal.time + 1000;
				_examineRopeState = EStateFinal;
			}
			break;
		case EStateFinal:
			if (gameLocal.time >= _waitEndTime)
			{
				// Set up search if latched

				if (owner->m_LatchedSearch)
				{
					owner->m_LatchedSearch = false;
					if (owner->AI_AlertLevel < owner->thresh_4)
					{
						memory.alertPos = _examineSpot;
						memory.alertClass = EAlertVisual_4; // grayman #3498 (was _2)
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

						// If the rope origin is close to your feet, do a close investigation

						float ropeDist = rope->GetPhysics()->GetOrigin().z - owner->GetPhysics()->GetOrigin().z;
						memory.investigateStimulusLocationClosely = ( abs(ropeDist) <= 20 );
						memory.alertedDueToCommunication = false;
					}
				}

				Wrapup(owner);
				return;
			}
			break;
		default:
			break;
	}
}

void ExamineRopeState::StartExaminingTop(idAI* owner)
{
	// Look at the top of the rope

	owner->movementSubsystem->ClearTasks();
	owner->StopMove(MOVE_STATUS_DONE);

	owner->Event_LookAtPosition(_rope.GetEntity()->GetPhysics()->GetOrigin(), 3.0f);
}

void ExamineRopeState::StartExaminingBottom(idAI* owner)
{
	// Look at the bottom of the rope

//	owner->movementSubsystem->ClearTasks();
//	owner->StopMove(MOVE_STATUS_DONE);

	idMat3 axis;
	idVec3 org;
	jointHandle_t joint = _rope.GetEntity()->GetAnimator()->GetJointHandle( "bone92" );
	_rope.GetEntity()->GetJointWorldTransform( joint, gameLocal.time, org, axis );
	owner->Event_LookAtPosition(org, 3.0f);
}

void ExamineRopeState::Save(idSaveGame* savefile) const
{
	State::Save(savefile);
	_rope.Save(savefile);

	savefile->WriteInt(_waitEndTime);
	savefile->WriteInt(static_cast<int>(_examineRopeState));
	savefile->WriteVec3(_examineSpot);
	savefile->WriteVec3(_point);
}

void ExamineRopeState::Restore(idRestoreGame* savefile)
{
	State::Restore(savefile);
	_rope.Restore(savefile);

	savefile->ReadInt(_waitEndTime);
	int temp;
	savefile->ReadInt(temp);
	_examineRopeState = static_cast<EExamineRopeState>(temp);
	savefile->ReadVec3(_examineSpot);
	savefile->ReadVec3(_point);
}

StatePtr ExamineRopeState::CreateInstance()
{
	return StatePtr(new ExamineRopeState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar examineRopeStateRegistrar(
	STATE_EXAMINE_ROPE, // Task Name
	StateLibrary::CreateInstanceFunc(&ExamineRopeState::CreateInstance) // Instance creation callback
);

} // namespace ai
