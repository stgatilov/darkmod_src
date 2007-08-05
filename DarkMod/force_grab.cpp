/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include "../game/game_local.h"
#include "force_grab.h"
#include "../DarkMod/PlayerData.h"
#include "../DarkMod/Grabber.h"

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
	m_centerOfMass		= vec3_zero;
	m_prevOrigin		= vec3_zero;

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

	// Don't save m_physics, gets restored from the parent class after load
	
	savefile->WriteVec3(m_p);
	savefile->WriteInt(m_id);
	savefile->WriteVec3(m_dragPosition);
	savefile->WriteVec3(m_prevOrigin);
	savefile->WriteBool(m_bApplyDamping);
	savefile->WriteBool(m_bLimitForce);
}

void CForce_Grab::Restore( idRestoreGame *savefile )
{
	m_RefEnt.Restore( savefile );
	savefile->ReadFloat(m_damping);
	savefile->ReadVec3(m_centerOfMass);

	m_physics = NULL; // gets restored from the parent class after loading

	savefile->ReadVec3(m_p);
	savefile->ReadInt(m_id);
	savefile->ReadVec3(m_dragPosition);
	savefile->ReadVec3(m_prevOrigin);
	savefile->ReadBool(m_bApplyDamping);
	savefile->ReadBool(m_bLimitForce);
}

/*
================
CForce_Grab::SetPhysics
================
*/
void CForce_Grab::SetPhysics( idPhysics *phys, int id, const idVec3 &p ) {
	float mass;
	idMat3 inertiaTensor;
	idClipModel *clipModel;

	m_physics = phys;
	m_id = id;
	m_p = p;

	clipModel = m_physics->GetClipModel( m_id );
	if ( clipModel != NULL && clipModel->IsTraceModel() ) {
		clipModel->GetMassProperties( 1.0f, mass, m_centerOfMass, inertiaTensor );
	} else {
		m_centerOfMass.Zero();
	}
}

/*
================
CForce_Grab::SetDragPosition
================
*/
void CForce_Grab::SetDragPosition( const idVec3 &pos ) {
	m_dragPosition = pos;
}

/*
================
CForce_Grab::GetDragPosition
================
*/
const idVec3 &CForce_Grab::GetDragPosition( void ) const {
	return m_dragPosition;
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
	float l1, Accel, MaxAccel, dT;
	idVec3 dragOrigin, dir1, dir2, velocity, COM, prevVel;
	idRotation rotation;

	if ( !m_physics ) 
	{
		return;
	}

	COM = this->GetCenterOfMass();
//	dragOrigin = m_physics->GetOrigin( m_id ) + m_p * m_physics->GetAxis( m_id );
	dragOrigin = COM;

	dir1 = m_dragPosition - dragOrigin;
	l1 = dir1.Normalize();
	dT = MS2SEC( USERCMD_MSEC ); // time elapsed is time between user mouse commands

	if( !m_bApplyDamping )
		m_damping = 0.0f;

	// Test: Realistic finite acceleration
	Accel = ( 1.0f - m_damping ) * l1 / (dT * dT);
	if( m_bLimitForce )
	{
		MaxAccel = g_Global.m_DarkModPlayer->grabber->m_MaxForce / m_physics->GetMass();
		Accel = idMath::ClampFloat(0.0f, MaxAccel, Accel );
	}

	// Test: Zero initial velocity when we start out colliding
	if( g_Global.m_DarkModPlayer->grabber->m_bIsColliding )
		prevVel = vec3_zero;
	else 
		prevVel = m_physics->GetLinearVelocity( m_id );

	velocity = prevVel * m_damping + dir1 * Accel * dT;

	if( m_RefEnt.GetEntity() )
	{
		// reference frame velocity
		velocity += m_RefEnt.GetEntity()->GetPhysics()->GetLinearVelocity();
	}
	m_physics->SetLinearVelocity( velocity, m_id );

	m_prevOrigin = m_physics->GetOrigin( m_id );
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
/*
	idVec3 temp;

	temp = this->GetDraggedPosition();
	r.Set( this->GetCenterOfMass(), vec, angle );
	r.RotatePoint( temp );

	m_p = temp * m_physics->GetAxis( m_id ).Transpose() - m_physics->GetOrigin( m_id );
*/
	r.Set( vec3_origin, vec, angle );
	r.RotatePoint( m_p );

	gameRenderWorld->DebugArrow( colorGreen, this->GetCenterOfMass(), this->GetCenterOfMass() + m_p, 1, 200 );
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
