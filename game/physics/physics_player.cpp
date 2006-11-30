/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.39  2006/11/30 09:21:20  ishtvan
 * *) leaning: removed the bending of 3rd person model
 * *) added leaning cvars
 *
 * Revision 1.38  2006/08/07 06:54:37  ishtvan
 * got rid of m_NoViewChange and replaced it with Gildoran's immobilization system
 *
 * Revision 1.37  2006/08/04 10:55:08  ishtvan
 * added GetDeltaYaw function to get player view change
 *
 * Revision 1.36  2006/07/09 02:40:13  ishtvan
 * rope arrow removal bugfix
 *
 * Revision 1.35  2006/06/21 13:07:08  sparhawk
 * Added version tracking per cpp module
 *
 * Revision 1.34  2005/12/11 19:53:11  ishtvan
 * disabled mantling when holding objects
 *
 * Revision 1.33  2005/11/21 06:41:06  ishtvan
 * lowered weapons when swimming, mantling
 *
 * Revision 1.32  2005/11/20 19:22:49  ishtvan
 * weapons lowered when on rope arrow
 *
 * Revision 1.31  2005/11/19 17:29:21  sparhawk
 * LogString with macro replaced
 *
 * Revision 1.30  2005/11/18 10:31:44  ishtvan
 * rope arrow fixes
 *
 * Revision 1.29  2005/11/17 09:14:15  ishtvan
 * rope arrow fixes
 * *) attaches to closest AF body of the rope, allowing climbing of draped ropes
 *
 * *) shouldn't attach to ropes that are lying on the ground
 *
 * Revision 1.28  2005/11/12 14:59:51  sparhawk
 * SDK 1.3 Merge
 *
 * Revision 1.27  2005/11/04 07:28:43  ishtvan
 * fixed bugs relating to the combination of mantling and ropes
 *
 * Revision 1.26  2005/10/16 02:18:31  ishtvan
 * *) completed rope orbiting
 *
 * *) added failsafe for rope arrow attachment
 *
 * Revision 1.25  2005/10/14 22:58:16  ishtvan
 * rope movement: crouch to detach, properly detects top and bottom of rope
 *
 * Revision 1.24  2005/10/14 09:04:18  ishtvan
 * updated rope climbing
 *
 * Revision 1.23  2005/10/13 06:54:51  ishtvan
 * fixed RopeMove so that it moves player up and down
 *
 * Revision 1.22  2005/10/12 14:52:52  domarius
 * Rope arrow - initial stage, just sticks you to the rope point of origin... permanently.
 *
 * Revision 1.21  2005/09/24 03:16:34  lloyd
 * Stop player from banging into objects he's picked up
 *
 * Revision 1.20  2005/09/17 07:15:28  sophisticatedzombie
 * Added function that applies damage to the player when mantling at a high relative velocity. The damage amount is computed from minimum and scale constants in DarkModGlobals.
 *
 * Revision 1.19  2005/09/17 00:32:39  lloyd
 * added copyBind event and arrow sticking functionality (additions to Projectile and modifications to idEntity::RemoveBind
 *
 * Revision 1.18  2005/09/14 04:21:07  domarius
 * no message
 *
 * Revision 1.17  2005/09/09 19:56:02  ishtvan
 * removed water jump, allowed mantling out of water
 *
 * Revision 1.16  2005/09/08 04:42:34  sophisticatedzombie
 * Added mantle and lean states to the save/restore methods.
 *
 * Revision 1.15  2005/09/04 20:38:20  sophisticatedzombie
 * The collision/render model leaning of the player model is now accomplished by rotation of the waist joint of the model skeleton.
 *
 * Revision 1.14  2005/08/19 00:28:02  lloyd
 * *** empty log message ***
 *
 * Revision 1.13  2005/08/14 23:29:04  sophisticatedzombie
 * Broke methods into smaller more logical units.
 * Changed header documentation to use doxygen format and doxygen tags.
 * Leaning and Mantling constants moved to DarkModGlobals
 * Fixed infinite loop that could occur in mantling test due to floating point number precision.
 *
 * Revision 1.12  2005/08/04 05:16:43  sophisticatedzombie
 * Lean angle is now 12 degrees rather than 20.
 * Leaning now uses sinusoidal velocity rather than linear velocity.
 * Its still to bumpy, but pushing the player up is the only way to keep them out of the floor that I have come up with yet.
 *
 * Revision 1.11  2005/08/02 00:29:28  sophisticatedzombie
 * I've added a line to CorrectAllSolid that bumps the player against gravity slightly if they are inside a solid object. This fixes the problem with getting stuck in the floor.
 *
 * Revision 1.10  2005/08/01 22:37:09  sophisticatedzombie
 * Added rotation test to the UpdateClipModelOrientation method which detects collisions due to changes in player yaw in between frames.  This can happen when leaning, leading to the clip model penetrating a nearby surface, thereby breaking collision "sidedness" calculations.  In order to get around the issue, if the rotation test detects that the change in player yaw between the last frame and this frame resulted in collision with another collision model, then the player snaps to the upright position.  It prevents the ability to rotate the view through objects.
 *
 * Revision 1.9  2005/07/30 01:31:27  sophisticatedzombie
 * Somewhat improved collision detection. Work needs to be done on handling collisions due to viewpoint rotation while in a leaned position.
 *
 * Revision 1.8  2005/07/27 21:54:35  sophisticatedzombie
 * Made "bumpiness" during lean movement much smaller... scaled the uplift to the number of degrees leaned.
 *
 * Revision 1.7  2005/07/27 20:48:09  sophisticatedzombie
 * Added leaning.  The clip model stuff still needs alot of work.
 *
 * Revision 1.6  2005/07/03 18:37:02  sophisticatedzombie
 * Added a time variable which accumulates the milliseconds that the jump button is held down.  If it gets greater than a constant (JUMP_HOLD_MANTLE_TRIGGER_MILLISECONDS) then a mantle attempt is initiated.  The timer is not reset until jump is released, so you can hold it in while falling and try to catch something.
 *
 * Revision 1.5  2005/07/03 14:03:22  sophisticatedzombie
 * Derp. I, um, forgot when I integrated the change to copy in the part where I had to initialize my new variables in the constructor.  Back to kindergarten for me.
 *
 * Revision 1.4  2005/07/01 21:21:23  sophisticatedzombie
 * This is my check in of the mantling code on July 1, 2005.  I've tested it agains the .3 sdk, but not the .2 one.  Any takers?
 *
 * Revision 1.3  2005/04/23 10:09:11  ishtvan
 * Added fix for slow player movement speeds being unable to overcome the floor friction
 *
 * Revision 1.2  2004/11/28 09:20:37  sparhawk
 * SDK V2 merge
 *
 * Revision 1.1.1.1  2004/10/30 15:52:34  sparhawk
 * Initial release
 *
 ***************************************************************************/

// Copyright (C) 2004 Id Software, Inc.
//

#include "../../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Source$  $Revision$   $Date$", init_version);

#include "../Game_local.h"
#include "../DarkMod/DarkModGlobals.h"
#include "../DarkMod/PlayerData.h"

CLASS_DECLARATION( idPhysics_Actor, idPhysics_Player )
END_CLASS

// movement parameters
const float PM_STOPSPEED		= 100.0f;
const float PM_SWIMSCALE		= 0.5f;
const float PM_ROPESPEED		= 100.0f;
const float PM_LADDERSPEED		= 100.0f;
const float PM_STEPSCALE		= 1.0f;

const float PM_ACCELERATE		= 10.0f;
const float PM_AIRACCELERATE	= 1.0f;
const float PM_WATERACCELERATE	= 4.0f;
const float PM_FLYACCELERATE	= 8.0f;

const float PM_FRICTION			= 6.0f;
const float PM_AIRFRICTION		= 0.0f;
const float PM_WATERFRICTION	= 1.0f;
const float PM_FLYFRICTION		= 3.0f;
const float PM_NOCLIPFRICTION	= 12.0f;

/**
*  Height unit increment for mantle test
* This value should be >= 1.0
* A larger value reduces the number of tests during a mantle
* initiation, but may not find some small mantleable "nooks"
* in a surface.
**/
const float MANTLE_TEST_INCREMENT = 1.0;

/**
* Desired player speed below which ground friction is neglected
*
* This was determined for PM_FRICTION = 6.0 and should change if
*	PM_FRICTION changes from 6.0.
**/
const float PM_NOFRICTION_SPEED = 69.0f;

const float MIN_WALK_NORMAL		= 0.7f;		// can't walk on very steep slopes
const float OVERCLIP			= 1.001f;

// TODO (ishtvan): Move the following to INI file or player def file:

/**
* Defines the spot above the player's origin where they are attached to the rope
**/
const float ROPE_GRABHEIGHT		= 50.0f;

/**
* Distance the player is set back from the rope
**/
const float ROPE_DISTANCE		= 20.0f;

/**
* Time the system waits before reattaching to the same rope after detaching [ms]
**/
const int	ROPE_REATTACHTIME	= 600;

/**
* Angular tolarance for looking at a rope and grabbing it [deg]
**/
const float ROPE_ATTACHANGLE = 45.0f*idMath::PI/180.0f;
/**
* Angular tolerance for when to start rotating around the rope
* (This one doesn't have to be in the def file)
**/
const float ROPE_ROTANG_TOL = 1.0f*idMath::PI/180.0f;

// movementFlags
const int PMF_DUCKED			= 1;		// set when ducking
const int PMF_JUMPED			= 2;		// set when the player jumped this frame
const int PMF_STEPPED_UP		= 4;		// set when the player stepped up this frame
const int PMF_STEPPED_DOWN		= 8;		// set when the player stepped down this frame
const int PMF_JUMP_HELD			= 16;		// set when jump button is held down
const int PMF_TIME_LAND			= 32;		// movementTime is time before rejump
const int PMF_TIME_KNOCKBACK	= 64;		// movementTime is an air-accelerate only time
const int PMF_TIME_WATERJUMP	= 128;		// movementTime is waterjump
const int PMF_ALL_TIMES			= (PMF_TIME_WATERJUMP|PMF_TIME_LAND|PMF_TIME_KNOCKBACK);

int c_pmove = 0;


/*
============
idPhysics_Player::CmdScale

Returns the scale factor to apply to cmd movements
This allows the clients to use axial -127 to 127 values for all directions
without getting a sqrt(2) distortion in speed.
============
*/
float idPhysics_Player::CmdScale( const usercmd_t &cmd ) const {
	int		max;
	float	total;
	float	scale;
	int		forwardmove;
	int		rightmove;
	int		upmove;

	forwardmove = cmd.forwardmove;
	rightmove = cmd.rightmove;

	// since the crouch key doubles as downward movement, ignore downward movement when we're on the ground
	// otherwise crouch speed will be lower than specified
	if ( walking ) {
		upmove = 0;
	} else {
		upmove = cmd.upmove;
	}

	max = abs( forwardmove );
	if ( abs( rightmove ) > max ) {
		max = abs( rightmove );
	}
	if ( abs( upmove ) > max ) {
		max = abs( upmove );
	}

	if ( !max ) {
		return 0.0f;
	}

	total = idMath::Sqrt( (float) forwardmove * forwardmove + rightmove * rightmove + upmove * upmove );
	scale = (float) playerSpeed * max / ( 127.0f * total );

	return scale;
}

/*
==============
idPhysics_Player::Accelerate

Handles user intended acceleration
==============
*/
void idPhysics_Player::Accelerate( const idVec3 &wishdir, const float wishspeed, const float accel ) {
#if 1
	// q2 style
	float addspeed, accelspeed, currentspeed;

	currentspeed = current.velocity * wishdir;
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0) {
		return;
	}
	accelspeed = accel * frametime * wishspeed;
	if (accelspeed > addspeed) {
		accelspeed = addspeed;
	}
	
	current.velocity += accelspeed * wishdir;
#else
	// proper way (avoids strafe jump maxspeed bug), but feels bad
	idVec3		wishVelocity;
	idVec3		pushDir;
	float		pushLen;
	float		canPush;

	wishVelocity = wishdir * wishspeed;
	pushDir = wishVelocity - current.velocity;
	pushLen = pushDir.Normalize();

	canPush = accel * frametime * wishspeed;
	if (canPush > pushLen) {
		canPush = pushLen;
	}

	current.velocity += canPush * pushDir;
#endif
}

/*
==================
idPhysics_Player::SlideMove

Returns true if the velocity was clipped in some way
==================
*/
#define	MAX_CLIP_PLANES	5

