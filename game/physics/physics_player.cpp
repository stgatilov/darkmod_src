/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.49  2007/01/23 01:24:07  thelvyn
 * Fixed a minor bug and cleaned up most of the warnings
 *
 * Revision 1.48  2007/01/21 12:58:34  ishtvan
 * rope arrow: rope segment vertical velocity now added to player's vertical velocity
 *
 * Revision 1.47  2007/01/21 12:23:18  ishtvan
 * removed lean debug output to console accidentally left in
 *
 * Revision 1.46  2007/01/21 11:15:51  ishtvan
 * listening thru doors when leaning against them implemented
 *
 * Revision 1.45  2007/01/21 02:10:13  ishtvan
 * updates in collision detection and actions taken as a result
 *
 * rewriting
 *
 * Revision 1.44  2007/01/09 12:57:58  ishtvan
 * *) lean collision test bugfixes
 *
 * *) view rotation while leaned fixed (m_leanTranslation now stored in local coordinates and multiplied by viewAngles minus pitch when needed)
 *
 * *) forward leaning fixed
 *
 * *) separate cvars for forward leaning
 *
 * Revision 1.43  2007/01/08 12:42:41  ishtvan
 * leaning collision test updates
 *
 * Revision 1.42  2007/01/03 04:17:16  ishtvan
 * player pushing objects now checks cv_pm_pushmod cvar to modify the push impulse, for physics tweaking
 *
 * Revision 1.41  2006/12/07 09:58:12  ishtvan
 * lean updates
 *
 * Revision 1.40  2006/12/04 00:32:15  ishtvan
 * *) added stretching of body to lean
 *
 * *) Leaning now checks cvars instead of ini file vars
 *
 * *) Disabled manual tilting of model in lean code (NOTE: Lean collision test does not work, WIP at the moment)
 *
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

#include "../game_local.h"
#include "../DarkMod/DarkModGlobals.h"
#include "../DarkMod/PlayerData.h"
#include "../DarkMod/BinaryFrobMover.h"
#include "../DarkMod/FrobDoor.h"

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

/**
* When moving on a climbable surface, the player's predicted position is checked by this delta ahead
* To make sure it's still on a ladder surface.
**/
const float CLIMB_SURFCHECK_DELTA = 5.0f;

/**
* How far to check from the player origin along the surface normal to hit the surface in the above test
* Needs to allow for worst case overhang
**/
const float CLIMB_SURFCHECK_NORMDELTA = 20.0f;

/**
* How far the edge of the player clipbox is away from the ladder
**/
const float LADDER_DISTANCE = 10.0f;

/**
* how far away is the player allowed to push out from the climbable section?
* Measured from the last good attachment point at the origin
**/
const float LADDER_DISTAWAY = 15.0f;

/**
* Velocity with which the player is shoved over the top of the ladder
* This depends on LADDER_DISTAWAY.  If this velocity is too low, the player will
* fall down before they can make it over the top of the ladder
**/
const float LADDER_TOPVELOCITY = 80.0f;

