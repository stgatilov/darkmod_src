/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#include "../../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include "force_push.h"
#include "../game_local.h"

CLASS_DECLARATION( idForce, CForcePush )
END_CLASS

CForcePush::CForcePush() :
	pushEnt(NULL),
	lastPushEnt(NULL),
	id(0),
	startPushTime(-1),
	impactVelocity(vec3_zero),
	owner(NULL)
{}

void CForcePush::Init( float damping ) {
	/*if ( damping >= 0.0f && damping < 1.0f ) {
		this->damping = damping;
	}*/
}

void CForcePush::SetOwner(idEntity* ownerEnt)
{
	owner = ownerEnt;
}

void CForcePush::SetPushEntity(idEntity* pushEnt, int id)
{
	if (pushEnt != lastPushEnt)
	{
		// entity has changed, reset the timer
		startPushTime = gameLocal.time;
	}

	this->pushEnt = pushEnt;
	this->id = id;
}

void CForcePush::SetContactInfo(const trace_t& contactInfo, const idVec3& impactVelocity)
{
	this->contactInfo = contactInfo;
	this->impactVelocity = impactVelocity;
}

/*void CForcePush::SetDragPosition( const idVec3 &pos ) {
	this->dragPosition = pos;
}

const idVec3 &CForcePush::GetDragPosition( void ) const {
	return this->dragPosition;
}

const idVec3 CForcePush::GetDraggedPosition( void ) const {
	return ( physics->GetOrigin( id ) + p * physics->GetAxis( id ) );
}*/

void CForcePush::Evaluate( int time )
{
	if (pushEnt == NULL) return; // nothing to do

	idPhysics* physics = pushEnt->GetPhysics();
	gameRenderWorld->DebugBox(colorRed, idBox(physics->GetBounds(), physics->GetOrigin(), physics->GetAxis()), 16);

	float mass = physics->GetMass();

	if (owner == NULL || owner->GetPhysics()->GetMass() > mass)
	{
		const idVec3& ownerVelocity = impactVelocity;

		idVec3 pushDirection = ownerVelocity;
		pushDirection.NormalizeFast();

		// No owner or mass of the pushed entity is lower than the owner's
		float scale = -contactInfo.c.normal * ownerVelocity;
		idVec3 pushImpulse = pushDirection * scale * owner->GetPhysics()->GetMass() * cv_pm_pushmod.GetFloat();

		gameRenderWorld->DrawText( idStr(pushImpulse.LengthFast()), physics->GetAbsBounds().GetCenter(), 0.1f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, gameLocal.msec );
		gameRenderWorld->DebugArrow( colorWhite, physics->GetAbsBounds().GetCenter(), physics->GetAbsBounds().GetCenter() + pushImpulse, 1, gameLocal.msec );

		physics->PropagateImpulse(id, contactInfo.c.point, pushImpulse);
	}
	else
	{
		// The pushed entity is heavier than the pushing entity

		if (pushEnt == lastPushEnt)
		{
			int pushTime = gameLocal.time - startPushTime;
			gameRenderWorld->DrawText( idStr(pushTime), physics->GetAbsBounds().GetCenter(), 0.1f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, gameLocal.msec );
		}
	
		// TODO
		//gameRenderWorld->DebugArrow( colorWhite, physics->GetAbsBounds().GetCenter(), physics->GetAbsBounds().GetCenter() + pushImpulse, 1, gameLocal.msec );
	}

	// Remember the last push entity
	lastPushEnt = pushEnt;

	// Clear the push entity again
	pushEnt = NULL;

	/*float l1, l2, mass;
	idVec3 dragOrigin, dir1, dir2, velocity, centerOfMass;
	idMat3 inertiaTensor;
	idRotation rotation;
	idClipModel *clipModel;

	if ( !physics ) {
		return;
	}

	clipModel = physics->GetClipModel( id );
	if ( clipModel != NULL && clipModel->IsTraceModel() ) {
		clipModel->GetMassProperties( 1.0f, mass, centerOfMass, inertiaTensor );
	} else {
		centerOfMass.Zero();
	}

	centerOfMass = physics->GetOrigin( id ) + centerOfMass * physics->GetAxis( id );
	dragOrigin = physics->GetOrigin( id ) + p * physics->GetAxis( id );

	dir1 = dragPosition - centerOfMass;
	dir2 = dragOrigin - centerOfMass;
	l1 = dir1.Normalize();
	l2 = dir2.Normalize();

	rotation.Set( centerOfMass, dir2.Cross( dir1 ), RAD2DEG( idMath::ACos( dir1 * dir2 ) ) );
	physics->SetAngularVelocity( rotation.ToAngularVelocity() / MS2SEC( USERCMD_MSEC ), id );

	velocity = physics->GetLinearVelocity( id ) * damping + dir1 * ( ( l1 - l2 ) * ( 1.0f - damping ) / MS2SEC( USERCMD_MSEC ) );
	physics->SetLinearVelocity( velocity, id );*/
}

void CForcePush::Save( idSaveGame *savefile ) const
{
	// Store the entity pointer behind the physics object
	savefile->WriteObject(pushEnt);
	savefile->WriteObject(lastPushEnt);
	savefile->WriteInt(id);
	savefile->WriteTrace(contactInfo);
	savefile->WriteVec3(impactVelocity);
	savefile->WriteInt(startPushTime);
	savefile->WriteObject(owner);
}

void CForcePush::Restore( idRestoreGame *savefile )
{
	savefile->ReadObject(reinterpret_cast<idClass*&>(pushEnt));
	savefile->ReadObject(reinterpret_cast<idClass*&>(lastPushEnt));
	savefile->ReadInt(id);
	savefile->ReadTrace(contactInfo);
	savefile->ReadVec3(impactVelocity);
	savefile->ReadInt(startPushTime);
	savefile->ReadObject(reinterpret_cast<idClass*&>(owner));
}