bool idPhysics_Player::SlideMove( bool gravity, bool stepUp, bool stepDown, bool push ) {
	int			i, j, k, pushFlags;
	int			bumpcount, numbumps, numplanes;
	float		d, time_left, into, totalMass;
	idVec3		dir, planes[MAX_CLIP_PLANES];
	idVec3		end, stepEnd, primal_velocity, endVelocity, endClipVelocity, clipVelocity;
	trace_t		trace, stepTrace, downTrace;
	bool		nearGround, stepped, pushed;

	numbumps = 4;

	primal_velocity = current.velocity;

	if ( gravity ) {
		endVelocity = current.velocity + gravityVector * frametime;
		current.velocity = ( current.velocity + endVelocity ) * 0.5f;
		primal_velocity = endVelocity;
		if ( groundPlane ) {
			// slide along the ground plane
			current.velocity.ProjectOntoPlane( groundTrace.c.normal, OVERCLIP );
		}
	}
	else {
		endVelocity = current.velocity;
	}

	time_left = frametime;

	// never turn against the ground plane
	if ( groundPlane ) {
		numplanes = 1;
		planes[0] = groundTrace.c.normal;
	} else {
		numplanes = 0;
	}

	// never turn against original velocity
	planes[numplanes] = current.velocity;
	planes[numplanes].Normalize();
	numplanes++;

	for ( bumpcount = 0; bumpcount < numbumps; bumpcount++ ) {

		// calculate position we are trying to move to
		end = current.origin + time_left * current.velocity;

		// see if we can make it there
		gameLocal.clip.Translation( trace, current.origin, end, clipModel, clipModel->GetAxis(), clipMask, self );

		time_left -= time_left * trace.fraction;
		current.origin = trace.endpos;

		// if moved the entire distance
		if ( trace.fraction >= 1.0f ) {
			break;
		}

		stepped = pushed = false;

		// if we are allowed to step up
		if ( stepUp ) {

			nearGround = groundPlane | ladder;

			if ( !nearGround ) {
				// trace down to see if the player is near the ground
				// step checking when near the ground allows the player to move up stairs smoothly while jumping
				stepEnd = current.origin + maxStepHeight * gravityNormal;
				gameLocal.clip.Translation( downTrace, current.origin, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );
				nearGround = ( downTrace.fraction < 1.0f && (downTrace.c.normal * -gravityNormal) > MIN_WALK_NORMAL );
			}

			// may only step up if near the ground or on a ladder
			if ( nearGround ) {

				// step up
				stepEnd = current.origin - maxStepHeight * gravityNormal;
				gameLocal.clip.Translation( downTrace, current.origin, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );

				// trace along velocity
				stepEnd = downTrace.endpos + time_left * current.velocity;
				gameLocal.clip.Translation( stepTrace, downTrace.endpos, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );

				// step down
				stepEnd = stepTrace.endpos + maxStepHeight * gravityNormal;
				gameLocal.clip.Translation( downTrace, stepTrace.endpos, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );

				if ( downTrace.fraction >= 1.0f || (downTrace.c.normal * -gravityNormal) > MIN_WALK_NORMAL ) {

					// if moved the entire distance
					if ( stepTrace.fraction >= 1.0f ) {
						time_left = 0;
						current.stepUp -= ( downTrace.endpos - current.origin ) * gravityNormal;
						current.origin = downTrace.endpos;
						current.movementFlags |= PMF_STEPPED_UP;
						current.velocity *= PM_STEPSCALE;
						break;
					}

					// if the move is further when stepping up
					if ( stepTrace.fraction > trace.fraction ) {
						time_left -= time_left * stepTrace.fraction;
						current.stepUp -= ( downTrace.endpos - current.origin ) * gravityNormal;
						current.origin = downTrace.endpos;
						current.movementFlags |= PMF_STEPPED_UP;
						current.velocity *= PM_STEPSCALE;
						trace = stepTrace;
						stepped = true;
					}
				}
			}
		}

		// if we can push other entities and not blocked by the world
		if ( push && trace.c.entityNum != ENTITYNUM_WORLD ) {

			clipModel->SetPosition( current.origin, clipModel->GetAxis() );

			// clip movement, only push idMoveables, don't push entities the player is standing on
			// apply impact to pushed objects
			pushFlags = PUSHFL_CLIP|PUSHFL_ONLYMOVEABLE|PUSHFL_NOGROUNDENTITIES|PUSHFL_APPLYIMPULSE;

			// clip & push
			totalMass = gameLocal.push.ClipTranslationalPush( trace, self, pushFlags, end, end - current.origin );

			if ( totalMass > 0.0f ) {
				// decrease velocity based on the total mass of the objects being pushed ?
				current.velocity *= 1.0f - idMath::ClampFloat( 0.0f, 1000.0f, totalMass - 20.0f ) * ( 1.0f / 950.0f );
				pushed = true;
			}
	
			current.origin = trace.endpos;
			time_left -= time_left * trace.fraction;

			// if moved the entire distance
			if ( trace.fraction >= 1.0f ) {
				break;
			}
		}

		if ( !stepped ) {
			// let the entity know about the collision
			self->Collide( trace, current.velocity );
		}

		if ( numplanes >= MAX_CLIP_PLANES ) {
			// MrElusive: I think we have some relatively high poly LWO models with a lot of slanted tris
			// where it may hit the max clip planes
			current.velocity = vec3_origin;
			return true;
		}

		//
		// if this is the same plane we hit before, nudge velocity
		// out along it, which fixes some epsilon issues with
		// non-axial planes
		//
		for ( i = 0; i < numplanes; i++ ) {
			if ( ( trace.c.normal * planes[i] ) > 0.999f ) {
				current.velocity += trace.c.normal;
				break;
			}
		}
		if ( i < numplanes ) {
			continue;
		}
		planes[numplanes] = trace.c.normal;
		numplanes++;

		//
		// modify velocity so it parallels all of the clip planes
		//

		// find a plane that it enters
		for ( i = 0; i < numplanes; i++ ) {
			into = current.velocity * planes[i];
			if ( into >= 0.1f ) {
				continue;		// move doesn't interact with the plane
			}

			// slide along the plane
			clipVelocity = current.velocity;
			clipVelocity.ProjectOntoPlane( planes[i], OVERCLIP );

			// slide along the plane
			endClipVelocity = endVelocity;
			endClipVelocity.ProjectOntoPlane( planes[i], OVERCLIP );

			// see if there is a second plane that the new move enters
			for ( j = 0; j < numplanes; j++ ) {
				if ( j == i ) {
					continue;
				}
				if ( ( clipVelocity * planes[j] ) >= 0.1f ) {
					continue;		// move doesn't interact with the plane
				}

				// try clipping the move to the plane
				clipVelocity.ProjectOntoPlane( planes[j], OVERCLIP );
				endClipVelocity.ProjectOntoPlane( planes[j], OVERCLIP );

				// see if it goes back into the first clip plane
				if ( ( clipVelocity * planes[i] ) >= 0 ) {
					continue;
				}

				// slide the original velocity along the crease
				dir = planes[i].Cross( planes[j] );
				dir.Normalize();
				d = dir * current.velocity;
				clipVelocity = d * dir;

				dir = planes[i].Cross( planes[j] );
				dir.Normalize();
				d = dir * endVelocity;
				endClipVelocity = d * dir;

				// see if there is a third plane the the new move enters
				for ( k = 0; k < numplanes; k++ ) {
					if ( k == i || k == j ) {
						continue;
					}
					if ( ( clipVelocity * planes[k] ) >= 0.1f ) {
						continue;		// move doesn't interact with the plane
					}

					// stop dead at a tripple plane interaction
					current.velocity = vec3_origin;
					return true;
				}
			}

			// if we have fixed all interactions, try another move
			current.velocity = clipVelocity;
			endVelocity = endClipVelocity;
			break;
		}
	}

	// step down
	if ( stepDown && groundPlane ) 
	{
		stepEnd = current.origin + gravityNormal * maxStepHeight;
		gameLocal.clip.Translation( downTrace, current.origin, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );
		if ( downTrace.fraction > 1e-4f && downTrace.fraction < 1.0f ) {
			current.stepUp -= ( downTrace.endpos - current.origin ) * gravityNormal;
			current.origin = downTrace.endpos;
			current.movementFlags |= PMF_STEPPED_DOWN;
			current.velocity *= PM_STEPSCALE;

			DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING ("performing step down, velocity now %.4f %.4f %.4f\n",  current.velocity.x, current.velocity.y, current.velocity.z);
		}
	}

	if ( gravity ) {
		current.velocity = endVelocity;
	}

	// come to a dead stop when the velocity orthogonal to the gravity flipped
	clipVelocity = current.velocity - gravityNormal * current.velocity * gravityNormal;
	endClipVelocity = endVelocity - gravityNormal * endVelocity * gravityNormal;
	if ( clipVelocity * endClipVelocity < 0.0f ) {
		current.velocity = gravityNormal * current.velocity * gravityNormal;
	}

	return (bool)( bumpcount == 0 );
}

/*
==================
idPhysics_Player::Friction

Handles both ground friction and water friction
==================
*/
void idPhysics_Player::Friction( void ) {
	idVec3	vel;
	float	speed, newspeed, control;
	float	drop;
	
	vel = current.velocity;
	if ( walking ) {
		// ignore slope movement, remove all velocity in gravity direction
		vel += (vel * gravityNormal) * gravityNormal;
	}

	speed = vel.Length();
	if ( speed < 1.0f ) {
		// remove all movement orthogonal to gravity, allows for sinking underwater
		if ( fabs( current.velocity * gravityNormal ) < 1e-5f ) {
			current.velocity.Zero();
		} else {
			current.velocity = (current.velocity * gravityNormal) * gravityNormal;
		}
		// FIXME: still have z friction underwater?
		return;
	}

	drop = 0;

	// spectator friction
	if ( current.movementType == PM_SPECTATOR ) {
		drop += speed * PM_FLYFRICTION * frametime;
	}
	// apply ground friction
	else if ( walking && waterLevel <= WATERLEVEL_FEET ) {
		// no friction on slick surfaces
		if ( !(groundMaterial && groundMaterial->GetSurfaceFlags() & SURF_SLICK) ) {
			// if getting knocked back, no friction
			if ( !(current.movementFlags & PMF_TIME_KNOCKBACK) ) 
			{
				control = speed < PM_STOPSPEED ? PM_STOPSPEED : speed;
				drop += control * PM_FRICTION * frametime;
			}
		}
	}
	// apply water friction even if just wading
	else if ( waterLevel ) {
		drop += speed * PM_WATERFRICTION * waterLevel * frametime;
	}
	// apply air friction
	else {
		drop += speed * PM_AIRFRICTION * frametime;
	}

	// scale the velocity
	newspeed = speed - drop;
	if (newspeed < 0) {
		newspeed = 0;
	}
	current.velocity *= ( newspeed / speed );
}

/*
===================
idPhysics_Player::WaterJumpMove

Flying out of the water

REMOVED from DarkMod
===================
*/


/*
===================
idPhysics_Player::WaterMove
===================
*/
void idPhysics_Player::WaterMove( void ) {
	idVec3	wishvel;
	float	wishspeed;
	idVec3	wishdir;
	float	scale;
	float	vel;


	// Keep track of whether jump is held down for mantling out of water
	if ( command.upmove > 10 ) 
	{
		current.movementFlags |= PMF_JUMP_HELD;
	}
	else	
		current.movementFlags &= ~PMF_JUMP_HELD;

	// Lower weapons while swimming
// TODO : In future, only disable some weapons, keep the sword for underwater bashing?
	if( !static_cast<idPlayer *>(self)->hiddenWeapon )
	{
		static_cast<idPlayer *>(self)->LowerWeapon();
		static_cast<idPlayer *>(self)->hiddenWeapon = true;
	}

	idPhysics_Player::Friction();

	scale = idPhysics_Player::CmdScale( command );

	// user intentions
	if ( !scale ) {
		wishvel = gravityNormal * 60; // sink towards bottom
	} else {
		wishvel = scale * (viewForward * command.forwardmove + viewRight * command.rightmove);
		wishvel -= scale * gravityNormal * command.upmove;
	}

	wishdir = wishvel;
	wishspeed = wishdir.Normalize();

	if ( wishspeed > playerSpeed * PM_SWIMSCALE ) {
		wishspeed = playerSpeed * PM_SWIMSCALE;
	}

	idPhysics_Player::Accelerate( wishdir, wishspeed, PM_WATERACCELERATE );

	// make sure we can go up slopes easily under water
	if ( groundPlane && ( current.velocity * groundTrace.c.normal ) < 0.0f ) {
		vel = current.velocity.Length();
		// slide along the ground plane
		current.velocity.ProjectOntoPlane( groundTrace.c.normal, OVERCLIP );

		current.velocity.Normalize();
		current.velocity *= vel;
	}

	idPhysics_Player::SlideMove( false, true, false, false );
}

/*
===================
idPhysics_Player::FlyMove
===================
*/
void idPhysics_Player::FlyMove( void ) {
	idVec3	wishvel;
	float	wishspeed;
	idVec3	wishdir;
	float	scale;

	// normal slowdown
	idPhysics_Player::Friction();

	scale = idPhysics_Player::CmdScale( command );

	if ( !scale ) {
		wishvel = vec3_origin;
	} else {
		wishvel = scale * (viewForward * command.forwardmove + viewRight * command.rightmove);
		wishvel -= scale * gravityNormal * command.upmove;
	}

	wishdir = wishvel;
	wishspeed = wishdir.Normalize();

	idPhysics_Player::Accelerate( wishdir, wishspeed, PM_FLYACCELERATE );

	idPhysics_Player::SlideMove( false, false, false, false );
}

/*
===================
idPhysics_Player::AirMove
===================
*/
void idPhysics_Player::AirMove( void ) {
	idVec3		wishvel;
	idVec3		wishdir;
	float		wishspeed;
	float		scale;

	idPhysics_Player::Friction();

	scale = idPhysics_Player::CmdScale( command );

	// project moves down to flat plane
	viewForward -= (viewForward * gravityNormal) * gravityNormal;
	viewRight -= (viewRight * gravityNormal) * gravityNormal;
	viewForward.Normalize();
	viewRight.Normalize();

	wishvel = viewForward * command.forwardmove + viewRight * command.rightmove;
	wishvel -= (wishvel * gravityNormal) * gravityNormal;
	wishdir = wishvel;
	wishspeed = wishdir.Normalize();
	wishspeed *= scale;

	// not on ground, so little effect on velocity
	idPhysics_Player::Accelerate( wishdir, wishspeed, PM_AIRACCELERATE );

	// we may have a ground plane that is very steep, even
	// though we don't have a groundentity
	// slide along the steep plane
	if ( groundPlane ) {
		current.velocity.ProjectOntoPlane( groundTrace.c.normal, OVERCLIP );
	}

	idPhysics_Player::SlideMove( true, false, false, false );
}

/*
===================
idPhysics_Player::WalkMove
===================
*/
void idPhysics_Player::WalkMove( void ) {
	idVec3		wishvel;
	idVec3		wishdir;
	float		wishspeed;
	float		scale;
	float		accelerate;
	idVec3		oldVelocity, vel;
	float		oldVel, newVel;

	if ( waterLevel > WATERLEVEL_WAIST && ( viewForward * groundTrace.c.normal ) > 0.0f ) {
		// begin swimming

		idPhysics_Player::WaterMove();
		return;
	}

	if ( idPhysics_Player::CheckJump() ) {
		// jumped away
		if ( waterLevel > WATERLEVEL_FEET ) {
			idPhysics_Player::WaterMove();
		}
		else {
			idPhysics_Player::AirMove();
		}
		return;
	}

	idPhysics_Player::Friction();

	scale = idPhysics_Player::CmdScale( command );

	// project moves down to flat plane
	viewForward -= (viewForward * gravityNormal) * gravityNormal;
	viewRight -= (viewRight * gravityNormal) * gravityNormal;

	// project the forward and right directions onto the ground plane
	viewForward.ProjectOntoPlane( groundTrace.c.normal, OVERCLIP );
	viewRight.ProjectOntoPlane( groundTrace.c.normal, OVERCLIP );
	//
	viewForward.Normalize();
	viewRight.Normalize();

	wishvel = viewForward * command.forwardmove + viewRight * command.rightmove;
	wishdir = wishvel;
	wishspeed = wishdir.Normalize();
	wishspeed *= scale;

	// clamp the speed lower if wading or walking on the bottom
	if ( waterLevel ) {
		float	waterScale;

		waterScale = waterLevel / 3.0f;
		waterScale = 1.0f - ( 1.0f - PM_SWIMSCALE ) * waterScale;
		if ( wishspeed > playerSpeed * waterScale ) {
			wishspeed = playerSpeed * waterScale;
		}
	}

	// when a player gets hit, they temporarily lose full control, which allows them to be moved a bit
	if ( ( groundMaterial && groundMaterial->GetSurfaceFlags() & SURF_SLICK ) || current.movementFlags & PMF_TIME_KNOCKBACK ) {
		accelerate = PM_AIRACCELERATE;
	}
	else 
	{
		accelerate = PM_ACCELERATE;
		
	//FIX: If the player is moving very slowly, bump up their acceleration
	// so they don't get stuck to the floor by friction.
		if( playerSpeed < PM_NOFRICTION_SPEED )
			accelerate *= 3.0f;
	}

	idPhysics_Player::Accelerate( wishdir, wishspeed, accelerate );

	if ( ( groundMaterial && groundMaterial->GetSurfaceFlags() & SURF_SLICK ) || current.movementFlags & PMF_TIME_KNOCKBACK ) {
		current.velocity += gravityVector * frametime;
	}

	oldVelocity = current.velocity;

	// slide along the ground plane
	current.velocity.ProjectOntoPlane( groundTrace.c.normal, OVERCLIP );


	// if not clipped into the opposite direction
	if ( oldVelocity * current.velocity > 0.0f ) {
		newVel = current.velocity.LengthSqr();
		if ( newVel > 1.0f ) {
			oldVel = oldVelocity.LengthSqr();
			if ( oldVel > 1.0f ) {
				// don't decrease velocity when going up or down a slope
				current.velocity *= idMath::Sqrt( oldVel / newVel );
			}
		}
	}

	// don't do anything if standing still
	vel = current.velocity - (current.velocity * gravityNormal) * gravityNormal;
	if ( !vel.LengthSqr() ) {
		return;
	}

	gameLocal.push.InitSavingPushedEntityPositions();

	idPhysics_Player::SlideMove( false, true, true, true );


}

/*
==============
idPhysics_Player::DeadMove
==============
*/
void idPhysics_Player::DeadMove( void ) {
	float	forward;

	if ( !walking ) {
		return;
	}

	// extra friction
	forward = current.velocity.Length();
	forward -= 20;
	if ( forward <= 0 ) {
		current.velocity = vec3_origin;
	}
	else {
		current.velocity.Normalize();
		current.velocity *= forward;
	}
}

/*
===============
idPhysics_Player::NoclipMove
===============
*/
void idPhysics_Player::NoclipMove( void ) {
	float		speed, drop, friction, newspeed, stopspeed;
	float		scale, wishspeed;
	idVec3		wishdir;

	// friction
	speed = current.velocity.Length();
	if ( speed < 20.0f ) {
		current.velocity = vec3_origin;
	}
	else {
		stopspeed = playerSpeed * 0.3f;
		if ( speed < stopspeed ) {
			speed = stopspeed;
		}
		friction = PM_NOCLIPFRICTION;
		drop = speed * friction * frametime;

		// scale the velocity
		newspeed = speed - drop;
		if (newspeed < 0) {
			newspeed = 0;
		}

		current.velocity *= newspeed / speed;
	}

	// accelerate
	scale = idPhysics_Player::CmdScale( command );

	wishdir = scale * (viewForward * command.forwardmove + viewRight * command.rightmove);
	wishdir -= scale * gravityNormal * command.upmove;
	wishspeed = wishdir.Normalize();
	wishspeed *= scale;

	idPhysics_Player::Accelerate( wishdir, wishspeed, PM_ACCELERATE );

	// move
	current.origin += frametime * current.velocity;
}

/*
===============
idPhysics_Player::SpectatorMove
===============
*/
void idPhysics_Player::SpectatorMove( void ) {
	idVec3	wishvel;
	float	wishspeed;
	idVec3	wishdir;
	float	scale;

	trace_t	trace;
	idVec3	end;

	// fly movement

	idPhysics_Player::Friction();

	scale = idPhysics_Player::CmdScale( command );

	if ( !scale ) {
		wishvel = vec3_origin;
	} else {
		wishvel = scale * (viewForward * command.forwardmove + viewRight * command.rightmove);
	}

	wishdir = wishvel;
	wishspeed = wishdir.Normalize();

	idPhysics_Player::Accelerate( wishdir, wishspeed, PM_FLYACCELERATE );

	idPhysics_Player::SlideMove( false, false, false, false );
}

