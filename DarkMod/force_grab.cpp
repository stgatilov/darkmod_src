/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.3  2006/08/04 10:53:26  ishtvan
 * preliminary grabber fixes
 *
 * Revision 1.2  2006/06/21 13:05:32  sparhawk
 * Added version tracking per cpp module
 *
 * Revision 1.1  2005/12/02 18:21:29  lloyd
 * Initial release
 *
 * Revision 1.1.1.1  2005/09/22 15:52:33  Lloyd
 * Initial release
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Source$  $Revision$   $Date$", init_version);

#include "../Game/Game_local.h"
#include "Force_Grab.h"
#include "../darkmod/playerdata.h"
#include "../darkmod/grabber.h"

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
	m_damping			= 0.5f;
	m_physics			= NULL;
	m_id				= 0;
	m_p					= vec3_zero;
	m_dragPosition		= vec3_zero;
	m_centerOfMass		= vec3_zero;
	m_prevOrigin		= vec3_zero;
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
// Ish: why do we set it to zero?
		m_damping = 0;
	}
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
	float l1;
	idVec3 dragOrigin, dir1, dir2, velocity, COM;
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

// If the object hasn't moved much and the velocity is really high, stop it
// and don't apply more velocity.
/*	idVec3 delta = m_physics->GetOrigin( m_id ) - m_prevOrigin;
	if( ((delta.LengthSqr()) < 1.0f) && (m_physics->GetLinearVelocity( m_id ).LengthSqr() > 100.0f) )
	{
		m_physics->SetLinearVelocity( vec3_zero );
		m_physics->SetAngularVelocity( vec3_zero );
		goto Quit;
	}
*/
/*
	rotation.Set( COM, dir2.Cross( dir1 ), RAD2DEG( idMath::ACos( dir1 * dir2 ) ) );
	m_physics->SetAngularVelocity( rotation.ToAngularVelocity() / MS2SEC( USERCMD_MSEC ), m_id );
*/

	velocity = m_physics->GetLinearVelocity( m_id ) * m_damping + dir1 * ( l1 * ( 1.0f - m_damping ) / MS2SEC( USERCMD_MSEC ) );
	m_physics->SetLinearVelocity( velocity, m_id );
	if( g_Global.m_DarkModPlayer->grabber->m_bIsColliding )
	{
		g_Global.m_DarkModPlayer->grabber->ClampVelocity( 1.0f, 0.0f, m_id );
		g_Global.m_DarkModPlayer->grabber->m_bIsColliding = false;
	}

Quit:
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
void CForce_Grab::Rotate( const idVec3 &vec, float angle ) {
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
