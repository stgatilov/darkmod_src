/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

// Copyright (C) 2007 The Dark Mod Authors
//

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "AIVehicle.h"
#include "../game/game_local.h"
#include "DarkModGlobals.h"

//===============================================================================
//CAIVehicle
//===============================================================================

const idEventDef EV_SetController( "setController", "e" );
const idEventDef EV_ClearController( "clearController" );
const idEventDef EV_FrobRidable( "frobRidable", "e" );

CLASS_DECLARATION( idAI, CAIVehicle )
	EVENT( EV_SetController,	CAIVehicle::Event_SetController )
	EVENT( EV_ClearController,	CAIVehicle::Event_ClearController )
	EVENT( EV_FrobRidable,		CAIVehicle::Event_FrobRidable )

END_CLASS

CAIVehicle::CAIVehicle( void )
{
	m_Controller			= NULL;
	m_RideJoint		= INVALID_JOINT;
	m_RideOffset.Zero();
	m_RideAngles.Zero();
	m_CurAngle		= 0.0f;
	
	m_SteerSpeed		= 0.0f;
	m_SpeedFrac			= 0.0f;
	m_SpeedTimeToMax	= 0.0f;
	m_MaxReverseSpeed	= 0.0f;

	m_MinWalkAnimRate	= 0.0f;
	m_MaxWalkAnimRate	= 0.0f;
	m_MinRunAnimRate	= 0.0f;
	m_MaxRunAnimRate	= 0.0f;

	m_WalkToRunSpeedFrac	= 0.0f;
}

CAIVehicle::~CAIVehicle( void )
{
}

void CAIVehicle::Spawn( void ) 
{
	// set ride joint, steering speed
	idStr JointName = spawnArgs.GetString("ride_joint", "origin");

	spawnArgs.GetFloat("steerSpeed", "5", m_SteerSpeed);
	spawnArgs.GetVector("ride_offset", "0 0 0", m_RideOffset);
	spawnArgs.GetAngles("ride_angles", "0 0 0", m_RideAngles);
	spawnArgs.GetFloat("time_to_max_speed", "2.0", m_SpeedTimeToMax);
	spawnArgs.GetFloat("max_reverse_speed", "1.0", m_MaxReverseSpeed);

	spawnArgs.GetFloat("min_walk_anim_rate", "1.0", m_MinWalkAnimRate);
	spawnArgs.GetFloat("max_walk_anim_rate", "1.0", m_MaxWalkAnimRate);
	spawnArgs.GetFloat("min_run_anim_rate", "1.0", m_MinRunAnimRate);
	spawnArgs.GetFloat("max_run_anim_rate", "1.0", m_MaxRunAnimRate);
	spawnArgs.GetFloat("walk_to_run_speed", "0.5", m_WalkToRunSpeedFrac);

	m_RideJoint = animator.GetJointHandle( JointName.c_str() );

	BecomeActive( TH_THINK );
}

void CAIVehicle::Save(idSaveGame *savefile) const
{
	m_Controller.Save( savefile );

	savefile->WriteJoint( m_RideJoint );
	savefile->WriteVec3( m_RideOffset );
	savefile->WriteAngles( m_RideAngles );

	savefile->WriteFloat( m_CurAngle );
	savefile->WriteFloat( m_SpeedFrac );
	savefile->WriteFloat( m_SteerSpeed );
	savefile->WriteFloat( m_SpeedTimeToMax );
	savefile->WriteFloat( m_MaxReverseSpeed );

	savefile->WriteFloat( m_MinWalkAnimRate );
	savefile->WriteFloat( m_MaxWalkAnimRate );
	savefile->WriteFloat( m_MinRunAnimRate );
	savefile->WriteFloat( m_MaxRunAnimRate );
	savefile->WriteFloat( m_WalkToRunSpeedFrac );
}

void CAIVehicle::Restore( idRestoreGame *savefile )
{
	m_Controller.Restore( savefile );

	savefile->ReadJoint( m_RideJoint );
	savefile->ReadVec3( m_RideOffset );
	savefile->ReadAngles( m_RideAngles );
	
	savefile->ReadFloat( m_CurAngle );
	savefile->ReadFloat( m_SpeedFrac );
	savefile->ReadFloat( m_SteerSpeed );
	savefile->ReadFloat( m_SpeedTimeToMax );
	savefile->ReadFloat( m_MaxReverseSpeed );

	savefile->ReadFloat( m_MinWalkAnimRate );
	savefile->ReadFloat( m_MaxWalkAnimRate );
	savefile->ReadFloat( m_MinRunAnimRate );
	savefile->ReadFloat( m_MaxRunAnimRate );
	savefile->ReadFloat( m_WalkToRunSpeedFrac );
}

void CAIVehicle::PlayerFrob( idPlayer *player ) 
{
	idVec3 origin;
	idMat3 axis;

	// =============== DISMOUNT ===============
	if ( m_Controller.GetEntity() ) 
	{
		if ( m_Controller.GetEntity() == player ) 
		{
			player->Unbind();
			SetController(NULL);
		}
	}
	// =============== MOUNT ===============
	else 
	{
		// attach player to the joint
		GetJointWorldTransform( m_RideJoint, gameLocal.time, origin, axis );

		// put the player in a crouch, so their view is low to the animal
		// without actually clipping into it
		idPhysics_Player *playerPhys = (idPhysics_Player *) player->GetPhysics();
		// TODO: look up PMF_DUCKED somehow (make those flags statics in the idPhysics_Player class?)
		// for now, using 1 since we know that is PMF_DUCKED
		// Or, we could move mount and dismount to idPhysics_Player...
		playerPhys->SetMovementFlags( playerPhys->GetMovementFlags() | 1 );

		player->SetOrigin( origin + m_RideOffset * axis );
		player->BindToJoint( this, m_RideJoint, true );

		SetController( player );
	}
}