/*
============
idPhysics_Player::RopeMove
============
*/
void idPhysics_Player::RopeMove( void ) 
{
	idVec3	wishdir, wishvel, right, ropePoint, offset, newOrigin;
	float	wishspeed, scale, temp, deltaYaw, deltaAng1, deltaAng2;
	float	upscale, ropeTop, ropeBot; // z coordinates of the top and bottom of rope
	idBounds ropeBounds;
	trace_t transTrace; // used for clipping tests when moving the player
	idVec3 transVec, forward, zeros(0,0,0), playerVel(0,0,0), PlayerPoint(0,0,0);
	int bodID(0);

	if( !m_RopeEntity )
	{
		RopeDetach();
		goto Quit;
	}

	// store and kill the player's transverse velocity
	playerVel = current.velocity;
	current.velocity.x = 0;
	current.velocity.y = 0;

	// stick the player to the rope at an AF origin point closest to their arms
	PlayerPoint = current.origin + -gravityNormal*ROPE_GRABHEIGHT;
	ropePoint = static_cast<idPhysics_AF *>(m_RopeEntity->GetPhysics())->NearestBodyOrig( PlayerPoint, &bodID );
	
	// apply the player's weight to the AF body - COMMENTED OUT DUE TO AF CRAZINESS
//	static_cast<idPhysics_AF *>(m_RopeEntity->GetPhysics())->AddForce(bodID, ropePoint, mass * gravityVector );

	// if the player has hit the rope this frame, apply an impulse based on their velocity
	// pretend the deceleration takes place over a number of frames for realism (100 ms?)
	if( m_bJustHitRope )
	{
		m_bJustHitRope = false;

		idVec3 vImpulse(playerVel.x, playerVel.y, 0);
		vImpulse *= mass;

		static_cast<idPhysics_AF *>(m_RopeEntity->GetPhysics())->AddForce( bodID, ropePoint, vImpulse/0.1f );
	}

	offset = (current.origin - ropePoint);
	offset.ProjectOntoPlane( -gravityNormal );
	offset.Normalize();
	offset *= ROPE_DISTANCE;

	newOrigin = ropePoint + offset;
	newOrigin.z = current.origin.z;
	transVec = newOrigin - current.origin;

	// check whether the player will clip anything, and only translate up to that point
	ClipTranslation(transTrace, transVec, NULL);
	newOrigin = current.origin + (transVec * transTrace.fraction); 
	Translate( newOrigin - current.origin );


	// Find the top and bottom of the rope
	// This must be done every frame since the rope may be deforming
	ropeBounds = m_RopeEntity->GetPhysics()->GetAbsBounds();
	ropeTop = ropeBounds[0].z;
	ropeBot = ropeBounds[1].z;

	if( ropeTop < ropeBot )
	{
		// switch 'em
		temp = ropeTop;
		ropeTop = ropeBot;
		ropeBot = temp;
	}

	// ============== read mouse input and orbit around the rope ===============

	// recalculate offset because the player may have moved to stick point
	offset = (ropePoint - current.origin);
	offset.ProjectOntoPlane( -gravityNormal );
	offset.Normalize();

	// forward vector orthogonal to gravity
	forward = viewForward - (gravityNormal * viewForward) * gravityNormal;
	forward.Normalize();

	deltaAng1 = offset * forward;
	deltaYaw = m_DeltaViewYaw;

	// use a different tolerance for rotating toward the rope vs away
	// rotate forward by deltaAng to see if we are rotating towards or away from the rope
	idRotation rotateView( zeros, -gravityNormal, -deltaYaw );
	rotateView.RotatePoint( forward );

	deltaAng2 = offset * forward;

	
	// only rotate around the rope if looking at the rope to within some angular tolerance
	// always rotate if shifting view away
	if( deltaAng1 >= idMath::Cos( ROPE_ROTANG_TOL ) 
		|| ( (deltaAng2 < deltaAng1) && deltaAng1 > 0 ) )
	{

		newOrigin = current.origin;

		// define the counter-rotation around the rope point using gravity axis
		idRotation rotatePlayer( ropePoint, -gravityNormal, -deltaYaw );
		rotatePlayer.RotatePoint( newOrigin );

		// check whether the player will clip anything when orbiting
		transVec = newOrigin - current.origin;
		ClipTranslation(transTrace, transVec, NULL);
		newOrigin = current.origin + (transVec * transTrace.fraction); 

		Translate( newOrigin - current.origin );
	}
	

	// ================ read control input for climbing movement ===============

	upscale = (-gravityNormal * viewForward + 0.5f) * 2.5f;
	if ( upscale > 1.0f ) 
	{
		upscale = 1.0f;
	}
	else if ( upscale < -1.0f ) 
	{
		upscale = -1.0f;
	}

	scale = idPhysics_Player::CmdScale( command );
	wishvel = -0.9f * gravityNormal * upscale * scale * (float)command.forwardmove;
   	// up down movement
	if ( command.upmove ) 
	{
		wishvel += -0.5f * gravityNormal * scale * (float) command.upmove;
	}

	// detach the player from the rope if they jump, or if they hit crouch
	if ( idPhysics_Player::CheckRopeJump() || command.upmove < 0 ) 
	{
		RopeDetach();
		goto Quit;
	}

	// if the player is above the top of the rope, don't climb up
	// if the player is at the bottom of the rope, don't climb down
	// subtract some amount to represent hanging on with arms above head
	if  ( 
			(
				wishvel * gravityNormal <= 0.0f 
				&& ((current.origin.z + ROPE_GRABHEIGHT ) > ropeTop)
			)
			||
			(
				wishvel * gravityNormal >= 0.0f
				&& ((current.origin.z + ROPE_GRABHEIGHT + 35.0f) < ropeBot)
			)
		)
	{
		current.velocity.z = 0;
		goto Quit;
	}

	// accelerate
	wishspeed = wishvel.Normalize();
	idPhysics_Player::Accelerate( wishvel, wishspeed, PM_ACCELERATE );

	// cap the vertical velocity
	upscale = current.velocity * -gravityNormal;
	if ( upscale < -PM_ROPESPEED ) 
	{
		current.velocity += gravityNormal * (upscale + PM_ROPESPEED);
	}
	else if ( upscale > PM_ROPESPEED ) 
	{
		current.velocity += gravityNormal * (upscale - PM_ROPESPEED);
	}

	// stop the player from sliding down or up when they let go of the button
	if ( (wishvel * gravityNormal) == 0.0f ) 
	{
		if ( current.velocity * gravityNormal < 0.0f ) 
		{
			current.velocity += gravityVector * frametime;
			if ( current.velocity * gravityNormal > 0.0f ) 
			{
				current.velocity -= (gravityNormal * current.velocity) * gravityNormal;
			}
		}
		else 
		{
			current.velocity -= gravityVector * frametime;
			if ( current.velocity * gravityNormal < 0.0f ) 
			{
				current.velocity -= (gravityNormal * current.velocity) * gravityNormal;
			}
		}
	}

	// If the player is climbing down and hits the ground, detach them from the rope
	if ( (wishvel * gravityNormal) > 0.0f && groundPlane )
	{
		RopeDetach();
		goto Quit;
	}

	// slide the player up and down with their calculated velocity
	idPhysics_Player::SlideMove( false, ( command.forwardmove > 0 ), false, false );

Quit:
	return;
}

/*
============
idPhysics_Player::RopeDetach
============
*/
void idPhysics_Player::RopeDetach( void ) 
{
		m_bRopeAttached = false;

		// start the reattach timer
		m_RopeDetachTimer = gameLocal.time;

		static_cast<idPlayer *>(self)->RaiseWeapon();
		static_cast<idPlayer *>(self)->hiddenWeapon = false;

		// switch movement modes to the appropriate one
		if ( waterLevel > WATERLEVEL_FEET ) 
		{
			idPhysics_Player::WaterMove();
		}
		else 
		{
			idPhysics_Player::AirMove();
		}
}

/*
============
idPhysics_Player::LadderMove
============
*/
void idPhysics_Player::LadderMove( void ) {
	idVec3	wishdir, wishvel, right;
	float	wishspeed, scale;
	float	upscale;

	// stick to the ladder
	wishvel = -100.0f * ladderNormal;
	current.velocity = (gravityNormal * current.velocity) * gravityNormal + wishvel;

	upscale = (-gravityNormal * viewForward + 0.5f) * 2.5f;
	if ( upscale > 1.0f ) {
		upscale = 1.0f;
	}
	else if ( upscale < -1.0f ) {
		upscale = -1.0f;
	}

	scale = idPhysics_Player::CmdScale( command );
	wishvel = -0.9f * gravityNormal * upscale * scale * (float)command.forwardmove;

	// strafe
	if ( command.rightmove ) {
		// right vector orthogonal to gravity
		right = viewRight - (gravityNormal * viewRight) * gravityNormal;
		// project right vector into ladder plane
		right = right - (ladderNormal * right) * ladderNormal;
		right.Normalize();

		// if we are looking away from the ladder, reverse the right vector
		if ( ladderNormal * viewForward > 0.0f ) {
			right = -right;
		}
		wishvel += 2.0f * right * scale * (float) command.rightmove;
	}

	// up down movement
	if ( command.upmove ) {
		wishvel += -0.5f * gravityNormal * scale * (float) command.upmove;
	}

	// do strafe friction
	idPhysics_Player::Friction();

	// accelerate
	wishspeed = wishvel.Normalize();
	idPhysics_Player::Accelerate( wishvel, wishspeed, PM_ACCELERATE );

	// cap the vertical velocity
	upscale = current.velocity * -gravityNormal;
	if ( upscale < -PM_LADDERSPEED ) {
		current.velocity += gravityNormal * (upscale + PM_LADDERSPEED);
	}
	else if ( upscale > PM_LADDERSPEED ) {
		current.velocity += gravityNormal * (upscale - PM_LADDERSPEED);
	}

	if ( (wishvel * gravityNormal) == 0.0f ) {
		if ( current.velocity * gravityNormal < 0.0f ) {
			current.velocity += gravityVector * frametime;
			if ( current.velocity * gravityNormal > 0.0f ) {
				current.velocity -= (gravityNormal * current.velocity) * gravityNormal;
			}
		}
		else {
			current.velocity -= gravityVector * frametime;
			if ( current.velocity * gravityNormal < 0.0f ) {
				current.velocity -= (gravityNormal * current.velocity) * gravityNormal;
			}
		}
	}

	idPhysics_Player::SlideMove( false, ( command.forwardmove > 0 ), false, false );
}

/*
=============
idPhysics_Player::CorrectAllSolid
=============
*/
void idPhysics_Player::CorrectAllSolid( trace_t &trace, int contents ) {
	if ( debugLevel ) {
		
		gameLocal.Printf( "%i:allsolid\n", c_pmove );
	}

	// SophisticatedZombie
	//DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING ("performing CorrectAllSolid due to player inside solid object\n");
	//
	// Don't bump player up if they're standing in a previously picked up objects.
	// This is complicated but because we want free object movement, we have to temporarily disable player clipping.
	// But, if a players releases an object when they're inside it they float to the surface.  By doing this check
	// we can avoid that.
	if( !g_Global.m_DarkModPlayer->grabber->HasClippedEntity() ) {
		current.origin -= (GetGravityNormal() * 0.2f);
	}


	// FIXME: jitter around to find a free spot ?

	if ( trace.fraction >= 1.0f ) {
		memset( &trace, 0, sizeof( trace ) );
		trace.endpos = current.origin;
		trace.endAxis = clipModelAxis;
		trace.fraction = 0.0f;
		trace.c.dist = current.origin.z;
		trace.c.normal.Set( 0, 0, 1 );
		trace.c.point = current.origin;
		trace.c.entityNum = ENTITYNUM_WORLD;
		trace.c.id = 0;
		trace.c.type = CONTACT_TRMVERTEX;
		trace.c.material = NULL;
		trace.c.contents = contents;
	}
}

/*
=============
idPhysics_Player::CheckGround
=============
*/
void idPhysics_Player::CheckGround( void ) {
	int i, contents;
	idVec3 point;
	bool hadGroundContacts;

	hadGroundContacts = HasGroundContacts();

	// set the clip model origin before getting the contacts
	clipModel->SetPosition( current.origin, clipModel->GetAxis() );

	EvaluateContacts();

	// setup a ground trace from the contacts
	groundTrace.endpos = current.origin;
	groundTrace.endAxis = clipModel->GetAxis();
	if ( contacts.Num() ) {
		groundTrace.fraction = 0.0f;
		groundTrace.c = contacts[0];
		for ( i = 1; i < contacts.Num(); i++ ) {
			groundTrace.c.normal += contacts[i].normal;
		}
		groundTrace.c.normal.Normalize();
	} else {
		groundTrace.fraction = 1.0f;
	}

	contents = gameLocal.clip.Contents( current.origin, clipModel, clipModel->GetAxis(), -1, self );
	if ( contents & MASK_SOLID ) {
		// do something corrective if stuck in solid
		idPhysics_Player::CorrectAllSolid( groundTrace, contents );
	}

	// if the trace didn't hit anything, we are in free fall
	if ( groundTrace.fraction == 1.0f ) 
	{

		groundPlane = false;
		walking = false;
		groundEntityPtr = NULL;
		return;
	}

	groundMaterial = groundTrace.c.material;
	groundEntityPtr = gameLocal.entities[ groundTrace.c.entityNum ];

	// check if getting thrown off the ground
	if ( (current.velocity * -gravityNormal) > 0.0f && ( current.velocity * groundTrace.c.normal ) > 10.0f ) {
		if ( debugLevel ) {
			gameLocal.Printf( "%i:kickoff\n", c_pmove );
		}

		groundPlane = false;
		walking = false;
		return;
	}
	
	// slopes that are too steep will not be considered onground
	if ( ( groundTrace.c.normal * -gravityNormal ) < MIN_WALK_NORMAL ) {
		if ( debugLevel ) {
			gameLocal.Printf( "%i:steep\n", c_pmove );
		}

		// FIXME: if they can't slide down the slope, let them walk (sharp crevices)

		// make sure we don't die from sliding down a steep slope
		if ( current.velocity * gravityNormal > 150.0f ) {
			current.velocity -= ( current.velocity * gravityNormal - 150.0f ) * gravityNormal;
		}

		groundPlane = true;
		walking = false;
		return;
	}

	groundPlane = true;
	walking = true;

	// hitting solid ground will end a waterjump
	if ( current.movementFlags & PMF_TIME_WATERJUMP ) {
		current.movementFlags &= ~( PMF_TIME_WATERJUMP | PMF_TIME_LAND );
		current.movementTime = 0;
	}

	// if the player didn't have ground contacts the previous frame
	if ( !hadGroundContacts ) {

		// don't do landing time if we were just going down a slope
		if ( (current.velocity * -gravityNormal) < -200.0f ) {
			// don't allow another jump for a little while
			current.movementFlags |= PMF_TIME_LAND;
			current.movementTime = 250;
		}
	}

	// let the entity know about the collision
	self->Collide( groundTrace, current.velocity );

	if ( groundEntityPtr.GetEntity() ) {
		impactInfo_t info;
		groundEntityPtr.GetEntity()->GetImpactInfo( self, groundTrace.c.id, groundTrace.c.point, &info );
		if ( info.invMass != 0.0f ) {
			groundEntityPtr.GetEntity()->ApplyImpulse( self, groundTrace.c.id, groundTrace.c.point, current.velocity / ( info.invMass * 10.0f ) );
		}
	}
}

/*
==============
idPhysics_Player::CheckDuck

Sets clip model size
==============
*/
void idPhysics_Player::CheckDuck( void ) {
	trace_t	trace;
	idVec3 end;
	idBounds bounds;
	float maxZ;

	if ( current.movementType == PM_DEAD ) {
		maxZ = pm_deadheight.GetFloat();
	} else {
		// stand up when up against a ladder or rope
		if ( command.upmove < 0 && !ladder && !m_bRopeAttached ) {
			// duck
			current.movementFlags |= PMF_DUCKED;
		}
		else if (!IsMantling()) // MantleMod: SophisticatedZombie (DH): Don't stand up if crouch during mantle
		{
			// stand up if possible
			if ( current.movementFlags & PMF_DUCKED ) {
				// try to stand up
				end = current.origin - ( pm_normalheight.GetFloat() - pm_crouchheight.GetFloat() ) * gravityNormal;
				gameLocal.clip.Translation( trace, current.origin, end, clipModel, clipModel->GetAxis(), clipMask, self );
				if ( trace.fraction >= 1.0f ) {
					current.movementFlags &= ~PMF_DUCKED;
				}
			}
		}

		if ( current.movementFlags & PMF_DUCKED ) {
			playerSpeed = crouchSpeed;
			maxZ = pm_crouchheight.GetFloat();
		} else {
			maxZ = pm_normalheight.GetFloat();
		}
	}
	// if the clipModel height should change
	if ( clipModel->GetBounds()[1][2] != maxZ ) {

		bounds = clipModel->GetBounds();
		bounds[1][2] = maxZ;
		if ( pm_usecylinder.GetBool() ) {
			clipModel->LoadModel( idTraceModel( bounds, 8 ) );
		} else {
			clipModel->LoadModel( idTraceModel( bounds ) );
		}
	}
}

