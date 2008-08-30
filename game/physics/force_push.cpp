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

void CForcePush::Evaluate( int time )
{
	if (pushEnt == NULL || owner == NULL) return; // nothing to do

	idPhysics* physics = pushEnt->GetPhysics();
	//gameRenderWorld->DebugBox(colorRed, idBox(physics->GetBounds(), physics->GetOrigin(), physics->GetAxis()), 16);

	float mass = physics->GetMass();
	float ownerMass = owner->GetPhysics()->GetMass();

	if (ownerMass * cv_pm_push_heavy_threshold.GetFloat() > mass)
	{
		// The pushed entity is not a heavy one, kick it 

		idVec3 pushDirection = impactVelocity;
		pushDirection.NormalizeFast();

		// No owner or mass of the pushed entity is lower than the owner's
		float scale = -contactInfo.c.normal * impactVelocity;
		idVec3 pushImpulse = pushDirection * scale * ownerMass * cv_pm_pushmod.GetFloat();

		//gameRenderWorld->DrawText( idStr(pushImpulse.LengthFast()), physics->GetAbsBounds().GetCenter(), 0.1f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, gameLocal.msec );
		//gameRenderWorld->DebugArrow( colorWhite, physics->GetAbsBounds().GetCenter(), physics->GetAbsBounds().GetCenter() + pushImpulse, 1, gameLocal.msec );

		physics->PropagateImpulse(id, contactInfo.c.point, pushImpulse);
	}
	else
	{
		// The pushed entity is considered heavy
		if (pushEnt == lastPushEnt)
		{
			int pushTime = gameLocal.time - startPushTime;
			//gameRenderWorld->DrawText( idStr(pushTime), physics->GetAbsBounds().GetCenter(), 0.1f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, gameLocal.msec );

			int pushStartDelay = cv_pm_push_start_delay.GetInteger();

			// If we've been pushing long enough, start moving the obstacle
			if (pushTime > pushStartDelay)
			{
				// Scale the velocity during the acceleration phase
				float scale = idMath::ClampFloat(0, 1, (pushTime - pushStartDelay)/cv_pm_push_accel_time.GetFloat());
				pushEnt->GetPhysics()->SetLinearVelocity(impactVelocity * scale);
			}
		}
	
		//gameRenderWorld->DebugArrow( colorWhite, physics->GetAbsBounds().GetCenter(), physics->GetAbsBounds().GetCenter() + pushImpulse, 1, gameLocal.msec );
	}

	// Remember the last push entity
	lastPushEnt = pushEnt;

	// Clear the push entity again
	pushEnt = NULL;
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
