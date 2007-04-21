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

	velocity = m_physics->GetLinearVelocity( m_id ) * m_damping + dir1 * ( l1 * ( 1.0f - m_damping ) / MS2SEC( USERCMD_MSEC ) );
	m_physics->SetLinearVelocity( velocity, m_id );
	if( g_Global.m_DarkModPlayer->grabber->m_bIsColliding )
	{
		g_Global.m_DarkModPlayer->grabber->ClampVelocity( 1.0f, 0.0f, m_id );
	}

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