/*
================
idPhysics_Player::CheckLadder
DarkMod: Also checks ropes
================
*/
void idPhysics_Player::CheckLadder( void ) 
{
	idVec3		forward, start, end, delta;
	trace_t		trace;
	float		tracedist, angleOff, dist, lookUpAng;
	bool		bLookingUp;
	idEntity    *testEnt;
	
	if ( current.movementTime ) 
		goto Quit;

	// if on the ground moving backwards
	if ( walking && command.forwardmove <= 0 ) 
		goto Quit;

	// Don't attach to ropes or ladders in the middle of a mantle
	if ( IsMantling() )
		goto Quit;

	// forward vector orthogonal to gravity
	forward = viewForward - (gravityNormal * viewForward) * gravityNormal;
	forward.Normalize();

	if ( walking ) 
	{
		// don't want to get sucked towards the ladder when still walking
		tracedist = 1.0f;
	} else 
	{
		tracedist = 48.0f;
	}

	end = current.origin + tracedist * forward;
	// modified to check contents_corpse to check for ropes
	gameLocal.clip.Translation( trace, current.origin, end, clipModel, clipModel->GetAxis(), clipMask | CONTENTS_CORPSE, self );

	// if near a surface
	if ( trace.fraction < 1.0f ) 
	{
		testEnt = gameLocal.entities[trace.c.entityNum];
		
// DarkMod: Check if we're looking at a rope and airborne
// TODO: Check the class type instead of the stringname, make new rope class
		if( testEnt && idStr::Cmp( testEnt->GetEntityDefName(), "env_rope" ) == 0 )
		{
			m_RopeEntTouched = static_cast<idAFEntity_Base *>(testEnt);

			delta = (trace.c.point - current.origin);
			delta = delta - (gravityNormal * delta) * gravityNormal;
			dist = delta.LengthFast();

			delta.Normalize();
			angleOff = delta * forward;

			// must be in the air to attach to the rope
			// this is kind've a hack, but the rope has a different attach distance than the ladder
			if( 
				!m_bRopeAttached
				&& ( (trace.endpos - current.origin).Length() <= 2.0f )
				&& !groundPlane
				&& angleOff >= idMath::Cos( ROPE_ATTACHANGLE )
				&& (testEnt != m_RopeEntity || gameLocal.time - m_RopeDetachTimer > ROPE_REATTACHTIME)
				)
			{
				// make sure rope segment is not touching the ground
				int bodyID = m_RopeEntTouched->BodyForClipModelId( trace.c.id );
				if( !static_cast<idPhysics_AF *>(m_RopeEntTouched->GetPhysics())->HasGroundContacts( bodyID ) )
				{
					m_bRopeContact = true;
					m_bJustHitRope = true;
					m_RopeEntity = static_cast<idAFEntity_Base *>(testEnt);

					goto Quit;
				}
			}
		}

		// if a ladder surface
		if ( trace.c.material && ( trace.c.material->GetSurfaceFlags() & SURF_LADDER ) ) 
		{
			// check a step height higher
			end = current.origin - gravityNormal * ( maxStepHeight * 0.75f );
			gameLocal.clip.Translation( trace, current.origin, end, clipModel, clipModel->GetAxis(), clipMask, self );
			start = trace.endpos;
			end = start + tracedist * forward;
			gameLocal.clip.Translation( trace, start, end, clipModel, clipModel->GetAxis(), clipMask, self );

			// if also near a surface a step height higher
			if ( trace.fraction < 1.0f ) 
			{
				// if it also is a ladder surface
				if ( trace.c.material && trace.c.material->GetSurfaceFlags() & SURF_LADDER ) 
				{
					ladder = true;
					ladderNormal = trace.c.normal;
					goto Quit;
				}
			}
		}
	}

	// Rope attachment failsafe: Check intersection with the rope as well
	if 
		( 
			!m_bRopeAttached 
			&& m_RopeEntTouched
			&& m_RopeEntTouched->GetPhysics()->GetAbsBounds().IntersectsBounds( self->GetPhysics()->GetAbsBounds() )
			&& !groundPlane
		)
	{
		// test distance against the nearest rope body
		int touchedBody = -1;
		idVec3 PlayerPoint = current.origin + -gravityNormal*ROPE_GRABHEIGHT;
		idVec3 RopeSegPoint = static_cast<idPhysics_AF *>(m_RopeEntTouched->GetPhysics())->NearestBodyOrig( PlayerPoint, &touchedBody );

		delta = ( RopeSegPoint - PlayerPoint);
		delta = delta - (gravityNormal * delta) * gravityNormal;
		dist = delta.LengthFast();

		delta.Normalize();
		angleOff = delta * forward;

		// if the player is looking high up, override the angle check
		lookUpAng = viewForward * -gravityNormal;
		// set lookup to true if the player is looking 60 deg up or more
		bLookingUp = lookUpAng >= idMath::Cos(idMath::PI/6);

		if
			(	
				dist <= ROPE_DISTANCE
				&& ( angleOff >= idMath::Cos( ROPE_ATTACHANGLE ) || bLookingUp )
				&& (m_RopeEntTouched != m_RopeEntity || gameLocal.time - m_RopeDetachTimer > ROPE_REATTACHTIME)
				&& !static_cast<idPhysics_AF *>(m_RopeEntTouched->GetPhysics())->HasGroundContacts( touchedBody )
			)
		{
				m_bRopeContact = true;
				m_bJustHitRope = true;
				m_RopeEntity = m_RopeEntTouched;
				goto Quit;
		}
	}

Quit:
	return;
}

/*
=============
idPhysics_Player::CheckJump
=============
*/
bool idPhysics_Player::CheckJump( void ) {
	idVec3 addVelocity;

	if ( command.upmove < 10 ) {
		// not holding jump
		return false;
	}

	// must wait for jump to be released
	if ( current.movementFlags & PMF_JUMP_HELD ) {
		return false;
	}

	// don't jump if we can't stand up
	if ( current.movementFlags & PMF_DUCKED ) {
		return false;
	}

	groundPlane = false;		// jumping away
	walking = false;
	current.movementFlags |= PMF_JUMP_HELD | PMF_JUMPED;

	addVelocity = 2.0f * maxJumpHeight * -gravityVector;
	addVelocity *= idMath::Sqrt( addVelocity.Normalize() );
	current.velocity += addVelocity;

	return true;
}

/*
=============
idPhysics_Player::CheckRopeJump
=============
*/
bool idPhysics_Player::CheckRopeJump( void ) {
	idVec3 addVelocity;
	idVec3 jumpDir;

	if ( command.upmove < 10 ) {
		// not holding jump
		return false;
	}

	// must wait for jump to be released
	if ( current.movementFlags & PMF_JUMP_HELD ) {
		return false;
	}

	// don't jump if we can't stand up
	if ( current.movementFlags & PMF_DUCKED ) {
		return false;
	}

	groundPlane = false;		// jumping away
	walking = false;
	current.movementFlags |= PMF_JUMP_HELD | PMF_JUMPED;

	// the jump direction is an equal sum of up and the direction we're looking
	jumpDir = viewForward - gravityNormal;
	jumpDir *= 1.0f/idMath::Sqrt(2.0f);

	addVelocity = 2.0f * maxJumpHeight * gravityVector.Length() * jumpDir;
	addVelocity *= idMath::Sqrt( addVelocity.Normalize() );

	current.velocity += addVelocity;

	return true;
}

/*
=============
idPhysics_Player::CheckWaterJump

REMOVED from DarkMod
=============
*/

/*

=============

idPhysics_Player::SetWaterLevel

For MOD_WATERPHYSICS this is moved to Physics_Actor.cpp

=============

*/

#ifndef MOD_WATERPHYSICS

void idPhysics_Player::SetWaterLevel( void ) {
	idVec3		point;
	idBounds	bounds;
	int			contents;

	//
	// get waterlevel, accounting for ducking
	//
	waterLevel = WATERLEVEL_NONE;
	waterType = 0;

	bounds = clipModel->GetBounds();

	// check at feet level
	point = current.origin - ( bounds[0][2] + 1.0f ) * gravityNormal;
	contents = gameLocal.clip.Contents( point, NULL, mat3_identity, -1, self );
	if ( contents & MASK_WATER ) {

		waterType = contents;
		waterLevel = WATERLEVEL_FEET;

		// check at waist level
		point = current.origin - ( bounds[1][2] - bounds[0][2] ) * 0.5f * gravityNormal;
		contents = gameLocal.clip.Contents( point, NULL, mat3_identity, -1, self );
		if ( contents & MASK_WATER ) {

			waterLevel = WATERLEVEL_WAIST;

			// check at head level
			point = current.origin - ( bounds[1][2] - 1.0f ) * gravityNormal;
			contents = gameLocal.clip.Contents( point, NULL, mat3_identity, -1, self );
			if ( contents & MASK_WATER ) {
				waterLevel = WATERLEVEL_HEAD;
			}
		}
	}
}
#endif

/*
================
idPhysics_Player::DropTimers
================
*/
void idPhysics_Player::DropTimers( void ) {
	// drop misc timing counter
	if ( current.movementTime ) {
		if ( framemsec >= current.movementTime ) {
			current.movementFlags &= ~PMF_ALL_TIMES;
			current.movementTime = 0;
		}
		else {
			current.movementTime -= framemsec;
		}
	}
}

/*
================
idPhysics_Player::MovePlayer
================
*/
void idPhysics_Player::MovePlayer( int msec ) {

	// this counter lets us debug movement problems with a journal
	// by setting a conditional breakpoint for the previous frame
	c_pmove++;

	walking = false;
	groundPlane = false;
	
	m_bRopeContact = false;
	ladder = false;

	// determine the time
	framemsec = msec;
	frametime = framemsec * 0.001f;

	// default speed
	playerSpeed = walkSpeed;

	// remove jumped and stepped up flag
	current.movementFlags &= ~(PMF_JUMPED|PMF_STEPPED_UP|PMF_STEPPED_DOWN);
	current.stepUp = 0.0f;

	if ( command.upmove < 10 ) {
		// not holding jump
		current.movementFlags &= ~PMF_JUMP_HELD;
	}

	// if no movement at all
	if ( current.movementType == PM_FREEZE ) {
		return;
	}

	// move the player velocity into the frame of a pusher
	current.velocity -= current.pushVelocity;

	// view vectors
	viewAngles.ToVectors( &viewForward, NULL, NULL );
	viewForward *= clipModelAxis;
	viewRight = gravityNormal.Cross( viewForward );
	viewRight.Normalize();

	// Leaning Mod: Test for clipping collisions caused between
	// physics frames due to looking around while leaning
	TestForViewRotationBasedCollisions();

	// fly in spectator mode
	if ( current.movementType == PM_SPECTATOR ) {
		SpectatorMove();
		idPhysics_Player::DropTimers();
		return;
	}

	// special no clip mode
	if ( current.movementType == PM_NOCLIP ) {
		idPhysics_Player::NoclipMove();
		idPhysics_Player::DropTimers();
		return;
	}

	// no control when dead
	if ( current.movementType == PM_DEAD ) {
		command.forwardmove = 0;
		command.rightmove = 0;
		command.upmove = 0;
	}

	// set watertype and waterlevel
	idPhysics_Player::SetWaterLevel();

	// check for ground
	idPhysics_Player::CheckGround();

	// check if up against a ladder or a rope
	idPhysics_Player::CheckLadder();

	// set clip model size
	idPhysics_Player::CheckDuck();

	// handle timers
	idPhysics_Player::DropTimers();

	// Mantle Mod: SophisticatdZombie (DH)
	idPhysics_Player::UpdateMantleTimers();

	// Lean Mod: Zaccheus and SophisticatedZombie (DH)
	idPhysics_Player::LeanMove();

	// Check if holding down jump
	if (CheckJumpHeldDown())
	{
		idPhysics_Player::PerformMantle();
	}

	// move
	if ( current.movementType == PM_DEAD ) {
		// dead
		idPhysics_Player::DeadMove();
	}
	// continue moving on the rope if still attached
	else if ( m_bRopeAttached )
	{
		idPhysics_Player::RopeMove();
	}
	else if ( m_bRopeContact ) 
	{
		// toggle m_bRopeAttached
		m_bRopeAttached = true;

		// lower weapon
		static_cast<idPlayer *>(self)->LowerWeapon();
		static_cast<idPlayer *>(self)->hiddenWeapon = true;

		idPhysics_Player::RopeMove();
	}
	else if ( ladder ) 
	{
		// going up or down a ladder
		idPhysics_Player::LadderMove();
	}
	// Mantle MOD
	// SophisticatedZombie (DH)
	else if (m_mantlePhase != notMantling_DarkModMantlePhase) 
	{
		idPhysics_Player::MantleMove();
	}
	else if ( waterLevel > 1 ) {
		// swimming
		idPhysics_Player::WaterMove();
	}
	else if ( walking ) {
		// walking on ground
		idPhysics_Player::WalkMove();
	}
	else {
		// airborne
		idPhysics_Player::AirMove();
	}

	// raise weapon if not swimming, mantling or on a rope
	if( m_mantlePhase == notMantling_DarkModMantlePhase && waterLevel <= 1 && !m_bRopeAttached )
	{
		if( static_cast<idPlayer *>(self)->hiddenWeapon )
		{
			static_cast<idPlayer *>(self)->RaiseWeapon();
			static_cast<idPlayer *>(self)->hiddenWeapon = false;
		}
	}

	// set watertype, waterlevel and groundentity
	idPhysics_Player::SetWaterLevel();
	idPhysics_Player::CheckGround();

	// move the player velocity back into the world frame
	current.velocity += current.pushVelocity;
	current.pushVelocity.Zero();

	// DEBUG
	/*
	gameRenderWorld->DebugBounds
	(
		idVec4 (1.0, 0.0, 1.0, 1.0), 
		clipModel->GetAbsBounds(),
		idVec3 (0.0, 0.0, 0.0)
	);
	*/

	m_lastCommandViewYaw = command.angles[1];

}

#ifndef MOD_WATERPHYSICS

/*
================
idPhysics_Player::GetWaterLevel

For MOD_WATERPHYSICS this is moved to Physics_Actor.cpp

================
*/
waterLevel_t idPhysics_Player::GetWaterLevel( void ) const {
	return waterLevel;
}

/*
================
idPhysics_Player::GetWaterType

For MOD_WATERPHYSICS this is moved to Physics_Actor.cpp

================
*/
int idPhysics_Player::GetWaterType( void ) const {
	return waterType;
}

#endif


/*
================
idPhysics_Player::HasJumped
================
*/
bool idPhysics_Player::HasJumped( void ) const {
	return ( ( current.movementFlags & PMF_JUMPED ) != 0 );
}

/*
================
idPhysics_Player::HasSteppedUp
================
*/
bool idPhysics_Player::HasSteppedUp( void ) const {
	return ( ( current.movementFlags & ( PMF_STEPPED_UP | PMF_STEPPED_DOWN ) ) != 0 );
}

/*
================
idPhysics_Player::GetStepUp
================
*/
float idPhysics_Player::GetStepUp( void ) const {
	return current.stepUp;
}

/*
================
idPhysics_Player::IsCrouching
================
*/
bool idPhysics_Player::IsCrouching( void ) const {
	return ( ( current.movementFlags & PMF_DUCKED ) != 0 );
}

/*
================
idPhysics_Player::OnRope
================
*/
bool idPhysics_Player::OnRope( void ) const 
{
	return m_bRopeAttached;
}


/*
================
idPhysics_Player::OnLadder
================
*/
bool idPhysics_Player::OnLadder( void ) const {
	return ladder;
}

/*
================
idPhysics_Player::idPhysics_Player
================
*/
idPhysics_Player::idPhysics_Player( void ) {
	debugLevel = false;
	clipModel = NULL;
	clipMask = 0;
	memset( &current, 0, sizeof( current ) );
	saved = current;
	walkSpeed = 0;
	crouchSpeed = 0;
	maxStepHeight = 0;
	maxJumpHeight = 0;
	memset( &command, 0, sizeof( command ) );
	viewAngles.Zero();
	framemsec = 0;
	frametime = 0;
	playerSpeed = 0;
	viewForward.Zero();
	viewRight.Zero();
	walking = false;
	groundPlane = false;
	memset( &groundTrace, 0, sizeof( groundTrace ) );
	groundMaterial = NULL;
	
	// rope climbing
	m_bRopeContact = false;
	m_bRopeAttached = false;
	m_bJustHitRope = false;
	m_RopeEntity = NULL;
	m_RopeEntTouched = NULL;
	m_RopeDetachTimer = 0;
	m_lastCommandViewYaw = 0;

	ladder = false;
	ladderNormal.Zero();
	waterLevel = WATERLEVEL_NONE;
	waterType = 0;

	// Mantle Mod
	m_mantlePhase = notMantling_DarkModMantlePhase;
	m_mantleTime = 0.0;
	m_p_mantledEntity = NULL;
	m_mantledEntityID = 0;
	m_jumpHeldDownTime = 0.0;

	// Leaning Mod
	m_leanYawAngleDegrees = 0.0;
	m_currentLeanTiltDegrees = 0.0;
	m_b_leanFinished = true;
	m_leanMoveStartTilt = 0.0;
	m_leanMoveEndTilt = 0.0;

	m_viewLeanAngles = ang_zero;
	m_viewLeanTranslation = vec3_zero;

	m_DeltaViewYaw = 0.0;

}