void CAIVehicle::SetController( idPlayer *player )
{
	// no change
	if( player == m_Controller.GetEntity() )
		return;

	if( player != NULL )
	{
		// new controller added
		m_Controller = player;
		
		// Initialize angle to current yaw angle
		idAngles FaceAngles = viewAxis.ToAngles();

		m_CurAngle = FaceAngles.yaw;

		ClearEnemy();
		m_bIgnoreAlerts = true;
	}
	else
	{
		// controller removed, stop and return control to AI mind
		m_Controller = NULL;
		m_SpeedFrac = 0.0f;

		m_bIgnoreAlerts = false;
		AI_RUN = false;

		StopMove( MOVE_STATUS_DONE );
	}
}

void CAIVehicle::Think( void )
{
	if ( !(thinkFlags & TH_THINK ) )
		goto Quit;

	if( m_Controller.GetEntity() )
	{
		// Exit the combat state if we somehow got in it.
		// Later we can fight as directed by player, but right now it's too independent
		ClearEnemy();

		// Update controls and movement dir
		UpdateSteering();

		// Speed controls, for now just AI_MOVE
		bool bMovementReq = UpdateSpeed();

		// Request move at direction
		if( bMovementReq )
		{
			MoveAlongVector( m_CurAngle );
		}	
		else
		{
			StopMove( MOVE_STATUS_DONE );
			// just turn if no forward/back movement is requested
			TurnToward( m_CurAngle );
		}
	}

	idAI::Think();

Quit:
	return;
}

void CAIVehicle::Event_FrobRidable(idPlayer *player )
{
	// Don't mount if dead, but still let the player dismount
	if( (AI_KNOCKEDOUT || AI_DEAD) && !m_Controller.GetEntity() )
	{
		// proceed with normal AI body frobbing code
		idAI::FrobAction(true);
	}
	else
		PlayerFrob(player);
}

void CAIVehicle::UpdateSteering( void )
{
	float idealSteerAngle(0.0f), angleDelta(0.0f), turnScale(0.0f);
	idPlayer *player = m_Controller.GetEntity();

	// TODO: steer speed dependent on velocity to simulate finite turn radius

	if( idMath::Fabs(player->usercmd.rightmove) > 0 )
	{
		turnScale = 30.0f / 128.0f;
		idealSteerAngle = m_CurAngle - player->usercmd.rightmove * turnScale;
		angleDelta = idealSteerAngle - m_CurAngle;

		if ( angleDelta > m_SteerSpeed ) 
		{
			m_CurAngle += m_SteerSpeed;
		} else if ( angleDelta < -m_SteerSpeed ) 
		{
			m_CurAngle -= m_SteerSpeed;
		} else 
		{
			m_CurAngle = idealSteerAngle;
		}
	}
}

bool CAIVehicle::UpdateSpeed( void )
{
	bool bReturnVal( false );
	idPlayer *player = m_Controller.GetEntity();
	
	float DeltaVMag = 1.0f/(m_SpeedTimeToMax * USERCMD_HZ);

	if( player->usercmd.forwardmove > 0 )
		m_SpeedFrac += DeltaVMag;
	else if( player->usercmd.forwardmove < 0 )
		m_SpeedFrac -= DeltaVMag;

	// TODO: Support walking backwards, needs AI base class changes?

	// m_SpeedFrac = idMath::ClampFloat( -m_MaxReverseSpeed, 1.0f, m_SpeedFrac );
	m_SpeedFrac = idMath::ClampFloat( 0.0f, 1.0f, m_SpeedFrac );

	// For now, only move forward
	// bReturnVal = ( idMath::Fabs(m_SpeedFrac) > 0.0001f; )
	if( m_SpeedFrac > 0.0001f )
	{
		bReturnVal = true;

		// TODO: Continuously adjust speed using Crispy's code
		// For now, just toggle run when we get to 50%
		if( m_SpeedFrac > m_WalkToRunSpeedFrac )
		{
			float animFrac = (m_SpeedFrac - m_WalkToRunSpeedFrac)/(1.0f - m_WalkToRunSpeedFrac);
			float animRate = m_MinRunAnimRate + animFrac * (m_MaxRunAnimRate - m_MinRunAnimRate);
			// TODO: Read anim name from spawnarg
			const char *animName = "run";
			// argh, there is no way to get anim num from the name other than search??
			for( int i =0; i < animator.NumAnims(); i++ )
			{
				if( animator.AnimName(i) == animName )
				{
					m_animRates[i] = animRate;
					break;
				}
			}

			AI_RUN = true;
		}
		else
		{
			float animFrac = m_SpeedFrac /m_WalkToRunSpeedFrac;
			float animRate = m_MinWalkAnimRate + animFrac * (m_MaxWalkAnimRate - m_MinWalkAnimRate);
			// TODO: Read anim name from spawnarg
			const char *animName = "walk";

			// argh, there is no way to get anim num from the name other than search??
			for( int i =0; i < animator.NumAnims(); i++ )
			{
				if( animator.AnimName(i) == animName )
				{
					m_animRates[i] = animRate;
					break;
				}
			}

			AI_RUN = false;
		}
	}

	return bReturnVal;
}

// script events
void CAIVehicle::Event_SetController( idPlayer *player )
{
	SetController( player );
}

// script events
void CAIVehicle::Event_ClearController( void )
{
	SetController( NULL );
}

