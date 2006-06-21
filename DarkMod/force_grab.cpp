/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
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

CLASS_DECLARATION( idForce, CForce_Grab )
END_CLASS

/*
================
CForce_Grab::CForce_Grab
================
*/
CForce_Grab::CForce_Grab( void ) {
	damping			= 0.5f;
	dragPosition	= vec3_zero;
	physics			= NULL;
	id				= 0;
	p				= vec3_zero;
	dragPosition	= vec3_zero;
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
	if ( damping >= 0.0f && damping < 1.0f ) {
		this->damping = damping;
		this->damping = 0;
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

	this->physics = phys;
	this->id = id;
	this->p = p;

	clipModel = physics->GetClipModel( id );
	if ( clipModel != NULL && clipModel->IsTraceModel() ) {
		clipModel->GetMassProperties( 1.0f, mass, this->centerOfMass, inertiaTensor );
	} else {
		this->centerOfMass.Zero();
	}
}

/*
================
CForce_Grab::SetDragPosition
================
*/
void CForce_Grab::SetDragPosition( const idVec3 &pos ) {
	this->dragPosition = pos;
}

/*
================
CForce_Grab::GetDragPosition
================
*/
const idVec3 &CForce_Grab::GetDragPosition( void ) const {
	return this->dragPosition;
}

/*
================
CForce_Grab::GetDraggedPosition
================
*/
const idVec3 CForce_Grab::GetDraggedPosition( void ) const {
	return ( physics->GetOrigin( id ) + p * physics->GetAxis( id ) );
}

/*
================
CForce_Grab::Evaluate
================
*/
void CForce_Grab::Evaluate( int time ) {
	float l1, l2;
	idVec3 dragOrigin, dir1, dir2, velocity, COM;
	idRotation rotation;

	if ( !physics ) {
		return;
	}

	COM = this->GetCenterOfMass();
	dragOrigin = physics->GetOrigin( id ) + p * physics->GetAxis( id );
	dragOrigin = COM;

	dir1 = dragPosition - COM;
	dir2 = dragOrigin - COM;
	l1 = dir1.Normalize();
	l2 = dir2.Normalize();

	rotation.Set( COM, dir2.Cross( dir1 ), RAD2DEG( idMath::ACos( dir1 * dir2 ) ) );
	physics->SetAngularVelocity( rotation.ToAngularVelocity() / MS2SEC( USERCMD_MSEC ), id );

	velocity = physics->GetLinearVelocity( id ) * damping + dir1 * ( ( l1 - l2 ) * ( 1.0f - damping ) / MS2SEC( USERCMD_MSEC ) );
	physics->SetLinearVelocity( velocity, id );
}

/*
================
CForce_Grab::RemovePhysics
================
*/
void CForce_Grab::RemovePhysics( const idPhysics *phys ) {
	if ( physics == phys ) {
		physics = NULL;
	}
}

/*
================
CForce_Grab::GetCenterOfMass
================
*/
idVec3 CForce_Grab::GetCenterOfMass( void ) const {
	return ( physics->GetOrigin( id ) + this->centerOfMass * physics->GetAxis( id ) );
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

	this->p = temp * physics->GetAxis( this->id ).Transpose() - physics->GetOrigin( this->id );
*/
	r.Set( vec3_origin, vec, angle );
	r.RotatePoint( this->p );

	gameRenderWorld->DebugArrow( colorGreen, this->GetCenterOfMass(), this->GetCenterOfMass() + this->p, 1, 200 );
}
