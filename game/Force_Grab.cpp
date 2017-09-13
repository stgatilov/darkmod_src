/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod (http://www.thedarkmod.com/)
 
******************************************************************************/

#include "precompiled.h"
#pragma hdrstop



#include "Game_local.h"
#include "Force_Grab.h"
#include "Grabber.h"

class CDarkModPlayer;

CLASS_DECLARATION( idForce, CForce_Grab )
END_CLASS

/*
================
CForce_Grab::CForce_Grab
================
*/
CForce_Grab::CForce_Grab( void ) 
{
	m_damping			= 0.0f;
	m_physics			= NULL;
	m_id				= 0;
	m_p					= vec3_zero;
	m_dragPosition		= vec3_zero;
	m_dragAxis.Zero();
	m_centerOfMass		= vec3_zero;
	m_inertiaTensor.Identity();

	m_bApplyDamping = false;
	m_bLimitForce = false;
	m_RefEnt = NULL;
}

/*
================
CForce_Grab::~CForce_Grab
================
*/
CForce_Grab::~CForce_Grab( void ) {
}

/*
================
CForce_Grab::Init
================
*/
void CForce_Grab::Init( float damping ) {
	if ( damping >= 0.0f && damping < 1.0f ) 
	{
		m_damping = damping;
	}
	else
	{
		m_damping = 0;
	}
}

void CForce_Grab::Save( idSaveGame *savefile ) const
{
	m_RefEnt.Save( savefile );
	savefile->WriteFloat(m_damping);
	savefile->WriteVec3(m_centerOfMass);
	savefile->WriteMat3(m_inertiaTensor);

	// Don't save m_physics, gets restored from the parent class after load
	
	savefile->WriteVec3(m_p);
	savefile->WriteInt(m_id);
	savefile->WriteVec3(m_dragPosition);
	savefile->WriteMat3(m_dragAxis);
	savefile->WriteBool(m_bApplyDamping);
	savefile->WriteBool(m_bLimitForce);
}

void CForce_Grab::Restore( idRestoreGame *savefile )
{
	m_RefEnt.Restore( savefile );
	savefile->ReadFloat(m_damping);
	savefile->ReadVec3(m_centerOfMass);
	savefile->ReadMat3(m_inertiaTensor);

	m_physics = NULL; // gets restored from the parent class after loading

	savefile->ReadVec3(m_p);
	savefile->ReadInt(m_id);
	savefile->ReadVec3(m_dragPosition);
	savefile->ReadMat3(m_dragAxis);
	savefile->ReadBool(m_bApplyDamping);
	savefile->ReadBool(m_bLimitForce);
}

/*
================
CForce_Grab::SetPhysics
================
*/
void CForce_Grab::SetPhysics( idPhysics *phys, int id, const idVec3 &p ) {
	float mass, MassOut, density;
	idClipModel *clipModel;

	m_physics = phys;
	m_id = id;
	m_p = p;

	clipModel = m_physics->GetClipModel( m_id );
	if ( clipModel != NULL && clipModel->IsTraceModel() ) 
	{
		mass = m_physics->GetMass( m_id );
		// PROBLEM: No way to query physics object for density!
		// Trick: Use a test density of 1.0 here, then divide the actual mass by output mass to get actual density
		clipModel->GetMassProperties( 1.0f, MassOut, m_centerOfMass, m_inertiaTensor );
		density = mass / MassOut;
		// Now correct the inertia tensor by using actual density
		clipModel->GetMassProperties( density, mass, m_centerOfMass, m_inertiaTensor );
	} else 
	{
		m_centerOfMass.Zero();
		m_inertiaTensor = mat3_identity;
	}
}

/*
================
CForce_Grab::SetDragPosition
================
*/
void CForce_Grab::SetDragPosition( const idVec3 &pos ) 
{
	m_dragPosition = pos;
}

/*
================
CForce_Grab::GetDragPosition
================
*/
const idVec3 &CForce_Grab::GetDragPosition( void ) const 
{
	return m_dragPosition;
}

/*
================
CForce_Grab::SetDragAxis
================
*/
void CForce_Grab::SetDragAxis( const idMat3 &Axis )
{
	m_dragAxis = Axis;
}

/*
================
CForce_Grab::GetDragAxis
================
*/
idMat3 CForce_Grab::GetDragAxis( void )
{
	return m_dragAxis;
}

/*
================
CForce_Grab::GetDraggedPosition
================
*/
const idVec3 CForce_Grab::GetDraggedPosition( void ) const 
{
	return ( m_physics->GetOrigin( m_id ) + m_p * m_physics->GetAxis( m_id ) );
}

/*
================
CForce_Grab::Evaluate
================
*/
void CForce_Grab::Evaluate( int time ) 
{
	float l1, Accel, dT;
	idVec3 dragOrigin, dir1, dir2, velocity, COM, prevVel;
	idRotation rotation;

	if ( !m_physics ) 
	{
		return;
	}

	CGrabber *grabber = gameLocal.m_Grabber;

// ======================== LINEAR =========================

	COM = this->GetCenterOfMass();
	dragOrigin = COM;

	dir1 = m_dragPosition - dragOrigin;
	l1 = dir1.Normalize();
	dT = MS2SEC(gameLocal.getMsec()); // duzenko 4409: fixed tic + USERCMD_MSEC -> flickering
	if (dT < MS2SEC( USERCMD_MSEC )) // BluePill : Fix grab speed for higher framerates
		dT = MS2SEC( USERCMD_MSEC ); // time elapsed is time between user mouse commands

	if( !m_bApplyDamping )
		m_damping = 0.0f;

	if( grabber->m_bIsColliding )
	{
		// Zero out previous velocity when we start out colliding
		prevVel = vec3_zero;

		idVec3 newDir = dir1;

		for( int i=0; i < grabber->m_CollNorms.Num(); i++ )
		{
			// subtract out component of desired dir going in to surface
			if( newDir * grabber->m_CollNorms[i] < 0.0f )
			{
				newDir -= (newDir * grabber->m_CollNorms[i]) * grabber->m_CollNorms[i];
			}

			if( cv_drag_debug.GetBool() )
				gameRenderWorld->DebugArrow( colorBlue, COM, (COM + 30 * grabber->m_CollNorms[i]), 4.0f, 1);
		}

		// Clear m_CollNorms so it can be filled next time there's a collision
		grabber->m_CollNorms.Clear();
		
		newDir.Normalize();

		float newl1 = l1 * (dir1 * newDir);

		// avoid jittering due to floating point error
		if( newl1 > 0.1f )
		{
			l1 = newl1; // project the magnitude in the new direction
			dir1 = newDir;
		}
		else
		{
			dir1 = vec3_zero;
			l1 = 0.0f;
		}

		if( cv_drag_debug.GetBool() )
			gameRenderWorld->DebugArrow( colorRed, COM, (COM + l1 * dir1), 4.0f, 1);
	}
	else 
	{
		prevVel = m_physics->GetLinearVelocity( m_id );
		if( cv_drag_debug.GetBool() )
			gameRenderWorld->DebugArrow( colorGreen, COM, (COM + l1 * dir1), 4.0f, 1);
	}

	// "Realistic" finite acceleration
	Accel = ( 1.0f - m_damping ) * l1 / (dT * dT);

	if( m_bLimitForce )
	{
		// max force our arms can exert
		float MaxArmAccel = grabber->m_MaxForce / m_physics->GetMass();
		// if player moves object down, gravity will help
		if( dir1 * m_physics->GetGravityNormal() > 0 )
		{
			idVec3 gravNormal = m_physics->GetGravity();
			float gravMag = gravNormal.Normalize();
			float MaxAccelDown = MaxArmAccel + gravMag;

			// break up desired motion into with gravity and the rest
			float DownAccel = Accel * (dir1 * gravNormal);
			idVec3 vDownAccel = DownAccel * gravNormal;
			idVec3 vOthAccel = Accel * dir1 - vDownAccel;
			float OthAccel = vOthAccel.NormalizeFast();

			OthAccel = idMath::ClampFloat(0.0f, MaxArmAccel, OthAccel );
			DownAccel = idMath::ClampFloat(0.0f, MaxAccelDown, DownAccel );
			// recalculate the vectors now that magnitude is clamped
			vDownAccel = DownAccel * gravNormal;
			vOthAccel = OthAccel * vOthAccel;
			velocity = prevVel * m_damping + (vDownAccel + vOthAccel) * dT;
		}
		else
		{
			// we're nice and don't make the player's arms fight gravity
			Accel = idMath::ClampFloat(0.0f, MaxArmAccel, Accel );
			velocity = prevVel * m_damping + dir1 * Accel * dT;
		}
	}
	else
	{
		// unlimited force
		velocity = prevVel * m_damping + dir1 * Accel * dT;
	}

	if( m_RefEnt.GetEntity() )
	{
		// reference frame velocity
		velocity += m_RefEnt.GetEntity()->GetPhysics()->GetLinearVelocity();

		// TODO: Add velocity due to spin angular momentum of reference entity
		// e.g., player standing on spinning thing
	}
	m_physics->SetLinearVelocity( velocity, m_id );

// ======================== ANGULAR =========================
	idVec3 Alph, DeltAngVec, RotDir, PrevOmega, Omega; // angular acceleration
	float AlphMag, MaxAlph, AlphMod, IAxis, DeltAngLen;
	idMat3 DesiredRot;

	// TODO: Make the following into cvars / spawnargs:
	float ang_damping = 0.0f;
	float max_torque = 100000 * 40;

	// Don't rotate AFs at all for now
	if( m_physics->GetSelf()->IsType( idAFEntity_Base::Type ) )
		return;

	// Find the rotation matrix needed to get from current rotation to desired rotation:
	DesiredRot =  m_dragAxis.Transpose() * m_physics->GetAxis( m_id );

	// DeltAngVec = DeltAng.ToAngularVelocity();
	DeltAngVec = DesiredRot.ToAngularVelocity();
	RotDir = DeltAngVec;
	DeltAngLen = RotDir.Normalize();
	DeltAngLen = DEG2RAD( idMath::AngleNormalize360( RAD2DEG(DeltAngLen) ) );
	DeltAngVec = DeltAngLen * RotDir;

	// DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Force_Grab Eval: Desired angular velocity is %s\r", DeltAngVec.ToString() );
	// test for FP error
	if( (DeltAngLen / dT) <= 0.00001 )
	{
		m_physics->SetAngularVelocity( vec3_zero, m_id );
		m_dragAxis = m_physics->GetAxis( m_id );
		return;
	}

	// Finite angular acceleration:
	Alph = DeltAngVec * (1.0f - ang_damping) / (dT * dT);
	AlphMag = Alph.Length();

	// DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Force_Grab Eval: Desird angular accel this frame is %s\r", Alph.ToString() );

	if( m_bLimitForce )
	{
		// Find the scalar moment of inertia about this axis:
		IAxis = (m_inertiaTensor * RotDir) * RotDir;

		// DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Force_Grab Eval: I about axis is %f\r", IAxis );

		// Calculate max alpha about this axis from max torque
		MaxAlph = max_torque / IAxis;
		AlphMod = idMath::ClampFloat(0.0f, MaxAlph, AlphMag );

		// Finally, adjust our angular acceleration vector
		Alph *= (AlphMod/AlphMag);
		// DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Force_Grab Eval: Modified alpha is %s\r", Alph.ToString() );
	}

	if( grabber->m_bIsColliding )
		PrevOmega = vec3_zero;
	else
		PrevOmega = m_physics->GetAngularVelocity( m_id );

	Omega = PrevOmega * ang_damping + Alph * dT;

	// TODO: Toggle visual debugging with cvar
	// gameRenderWorld->DebugLine( colorGreen, COM, (COM + 30 * RotDir), 1);

	// DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Force_Grab Eval: Setting angular velocity to %s\r", Omega.ToString() );
	m_physics->SetAngularVelocity( Omega, m_id );
}

/*
================
CForce_Grab::RemovePhysics
================
*/
void CForce_Grab::RemovePhysics( const idPhysics *phys ) {
	if ( m_physics == phys ) {
		m_physics = NULL;
	}
}

/*
================
CForce_Grab::GetCenterOfMass
================
*/
idVec3 CForce_Grab::GetCenterOfMass( void ) const 
{
	return ( m_physics->GetOrigin( m_id ) + m_centerOfMass * m_physics->GetAxis( m_id ) );
}


/*
================
CForce_Grab::Rotate
================
*/
void CForce_Grab::Rotate( const idVec3 &vec, float angle ) 
{
	idRotation r;

	r.Set( vec3_origin, vec, angle );
	r.RotatePoint( m_p );
}

void CForce_Grab::ApplyDamping( bool bVal )
{
	m_bApplyDamping = bVal;
}

void CForce_Grab::LimitForce( bool bVal )
{
	m_bLimitForce = bVal;
}

void CForce_Grab::SetRefEnt( idEntity *InputEnt )
{
	m_RefEnt = InputEnt;
}