/*
================
idPhysics_Player_SavePState
================
*/
void idPhysics_Player_SavePState( idSaveGame *savefile, const playerPState_t &state ) {
	savefile->WriteVec3( state.origin );
	savefile->WriteVec3( state.velocity );
	savefile->WriteVec3( state.localOrigin );
	savefile->WriteVec3( state.pushVelocity );
	savefile->WriteFloat( state.stepUp );
	savefile->WriteInt( state.movementType );
	savefile->WriteInt( state.movementFlags );
	savefile->WriteInt( state.movementTime );
}

/*
================
idPhysics_Player_RestorePState
================
*/
void idPhysics_Player_RestorePState( idRestoreGame *savefile, playerPState_t &state ) {
	savefile->ReadVec3( state.origin );
	savefile->ReadVec3( state.velocity );
	savefile->ReadVec3( state.localOrigin );
	savefile->ReadVec3( state.pushVelocity );
	savefile->ReadFloat( state.stepUp );
	savefile->ReadInt( state.movementType );
	savefile->ReadInt( state.movementFlags );
	savefile->ReadInt( state.movementTime );
}

/*
================
idPhysics_Player::Save
================
*/
void idPhysics_Player::Save( idSaveGame *savefile ) const {

	idPhysics_Player_SavePState( savefile, current );
	idPhysics_Player_SavePState( savefile, saved );

	savefile->WriteFloat( walkSpeed );
	savefile->WriteFloat( crouchSpeed );
	savefile->WriteFloat( maxStepHeight );
	savefile->WriteFloat( maxJumpHeight );
	savefile->WriteInt( debugLevel );

	savefile->WriteUsercmd( command );
	savefile->WriteAngles( viewAngles );

	savefile->WriteInt( framemsec );
	savefile->WriteFloat( frametime );
	savefile->WriteFloat( playerSpeed );
	savefile->WriteVec3( viewForward );
	savefile->WriteVec3( viewRight );

	savefile->WriteBool( walking );
	savefile->WriteBool( groundPlane );
	savefile->WriteTrace( groundTrace );
	savefile->WriteMaterial( groundMaterial );

	savefile->WriteBool( m_bRopeContact );
	savefile->WriteBool( m_bJustHitRope );
	savefile->WriteBool( m_bRopeAttached );
	savefile->WriteInt( m_RopeDetachTimer );
	savefile->WriteObject( m_RopeEntity );
	savefile->WriteFloat( m_lastCommandViewYaw );

	savefile->WriteBool( ladder );
	savefile->WriteVec3( ladderNormal );

	savefile->WriteInt( (int)waterLevel );
	savefile->WriteInt( waterType );

	// Mantle mod
	savefile->WriteInt (m_mantlePhase);
	savefile->WriteVec3 (m_mantlePullStartPos);
	savefile->WriteVec3 (m_mantlePullEndPos);
	savefile->WriteVec3 (m_mantlePushEndPos);
	savefile->WriteString (m_mantledEntityName);
	savefile->WriteFloat (m_mantleTime);
	savefile->WriteFloat (m_jumpHeldDownTime);

	// Lean mod
	savefile->WriteFloat (m_leanYawAngleDegrees);
	savefile->WriteFloat (m_currentLeanTiltDegrees);
	savefile->WriteFloat (m_leanMoveStartTilt);
	savefile->WriteFloat (m_leanMoveEndTilt);
	savefile->WriteBool (m_b_leanFinished);
	savefile->WriteFloat (m_leanTime);
	savefile->WriteAngles (m_lastPlayerViewAngles);
	savefile->WriteAngles (m_viewLeanAngles);
	savefile->WriteVec3 (m_viewLeanTranslation);



}

/*
================
idPhysics_Player::Restore
================
*/
void idPhysics_Player::Restore( idRestoreGame *savefile ) {

	idPhysics_Player_RestorePState( savefile, current );
	idPhysics_Player_RestorePState( savefile, saved );

	savefile->ReadFloat( walkSpeed );
	savefile->ReadFloat( crouchSpeed );
	savefile->ReadFloat( maxStepHeight );
	savefile->ReadFloat( maxJumpHeight );
	savefile->ReadInt( debugLevel );

	savefile->ReadUsercmd( command );
	savefile->ReadAngles( viewAngles );

	savefile->ReadInt( framemsec );
	savefile->ReadFloat( frametime );
	savefile->ReadFloat( playerSpeed );
	savefile->ReadVec3( viewForward );
	savefile->ReadVec3( viewRight );

	savefile->ReadBool( walking );
	savefile->ReadBool( groundPlane );
	savefile->ReadTrace( groundTrace );
	savefile->ReadMaterial( groundMaterial );

	savefile->ReadBool( m_bRopeContact );
	savefile->ReadBool( m_bJustHitRope );
	savefile->ReadBool( m_bRopeAttached );
	savefile->ReadInt( m_RopeDetachTimer );
	savefile->ReadObject( reinterpret_cast<idClass *&>( m_RopeEntity ) );
	savefile->ReadFloat( m_lastCommandViewYaw );

	savefile->ReadBool( ladder );
	savefile->ReadVec3( ladderNormal );

	savefile->ReadInt( (int &)waterLevel );
	savefile->ReadInt( waterType );

	// Mantle mod
	savefile->ReadInt ((int&) m_mantlePhase);
	savefile->ReadVec3 (m_mantlePullStartPos);
	savefile->ReadVec3 (m_mantlePullEndPos);
	savefile->ReadVec3 (m_mantlePushEndPos);
	savefile->ReadString (m_mantledEntityName);
	savefile->ReadFloat (m_mantleTime);
	savefile->ReadFloat (m_jumpHeldDownTime);

	// Mantle mod... it would be nice to restore the mantled
	// entity pointers here, but we can't, because during a load
	// this method is called before the entites are created.
	// (Yes, I tried this and checked).
	// Therefore, we have a section in "MantleMove" that
	// finds the entity if we have not found it yet, and cancels
	// the mantle if it can not be found.
	m_mantledEntityID = 0;
	m_p_mantledEntity = NULL;

	// Lean mod
	savefile->ReadFloat (m_leanYawAngleDegrees);
	savefile->ReadFloat (m_currentLeanTiltDegrees);
	savefile->ReadFloat (m_leanMoveStartTilt);
	savefile->ReadFloat (m_leanMoveEndTilt);
	savefile->ReadBool (m_b_leanFinished);
	savefile->ReadFloat (m_leanTime);
	savefile->ReadAngles (m_lastPlayerViewAngles);
	savefile->ReadAngles (m_viewLeanAngles);
	savefile->ReadVec3 (m_viewLeanTranslation);

	
	if (!m_mantledEntityName.IsEmpty())
	{
		m_p_mantledEntity = gameLocal.FindEntity (m_mantledEntityName.c_str());
		if (m_p_mantledEntity == NULL)
		{
			DM_LOG (LC_MOVEMENT, LT_DEBUG)LOGSTRING ("Entity being mantled during save, '%s', was not found\n", m_mantledEntityName.c_str());
			m_mantledEntityID = 0;
		}
		else
		{
			DM_LOG (LC_MOVEMENT, LT_DEBUG)LOGSTRING ("Found entity %s\n", m_mantledEntityName.c_str());
			m_mantledEntityID = 0;
		}
	}


	DM_LOG (LC_MOVEMENT, LT_DEBUG)LOGSTRING ("REstore finished\n");
}

/*
================
idPhysics_Player::SetPlayerInput
================
*/
void idPhysics_Player::SetPlayerInput( const usercmd_t &cmd, const idAngles &newViewAngles ) 
{
	command = cmd;
	viewAngles = newViewAngles;		// can't use cmd.angles cause of the delta_angles

	// TDM: Set m_DeltaViewYaw
	m_DeltaViewYaw = command.angles[1] - m_lastCommandViewYaw;
	m_DeltaViewYaw = SHORT2ANGLE(m_DeltaViewYaw);
	
	// don't return a change if the player's view is locked in place
	if( static_cast<idPlayer *>(self)->GetImmobilization() & EIM_VIEW_ANGLE )
		m_DeltaViewYaw = 0;

	m_lastCommandViewYaw = command.angles[1];
}

/*
================
idPhysics_Player::SetSpeed
================
*/
void idPhysics_Player::SetSpeed( const float newWalkSpeed, const float newCrouchSpeed ) {
	walkSpeed = newWalkSpeed;
	crouchSpeed = newCrouchSpeed;
}

/*
================
idPhysics_Player::SetMaxStepHeight
================
*/
void idPhysics_Player::SetMaxStepHeight( const float newMaxStepHeight ) {
	maxStepHeight = newMaxStepHeight;
}

/*
================
idPhysics_Player::GetMaxStepHeight
================
*/
float idPhysics_Player::GetMaxStepHeight( void ) const {
	return maxStepHeight;
}

/*
================
idPhysics_Player::SetMaxJumpHeight
================
*/
void idPhysics_Player::SetMaxJumpHeight( const float newMaxJumpHeight ) {
	maxJumpHeight = newMaxJumpHeight;
}

/*
================
idPhysics_Player::SetMovementType
================
*/
void idPhysics_Player::SetMovementType( const pmtype_t type ) {
	current.movementType = type;
}

/*
================
idPhysics_Player::SetKnockBack
================
*/
void idPhysics_Player::SetKnockBack( const int knockBackTime ) {
	if ( current.movementTime ) {
		return;
	}
	current.movementFlags |= PMF_TIME_KNOCKBACK;
	current.movementTime = knockBackTime;
}

/*
================
idPhysics_Player::SetDebugLevel
================
*/
void idPhysics_Player::SetDebugLevel( bool set ) {
	debugLevel = set;
}

/*
================
idPhysics_Player::Evaluate
================
*/
bool idPhysics_Player::Evaluate( int timeStepMSec, int endTimeMSec ) {
	idVec3 masterOrigin, oldOrigin;
	idMat3 masterAxis;

	waterLevel = WATERLEVEL_NONE;
	waterType = 0;
	oldOrigin = current.origin;

	clipModel->Unlink();

	// if bound to a master
	if ( masterEntity ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.origin = masterOrigin + current.localOrigin * masterAxis;
		clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() );
		current.velocity = ( current.origin - oldOrigin ) / ( timeStepMSec * 0.001f );
		masterDeltaYaw = masterYaw;
		masterYaw = masterAxis[0].ToYaw();
		masterDeltaYaw = masterYaw - masterDeltaYaw;
		return true;
	}

	ActivateContactEntities();

	idPhysics_Player::MovePlayer( timeStepMSec );

	clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() );

	if ( IsOutsideWorld() ) {
		gameLocal.Warning( "clip model outside world bounds for entity '%s' at (%s)", self->name.c_str(), current.origin.ToString(0) );
	}

	return true; //( current.origin != oldOrigin );
}

/*
================
idPhysics_Player::UpdateTime
================
*/
void idPhysics_Player::UpdateTime( int endTimeMSec ) {
}

/*
================
idPhysics_Player::GetTime
================
*/
int idPhysics_Player::GetTime( void ) const {
	return gameLocal.time;
}

/*
================
idPhysics_Player::GetImpactInfo
================
*/
void idPhysics_Player::GetImpactInfo( const int id, const idVec3 &point, impactInfo_t *info ) const {
	info->invMass = invMass;
	info->invInertiaTensor.Zero();
	info->position.Zero();
	info->velocity = current.velocity;
}

/*
================
idPhysics_Player::ApplyImpulse
================
*/
void idPhysics_Player::ApplyImpulse( const int id, const idVec3 &point, const idVec3 &impulse ) {
	if ( current.movementType != PM_NOCLIP ) {
		current.velocity += impulse * invMass;
	}
}

/*
================
idPhysics_Player::IsAtRest
================
*/
bool idPhysics_Player::IsAtRest( void ) const {
	return false;
}

/*
================
idPhysics_Player::GetRestStartTime
================
*/
int idPhysics_Player::GetRestStartTime( void ) const {
	return -1;
}

/*
================
idPhysics_Player::SaveState
================
*/
void idPhysics_Player::SaveState( void ) {
	saved = current;
}

/*
================
idPhysics_Player::RestoreState
================
*/
void idPhysics_Player::RestoreState( void ) {
	current = saved;

	clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() );

	EvaluateContacts();
}

/*
================
idPhysics_Player::SetOrigin
================
*/
void idPhysics_Player::SetOrigin( const idVec3 &newOrigin, int id ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	current.localOrigin = newOrigin;
	if ( masterEntity ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.origin = masterOrigin + newOrigin * masterAxis;
	}
	else {
		current.origin = newOrigin;
	}

	clipModel->Link( gameLocal.clip, self, 0, newOrigin, clipModel->GetAxis() );
}

/*
================
idPhysics_Player::GetOrigin
================
*/
const idVec3 & idPhysics_Player::PlayerGetOrigin( void ) const {
	return current.origin;
}

/*
================
idPhysics_Player::SetAxis
================
*/
void idPhysics_Player::SetAxis( const idMat3 &newAxis, int id ) {
	clipModel->Link( gameLocal.clip, self, 0, clipModel->GetOrigin(), newAxis );
}

/*
================
idPhysics_Player::Translate
================
*/
void idPhysics_Player::Translate( const idVec3 &translation, int id ) {

	current.localOrigin += translation;
	current.origin += translation;

	clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() );
}

/*
================
idPhysics_Player::Rotate
================
*/
void idPhysics_Player::Rotate( const idRotation &rotation, int id ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	current.origin *= rotation;
	if ( masterEntity ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.localOrigin = ( current.origin - masterOrigin ) * masterAxis.Transpose();
	}
	else {
		current.localOrigin = current.origin;
	}

	clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() * rotation.ToMat3() );
}

/*
================
idPhysics_Player::SetLinearVelocity
================
*/
void idPhysics_Player::SetLinearVelocity( const idVec3 &newLinearVelocity, int id ) {
	current.velocity = newLinearVelocity;
}

/*
================
idPhysics_Player::GetLinearVelocity
================
*/
const idVec3 &idPhysics_Player::GetLinearVelocity( int id ) const {
	return current.velocity;
}

/*
================
idPhysics_Player::SetPushed
================
*/
void idPhysics_Player::SetPushed( int deltaTime ) {
	idVec3 velocity;
	float d;

	// velocity with which the player is pushed
	velocity = ( current.origin - saved.origin ) / ( deltaTime * idMath::M_MS2SEC );

	// remove any downward push velocity
	d = velocity * gravityNormal;
	if ( d > 0.0f ) {
		velocity -= d * gravityNormal;
	}

	current.pushVelocity += velocity;
}

/*
================
idPhysics_Player::GetPushedLinearVelocity
================
*/
const idVec3 &idPhysics_Player::GetPushedLinearVelocity( const int id ) const {
	return current.pushVelocity;
}

/*
================
idPhysics_Player::ClearPushedVelocity
================
*/
void idPhysics_Player::ClearPushedVelocity( void ) {
	current.pushVelocity.Zero();
}

/*
================
idPhysics_Player::SetMaster

  the binding is never orientated
================
*/
void idPhysics_Player::SetMaster( idEntity *master, const bool orientated ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	if ( master ) {
		if ( !masterEntity ) {
			// transform from world space to master space
			self->GetMasterPosition( masterOrigin, masterAxis );
			current.localOrigin = ( current.origin - masterOrigin ) * masterAxis.Transpose();
			masterEntity = master;
			masterYaw = masterAxis[0].ToYaw();
		}
		ClearContacts();
	}
	else {
		if ( masterEntity ) {
			masterEntity = NULL;
		}
	}
}

const float	PLAYER_VELOCITY_MAX				= 4000;
const int	PLAYER_VELOCITY_TOTAL_BITS		= 16;
const int	PLAYER_VELOCITY_EXPONENT_BITS	= idMath::BitsForInteger( idMath::BitsForFloat( PLAYER_VELOCITY_MAX ) ) + 1;
const int	PLAYER_VELOCITY_MANTISSA_BITS	= PLAYER_VELOCITY_TOTAL_BITS - 1 - PLAYER_VELOCITY_EXPONENT_BITS;
const int	PLAYER_MOVEMENT_TYPE_BITS		= 3;
const int	PLAYER_MOVEMENT_FLAGS_BITS		= 8;