/**
* Angle at which the player detaches from a ladder when their feet are walking on a surface
**/
const float LADDER_WALKDETACH_ANGLE = 45.0f;
const float LADDER_WALKDETACH_DOT = idMath::Cos( DEG2RAD( LADDER_WALKDETACH_ANGLE ) );

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
		if ( stepUp ) 
		{

			nearGround = groundPlane || m_bOnClimb || m_bClimbDetachThisFrame;

			if ( !nearGround ) 
			{
				// trace down to see if the player is near the ground
				// step checking when near the ground allows the player to move up stairs smoothly while jumping
				stepEnd = current.origin + maxStepHeight * gravityNormal;
				gameLocal.clip.Translation( downTrace, current.origin, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );
				nearGround = ( downTrace.fraction < 1.0f && (downTrace.c.normal * -gravityNormal) > MIN_WALK_NORMAL );
			}

			// may only step up if near the ground or climbing
			if ( nearGround ) 
			{

				// step up
				stepEnd = current.origin - maxStepHeight * gravityNormal;

				gameLocal.clip.Translation( downTrace, current.origin, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );

				// trace along velocity
				stepEnd = downTrace.endpos + time_left * current.velocity;

				gameLocal.clip.Translation( stepTrace, downTrace.endpos, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );

				// step down
				stepEnd = stepTrace.endpos + maxStepHeight * gravityNormal;
				gameLocal.clip.Translation( downTrace, stepTrace.endpos, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );

				if ( downTrace.fraction >= 1.0f || (downTrace.c.normal * -gravityNormal) > MIN_WALK_NORMAL ) 
				{

					// if moved the entire distance
					if ( stepTrace.fraction >= 1.0f ) 
					{
						time_left = 0;
						current.stepUp -= ( downTrace.endpos - current.origin ) * gravityNormal;
						current.origin = downTrace.endpos;
						current.movementFlags |= PMF_STEPPED_UP;
						current.velocity *= PM_STEPSCALE;
						break;
					}

					// if the move is further when stepping up
					if ( stepTrace.fraction > trace.fraction ) 
					{
						time_left -= time_left * stepTrace.fraction;
						current.stepUp -= ( downTrace.endpos - current.origin ) * gravityNormal;
						current.origin = downTrace.endpos;
						current.movementFlags |= PMF_STEPPED_UP;
						current.velocity *= PM_STEPSCALE;
						trace = stepTrace;
						stepped = true;
					}
				}

				DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING ("performing step up, velocity now %.4f %.4f %.4f\n",  current.velocity.x, current.velocity.y, current.velocity.z);
			}
		}

		// if we can push other entities and not blocked by the world
		if ( push && trace.c.entityNum != ENTITYNUM_WORLD ) {

			clipModel->SetPosition( current.origin, clipModel->GetAxis() );

			// clip movement, only push idMoveables, don't push entities the player is standing on
			// apply impact to pushed objects
			pushFlags = PUSHFL_CLIP|PUSHFL_ONLYMOVEABLE|PUSHFL_NOGROUNDENTITIES|PUSHFL_APPLYIMPULSE;

			// clip & push
			totalMass = gameLocal.push.ClipTranslationalPush( trace, self, pushFlags, end, end - current.origin, cv_pm_pushmod.GetFloat() );

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
		if ( downTrace.fraction > 1e-4f && downTrace.fraction < 1.0f ) 
		{
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
void idPhysics_Player::WalkMove( void ) 
{
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
#pragma warning( disable : 4533 )
void idPhysics_Player::RopeMove( void ) 
{
	idVec3	wishdir, wishvel, right, ropePoint, offset, newOrigin;
	float	wishspeed, scale, temp, deltaYaw, deltaAng1, deltaAng2;
	float	upscale, ropeTop, ropeBot; // z coordinates of the top and bottom of rope
	idBounds ropeBounds;
	trace_t transTrace; // used for clipping tests when moving the player
	idVec3 transVec, forward, playerVel(0,0,0), PlayerPoint(0,0,0);
	int bodID(0);

	if( !m_RopeEntity.GetEntity() )
	{
		RopeDetach();
#ifdef __linux__
		return;
#else
		goto Quit;
#endif
	}

	// store and kill the player's transverse velocity
	playerVel = current.velocity;
	current.velocity.x = 0;
	current.velocity.y = 0;

	// stick the player to the rope at an AF origin point closest to their arms
	PlayerPoint = current.origin + -gravityNormal*ROPE_GRABHEIGHT;
	ropePoint = static_cast<idPhysics_AF *>(m_RopeEntity.GetEntity()->GetPhysics())->NearestBodyOrig( PlayerPoint, &bodID );
	
	// apply the player's weight to the AF body - COMMENTED OUT DUE TO AF CRAZINESS
//	static_cast<idPhysics_AF *>(m_RopeEntity.GetEntity()->GetPhysics())->AddForce(bodID, ropePoint, mass * gravityVector );

	// if the player has hit the rope this frame, apply an impulse based on their velocity
	// pretend the deceleration takes place over a number of frames for realism (100 ms?)
	if( m_bJustHitRope )
	{
		m_bJustHitRope = false;

		idVec3 vImpulse(playerVel.x, playerVel.y, 0);
		vImpulse *= mass;

		static_cast<idPhysics_AF *>(m_RopeEntity.GetEntity()->GetPhysics())->AddForce( bodID, ropePoint, vImpulse/0.1f );
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
	ropeBounds = m_RopeEntity.GetEntity()->GetPhysics()->GetAbsBounds();
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
	idRotation rotateView( vec3_origin, -gravityNormal, -deltaYaw );
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

	// Add in the z velocity of the rope segment they're clinging to
	current.velocity += gravityNormal * (gravityNormal * m_RopeEntity.GetEntity()->GetPhysics()->GetLinearVelocity( bodID )); 

	// slide the player up and down with their calculated velocity
	idPhysics_Player::SlideMove( false, ( command.forwardmove > 0 ), false, false );

Quit:
	return;
}
#pragma warning( default : 4533 )

/*
============
idPhysics_Player::RopeDetach
============
*/
void idPhysics_Player::RopeDetach( void ) 
{
		m_bOnRope = false;

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
idPhysics_Player::ClimbDetach
============
*/
void idPhysics_Player::ClimbDetach( bool bStepUp ) 
{
		m_bOnClimb = false;
		m_ClimbingOnEnt = NULL;
		m_bClimbDetachThisFrame = true;

		static_cast<idPlayer *>(self)->RaiseWeapon();
		static_cast<idPlayer *>(self)->hiddenWeapon = false;

		// switch movement modes to the appropriate one
		if( bStepUp )
		{
			idVec3 ClimbNormXY = m_vClimbNormal - (gravityNormal * m_vClimbNormal) * gravityNormal;
			ClimbNormXY.Normalize();
			current.velocity += -ClimbNormXY * LADDER_TOPVELOCITY;
			idPhysics_Player::SlideMove( false, true, false, true );
		}
		else if ( waterLevel > WATERLEVEL_FEET ) 
		{
			idPhysics_Player::WaterMove();
		}
		else 
		{
			// TODO: This doesn't work to step up at the end of a ladder
			idPhysics_Player::AirMove();
		}
}

/*
============
idPhysics_Player::LadderMove
============
*/
void idPhysics_Player::LadderMove( void ) 
{
	idVec3	wishdir( vec3_zero ), wishvel( vec3_zero ), right( vec3_zero );
	idVec3  dir( vec3_zero ), start( vec3_zero ), end( vec3_zero ), delta( vec3_zero );
	idVec3	AttachVel( vec3_zero ), RefFrameVel( vec3_zero );
	idVec3	vReqVert( vec3_zero ), vReqHoriz( vec3_zero ), vHorizVect( vec3_zero );
	float	wishspeed(0.0f), scale(0.0f), accel(0.0f);
	float	upscale(0.0f), horizscale(0.0f), NormalDot(0.0f);
	trace_t SurfTrace;
	bool	bMoveAllowed( true );

	accel = PM_ACCELERATE;

	idVec3 ClimbNormXY = m_vClimbNormal - (m_vClimbNormal * gravityNormal) * gravityNormal;
	ClimbNormXY.Normalize();

	// jump off the climbable surface if they jump, or fall off if they hit crouch
	if ( idPhysics_Player::CheckRopeJump() || command.upmove < 0 ) 
	{
		ClimbDetach();
#ifdef __linux__
		return;
#else
		goto Quit;
#endif
	}

	NormalDot = ClimbNormXY * viewForward;
	// detach if their feet are on the ground walking away from the surface
	if ( walking && -NormalDot * command.forwardmove < LADDER_WALKDETACH_DOT )
	{
		ClimbDetach();
#ifdef __linux__
		return;
#else
		goto Quit;
#endif
	}

	// Add the velocity of whatever entity they're climbing on:
	// TODO: ADD REF FRAME ANGULAR VELOCITY!!
	if( m_ClimbingOnEnt.GetEntity() )
	{
		idEntity *ent = m_ClimbingOnEnt.GetEntity();
		if( ent->GetBindMaster() )
			ent = ent->GetBindMaster();

		if( ent->GetPhysics() )
		{
			DM_LOG(LC_MOVEMENT,LT_DEBUG)LOGSTRING("Adding ref frame velocity %s for entity %s \r", RefFrameVel.ToString(), ent->name.c_str() );
			RefFrameVel = ent->GetPhysics()->GetLinearVelocity();
			//RefFrameVel += ent->GetPhysics()->GetPushedLinearVelocity();
		}
	}
	DM_LOG(LC_MOVEMENT,LT_DEBUG)LOGSTRING("Climb ref frame velocity = %s \r", RefFrameVel.ToString() );

	// ====================== stick to the ladder ========================
	// Do a trace to figure out where to attach the player:
	start = current.origin;
	end = start - 48.0f * ClimbNormXY;
	gameLocal.clip.Translation( SurfTrace, start, end, clipModel, clipModel->GetAxis(), clipMask, self );

	// if there is a climbable surface in front of the player, stick to it
	if( SurfTrace.fraction != 1.0f && SurfTrace.c.material 
		&& (SurfTrace.c.material->GetSurfaceFlags() & SURF_LADDER ) )
	{
		m_vClimbPoint = SurfTrace.endpos + LADDER_DISTANCE * ClimbNormXY;
		AttachVel = 10 * (m_vClimbPoint - current.origin);

		// Now that we have a valid point, don't need to use the initial one
		m_bClimbInitialPhase = false;

		// Update sounds and movement speed caps for the surface if we change surfaces
		idStr SurfName;
		g_Global.GetSurfName( SurfTrace.c.material, SurfName );
		if( SurfName != m_ClimbSurfName )
		{
			idStr LookUpName, TempStr;
			idKeyValue *kv = NULL;
			
			m_ClimbSurfName = SurfName;

			LookUpName = "climb_max_speed_vert_";
			TempStr = LookUpName + SurfName;
			if( ( kv = const_cast<idKeyValue *>( self->spawnArgs.FindKey(LookUpName.c_str())) ) != NULL )
				m_ClimbMaxVelVert = atof( kv->GetValue().c_str() );
			else
			{
				TempStr = LookUpName + "default";
				m_ClimbMaxVelVert = self->spawnArgs.GetFloat( LookUpName.c_str(), "1.0" );
			}

			LookUpName = "climb_max_speed_horiz_";
			TempStr = LookUpName + SurfName;
			if( ( kv = const_cast<idKeyValue *>( self->spawnArgs.FindKey(LookUpName.c_str())) ) != NULL )
				m_ClimbMaxVelHoriz = atof( kv->GetValue().c_str() );
			else
			{
				TempStr = LookUpName + "default";
				m_ClimbMaxVelHoriz = self->spawnArgs.GetFloat( LookUpName.c_str(), "2.3" );
			}

			// sound repitition distances
			LookUpName = "climb_snd_repdist_vert_";
			TempStr = LookUpName + SurfName;
			if( ( kv = const_cast<idKeyValue *>( self->spawnArgs.FindKey(LookUpName.c_str())) ) != NULL )
				m_ClimbSndRepDistVert = atoi( kv->GetValue().c_str() );
			else
			{
				TempStr = LookUpName + "default";
				m_ClimbSndRepDistVert = self->spawnArgs.GetInt( LookUpName.c_str(), "32" );
			}

			// sound repitition distances
			LookUpName = "climb_snd_repdist_horiz_";
			TempStr = LookUpName + SurfName;
			if( ( kv = const_cast<idKeyValue *>( self->spawnArgs.FindKey(LookUpName.c_str())) ) != NULL )
				m_ClimbSndRepDistHoriz = atoi( kv->GetValue().c_str() );
			else
			{
				TempStr = LookUpName + "default";
				m_ClimbSndRepDistHoriz = self->spawnArgs.GetInt( LookUpName.c_str(), "32" );
			}
		}
	}
	else if( m_bClimbInitialPhase )
	{
		// We should already have m_vClimbPoint stored from the initial trace
		AttachVel = 12.0f * (m_vClimbPoint - current.origin);
	}

	current.velocity = (gravityNormal * current.velocity) * gravityNormal + AttachVel;

	scale = idPhysics_Player::CmdScale( command );

	float lenVert = viewForward * -gravityNormal;
	float lenTransv = idMath::Sqrt( 1.0f - NormalDot * NormalDot - lenVert * lenVert );
	// Dump everything that's not in the transverse direction into the vertical direction
	float lenVert2 = idMath::Sqrt( 1.0f - lenTransv * lenTransv );

	// resolve up/down, with some tolerance so player can still go up looking slightly down
	if( lenVert < -0.3 )
		lenVert2 = -lenVert2;

	vReqVert = lenVert2 * -gravityNormal * scale * (float)command.forwardmove;
	vReqVert *= m_ClimbMaxVelVert;

	// obtain the horizontal direction
	vReqHoriz = viewForward - (ClimbNormXY * viewForward) * ClimbNormXY;
	vReqHoriz -= (vReqHoriz * gravityNormal) * gravityNormal;
	vReqHoriz.Normalize();
	vReqHoriz *= lenTransv * scale * (float)command.forwardmove;
	vReqHoriz *= m_ClimbMaxVelHoriz;


	// Pure horizontal motion if looking close enough to horizontal:
	if( lenTransv > 0.906 )
		wishvel = vReqHoriz;
	else
		wishvel = vReqVert + vReqHoriz;
	
	// strafe
	if ( command.rightmove ) 
	{
		// right vector orthogonal to gravity
		right = viewRight - (gravityNormal * viewRight) * gravityNormal;
		// project right vector into ladder plane
		right = right - (ClimbNormXY * right) * ClimbNormXY;
		right.Normalize();

		wishvel += m_ClimbMaxVelHoriz * right * scale * (float) command.rightmove;
	}

	// ========================== Surface Extent Test ======================
	// This now just checks distance from the last valid climbing point
	dir = wishvel;
	dir.Normalize();

	end = start + wishvel * frametime + dir * CLIMB_SURFCHECK_DELTA;
	delta = m_vClimbPoint - end;
	if( delta.LengthSqr() > LADDER_DISTAWAY * LADDER_DISTAWAY )
		bMoveAllowed = false;

	if( !bMoveAllowed )
	{
		// If we were trying to go up and reached the extent, attempt to step off the ladder
		// Make sure we are really trying to go up, not first going off to the side and then up
		// TODO: Tweak this delta.lengthsqr parameter of 25.0, only measure in the horizontal axis?
		delta = current.origin - m_vClimbPoint;
		delta -= (delta * gravityNormal) * gravityNormal;

		if( NormalDot < 0.0f && -wishvel * gravityNormal > 0 && delta.LengthSqr() < 25.0f )
		{
			ClimbDetach( true );
#ifdef __linux__
		return;
#else
		goto Quit;
#endif
		}

		accel = idMath::INFINITY;
		//wishvel = RefFrameVel;
		wishvel = vec3_zero;
	}

	// ========================== End Surface Extent Test ==================

	// do strafe friction
	idPhysics_Player::Friction();

	// accelerate
	wishspeed = wishvel.Normalize();
	idPhysics_Player::Accelerate( wishvel, wishspeed, accel );

	// cap the vertical travel velocity
	upscale = current.velocity * -gravityNormal;
	if ( upscale < -m_ClimbMaxVelVert * playerSpeed )
		current.velocity += gravityNormal * (upscale + m_ClimbMaxVelVert * playerSpeed );
	else if ( upscale > m_ClimbMaxVelVert * playerSpeed  )
		current.velocity += gravityNormal * (upscale - m_ClimbMaxVelVert * playerSpeed );

	// cap the horizontal travel velocity
	vHorizVect = current.velocity - (current.velocity * gravityNormal) * gravityNormal;
	horizscale = vHorizVect.Normalize();
	float horizDelta = horizscale;
	horizscale = idMath::ClampFloat( -m_ClimbMaxVelHoriz * playerSpeed, m_ClimbMaxVelHoriz * playerSpeed, horizscale );
	horizDelta -= horizscale;
	current.velocity -= vHorizVect * horizDelta;
	
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
		else {
			current.velocity -= gravityVector * frametime;
			if ( current.velocity * gravityNormal < 0.0f ) 
			{
				current.velocity -= (gravityNormal * current.velocity) * gravityNormal;
			}
		}
	}

	current.velocity += RefFrameVel;
	
	idPhysics_Player::SlideMove( false, ( command.forwardmove > 0 ), false, false );

Quit:
	return;
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
		if ( command.upmove < 0 && !m_bOnClimb && !m_bClimbableAhead && !m_bOnRope ) {
			// duck
			current.movementFlags |= PMF_DUCKED;
		}
		else if (!IsMantling()) // MantleMod: SophisticatedZombie (DH): Don't stand up if crouch during mantle
		{
			// stand up if possible
			if ( current.movementFlags & PMF_DUCKED ) 
			{
				// try to stand up
				end = current.origin - ( pm_normalheight.GetFloat() - pm_crouchheight.GetFloat() ) * gravityNormal;
				gameLocal.clip.Translation( trace, current.origin, end, clipModel, clipModel->GetAxis(), clipMask, self );
				if ( trace.fraction >= 1.0f )
					current.movementFlags &= ~PMF_DUCKED;
			}
		}

		if ( current.movementFlags & PMF_DUCKED ) 
		{
			playerSpeed = crouchSpeed;
			maxZ = pm_crouchheight.GetFloat();
		} else 
		{
			maxZ = pm_normalheight.GetFloat();
		}
	}
	// if the clipModel height should change
	if ( clipModel->GetBounds()[1][2] != maxZ ) 
	{
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
idPhysics_Player::CheckClimbable
DarkMod: Checks ropes, ladders and other climbables
================
*/
void idPhysics_Player::CheckClimbable( void ) 
{
	idVec3		forward, start, end, delta;
	trace_t		trace;
	float		tracedist, angleOff, dist, lookUpAng;
	bool		bLookingUp;
	idEntity    *testEnt;
	
	if( current.movementTime ) 
		goto Quit;

	// if on the ground moving backwards
	if( walking && command.forwardmove <= 0 ) 
		goto Quit;

	// Don't attach to ropes or ladders in the middle of a mantle
	if ( IsMantling() )
		goto Quit;

	// Don't attach if we are holding an object in our hands
	if( g_Global.m_DarkModPlayer->grabber->GetSelected() != NULL )
		goto Quit;

	// forward vector orthogonal to gravity
	forward = viewForward - (gravityNormal * viewForward) * gravityNormal;
	forward.Normalize();

	if ( walking ) 
	{
		// don't want to get sucked towards the ladder when still walking or when climbing
		tracedist = 1.0f;
	} 
	else 
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
		if( testEnt && testEnt->m_bIsClimbableRope )
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
				!m_bOnRope
				&& ( (trace.endpos - current.origin).Length() <= 2.0f )
				&& !groundPlane
				&& angleOff >= idMath::Cos( ROPE_ATTACHANGLE )
				&& (testEnt != m_RopeEntity.GetEntity() || gameLocal.time - m_RopeDetachTimer > ROPE_REATTACHTIME)
				)
			{
				// make sure rope segment is not touching the ground
				int bodyID = m_RopeEntTouched.GetEntity()->BodyForClipModelId( trace.c.id );
				if( !static_cast<idPhysics_AF *>(m_RopeEntTouched.GetEntity()->GetPhysics())->HasGroundContacts( bodyID ) )
				{
					m_bRopeContact = true;
					m_bJustHitRope = true;
					m_RopeEntity = static_cast<idAFEntity_Base *>(testEnt);

					goto Quit;
				}
			}
		}

		// if a climbable surface
		if ( 
			trace.c.material 
			&& ( trace.c.material->GetSurfaceFlags() & SURF_LADDER )
			) 
		{
			idVec3 vStickPoint = trace.endpos;
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
					m_vClimbNormal = trace.c.normal;
					m_ClimbingOnEnt = gameLocal.entities[ trace.c.entityNum ];
					
					// FIX: Used to get stuck hovering in some cases, now there's an initial phase
					if( !m_bOnClimb )
					{
						m_bClimbInitialPhase = true;
						m_vClimbPoint = vStickPoint;
					}

					m_bClimbableAhead = true;
					m_bOnClimb = true;					

					goto Quit;
				}
			}
		}
	}

	// Rope attachment failsafe: Check intersection with the rope as well
	if 
		( 
			!m_bOnRope 
			&& m_RopeEntTouched.GetEntity() != NULL
			&& m_RopeEntTouched.GetEntity()->GetPhysics()->GetAbsBounds().IntersectsBounds( self->GetPhysics()->GetAbsBounds() )
			&& !groundPlane
		)
	{
		// test distance against the nearest rope body
		int touchedBody = -1;
		idVec3 PlayerPoint = current.origin + -gravityNormal*ROPE_GRABHEIGHT;
		idVec3 RopeSegPoint = static_cast<idPhysics_AF *>(m_RopeEntTouched.GetEntity()->GetPhysics())->NearestBodyOrig( PlayerPoint, &touchedBody );

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
				&& (m_RopeEntTouched.GetEntity() != m_RopeEntity.GetEntity() || gameLocal.time - m_RopeDetachTimer > ROPE_REATTACHTIME)
				&& !static_cast<idPhysics_AF *>(m_RopeEntTouched.GetEntity()->GetPhysics())->HasGroundContacts( touchedBody )
			)
		{
				m_bRopeContact = true;
				m_bJustHitRope = true;
				m_RopeEntity = m_RopeEntTouched.GetEntity();
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
bool idPhysics_Player::CheckRopeJump( void ) 
{
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

// TODO: Make this an adjustable cvar, currently too high?
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
	m_bClimbableAhead = false;
	m_bClimbDetachThisFrame = false;

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

	// check if a ladder or a rope is straight ahead
	idPhysics_Player::CheckClimbable();

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
	else if ( m_bOnRope )
	{
		idPhysics_Player::RopeMove();
	}
	else if ( m_bRopeContact ) 
	{
		// toggle m_bOnRope
		m_bOnRope = true;

		// lower weapon
		static_cast<idPlayer *>(self)->LowerWeapon();
		static_cast<idPlayer *>(self)->hiddenWeapon = true;

		idPhysics_Player::RopeMove();
	}
	else if ( m_bOnClimb ) 
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
	if( m_mantlePhase == notMantling_DarkModMantlePhase && waterLevel <= 1 && !m_bOnRope )
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
	return m_bOnRope;
}


/*
================
idPhysics_Player::OnLadder
================
*/
bool idPhysics_Player::OnLadder( void ) const {
	return m_bOnClimb;
}

/*
================
idPhysics_Player::idPhysics_Player
================
*/
idPhysics_Player::idPhysics_Player( void ) 
{
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
	m_bOnRope = false;
	m_bJustHitRope = false;
	m_RopeEntity = NULL;
	m_RopeEntTouched = NULL;
	m_RopeDetachTimer = 0;
	m_lastCommandViewYaw = 0;

	// wall/ladder climbing
	m_bClimbableAhead = false;
	m_bOnClimb = false;
	m_bClimbDetachThisFrame = false;
	m_bClimbInitialPhase = false;
	m_vClimbNormal.Zero();
	m_vClimbPoint.Zero();
	m_ClimbingOnEnt = NULL;
	m_ClimbSurfName.Clear();
	m_ClimbMaxVelHoriz = 0.0f;
	m_ClimbMaxVelVert = 0.0f;
	m_ClimbSndRepDistVert = 0;
	m_ClimbSndRepDistHoriz = 0;

	// swimming
	waterLevel = WATERLEVEL_NONE;
	waterType = 0;

	// Mantle Mod
	m_mantlePhase = notMantling_DarkModMantlePhase;
	m_mantleTime = 0.0;
	m_p_mantledEntity = NULL;
	m_mantledEntityID = 0;
	m_jumpHeldDownTime = 0.0;

	// Leaning Mod
	m_bIsLeaning = false;
	m_leanYawAngleDegrees = 0.0;
	m_CurrentLeanTiltDegrees = 0.0;
	m_CurrentLeanStretch = 0.0;
	m_b_leanFinished = true;
	m_leanMoveStartTilt = 0.0;
	m_leanMoveEndTilt = 0.0;
	m_leanMoveMaxAngle = 0.0;
	m_leanMoveMaxStretch = 0.0;

	m_viewLeanAngles = ang_zero;
	m_viewLeanTranslation = vec3_zero;

	m_LeanDoorListenPos = vec3_zero;
	m_LeanDoorEnt = NULL;

	m_DeltaViewYaw = 0.0;

	// Initialize lean view bounds used for collision
	m_LeanViewBounds.Zero();
	m_LeanViewBounds.ExpandSelf( 4.0f );
	// bounds extend downwards so that player can't lean over very high ledges
	idVec3 lowerPoint(0,0,-15.0f);
	m_LeanViewBounds.AddPoint( lowerPoint );
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
	savefile->WriteBool( m_bOnRope );
	savefile->WriteInt( m_RopeDetachTimer );
	m_RopeEntity.Save( savefile );
	m_RopeEntTouched.Save( savefile );

	savefile->WriteBool( m_bClimbableAhead );
	savefile->WriteBool( m_bOnClimb );
	savefile->WriteBool( m_bClimbDetachThisFrame );
	savefile->WriteBool( m_bClimbInitialPhase );
	savefile->WriteVec3( m_vClimbNormal );
	savefile->WriteVec3( m_vClimbPoint );
	savefile->WriteString( m_ClimbSurfName.c_str() );
	savefile->WriteFloat( m_ClimbMaxVelHoriz );
	savefile->WriteFloat( m_ClimbMaxVelVert );
	savefile->WriteInt( m_ClimbSndRepDistVert );
	savefile->WriteInt( m_ClimbSndRepDistHoriz );
	m_ClimbingOnEnt.Save( savefile );

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
	savefile->WriteFloat (m_CurrentLeanTiltDegrees);
	savefile->WriteFloat (m_CurrentLeanStretch);
	savefile->WriteFloat (m_leanMoveStartTilt);
	savefile->WriteFloat (m_leanMoveEndTilt);
	savefile->WriteFloat (m_leanMoveMaxAngle);
	savefile->WriteFloat (m_leanMoveMaxStretch);
	savefile->WriteBool (m_b_leanFinished);
	savefile->WriteFloat (m_leanTime);
	savefile->WriteAngles (m_lastPlayerViewAngles);
	savefile->WriteAngles (m_viewLeanAngles);
	savefile->WriteVec3 (m_viewLeanTranslation);
	savefile->WriteVec3 (m_LeanDoorListenPos);
	m_LeanDoorEnt.Save( savefile );



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
	savefile->ReadBool( m_bOnRope );
	savefile->ReadInt( m_RopeDetachTimer );
	m_RopeEntity.Restore( savefile );
	m_RopeEntTouched.Restore( savefile );
	// Angle storage vars need to be reset on a restore, since D3 resets the command angle to 0
	m_lastCommandViewYaw = 0.0f;
	m_DeltaViewYaw = 0.0f;

	savefile->ReadBool( m_bClimbableAhead );
	savefile->ReadBool( m_bOnClimb );
	savefile->ReadBool( m_bClimbDetachThisFrame );
	savefile->ReadBool( m_bClimbInitialPhase );
	savefile->ReadVec3( m_vClimbNormal );
	savefile->ReadVec3( m_vClimbPoint );
	savefile->ReadString( m_ClimbSurfName );
	savefile->ReadFloat( m_ClimbMaxVelHoriz );
	savefile->ReadFloat( m_ClimbMaxVelVert );
	savefile->ReadInt( m_ClimbSndRepDistVert );
	savefile->ReadInt( m_ClimbSndRepDistHoriz );
	m_ClimbingOnEnt.Restore( savefile );

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
	savefile->ReadFloat (m_CurrentLeanTiltDegrees);
	savefile->ReadFloat (m_CurrentLeanStretch);
	savefile->ReadFloat (m_leanMoveStartTilt);
	savefile->ReadFloat (m_leanMoveEndTilt);
	savefile->ReadFloat (m_leanMoveMaxAngle);
	savefile->ReadFloat (m_leanMoveMaxStretch);
	savefile->ReadBool (m_b_leanFinished);
	savefile->ReadFloat (m_leanTime);
	savefile->ReadAngles (m_lastPlayerViewAngles);
	savefile->ReadAngles (m_viewLeanAngles);
	savefile->ReadVec3 (m_viewLeanTranslation);
	savefile->ReadVec3 (m_LeanDoorListenPos);
	m_LeanDoorEnt.Restore( savefile );

	
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


	DM_LOG (LC_MOVEMENT, LT_DEBUG)LOGSTRING ("Restore finished\n");
}

/*
================
idPhysics_Player::SetPlayerInput
================
*/
void idPhysics_Player::SetPlayerInput( const usercmd_t &cmd, const idAngles &newViewAngles ) 
{
	command = cmd;

	m_DeltaViewYaw = command.angles[1] - m_lastCommandViewYaw;
	m_DeltaViewYaw = SHORT2ANGLE(m_DeltaViewYaw);

	// don't return a change if the player's view is locked in place
	if( static_cast<idPlayer *>(self)->GetImmobilization() & EIM_VIEW_ANGLE )
		m_DeltaViewYaw = 0;

	viewAngles = newViewAngles;	// can't use cmd.angles cause of the delta_angles

	m_lastCommandViewYaw = command.angles[1];
}

/*
================
idPhysics_Player::SetSpeed
================
*/
void idPhysics_Player::SetSpeed( const float newWalkSpeed, const float newCrouchSpeed ) 
{
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
	if ( masterEntity ) 
	{
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
/**
* Ishtvan: Commented this out because we now have global decelaration damage
* Please see previous revision for the code that was here
**/
	// Return amount
	return 0.0f;	

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
	if ( m_bOnRope )
	{
		RopeDetach();
	}

	// If mantling starts while climbing, detach from climbing surface
	if ( m_bOnClimb )
	{
		ClimbDetach();
	}

	// Ishtvan 11/20/05 - Lower weapons when mantling
	static_cast<idPlayer *>(self)->LowerWeapon();
	static_cast<idPlayer *>(self)->hiddenWeapon = true;

	// If mantling from a jump, cancel any velocity so that it does
	// not continue after the mantle is completed.
	current.velocity.Zero();

	// Calculate mantle distance
	idVec3 mantleDistanceVec = endPos - startPos;
//	float mantleDistance = mantleDistanceVec.Length();

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
	// Nyarlathotep 4/12/07 - use cv_pm_mantle_height instead of 
	// g_Global.m_armLengthAsFractionOfPlayerHeight
	float armLength = pm_normalheight.GetFloat() * cv_pm_mantle_height.GetFloat();

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
		
		if (m_p_mantledEntity->IsMantleable())
		{
			m_mantledEntityName = m_p_mantledEntity->name;
			m_mantledEntityID = out_trace.c.id;

			idStr targetMessage;
			DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING
			(
				"Mantle target entity is called '%s'\n", 
				m_p_mantledEntity->name.c_str()
			);
		}
		else
		{
			// Oops, this entity isn't mantleable
			m_p_mantledEntity = NULL;
			out_trace.fraction = 0.0f; // Pretend we didn't hit anything
		}
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
	// Never mantle onto non-mantleable entities (early exit)
	if (in_targetTraceResult.fraction<1.0f)
	{
		idEntity* ent = gameLocal.entities[in_targetTraceResult.c.entityNum];
		if (ent!=NULL && !ent->IsMantleable())
		{
			// The mantle target is an unmantleable entity
			return false;
		}
	}

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
	bool b_lastCollisionWasMantleable = true;
	while (b_keepTesting)
	{

		// Try collision in_targetTraceResult
		trace_t worldMantleTrace;
		idVec3 mantleTraceStart = testPosition;
		gameLocal.clip.Translation( worldMantleTrace, mantleTraceStart, testPosition, clipModel, clipModel->GetAxis(), clipMask, self );


		if (worldMantleTrace.fraction >= 1.0)
		{
			// We can mantle to there, unless the last test collided with something non-mantleable.
			// Either way we're done here.
			b_keepTesting = false;
			if (b_lastCollisionWasMantleable)
			{
				b_mantlePossible = true;
				out_mantleEndPoint = testPosition;
			}
		}
		else
		{
			idEntity* ent = gameLocal.entities[ worldMantleTrace.c.entityNum ];
			if (ent && !ent->IsMantleable())
			{
				// If we collided with a non-mantleable entity, then flag that.
				// This is to prevent situations where we start out mantling on a low ledge
				// (like a stair) on which a non-mantleable entity (like an AI) is standing,
				// and proceed to mantle over the AI.
				b_lastCollisionWasMantleable = false;
			}
			else
			{
				// On the other hand, if there's a shelf above the AI, then we can still mantle
				// the shelf.
				b_lastCollisionWasMantleable = true;
			}

			if (verticalReachDistanceUsed < maxVerticalReachDistance)
			{
				// Try next test position

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
	//if (m_CurrentLeanTiltDegrees < 0.0001) // prevent floating point compare errors
	if (m_CurrentLeanTiltDegrees < 0.00001) // prevent floating point compare errors
	{
		// Start the lean
		m_leanMoveStartTilt = m_CurrentLeanTiltDegrees;
		m_leanYawAngleDegrees = leanYawAngleDegrees;

		// Hack: Use different values for forward/backward lean than side/side
		if( leanYawAngleDegrees == 90.0f || leanYawAngleDegrees == -90.0f )
		{
			m_leanTime = cv_pm_lean_forward_time.GetFloat();
			m_leanMoveEndTilt = cv_pm_lean_forward_angle.GetFloat();
			m_leanMoveMaxStretch = cv_pm_lean_forward_stretch.GetFloat();
			m_leanMoveMaxAngle = cv_pm_lean_forward_angle.GetFloat();
		}
		else
		{
			m_leanTime = cv_pm_lean_time.GetFloat();
			m_leanMoveEndTilt = cv_pm_lean_angle.GetFloat();
			m_leanMoveMaxStretch = cv_pm_lean_stretch.GetFloat();
			m_leanMoveMaxAngle = cv_pm_lean_angle.GetFloat();
		}

		m_b_leanFinished = false;

		DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING ("ToggleLean staring lean\r\n");
	}
	else
	{
		// End the lean
		m_leanMoveStartTilt = m_CurrentLeanTiltDegrees;
		m_leanMoveEndTilt = 0.0;
		m_b_leanFinished = false;

		// Hack: Use different values for forward/backward lean than side/side
		if( leanYawAngleDegrees == 90.0f || leanYawAngleDegrees == -90.0f )
		{
			m_leanTime = cv_pm_lean_forward_time.GetFloat();
			m_leanMoveMaxStretch = cv_pm_lean_forward_stretch.GetFloat();
			m_leanMoveMaxAngle = cv_pm_lean_forward_angle.GetFloat();
		}
		else
		{
			m_leanTime = cv_pm_lean_time.GetFloat();
			m_leanMoveMaxStretch = cv_pm_lean_stretch.GetFloat();
			m_leanMoveMaxAngle = cv_pm_lean_angle.GetFloat();
		}

		DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING ("ToggleLean ending lean\r\n");
	}

}

//----------------------------------------------------------------------

__inline bool idPhysics_Player::IsLeaning()
{
	if (m_CurrentLeanTiltDegrees < 0.001)
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
	idAngles viewAngNoPitch = viewAngles;
	viewAngNoPitch.pitch = 0.0f;
	return viewAngNoPitch.ToMat4() * m_viewLeanTranslation;
}

//----------------------------------------------------------------------

void idPhysics_Player::UpdateLeanAngle (float deltaLeanTiltDegrees, float deltaLeanStretch)
{
	trace_t trTest;
	float newLeanTiltDegrees(0.0), newLeanStretch(0.0);
	idVec3 origPoint, newPoint; // test point
	bool bWouldClip(false);
	idPlayer *p_player = (idPlayer *) self;
	idEntity *TrEnt( NULL ); // entity hit by trace

	// What would the new lean angle be?
	newLeanTiltDegrees = m_CurrentLeanTiltDegrees + deltaLeanTiltDegrees;

	DM_LOG(LC_MOVEMENT,LT_DEBUG)LOGSTRING("newLeanTiltDegrees = %f", newLeanTiltDegrees );
	if (newLeanTiltDegrees < 0.0)
	{
		// Adjust delta
		deltaLeanTiltDegrees = 0.0 - m_CurrentLeanTiltDegrees;
		deltaLeanStretch = 0.0 - m_CurrentLeanStretch;
		m_leanTime = 0.0;
		m_b_leanFinished = true;
	}
	else if (newLeanTiltDegrees > m_leanMoveMaxAngle)
	{
		// Adjust delta
		deltaLeanTiltDegrees = m_leanMoveMaxAngle - m_CurrentLeanTiltDegrees;
		deltaLeanStretch = m_leanMoveMaxStretch - m_CurrentLeanStretch;
		m_leanTime = 0.0;
		m_b_leanFinished = true;
	}

	// Log max possible lean
	DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING
	(
		"Currently leaning %.2f degrees, can lean up to %.2f more degrees this frame\n",
		m_CurrentLeanTiltDegrees,
		deltaLeanTiltDegrees
	);

	newLeanTiltDegrees = m_CurrentLeanTiltDegrees + deltaLeanTiltDegrees;
	newLeanStretch = m_CurrentLeanStretch + deltaLeanStretch;

    // Collision test: do not change lean angles any more if collision has occurred
	// convert proposed angle and stretch to a viewpoint in space:
	newPoint = LeanParmsToPoint( newLeanTiltDegrees, newLeanStretch );

	origPoint = p_player->GetEyePosition();

	// Add some delta so we can lean back afterwards without already being clipped
	idVec3 vDelta = newPoint - origPoint;
	vDelta.Normalize();
	float fLeanTestDelta = 6.0f;
	vDelta *= fLeanTestDelta;

	gameLocal.clip.TraceBounds( trTest, origPoint, newPoint + vDelta, m_LeanViewBounds, MASK_SOLID | CONTENTS_BODY, self );
	bWouldClip = trTest.fraction < 1.0f;
	//DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING("Collision trace between old view point ( %d, %d, %d ) and newPoint: ( %d, %d, %d )\r", origPoint.x, origPoint.y, origPoint.z, newPoint.x, newPoint.y, newPoint.z );

	// Do not lean farther if the player would hit the wall
	// TODO: This may lead to early stoppage at low FPS, so might want to interpolate
	if( bWouldClip )
	{
		TrEnt = gameLocal.GetTraceEntity( trTest );

		DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING("Lean test point within solid, lean motion stopped.\r" );
		
		// Door leaning test
		if( TrEnt 
			&& TrEnt->IsType(CFrobDoor::Type)
			&& !( static_cast<CFrobDoor *>(TrEnt)->isOpen() )
			&& m_LeanDoorEnt.GetEntity() == NULL )
		{
			// If it is a door, can it be listened through?
			if( FindLeanDoorListenPos( trTest.c.point, (CFrobDoor *) TrEnt ) )
				m_LeanDoorEnt = (CFrobDoor *) TrEnt;
		}

		// Detect AI collision, if entity hit or its bindmaster is an AI:
		if( TrEnt && TrEnt->IsType(idAI::Type) )
		{
			static_cast<idAI *>( TrEnt )->HadTactile( (idActor *) self );
		}

		goto Quit;
	}

	// Adjust lean angle by delta which was allowed
	m_CurrentLeanTiltDegrees += deltaLeanTiltDegrees;
	m_CurrentLeanStretch += deltaLeanStretch;

	// Update the physics:
	UpdateLeanPhysics();
		
	// Log activity
	DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING
	(
		"Lean tilt is now %.2f degrees, lean stretch is now %.2f fractional\r",
		m_CurrentLeanTiltDegrees,
		m_CurrentLeanStretch
	);

Quit:
	return;
}

//----------------------------------------------------------------------

void idPhysics_Player::LeanMove()
{

	// Change in lean tilt this frame
	float deltaLeanTiltDegrees = 0.0;
	float deltaLeanStretch = 0.0;
	float newLeanTiltDegrees = 0.0;
	float newLeanStretch = 0.0;

	if ( !m_b_leanFinished ) 
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
		
		if (m_leanMoveEndTilt > m_leanMoveStartTilt)
		{
			newLeanTiltDegrees = (idMath::Sin(timeRadians) * (m_leanMoveEndTilt - m_leanMoveStartTilt))
			 + m_leanMoveStartTilt;
			newLeanStretch = idMath::Sin(timeRadians);
		}
		else if (m_leanMoveStartTilt > m_leanMoveEndTilt)
		{
			newLeanTiltDegrees = m_leanMoveStartTilt - (idMath::Sin(timeRadians) * (m_leanMoveStartTilt - m_leanMoveEndTilt));
			newLeanStretch = idMath::Sin( (idMath::PI/2.0f) * (1.0f - timeRatio) );
		}

		deltaLeanTiltDegrees = newLeanTiltDegrees - m_CurrentLeanTiltDegrees;
		deltaLeanStretch = newLeanStretch - m_CurrentLeanStretch;

	}

	// Perform any change to leaning
	if (deltaLeanTiltDegrees != 0.0)
	{
		// Re-orient clip model before change so that collision tests
		// are accurate (player may have rotated mid-lean)
		UpdateLeanAngle (deltaLeanTiltDegrees, deltaLeanStretch);
	}

	// If player is leaned at all, do an additional clip test and unlean them
	// In case they lean and walk into something, or a moveable moves into them, etc.
	if( m_CurrentLeanTiltDegrees != 0.0
		&& TestLeanClip() )
	{
		DM_LOG(LC_MOVEMENT,LT_DEBUG)LOGSTRING("Leaned player clipped solid, unleaning to valid position \r");

		UnleanToValidPosition();
	}

	// Lean door test
	if( IsLeaning() )
		UpdateLeanDoor();

	// TODO: Update lean radius if player is crouching/uncrouching
}

bool idPhysics_Player::TestLeanClip( void )
{
	idVec3 vTest;
	idEntity *TrEnt(NULL);

	idPlayer *p_player = (idPlayer *) self;
	// convert proposed angle and stretch to a viewpoint in space:

	vTest = p_player->GetEyePosition();
	idVec3 vEyeOffset = -GetGravityNormal()*p_player->EyeHeight();
	
	trace_t trTest;
	gameLocal.clip.TraceBounds( trTest, current.origin + vEyeOffset, vTest, m_LeanViewBounds, MASK_SOLID | CONTENTS_BODY, self );

	// Detect AI collision, if entity hit or its bindmaster is an AI:
	if( trTest.fraction != 1.0f 
		&& ( TrEnt = gameLocal.GetTraceEntity( trTest ) ) != NULL
		&& TrEnt->IsType(idAI::Type) )
	{
		static_cast<idAI *>( TrEnt )->HadTactile( (idActor *) self );
	}


	// Uncomment for debug bounds display
	//gameRenderWorld->DebugBounds( colorGreen, m_LeanViewBounds, vTest ); 
	
	return (trTest.fraction != 1.0f);
}

idVec3 idPhysics_Player::LeanParmsToPoint( float AngTilt, float Stretch )
{
	float fLeanFulcrumHeight, fEyeHeight, radius, stretchedDist;
	idVec3 vPoint;

	idPlayer* p_player = (idPlayer*) self;
	
	// Find the lean fulcrum to rotate about, and radius of lean
	fEyeHeight = p_player->EyeHeight();
	fLeanFulcrumHeight = cv_pm_lean_height.GetFloat() * fEyeHeight;
	radius = fEyeHeight - fLeanFulcrumHeight;

	// Set lean view angles
	float pitchAngle = AngTilt;
	float rollAngle = pitchAngle;

	pitchAngle *= idMath::Sin(m_leanYawAngleDegrees * ((2.0 * idMath::PI) / 360.0) );
	rollAngle *= idMath::Cos(m_leanYawAngleDegrees * ((2.0 * idMath::PI) / 360.0) );

	// Set lean translate vector
	stretchedDist = radius * (1.0f + m_leanMoveMaxStretch * Stretch );
	
	vPoint.x = stretchedDist * idMath::Sin (-pitchAngle * ((2.0 * idMath::PI) / 360.0) );
	vPoint.y = stretchedDist * idMath::Sin(rollAngle * ((2.0 * idMath::PI) / 360.0) );
	vPoint.z = 0.0;

	vPoint.ProjectSelfOntoSphere( stretchedDist );

	vPoint.z = vPoint.z - stretchedDist;

	// Rotate to player's facing
	// this worked for yaw, but had issues with pitch, try something instead
	//idMat4 rotMat = viewAngles.ToMat4();
	idAngles viewAngNoPitch = viewAngles;
	viewAngNoPitch.pitch = 0.0f;
	idMat4 rotMat = viewAngNoPitch.ToMat4();

	vPoint *= rotMat;

	// Sign h4x0rx
	vPoint.x *= -1;
	vPoint.y *= -1;

	// Extract what the player's eye position would be without lean
	// Need to do this rather than just adding origin and eye offset due to smoothing
	vPoint += p_player->GetEyePosition() - rotMat * m_viewLeanTranslation;

	return vPoint;
}

void idPhysics_Player::RopeRemovalCleanup( idEntity *RopeEnt )
{
	if( RopeEnt && m_RopeEntity.GetEntity() && m_RopeEntity.GetEntity() == RopeEnt )
		m_RopeEntity = NULL;
	if( RopeEnt && m_RopeEntTouched.GetEntity() && m_RopeEntTouched.GetEntity() == RopeEnt )
		m_RopeEntTouched = NULL;
}

void idPhysics_Player::UpdateLeanPhysics( void )
{
	idMat4 rotWorldToPlayer, rotPlayerToWorld;
	rotWorldToPlayer.Zero(); 
	rotPlayerToWorld.Zero();
	idAngles viewAngNoPitch(0.0f, 0.0f, 0.0f);
	idVec3 viewOrig; // unleaned player view origin
	idVec3 newPoint;
	idPlayer *p_player = (idPlayer *) self;

	viewAngNoPitch = viewAngles;
	viewAngNoPitch.pitch = 0;
	rotPlayerToWorld = viewAngNoPitch.ToMat4();
	rotWorldToPlayer = rotPlayerToWorld.Transpose();

	viewOrig = p_player->GetEyePosition();
	// convert angle and stretch to a viewpoint in space:
	newPoint = LeanParmsToPoint( m_CurrentLeanTiltDegrees, m_CurrentLeanStretch );
	
	// This is cumbersome, but it lets us extract the smoothed view origin from idPlayer
	m_viewLeanTranslation = newPoint - (viewOrig - rotPlayerToWorld * m_viewLeanTranslation);
	m_viewLeanTranslation *= rotWorldToPlayer;

	float angle = m_CurrentLeanTiltDegrees;

	m_viewLeanAngles.pitch = angle * idMath::Sin(m_leanYawAngleDegrees * ((2.0 * idMath::PI) / 360.0) );
	m_viewLeanAngles.roll = angle * idMath::Cos(m_leanYawAngleDegrees * ((2.0 * idMath::PI) / 360.0) );
}

float idPhysics_Player::GetDeltaViewYaw( void )
{
	return m_DeltaViewYaw;
}

void idPhysics_Player::UpdateLeanedInputYaw( idAngles &InputAngles )
{
	idPlayer *pPlayer = (idPlayer *) self;
	float TestDeltaYaw(0.0f);

	/**
	* Leaned view yaw check for clipping
	**/
	if( IsLeaning() )
	{
		trace_t TrResults;
		idEntity *TrEnt = NULL;
		idVec3 startPoint, endPoint, vDelta;
		// Have a delta so that we don't get stuck on the wall due to floating point errors
		float AddedYawDelt = 4.0f; // amount to check ahead of the yaw change, in degrees
		TestDeltaYaw = idMath::AngleNormalize180( InputAngles.yaw - viewAngles.yaw );

		// Add delta
		if( TestDeltaYaw < 0.0f )
			TestDeltaYaw -= AddedYawDelt;
		else
			TestDeltaYaw += AddedYawDelt;


		startPoint = pPlayer->GetEyePosition();
		idVec3 vEyeOffset = -GetGravityNormal()*pPlayer->EyeHeight();

		// make the test bounds go back to the unleaned eye point
		idBounds ViewBoundsExp = m_LeanViewBounds;
		ViewBoundsExp.AddPoint( -m_viewLeanTranslation );

		idClipModel ViewClip( ViewBoundsExp );
		idAngles viewAngYaw;
		viewAngYaw.Zero();
		viewAngYaw.yaw = viewAngles.yaw;

		ViewClip.SetPosition( pPlayer->GetEyePosition(), viewAngYaw.ToMat3() );

		//idRotation ViewYawRot( current.origin, -GetGravityNormal(), TestDeltaYaw );
		idRotation ViewYawRot( current.origin, GetGravityNormal(), TestDeltaYaw );

		startPoint = pPlayer->GetEyePosition();
		endPoint = startPoint;
		ViewYawRot.RotatePoint( endPoint );

		gameLocal.clip.Rotation( TrResults, startPoint, ViewYawRot, &ViewClip, ViewClip.GetAxis(), MASK_SOLID | CONTENTS_BODY, self );

		DM_LOG(LC_MOVEMENT,LT_DEBUG)LOGSTRING("Leaned View Yaw Test: Original viewpoint (%f, %f, %f) Tested viewpoint: (%f, %f, %f) \r", startPoint.x, startPoint.y, startPoint.z, endPoint.x, endPoint.y, endPoint.z );

		// Cancel rotation if check-ahead rotation trace fails
		if( TrResults.fraction != 1.0f )
		{
			DM_LOG(LC_MOVEMENT,LT_DEBUG)LOGSTRING("Leaned View Yaw Test: Clipped with rotation trace fraction %f.  Delta yaw not allowed \r", TrResults.fraction );
			InputAngles.yaw = viewAngles.yaw;

			// Detect AI collision, if entity hit or its bindmaster is an AI:
			if( ( TrEnt = gameLocal.GetTraceEntity( TrResults ) ) != NULL
				&& TrEnt->IsType(idAI::Type) )
			{
				static_cast<idAI *>( TrEnt )->HadTactile( (idActor *) self );
			}
			
			goto Quit;
		}

		// debug draw the test clip model
		/*
		collisionModelManager->DrawModel( ViewClip.Handle(), ViewClip.GetOrigin(),
										ViewClip.GetAxis(), vec3_origin, 0.0f );
		*/
	}

Quit:
	return;
}

void idPhysics_Player::UnleanToValidPosition( void )
{
	trace_t trTest;
	idVec3 vTest(vec3_zero);

	idPlayer *p_player = (idPlayer *) self;
	idVec3 vEyeOffset = -GetGravityNormal()*p_player->EyeHeight();

	float TestLeanDegrees = m_CurrentLeanTiltDegrees;
	float TestLeanStretch = m_CurrentLeanStretch;
	float DeltaDeg = TestLeanDegrees / (float) cv_pm_lean_to_valid_increments.GetInteger();
	float DeltaStretch = TestLeanStretch / (float) cv_pm_lean_to_valid_increments.GetInteger();

	// Must temporarily set these to get proper behavior from max angle
	m_leanMoveMaxAngle = m_CurrentLeanTiltDegrees;
	m_leanMoveMaxStretch = m_CurrentLeanStretch;
	m_leanMoveStartTilt = m_CurrentLeanTiltDegrees;
	m_leanMoveEndTilt = 0.0f;

	for( int i=0; i < cv_pm_lean_to_valid_increments.GetInteger(); i++ )
	{
		// Lean degrees are always positive
		TestLeanDegrees -= DeltaDeg;
		TestLeanStretch -= DeltaStretch;

		// convert proposed angle and stretch to a viewpoint in world space:
		vTest = LeanParmsToPoint( TestLeanDegrees, TestLeanStretch );
		gameLocal.clip.TraceBounds( trTest, current.origin + vEyeOffset, vTest, m_LeanViewBounds, MASK_SOLID | CONTENTS_BODY, self );

		// break if valid point was found
		if( trTest.fraction == 1.0f )
			break;
	}

	// transform lean parameters with final answer
	m_CurrentLeanTiltDegrees = TestLeanDegrees;
	m_CurrentLeanStretch = TestLeanStretch;
	
	UpdateLeanPhysics();
}

bool idPhysics_Player::FindLeanDoorListenPos( idVec3 IncidencePoint, CFrobDoor *door )
{
	bool bFoundEmptySpace( false );
	int contents = -1;
	idVec3 vTest( IncidencePoint ), vDirTest( vec3_zero ), vLeanDir( 1.0f, 0.0f, 0.0f );
	idAngles LeanYaw;
	idAngles viewYawOnly = viewAngles;
	
	LeanYaw.Zero();
	LeanYaw.yaw = m_leanYawAngleDegrees - 90.0f;

	viewYawOnly.pitch = 0.0f;
	viewYawOnly.roll = 0.0f;

	vDirTest = viewYawOnly.ToMat3() * LeanYaw.ToMat3() * vLeanDir;
	vDirTest.Normalize();

	int MaxCount = cv_pm_lean_door_increments.GetInteger();


	for( int count = 1; count < MaxCount; count++ )
	{
		vTest += vDirTest * ( (float) count / (float) MaxCount ) * cv_pm_lean_door_max.GetFloat();
		
		contents = gameLocal.clip.Contents( vTest, NULL, mat3_identity, CONTENTS_SOLID, self );
		
		// found empty space on other side of door
		if( !( (contents & MASK_SOLID) > 0 ) )
		{
			DM_LOG(LC_MOVEMENT,LT_DEBUG)LOGSTRING("Lean Into Door: Found empty space on other side of door.  Incidence point: %s Empty space point: %s \r", IncidencePoint.ToString(), vTest.ToString() );
			
			bFoundEmptySpace = true;
			m_LeanDoorListenPos = vTest;
			break;
		}
	}

	// uncomment to debug the lean direction line
	// gameRenderWorld->DebugArrow( colorBlue, IncidencePoint, IncidencePoint + 15.0f * vDirTest, 5.0f, 10000 );
	
	return bFoundEmptySpace;
}

void idPhysics_Player::UpdateLeanDoor( void )
{
	CFrobDoor *door(NULL);
	idPlayer *pPlayer = (idPlayer *) self;

	door = m_LeanDoorEnt.GetEntity();

	if( door && pPlayer )
	{
		if( !m_LeanDoorEnt.IsValid() 
			|| door->isOpen()
			|| !IsLeaning() )
		{
			m_LeanDoorEnt = NULL;
			goto Quit;
		}

		idBounds TestBounds = m_LeanViewBounds;
		TestBounds.ExpandSelf( cv_pm_lean_door_bounds_exp.GetFloat() );
		TestBounds.TranslateSelf( pPlayer->GetEyePosition() );

/** More precise test (Not currently used)
	
		int numEnts = 0;
		idEntity *ents[MAX_GENTITIES];
		idEntity *ent = NULL;
		bool bMatchedDoor(false);

		numEnts = gameLocal.clip.EntitiesTouchingBounds( TestBounds, CONTENTS_SOLID, ents, MAX_GENTITIES);
		for( int i=0; i < numEnts; i++ )
		{
			if( ents[i] == (idEntity *) door )
			{
				bMatchedDoor = true;
				break;
			}
		}

		if( !bMatchedDoor )
		{
			m_LeanDoorEnt = NULL;
			goto Quit;
		}
**/

		if( !TestBounds.IntersectsBounds( door->GetPhysics()->GetAbsBounds() ) )
		{
			m_LeanDoorEnt = NULL;
			goto Quit;
		}
		
		// We are leaning into a door
		// overwrite the current player listener loc with that calculated for the door
		pPlayer->SetDoorListenLoc( m_LeanDoorListenPos );
	}

Quit:
	return;
}

bool idPhysics_Player::IsDoorLeaning( void )
{
	return (m_LeanDoorEnt.GetEntity() != NULL) && m_LeanDoorEnt.IsValid();
}

idStr idPhysics_Player::GetClimbSurfaceType( void ) const
{
	idStr ReturnVal;
	ReturnVal.Clear();
	if( m_bOnClimb )
		ReturnVal = m_ClimbSurfName;

	return ReturnVal;
}

float idPhysics_Player::GetClimbLateralCoord( idVec3 OrigVec ) const
{
	float ReturnVal = 0.0f;

	if( m_bOnClimb )
	{
		OrigVec -= (OrigVec * gravityNormal) * gravityNormal;
		idVec3 ClimbNormXY = m_vClimbNormal - (m_vClimbNormal * gravityNormal) * gravityNormal;
		idVec3 LatNormal = ClimbNormXY.Cross( gravityNormal );
		LatNormal.NormalizeFast();

		ReturnVal = OrigVec * LatNormal;
	}
	
	return ReturnVal;
}