/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 09:53:28 -0700 (Tue, 16 Oct 2007) $
 * $Author: greebo $
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

CLASS_DECLARATION( idAI, CAIVehicle )
END_CLASS

CAIVehicle::CAIVehicle( void )
{
	m_Rider			= NULL;
	m_RideJoint		= INVALID_JOINT;
	m_RideOffset.Zero();
	m_RideAngles.Zero();
	m_CurAngle		= 0.0f;
	
	m_SteerSpeed		= 0.0f;
	m_SpeedFrac			= 0.0f;
	m_SpeedTimeToMax	= 0.0f;
	m_MaxReverseSpeed	= 0.0f;
}

CAIVehicle::~CAIVehicle( void )
{
}

void CAIVehicle::Spawn( void ) 
{
	// set ride joint, steering speed
	idStr JointName = spawnArgs.GetString( "ride_joint", "origin" );

	spawnArgs.GetFloat( "steerSpeed", "5", m_SteerSpeed );
	spawnArgs.GetVector( "ride_offset", "0 0 0", m_RideOffset );
	spawnArgs.GetAngles( "ride_angles", "0 0 0", m_RideAngles );
	spawnArgs.GetFloat("time_to_max_speed", "2.0", m_SpeedTimeToMax );
	spawnArgs.GetFloat("max_reverse_speed", "1.0", m_MaxReverseSpeed );

	m_RideJoint = animator.GetJointHandle( JointName.c_str() );

	BecomeActive( TH_THINK );
}

void CAIVehicle::Save(idSaveGame *savefile) const
{
	m_Rider.Save( savefile );

	savefile->WriteJoint( m_RideJoint );
	savefile->WriteVec3( m_RideOffset );
	savefile->WriteAngles( m_RideAngles );

	savefile->WriteFloat( m_CurAngle );
	savefile->WriteFloat( m_SpeedFrac );
	savefile->WriteFloat( m_SteerSpeed );
	savefile->WriteFloat( m_SpeedTimeToMax );
	savefile->WriteFloat( m_MaxReverseSpeed );
}

void CAIVehicle::Restore( idRestoreGame *savefile )
{
	m_Rider.Restore( savefile );

	savefile->ReadJoint( m_RideJoint );
	savefile->ReadVec3( m_RideOffset );
	savefile->ReadAngles( m_RideAngles );
	
	savefile->ReadFloat( m_CurAngle );
	savefile->ReadFloat( m_SpeedFrac );
	savefile->ReadFloat( m_SteerSpeed );
	savefile->ReadFloat( m_SpeedTimeToMax );
	savefile->ReadFloat( m_MaxReverseSpeed );
}

void CAIVehicle::PlayerFrob( idPlayer *player ) 
{
	idVec3 origin;
	idMat3 axis;

	// =============== DISMOUNT ===============
	if ( m_Rider.GetEntity() ) 
	{
		if ( m_Rider.GetEntity() == player ) 
		{
			player->Unbind();
			m_Rider = NULL;
			m_SpeedFrac = 0.0f;

			// Return control to scripts
			m_bIgnoreAlerts = false;
			AI_RUN = false;

			StopMove( MOVE_STATUS_DONE );
		}
	}
	// =============== MOUNT ===============
	else 
	{
		m_Rider = player;

		// attach player to the joint
		GetJointWorldTransform( m_RideJoint, gameLocal.time, origin, axis );

		player->SetOrigin( origin + m_RideOffset * axis );
		player->BindToJoint( this, m_RideJoint, true );

		// Initialize angle to current yaw angle
		idAngles FaceAngles = viewAxis.ToAngles();

		m_CurAngle = FaceAngles.yaw;

		ClearEnemy();
		m_bIgnoreAlerts = true;
	}
}

void CAIVehicle::Think( void )
{
	if ( !(thinkFlags & TH_THINK ) )
		goto Quit;

	if( m_Rider.GetEntity() )
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

void CAIVehicle::FrobAction(bool bMaster, bool bPeer)
{
	// If someone does multiplayer, they'll have to pass the player along with frobaction
	idPlayer *localPlayer = gameLocal.GetLocalPlayer();

	// Don't mount if dead, but still let the player dismount
	if( (AI_KNOCKEDOUT || AI_DEAD) && !m_Rider.GetEntity() )
		idAI::FrobAction(bMaster,bPeer);
	else
		PlayerFrob(localPlayer);
}

void CAIVehicle::UpdateSteering( void )
{
	float idealSteerAngle(0.0f), angleDelta(0.0f), turnScale(0.0f);
	idPlayer *player = m_Rider.GetEntity();

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
	idPlayer *player = m_Rider.GetEntity();
	
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
		if( m_SpeedFrac > 0.5f )
			AI_RUN = true;
		else
			AI_RUN = false;
	}

	return bReturnVal;
}