/*
================
idPhysics_Player::WriteToSnapshot
================
*/
void idPhysics_Player::WriteToSnapshot( idBitMsgDelta &msg ) const {
	msg.WriteFloat( current.origin[0] );
	msg.WriteFloat( current.origin[1] );
	msg.WriteFloat( current.origin[2] );
	msg.WriteFloat( current.velocity[0], PLAYER_VELOCITY_EXPONENT_BITS, PLAYER_VELOCITY_MANTISSA_BITS );
	msg.WriteFloat( current.velocity[1], PLAYER_VELOCITY_EXPONENT_BITS, PLAYER_VELOCITY_MANTISSA_BITS );
	msg.WriteFloat( current.velocity[2], PLAYER_VELOCITY_EXPONENT_BITS, PLAYER_VELOCITY_MANTISSA_BITS );
	msg.WriteDeltaFloat( current.origin[0], current.localOrigin[0] );
	msg.WriteDeltaFloat( current.origin[1], current.localOrigin[1] );
	msg.WriteDeltaFloat( current.origin[2], current.localOrigin[2] );
	msg.WriteDeltaFloat( 0.0f, current.pushVelocity[0], PLAYER_VELOCITY_EXPONENT_BITS, PLAYER_VELOCITY_MANTISSA_BITS );
	msg.WriteDeltaFloat( 0.0f, current.pushVelocity[1], PLAYER_VELOCITY_EXPONENT_BITS, PLAYER_VELOCITY_MANTISSA_BITS );
	msg.WriteDeltaFloat( 0.0f, current.pushVelocity[2], PLAYER_VELOCITY_EXPONENT_BITS, PLAYER_VELOCITY_MANTISSA_BITS );
	msg.WriteDeltaFloat( 0.0f, current.stepUp );
	msg.WriteBits( current.movementType, PLAYER_MOVEMENT_TYPE_BITS );
	msg.WriteBits( current.movementFlags, PLAYER_MOVEMENT_FLAGS_BITS );
	msg.WriteDeltaLong( 0, current.movementTime );
}

/*
================
idPhysics_Player::ReadFromSnapshot
================
*/
void idPhysics_Player::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	current.origin[0] = msg.ReadFloat();
	current.origin[1] = msg.ReadFloat();
	current.origin[2] = msg.ReadFloat();
	current.velocity[0] = msg.ReadFloat( PLAYER_VELOCITY_EXPONENT_BITS, PLAYER_VELOCITY_MANTISSA_BITS );
	current.velocity[1] = msg.ReadFloat( PLAYER_VELOCITY_EXPONENT_BITS, PLAYER_VELOCITY_MANTISSA_BITS );
	current.velocity[2] = msg.ReadFloat( PLAYER_VELOCITY_EXPONENT_BITS, PLAYER_VELOCITY_MANTISSA_BITS );
	current.localOrigin[0] = msg.ReadDeltaFloat( current.origin[0] );
	current.localOrigin[1] = msg.ReadDeltaFloat( current.origin[1] );
	current.localOrigin[2] = msg.ReadDeltaFloat( current.origin[2] );
	current.pushVelocity[0] = msg.ReadDeltaFloat( 0.0f, PLAYER_VELOCITY_EXPONENT_BITS, PLAYER_VELOCITY_MANTISSA_BITS );
	current.pushVelocity[1] = msg.ReadDeltaFloat( 0.0f, PLAYER_VELOCITY_EXPONENT_BITS, PLAYER_VELOCITY_MANTISSA_BITS );
	current.pushVelocity[2] = msg.ReadDeltaFloat( 0.0f, PLAYER_VELOCITY_EXPONENT_BITS, PLAYER_VELOCITY_MANTISSA_BITS );
	current.stepUp = msg.ReadDeltaFloat( 0.0f );
	current.movementType = msg.ReadBits( PLAYER_MOVEMENT_TYPE_BITS );
	current.movementFlags = msg.ReadBits( PLAYER_MOVEMENT_FLAGS_BITS );
	current.movementTime = msg.ReadDeltaLong( 0 );

	if ( clipModel ) {
		clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() );
	}
}

//################################################################
// Start Mantling Mod
//################################################################



float idPhysics_Player::getMantleTimeForPhase 
(
	EDarkMod_MantlePhase mantlePhase
)
{
	float retValue;

	// Current implementation uses constants
	switch (mantlePhase)
	{
	case hang_DarkModMantlePhase:
		retValue = g_Global.m_mantleHang_Milliseconds;
		break;

	case pull_DarkModMantlePhase:
		retValue = g_Global.m_mantlePull_Milliseconds;
		break;

	case shiftHands_DarkModMantlePhase:
		retValue = g_Global.m_mantleShiftHands_Milliseconds;
		break;

	case push_DarkModMantlePhase:
		retValue = g_Global.m_mantlePush_Milliseconds;
		break;

	default:
		retValue = 0.0;
		break;

	}

	return retValue;
}

//----------------------------------------------------------------------

void idPhysics_Player::MantleMove()
{

	idVec3 newPosition = current.origin;
	idVec3 totalMove;
	idVec3 moveSoFar;

	
	totalMove.x = 0.0;
	totalMove.y = 0.0;
	totalMove.z = 0.0;
	float timeForMantlePhase = getMantleTimeForPhase(m_mantlePhase);

	// Compute proportion into the current movement phase which we are
	float timeRatio = 0.0;
	if (timeForMantlePhase != 0.0)
	{
		timeRatio = (timeForMantlePhase - m_mantleTime) /  timeForMantlePhase;
	}

	// Branch based on phase
	if (m_mantlePhase == hang_DarkModMantlePhase)
	{
		// Starting at current position, hanging, rocking a bit.
		float rockDistance = 2.0;

		newPosition = m_mantlePullStartPos;
		float timeRadians = (idMath::PI) * timeRatio;
		viewAngles.roll = (idMath::Sin (timeRadians) * rockDistance);
		newPosition += ((idMath::Sin (timeRadians) * rockDistance) * viewRight );
		
		if (self)
		{
			((idPlayer*)self)->SetViewAngles (viewAngles);
		}

	}
	else if (m_mantlePhase == pull_DarkModMantlePhase)
	{
		// Player pulls themself up to shoulder even with the surface
		totalMove = m_mantlePullEndPos - m_mantlePullStartPos;
		newPosition = m_mantlePullStartPos + (totalMove * idMath::Sin(timeRatio * (idMath::PI/2)) );
	}
	else if (m_mantlePhase == shiftHands_DarkModMantlePhase)
	{
		// Rock back and forth a bit?
		float rockDistance = 1.0;

		newPosition = m_mantlePullEndPos;
		float timeRadians = (idMath::PI) * timeRatio;
		newPosition += ((idMath::Sin (timeRadians) * rockDistance) * viewRight );
		viewAngles.roll = (idMath::Sin (timeRadians) * rockDistance);

		if (self)
		{
			((idPlayer*)self)->SetViewAngles (viewAngles);
		}

	}
	else if (m_mantlePhase == push_DarkModMantlePhase)
	{
		// Rocking back and forth to get legs up over edge
		float rockDistance = 10.0;

		// Player pushes themselves upward to get their legs onto the surface
		totalMove = m_mantlePushEndPos - m_mantlePullEndPos;
		newPosition = m_mantlePullEndPos + (totalMove * idMath::Sin(timeRatio * (idMath::PI/2)) );

		// We go into duck during this phase and stay there until end
		current.movementFlags |= PMF_DUCKED;

		float timeRadians = (idMath::PI) * timeRatio;
		newPosition += ((idMath::Sin (timeRadians) * rockDistance) * viewRight );
		viewAngles.roll = (idMath::Sin (timeRadians) * rockDistance);

		if (self)
		{
			((idPlayer*)self)->SetViewAngles (viewAngles);
		}

	}

	// Try to re-establish mantled entity if we have its name
	// When the player save state is loaded, the entities were not
	// re-created yet, so we need to look now that the whole engine
	// is running
	if ( (!m_mantledEntityName.IsEmpty()) && (m_p_mantledEntity == NULL))
	{
		m_p_mantledEntity = gameLocal.FindEntity (m_mantledEntityName.c_str());
		if (m_p_mantledEntity == NULL)
		{
			DM_LOG (LC_MOVEMENT, LT_DEBUG)LOGSTRING ("MantleMove: Entity being mantled during save, '%s', was not found\n", m_mantledEntityName.c_str());
			m_mantledEntityID = 0;
			CancelMantle();
		}
		else
		{
			DM_LOG (LC_MOVEMENT, LT_DEBUG)LOGSTRING ("MantleMove: Found entity %s\n", m_mantledEntityName.c_str());
			m_mantledEntityID = 0;
		}
	}

	// If there is a mantled entity, positions are relative to it.
	// Transform position to be relative to world origin.
	// (For now, translation only, TODO: Add rotation)
	if (m_p_mantledEntity != NULL)
	{
		idPhysics* p_physics = m_p_mantledEntity->GetPhysics();
		if (p_physics != NULL)
		{
			idVec3 mantledEntityOrigin = p_physics->GetOrigin();
			newPosition += mantledEntityOrigin;

		}
	}

	SetOrigin (newPosition);
	
}

//----------------------------------------------------------------------

void idPhysics_Player::UpdateMantleTimers()
{
	// Frame seconds left
	float framemSecLeft = framemsec;

	// Update jump held down timer: This actually grows, not drops
	if (!( current.movementFlags & PMF_JUMP_HELD ) ) 
	{
		m_jumpHeldDownTime = 0.0;
	}
	else
	{
		m_jumpHeldDownTime += framemsec;
	}

	// Skip all this if not mantling
	if (m_mantlePhase != notMantling_DarkModMantlePhase)
	{

		// Handle expiring mantle phases
		while 
		(
			(framemSecLeft >= m_mantleTime) && 
			(m_mantlePhase != notMantling_DarkModMantlePhase)
		)
		{
			framemSecLeft -= m_mantleTime;
			m_mantleTime = 0.0;

			// Advance mantle phase
			switch (m_mantlePhase)
			{
			case hang_DarkModMantlePhase:
				DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING ("MantleMod: Pulling up...\r\n");
				m_mantlePhase = pull_DarkModMantlePhase;
				break;

			case pull_DarkModMantlePhase:
				DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING ("MantleMod: Shifting hand position...\r\r\n");
				m_mantlePhase = shiftHands_DarkModMantlePhase;
				break;

			case shiftHands_DarkModMantlePhase:
				DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING ("MantleMod: Pushing self up...\r\r\n");
				m_mantlePhase = push_DarkModMantlePhase;

				// Go into crouch
				current.movementFlags |= PMF_DUCKED;
				break;

			case push_DarkModMantlePhase:
				DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING ("MantleMod: mantle completed\r\n");
				m_mantlePhase = notMantling_DarkModMantlePhase;

				break;

			default:
				m_mantlePhase = notMantling_DarkModMantlePhase;
				break;

			}

			// Get time it takes to perform a mantling phase
			m_mantleTime = getMantleTimeForPhase (m_mantlePhase);
			
			// Handle end of mantle
			if (m_mantlePhase == notMantling_DarkModMantlePhase)
			{
				// Handle end of mantle
				// Ishtvan 11/20/05 - Raise weapons after mantle is done
				static_cast<idPlayer *>(self)->RaiseWeapon();
				static_cast<idPlayer *>(self)->hiddenWeapon = false;
				
			}
					
		}

		// Reduce mantle timer
		if (m_mantlePhase == notMantling_DarkModMantlePhase)
		{
			m_mantleTime = 0;
		}
		else
		{
			m_mantleTime -= framemSecLeft;
		}

	} // This code block is executed only if phase != notMantling
	
	// Done
}

//----------------------------------------------------------------------

bool idPhysics_Player::IsMantling (void) const
{
	// Use state boolean
	return m_mantlePhase != notMantling_DarkModMantlePhase;
}

//----------------------------------------------------------------------

EDarkMod_MantlePhase idPhysics_Player::GetMantlePhase (void) const
{
	// Use state boolean
	return m_mantlePhase;
}

//----------------------------------------------------------------------

void idPhysics_Player::CancelMantle()
{
	DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING ("Mantle cancelled\r\n");

	m_mantlePhase = notMantling_DarkModMantlePhase;
	m_mantleTime = 0.0f;
}

//----------------------------------------------------------------------

int idPhysics_Player::CalculateMantleCollisionDamage
(
	idEntity* p_mantledEntityRef,
	idVec3 mantlePos
)
{
	int damageAmount = 0;
	idVec3 relativeVelocity = current.velocity;

	
	if (p_mantledEntityRef)
	{
		idPhysics* p_targetPhysics = p_mantledEntityRef->GetPhysics();
		if (p_targetPhysics != NULL)
		{
			relativeVelocity -= p_targetPhysics->GetLinearVelocity();
			idVec3 relativeAngularVelocity = p_targetPhysics->GetAngularVelocity();

			// Multiply angular velocity by distance from center to get
			// linear velocity due to angular velocity and add it
			// to the other linear velocity.
			idVec3 linearVelocityDueToAngularVelocity;
			idVec3 targetOrigin = p_targetPhysics->GetOrigin();
			idVec3 forceArm = mantlePos - targetOrigin;
			
			linearVelocityDueToAngularVelocity.x =  relativeAngularVelocity.x * forceArm.x;
			linearVelocityDueToAngularVelocity.y =  relativeAngularVelocity.y * forceArm.y;
			linearVelocityDueToAngularVelocity.z =  relativeAngularVelocity.z * forceArm.z;

			relativeVelocity += linearVelocityDueToAngularVelocity;
		}

	}

	// Analyze velocity: TODO make constant somewhere global
	float velocityMagnitude_MetersPerSecond = relativeVelocity.Length() * 0.0254f;
	

	if (velocityMagnitude_MetersPerSecond > g_Global.m_minimumVelocityForMantleDamage)
	{
		// 15.0 m/s over 1 second is bad for you according to restraining harness laws, so
		// we will use that as a guideline
		// How we scale this is fairly arbitrary
		// I should move the scale and velocity to the darkmod globals

		damageAmount = (velocityMagnitude_MetersPerSecond - g_Global.m_minimumVelocityForMantleDamage) * g_Global.m_damagePointsPerMetersPerSecondOverMinimum;

	}

	DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING 
	(
		"Velocity = %.5f, damage = %d", 
		velocityMagnitude_MetersPerSecond, 
		damageAmount
	);

	// Return amount
	return damageAmount;

			

}

//----------------------------------------------------------------------

void idPhysics_Player::StartMantle
(
	EDarkMod_MantlePhase initialMantlePhase,
	idVec3 eyePos,
	idVec3 startPos,
	idVec3 endPos
)
{
	// 9/17/05:
	// SophisticatedZombie
	// Check the player's velocity.  If they are moving
	// at a very high velocity relative to the mantle target, then
	// apply damage to the player.
	int damageAmount = CalculateMantleCollisionDamage
	(
		m_p_mantledEntity,
		startPos
	);

	if ((self != NULL) && (damageAmount != 0))
	{
		idPlayer* p_Player = (idPlayer*) self;
		p_Player->Damage
		(
			NULL, // no inflictor
			NULL, // no attacker,
			startPos - current.origin, // direction damage came from
			"damage_hardfall",
			1.0f,
			0
		);

		if (p_Player->health <= 0)
		{
			// Can't start mantle if dead
			return;
		}
	}

	// Ishtvan 10/16/05
	// If mantling starts while on a rope, detach from that rope
	if ( m_bRopeAttached )
	{
		RopeDetach();
	}

	// Ishtvan 11/20/05 - Lower weapons when mantling
	static_cast<idPlayer *>(self)->LowerWeapon();
	static_cast<idPlayer *>(self)->hiddenWeapon = true;

	// If mantling from a jump, cancel any velocity so that it does
	// not continue after the mantle is completed.
	current.velocity.Zero();

	// Calculate mantle distance
	idVec3 mantleDistanceVec = endPos - startPos;
	float mantleDistance = mantleDistanceVec.Length();

	// Log starting phase
	if (initialMantlePhase == hang_DarkModMantlePhase)
	{
		DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING
		(
			"Mantle starting with hang\n"
		);

		// Impart a force on mantled object?
		if ((m_p_mantledEntity != NULL) && (self != NULL))
		{
			impactInfo_t info;
			m_p_mantledEntity->GetImpactInfo( self, m_mantledEntityID, endPos, &info );
			if ( info.invMass != 0.0f ) 
			{
				m_p_mantledEntity->ActivatePhysics(self);
				m_p_mantledEntity->ApplyImpulse( self, m_mantledEntityID, endPos, current.velocity / ( info.invMass * 2.0f ) );
			}
		}

	}
	else if (initialMantlePhase == pull_DarkModMantlePhase)
	{
		DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING
		(
			"Mantle starting with pull upward\n"
		);
	}
	else if (initialMantlePhase == shiftHands_DarkModMantlePhase)
	{
		DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING
		(
			"Mantle starting with shift hands\n"
		);
	}
	else if (initialMantlePhase == push_DarkModMantlePhase)
	{
		// Go into crouch
		current.movementFlags |= PMF_DUCKED;

		// Start with push upward
		DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING
		(
			"Mantle starting with push upward\n"
		);
	}

	m_mantlePhase = initialMantlePhase;
	m_mantleTime = getMantleTimeForPhase (m_mantlePhase);

	// Make positions relative to entity
	if (m_p_mantledEntity != NULL)
	{
		idPhysics* p_physics = m_p_mantledEntity->GetPhysics();
		if (p_physics != NULL)
		{
			idVec3 mantledEntityOrigin = p_physics->GetOrigin();

			startPos -= mantledEntityOrigin;
			eyePos -= mantledEntityOrigin;
			endPos -= mantledEntityOrigin;
		}
	}

	// Set end position
	m_mantlePushEndPos = endPos;
	if 
	(
		(initialMantlePhase == pull_DarkModMantlePhase) ||
		(initialMantlePhase == hang_DarkModMantlePhase)
	)
	{
		// Pull from start position up to about 2/3 of eye height
		m_mantlePullStartPos = startPos;
		m_mantlePullEndPos = eyePos;
		m_mantlePullEndPos += 
		(
			GetGravityNormal() * 
			(pm_normalheight.GetFloat() / 3.0)
		);

	}
	else
	{
		// Starting with push from current position
		m_mantlePullEndPos = startPos;
	}

}

//----------------------------------------------------------------------

bool idPhysics_Player::CheckJumpHeldDown( void )
{
	if (m_jumpHeldDownTime > g_Global.m_jumpHoldMantleTrigger_Milliseconds)
	{
		return true;
	}
	else
	{
		return false;
	}

}

//----------------------------------------------------------------------

void idPhysics_Player::GetCurrentMantlingReachDistances
(
	float& out_maxVerticalReachDistance,
	float& out_maxHorizontalReachDistance,
	float& out_maxMantleTraceDistance
)
{
	// Determine arm length
	float armLength = pm_normalheight.GetFloat() * g_Global.m_armLengthAsFractionOfPlayerHeight;

	// Trace out as far as arm length from player
	out_maxMantleTraceDistance = armLength;

	// Determine maximum vertical and horizontal distance components for
	// a mantleable surface
	if (current.movementFlags & PMF_DUCKED )
	{
		out_maxVerticalReachDistance = pm_crouchheight.GetFloat() + armLength;
		out_maxHorizontalReachDistance = armLength;
	}
	else
	{
		// This vertical distance is up from the players feet
		out_maxVerticalReachDistance = pm_normalheight.GetFloat() + armLength;
		out_maxHorizontalReachDistance = armLength;
	}

}

//----------------------------------------------------------------------

void idPhysics_Player::MantleTargetTrace
(
	float maxMantleTraceDistance,
	idVec3 eyePos,
	idVec3 forwardVec,
	trace_t& out_trace
)
{
	// Calculate end point of gaze trace
	idVec3 end = eyePos + (maxMantleTraceDistance * forwardVec);

	// Run gaze trace
	gameLocal.clip.TracePoint( out_trace, eyePos, end, MASK_SOLID, self );

	// If that trace didn't hit anything, try a taller trace forward along the midline
	// of the player's body for the full player's height out the trace distance.
	if ( out_trace.fraction >= 1.0f ) 
	{
		idVec3 upVector = -GetGravityNormal();

		// Project forward vector onto the a plane perpendicular to gravity
		idVec3 forwardPerpGrav = forwardVec;
		forwardPerpGrav.ProjectOntoPlane (upVector);

		// Create bounds for translation trace model
		idBounds bounds;
		idBounds savedBounds;
		bounds = clipModel->GetBounds();
		savedBounds = bounds;

		bounds[0][1] = (savedBounds[0][1] + savedBounds[1][1]) / 2;
		bounds[0][1] -= 0.01f;
		bounds[1][1] = bounds[0][1] + 0.02f;
		bounds[0][0] = bounds[0][1];
		bounds[1][0] = bounds[1][1];
		
		
		if ( pm_usecylinder.GetBool() ) 
		{
			clipModel->LoadModel( idTraceModel( bounds, 8 ) );
		}
		else 
		{
			clipModel->LoadModel( idTraceModel( bounds ) );
		}

		DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING
		(
			"Mantle gaze trace didn't hit anything, so doing forward movement trace for mantle target\n"
		);
		gameLocal.clip.Translation 
		(
			out_trace, 
			current.origin, 
			current.origin + (maxMantleTraceDistance * forwardPerpGrav), 
			clipModel, 
			clipModel->GetAxis(), 
			MASK_SOLID, 
			self
		);

		// Restore player clip model to normal
		if ( pm_usecylinder.GetBool() ) 
		{
			clipModel->LoadModel( idTraceModel( savedBounds, 8 ) );
		}
		else 
		{
			clipModel->LoadModel( idTraceModel( savedBounds ) );
		}

	}
	
	// Get the entity to be mantled
	if (out_trace.c.entityNum != ENTITYNUM_NONE)
	{

		// Track entity which is was the chosen target
		m_p_mantledEntity = gameLocal.entities[out_trace.c.entityNum];
		m_mantledEntityName = m_p_mantledEntity->name;
		m_mantledEntityID = out_trace.c.id;

		idStr targetMessage;
		DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING
		(
			"Mantle target entity is called '%s'\n", 
			m_p_mantledEntity->name.c_str()
		);
	}

}

//----------------------------------------------------------------------

bool idPhysics_Player::DetermineIfMantleTargetHasMantleableSurface
(	
	float maxVerticalReachDistance,
	float maxHorizontalReachDistance,
	trace_t& in_targetTraceResult,
	idVec3& out_mantleEndPoint
)
{

	// Try moving player's bounding box up from the trace hit point
	// in steps up to the maximum distance and see if at any point
	// there are no collisions. If so, we can mantle.

	// First point to test has gravity orthogonal coordinates set
	// to the ray trace collision point. It then has gravity non-orthogonal
	// coordinates set from the current player origin.  However,
	// for the non-orthogonal-to-gravity coordinates, the trace.c.point
	// location is a better starting place.  Because of rear surface occlusion,
	// it will always be closer to the actual "upper" surface than the player
	// origin unless the object is "below" the player relative to gravity.
	// And, in that "below" case, mantling isn't possible anyway.
	
	// This sets coordinates to their components which are orthogonal
	// to gravity.
	idVec3 componentOrthogonalToGravity = in_targetTraceResult.c.point;
	componentOrthogonalToGravity.ProjectOntoPlane (-gravityNormal);

	// This sets coordintes to their components parallel to gravity
	idVec3 componentParallelToGravity;
	componentParallelToGravity.x = -gravityNormal.x * in_targetTraceResult.c.point.x;
	componentParallelToGravity.y = -gravityNormal.y * in_targetTraceResult.c.point.y;
	componentParallelToGravity.z = -gravityNormal.z * in_targetTraceResult.c.point.z;

	// What parallel to gravity reach distance is already used up at this point
	idVec3 originParallelToGravity;
	originParallelToGravity.x = -gravityNormal.x * current.origin.x;
	originParallelToGravity.y = -gravityNormal.y * current.origin.y;
	originParallelToGravity.z = -gravityNormal.z * current.origin.z;

	float verticalReachDistanceUsed = (componentParallelToGravity - originParallelToGravity).Length();
	
	DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING
	(
		"Initial vertical reach distance used = %f out of maximum of %f\n", 
		verticalReachDistanceUsed, 
		maxVerticalReachDistance
	);

	// The first test point 
	idVec3 testPosition = componentOrthogonalToGravity + componentParallelToGravity;

	// Load crouch model
	// as mantling ends in a crouch
	if (!(current.movementFlags & PMF_DUCKED))
	{
		idBounds bounds;
		bounds = clipModel->GetBounds();
		bounds[1][2] = pm_crouchheight.GetFloat();
		if ( pm_usecylinder.GetBool() ) 
		{
			clipModel->LoadModel( idTraceModel( bounds, 8 ) );
		}
		else 
		{
			clipModel->LoadModel( idTraceModel( bounds ) );
		}
	}

	// We try moving it up by the step distance up to the maximum height until
	// there are no collisions
	bool b_keepTesting = verticalReachDistanceUsed < maxVerticalReachDistance;
	bool b_mantlePossible = false;
	while (b_keepTesting)
	{

		// Try collision in_targetTraceResult
		trace_t worldMantleTrace;
		idVec3 mantleTraceStart = testPosition;
		gameLocal.clip.Translation( worldMantleTrace, mantleTraceStart, testPosition, clipModel, clipModel->GetAxis(), clipMask, self );


		if (worldMantleTrace.fraction >= 1.0)
		{
			// We can mantle to there
			b_keepTesting = false;
			b_mantlePossible = true;
			out_mantleEndPoint = testPosition;
		}
		else
		{
			// Try next test position
			if (verticalReachDistanceUsed < maxVerticalReachDistance)
			{

				float testIncrementAmount = maxVerticalReachDistance - verticalReachDistanceUsed;

				// Establish upper bound for increment test size
				if (testIncrementAmount > MANTLE_TEST_INCREMENT)
				{
					testIncrementAmount = MANTLE_TEST_INCREMENT;
				}

				// Establish absolute minimum increment size so that
				// we don't approach increment size below floating point precision,
				// which would cause an infinite loop.
				if (testIncrementAmount < 1.0f)
				{
					testIncrementAmount = 1.0f;
				}

				// Update location by increment size
				componentParallelToGravity += (-gravityNormal * testIncrementAmount);
				verticalReachDistanceUsed = (componentParallelToGravity - originParallelToGravity).Length();

				DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING
				(
					"Ledge Search: Vertical reach distance used = %f out of maximum of %f\n", 
					verticalReachDistanceUsed, 
					maxVerticalReachDistance
				);

				// Modify test position
				testPosition = componentOrthogonalToGravity + componentParallelToGravity;

			}
			else
			{
				// No surface we could fit on against gravity from raytrace hit point
				// up as far as we can reach
				b_keepTesting = false;
				DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING ("No mantleable surface within reach distance\r\n");
			}
		}
		
	}
			
	// Must restore standing model if player is not crouched
	if (!(current.movementFlags & PMF_DUCKED))
	{
		// Load back standing model
		idBounds bounds;
		bounds = clipModel->GetBounds();
		bounds[1][2] = pm_normalheight.GetFloat();
		if ( pm_usecylinder.GetBool() ) 
		{
			clipModel->LoadModel( idTraceModel( bounds, 8 ) );
		}
		else 
		{
			clipModel->LoadModel( idTraceModel( bounds ) );
		}
	}

	// Return result
	return b_mantlePossible;

}

//----------------------------------------------------------------------

bool idPhysics_Player::DetermineIfPathToMantleSurfaceIsPossible
(
	float maxVerticalReachDistance,
	float maxHorizontalReachDistance,
	idVec3 in_eyePos,
	idVec3 in_mantleStartPoint,
	idVec3 in_mantleEndPoint
)
{
	// Make sure path from current location
	// upward can be traversed.
	trace_t roomForMoveUpTrace;
	idVec3 MoveUpStart = in_mantleStartPoint;
	idVec3 MoveUpEnd;

	// Go to coordinate components against gravity from current location
	idVec3 componentOrthogonalToGravity;
	componentOrthogonalToGravity = in_mantleStartPoint;
	componentOrthogonalToGravity.ProjectOntoPlane (-gravityNormal);
	MoveUpEnd = componentOrthogonalToGravity;

	MoveUpEnd.x += -gravityNormal.x * in_mantleEndPoint.x;
	MoveUpEnd.y += -gravityNormal.y * in_mantleEndPoint.y;
	MoveUpEnd.z += -gravityNormal.z * in_mantleEndPoint.z;

	// Use crouch clip model
	if (!(current.movementFlags & PMF_DUCKED))
	{
		// Load crouching model
		idBounds bounds;
		bounds = clipModel->GetBounds();
		bounds[1][2] = pm_crouchheight.GetFloat();
		if ( pm_usecylinder.GetBool() ) 
		{
			clipModel->LoadModel( idTraceModel( bounds, 8 ) );
		}
		else 
		{
			clipModel->LoadModel( idTraceModel( bounds ) );
		}
	}

	gameLocal.clip.Translation
	(
		roomForMoveUpTrace, 
		MoveUpStart, 
		MoveUpEnd, 
		clipModel, 
		clipModel->GetAxis(), 
		clipMask, 
		self 
	);

	// Done with crouch model if not currently crouched
	if (!(current.movementFlags & PMF_DUCKED))
	{
		// Load back standing model
		idBounds bounds;
		bounds = clipModel->GetBounds();
		bounds[1][2] = pm_normalheight.GetFloat();
		if ( pm_usecylinder.GetBool() ) 
		{
			clipModel->LoadModel( idTraceModel( bounds, 8 ) );
		}
		else 
		{
			clipModel->LoadModel( idTraceModel( bounds ) );
		}
	}

	// Log
	if (roomForMoveUpTrace.fraction < 1.0)
	{
		DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING
		(
			"Collision test from (%f %f %f) to (%f %f %f) yieled trace fraction %f\n",
			MoveUpStart.x,
			MoveUpStart.y,
			MoveUpStart.z,
			MoveUpEnd.x,
			MoveUpEnd.y,
			MoveUpEnd.z,
			roomForMoveUpTrace.fraction
		);
		DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING ("Not enough vertical clearance along mantle path\r\n");
		return false;
	}
	else
	{
		return true;
	}

}

//----------------------------------------------------------------------

bool idPhysics_Player::ComputeMantlePathForTarget
(	
	float maxVerticalReachDistance,
	float maxHorizontalReachDistance,
	idVec3 eyePos,
	trace_t& in_targetTraceResult,
	idVec3& out_mantleEndPoint
)
{
	// Up vector
	idVec3 upVector;
	upVector = -GetGravityNormal();

	// Mantle start point is origin
	idVec3 mantleStartPoint;
	mantleStartPoint = GetOrigin();

	// Check if trace target has a mantleable surface
	bool b_canBeMantled = DetermineIfMantleTargetHasMantleableSurface
	(
		maxVerticalReachDistance,
		maxHorizontalReachDistance,
		in_targetTraceResult,
		out_mantleEndPoint
	);

	if (b_canBeMantled)
	{
		// Check if path to mantle end point is not blocked
		b_canBeMantled &= DetermineIfPathToMantleSurfaceIsPossible
		(
			maxVerticalReachDistance,
			maxHorizontalReachDistance,
			eyePos,
			mantleStartPoint,
			out_mantleEndPoint
		);

		if (b_canBeMantled)
		{
			// Is end point too far away?
			idVec3 endDistanceVector = out_mantleEndPoint - eyePos;
			float endDistance = endDistanceVector.Length();
			idVec3 upDistance = endDistanceVector;
			
			upDistance.x *= upVector.x;
			upDistance.y *= upVector.y;
			upDistance.z *= upVector.z;
			float upDist = upDistance.Length();

			float nonUpDist = idMath::Sqrt 
			(
				(endDistance * endDistance) -(upDist * upDist)
			);

			// Check the calculated distances
			if (upDist < 0.0)
			{
				DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING
				(
					"Mantleable surface was below player's feet. No belly slide allowed.\n"
				);
				b_canBeMantled = false;
			}
			else if
			(
				(upDist  > maxVerticalReachDistance) || 
				(nonUpDist > maxHorizontalReachDistance)
			)
			{
				// Its too far away either horizontally or vertically
				DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING
				(
					"Distance to end point was (%f, %f) (horizontal, vertical) which is greater than limits of (%f %f), so mantle cannot be done\n",
					upDist,
					nonUpDist,
					maxVerticalReachDistance,
					maxHorizontalReachDistance
				);

				b_canBeMantled = false;
			}

			// Distances are reasonable
		}
	}

	// Return result
	return b_canBeMantled;
}


//----------------------------------------------------------------------

void idPhysics_Player::PerformMantle()
{
	trace_t		trace;

	// Can't start mantle if already mantling
	if (m_mantlePhase != notMantling_DarkModMantlePhase)
	{
		return;
	}

	// Clear mantled entity members to indicate nothing is
	// being mantled
	m_p_mantledEntity = NULL;
	m_mantledEntityID = 0;

	// Forward vector is direction player is looking
	idVec3 forward = viewAngles.ToForward();
	forward.Normalize();

	// We use gravity alot here...
	idVec3 gravityNormal = GetGravityNormal();
	idVec3 upVector = -gravityNormal;

	// Get maximum reach distances for mantling
	float maxVerticalReachDistance; 
	float maxHorizontalReachDistance;
	float maxMantleTraceDistance;

	GetCurrentMantlingReachDistances
	(
		maxVerticalReachDistance,
		maxHorizontalReachDistance,
		maxMantleTraceDistance
	);

	// Get start position of gaze trace, which is player's eye position
	idVec3 eyePos;
	idPlayer* p_player = (idPlayer*) self;
	if (p_player == NULL)
	{
		DM_LOG(LC_MOVEMENT, LT_ERROR)LOGSTRING
		(
			"p_player is NULL\n"
		);
		return;
	}
	else
	{
		DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING ("Getting eye position\r\n");
		eyePos = p_player->GetEyePosition();
	}

	// Ishtvan: Do not attempt to mantle if holding an object
	if( g_Global.m_DarkModPlayer->grabber->GetSelected() )
		return;

	// Run mantle trace
	MantleTargetTrace
	(
		maxMantleTraceDistance,
		eyePos,
		forward,
		trace
	);

	// If the trace found a target, see if it is mantleable
	if ( trace.fraction < 1.0f ) 
	{
		// Log trace hit point
		DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING
		(
			"Mantle target trace collision point (%f %f %f)\n", 
			trace.c.point.x,
			trace.c.point.y,
			trace.c.point.z
		);

		// Find mantle end point and make sure mantle is
		// possible
		idVec3 mantleEndPoint;
		if (ComputeMantlePathForTarget
		(
			maxVerticalReachDistance,
			maxHorizontalReachDistance,
			eyePos,
			trace,
			mantleEndPoint
		))
		{
			// Log the end point
			DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING 
			(
				"Mantle end position = (%f %f %f)\n", 
				mantleEndPoint.x,
				mantleEndPoint.y,
				mantleEndPoint.z
			);

			// Start with log phase dependent on position relative
			// to the mantle end point
			if 
			(
				(mantleEndPoint * gravityNormal) < 
				(eyePos * gravityNormal)
			)
			{
				// Start with pull if on the ground, hang if not
				if (groundPlane)
				{
					StartMantle (pull_DarkModMantlePhase, eyePos, GetOrigin(), mantleEndPoint);
				}
				else
				{
					StartMantle (hang_DarkModMantlePhase, eyePos, GetOrigin(), mantleEndPoint);
				}
			}
			else
			{
				// We are above it, start with push
				StartMantle (push_DarkModMantlePhase, eyePos, GetOrigin(), mantleEndPoint);
			}

		} // Mantle target passed mantleability tests
	
	} // End mantle target found
}

//####################################################################
// End Mantle Mod
// SophisticatedZombie (DH)
//####################################################################

//####################################################################
// Start Leaning Mod
//	Zaccheus (some original geometric drawings)
//	SophsiticatedZombie (DH) 
//
//####################################################################



void idPhysics_Player::ToggleLean
(
	float leanYawAngleDegrees
)
{
	if (m_currentLeanTiltDegrees < 0.0001) // prevent floating point compare errors
	{
		// Start the lean
		m_leanMoveStartTilt = m_currentLeanTiltDegrees;
		m_leanMoveEndTilt = cv_pm_lean_angle.GetFloat();

		m_leanYawAngleDegrees = leanYawAngleDegrees;
		m_leanTime = cv_pm_lean_time.GetFloat();

		m_b_leanFinished = false;

		DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING ("ToggleLean staring lean\r\n");
	}
	else
	{
		// End the lean
		m_leanMoveStartTilt = m_currentLeanTiltDegrees;
		m_leanMoveEndTilt = 0.0;

		m_leanTime = cv_pm_lean_time.GetFloat();

		m_b_leanFinished = false;

		DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING ("ToggleLean ending lean\r\n");
	}

}

//----------------------------------------------------------------------

__inline bool idPhysics_Player::IsLeaning()
{
	if (m_currentLeanTiltDegrees < 0.001)
	{
		return false;
	}
	else
	{
		// entering, exiting, or holding lean
		return true;
	}
}	

//----------------------------------------------------------------------

idAngles idPhysics_Player::GetViewLeanAngles()
{
	return m_viewLeanAngles;
}

//----------------------------------------------------------------------

idVec3 idPhysics_Player::GetViewLeanTranslation()
{
	return m_viewLeanTranslation;
}

//----------------------------------------------------------------------

void idPhysics_Player::UpdateViewLeanAnglesAndTranslation
(
	float viewpointHeight,
	float distanceFromWaistToViewpoint
)
{

	// Set lean view angles
	float pitchAngle = m_currentLeanTiltDegrees;
	float rollAngle = pitchAngle;

	pitchAngle *= idMath::Sin(m_leanYawAngleDegrees * ((2.0 * idMath::PI) / 360.0) );
	rollAngle *= idMath::Cos(m_leanYawAngleDegrees * ((2.0 * idMath::PI) / 360.0) );
	
	m_viewLeanAngles.Set ( pitchAngle, 0.0, rollAngle);

	// Set lean translate vector
	
	m_viewLeanTranslation.x = distanceFromWaistToViewpoint * idMath::Sin (-pitchAngle * ((2.0 * idMath::PI) / 360.0) );
	m_viewLeanTranslation.y = distanceFromWaistToViewpoint * idMath::Sin(rollAngle * ((2.0 * idMath::PI) / 360.0) );
	m_viewLeanTranslation.z = 0.0;
	
	m_viewLeanTranslation.ProjectSelfOntoSphere
	(
		distanceFromWaistToViewpoint
	);

	m_viewLeanTranslation.z = m_viewLeanTranslation.z - distanceFromWaistToViewpoint;

	// Rotate to player's facing
	idMat4 rotMat = viewAngles.ToMat4();

	m_viewLeanTranslation *= rotMat;

	// Sign h4x0rx
	m_viewLeanTranslation.x = -m_viewLeanTranslation.x;
	m_viewLeanTranslation.y = -m_viewLeanTranslation.y;



}
//----------------------------------------------------------------------

void idPhysics_Player::TestForViewRotationBasedCollisions()
{
	// This checks to see if player view facing changes
	// cause a collision with another object in between render
	// frames when the player turns their body (triggered by turning viewpoint)

	idAngles deltaAngles;
	deltaAngles.Zero();

	if (self != NULL)
	{
		idPlayer* p_player = (idPlayer*) self;
		deltaAngles = p_player->viewAngles - m_lastPlayerViewAngles;
		m_lastPlayerViewAngles = p_player->viewAngles;

		// Only interested in yaw
		deltaAngles.pitch = 0.0;
		deltaAngles.roll = 0.0;
	}

	idRotation testRotation= deltaAngles.ToRotation();
	if (testRotation.GetAngle() != 0.0)
	{
		testRotation.SetOrigin (current.origin);

		trace_t rotationTraceResults;
		ClipRotation
		(
			rotationTraceResults,
			testRotation,
			NULL // We are not comparing against one specific clip model
		);

		if (rotationTraceResults.fraction < 1.0)
		{
			DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING
			(
				"Orientation change by delta angles pitch %e yaw %e roll %e\n",
				deltaAngles.pitch,
				deltaAngles.yaw,
				deltaAngles.roll
			);

			DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING
			(
				"Orientation change by %e degrees around axis %e %e %e yielded trace fraction %e",
				testRotation.GetVec()[0],
				testRotation.GetVec()[1],
				testRotation.GetVec()[2],
				testRotation.GetAngle(),
				rotationTraceResults.fraction
			);

			// Can't do it.
			// Must undo lean
			m_b_leanFinished = true;
			m_currentLeanTiltDegrees = 0.0;
			m_leanMoveStartTilt = 0.0;
			m_leanMoveEndTilt = 0.0;
			m_leanTime = 0.0;

			// Update player model
			LeanPlayerModelAtWaistJoint();

		}
	}


}
//----------------------------------------------------------------------

void idPhysics_Player::LeanPlayerModelAtWaistJoint()
{

	// Get player view height
	float playerViewHeight;
	if ( current.movementFlags & PMF_DUCKED )
	{
		playerViewHeight = pm_crouchviewheight.GetFloat();
	}
	else
	{
		playerViewHeight = pm_normalviewheight.GetFloat();
	}

	// Get the distance from the waist to the viewpoint
	// We set this to a little under half the model height in case
	// there is no model
	float distanceFromWaistToViewpoint = playerViewHeight * 0.6;

	// Use the idPlayer object's animator to get and change
	// the waist joint rotation in the player model skeleton.
	if (self != NULL)
	{
		idPlayer* p_player = (idPlayer*) self;

		// Get more accureate interpolated eye height used elsewhere since
		// we have the player
		playerViewHeight = p_player->EyeHeight();

		/**
		* Commented out 3rd person model joint rotation, since we now have lean animations
		* See previous CVS version for joint-rotating the code
		*	-Ishtvan
		**/

		float waistHeight = (playerViewHeight * 0.75);
		distanceFromWaistToViewpoint = playerViewHeight - waistHeight;

	} // Had access to player object
	else
	{
		DM_LOG(LC_MOVEMENT, LT_ERROR)LOGSTRING ("Cannot lean player at waist, as player has no idPlayer object assigned\n");
	}

	// Update view lean angles and translation for first person
	// view (and translation only for third person view)
	UpdateViewLeanAnglesAndTranslation 
	(
		playerViewHeight,
		distanceFromWaistToViewpoint	
	);
	

}

//----------------------------------------------------------------------

void idPhysics_Player::UpdateLeanAngle (float deltaLeanTiltDegrees)
{
	
	float newLeanTiltDegrees = 0.0;

	// What would the new lean angle be?
	newLeanTiltDegrees = m_currentLeanTiltDegrees + deltaLeanTiltDegrees;
	if (newLeanTiltDegrees < 0.0)
	{
		// Adjust delta
		deltaLeanTiltDegrees = 0.0 - m_currentLeanTiltDegrees;
		m_leanTime = 0.0;
		m_b_leanFinished = true;
	}
	else if (newLeanTiltDegrees > cv_pm_lean_angle.GetFloat())
	{
		// Adjust delta
		deltaLeanTiltDegrees = cv_pm_lean_angle.GetFloat() - m_currentLeanTiltDegrees;
		m_leanTime = 0.0;
		m_b_leanFinished = true;
	}

	// Log max possible lean
	DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING
	(
		"Currently leaning %.2f degrees, can lean up to %.2f more degrees this frame\n",
		m_currentLeanTiltDegrees,
		deltaLeanTiltDegrees
	);

	// Axis-Vector around which tilt takes place is the part of the
	// viewForward vector that is perpendicular to the gravity normal
	// rotated around the gravity normal by m_leanYawDegrees;
	idVec3 leanTiltAxis = viewForward;
	leanTiltAxis.ProjectAlongPlane (-GetGravity(), 0.0);
	leanTiltAxis.Normalize();

	idAngles leanYawOnly;
	leanYawOnly.yaw = m_leanYawAngleDegrees;
	leanYawOnly.pitch = 0.0;
	leanYawOnly.roll = 0.0;

	idMat3 leanTiltYawMat;
	leanTiltYawMat = leanYawOnly.ToMat3();

	leanTiltAxis *= leanTiltYawMat;
	leanTiltAxis.Normalize();

    DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING
	(
		"view forward axis (%.2f %.2f %.2f), leanTiltAxis is (%.2f %.2f %.2f)\n",
		viewForward.x,
		viewForward.y,
		viewForward.z,
		leanTiltAxis.x,
		leanTiltAxis.y,
		leanTiltAxis.z
	);


    // Test for collision, and adjust to less lean change 
	// if collision occurs during leaning move

	// Lift origin off of ground a little
	idVec3 originSave = current.origin;
	current.origin += -GetGravityNormal() * 0.5;
	clipModel->SetPosition (current.origin, clipModel->GetAxis());

	// build rotation
	idRotation clipRotation;
	clipRotation.Set
	(
		current.origin,
		leanTiltAxis,
		-deltaLeanTiltDegrees
	);

	// test rotation
	trace_t rotationTraceResults;
	ClipRotation
	(
		rotationTraceResults,
		clipRotation,
		NULL // We are not comparing against one specific clip model
	);
	
	// Log max possible lean with clipping
	DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING
	(
		"Rotation test around axis (%.2f %.2f %.2f) by %.2f degrees yielded trace fraction of %.2f\n",
		leanTiltAxis.x,
		leanTiltAxis.y,
		leanTiltAxis.z,
		deltaLeanTiltDegrees,
		rotationTraceResults.fraction
	);

	// Handle case of not able to rotate that far by adjusting tilt delta angle
	// to rotation at which collision stopped rotation
	if (rotationTraceResults.fraction < 1.0)
	{
		// Can't rotate
		current.origin = originSave;
		clipModel->SetPosition (current.origin, clipModel->GetAxis());

		// Lean is finished
		m_leanTime = 0.0;
		deltaLeanTiltDegrees *= rotationTraceResults.fraction;
		m_b_leanFinished = true;

	}
		

	// Adjust lean angle by delta which was allowed
	m_currentLeanTiltDegrees += deltaLeanTiltDegrees;

	// Bend at waist in player skeleton
	LeanPlayerModelAtWaistJoint();

	// Make sure player didn't hit head
	TestForViewRotationBasedCollisions();

	// To remove "bump up" required for rotation test, translate
	// player downward from the bumped up origin toward the new one
	// as far as they can go before hitting the ground
	trace_t regroundTrace;
	idVec3 regroundTranslateVector = originSave - current.origin;

	ClipTranslation 
	(
		regroundTrace, 
		regroundTranslateVector, 
		NULL // Not comparing against one specific clip model
	);

	// New origin is point of just barely touching the ground
	idVec3 regroundedOrigin = current.origin + (regroundTranslateVector * regroundTrace.fraction); 
	DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING
	(
		"Player regrounded after lean collision test trace fraction of %.3f\n", 
		regroundTrace.fraction
	);
	DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING
	(
		"Pre rotation test origin %.2f %.2f %.2f\n rotation test origin was %.2f %.2f %.2f\n regrounded at %.2f %.2f %.2f\n current velocity is %.4f %.4f %.4f\n", 
		originSave.x,
		originSave.y,
		originSave.z,
		current.origin.x,
		current.origin.y,
		current.origin.z,
		regroundedOrigin.x,
		regroundedOrigin.y,
		regroundedOrigin.z,
		current.velocity.x,
		current.velocity.y,
		current.velocity.z
	);

	// Move player to regrounded origin
	current.origin = regroundedOrigin;
	clipModel->SetPosition (current.origin, clipModel->GetAxis());

		
	// Log activity
	DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING
	(
		"Lean tilt is now %.2f degrees\n",
		m_currentLeanTiltDegrees
	);

}

//----------------------------------------------------------------------

void idPhysics_Player::LeanMove()
{

	// Change in lean tilt this frame
	float deltaLeanTiltDegrees = 0.0;

	if ( !m_b_leanFinished) 
	{

		// Update lean time
		m_leanTime -= framemsec;
		if (m_leanTime <= 0.0)
		{
			m_leanTime = 0.0;
			m_b_leanFinished = true;
		}

		// Try sinusoidal movement
		float timeRatio = 0.0;
		timeRatio = ( cv_pm_lean_time.GetFloat() - m_leanTime) /  cv_pm_lean_time.GetFloat();

		float timeRadians = (idMath::PI/2.0f) * timeRatio;
		
		float newLeanTiltDegrees = 0.0;
		
		if (m_leanMoveEndTilt > m_leanMoveStartTilt)
		{
			newLeanTiltDegrees = (idMath::Sin(timeRadians) * (m_leanMoveEndTilt - m_leanMoveStartTilt))
			 + m_leanMoveStartTilt;
		}
		else if (m_leanMoveStartTilt > m_leanMoveEndTilt)
		{
			newLeanTiltDegrees = m_leanMoveStartTilt - (idMath::Sin(timeRadians) * (m_leanMoveStartTilt - m_leanMoveEndTilt));
		}

		deltaLeanTiltDegrees = newLeanTiltDegrees - m_currentLeanTiltDegrees;

	}

	// Perform any change to leaning
	if (deltaLeanTiltDegrees != 0.0)
	{
		// Re-orient clip model before change so that collision tests
		// are accurate (player may have rotated mid-lean)
		UpdateLeanAngle (deltaLeanTiltDegrees);
	}
	else
	{
		// Update player model for smooth animation 
		// incase player is entering or exiting a crouch
		LeanPlayerModelAtWaistJoint();
	}


}

void idPhysics_Player::RopeRemovalCleanup( idEntity *RopeEnt )
{
	if( RopeEnt && m_RopeEntity && m_RopeEntity == RopeEnt )
		m_RopeEntity = NULL;
	if( RopeEnt && m_RopeEntTouched && m_RopeEntTouched == RopeEnt )
		m_RopeEntTouched = NULL;
}

float idPhysics_Player::GetDeltaViewYaw( void )
{
	return m_DeltaViewYaw;
}
