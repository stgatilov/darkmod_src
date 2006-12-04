/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.23  2006/12/04 00:32:15  ishtvan
 * *) added stretching of body to lean
 *
 * *) Leaning now checks cvars instead of ini file vars
 *
 * *) Disabled manual tilting of model in lean code (NOTE: Lean collision test does not work, WIP at the moment)
 *
 * Revision 1.22  2006/08/04 10:55:08  ishtvan
 * added GetDeltaYaw function to get player view change
 *
 * Revision 1.21  2006/07/09 02:40:12  ishtvan
 * rope arrow removal bugfix
 *
 * Revision 1.20  2005/11/18 10:31:44  ishtvan
 * rope arrow fixes
 *
 * Revision 1.19  2005/11/17 09:12:33  ishtvan
 * minor ropearrow update, variable type change
 *
 * Revision 1.18  2005/11/12 14:59:51  sparhawk
 * SDK 1.3 Merge
 *
 * Revision 1.17  2005/10/16 02:18:31  ishtvan
 * *) completed rope orbiting
 *
 * *) added failsafe for rope arrow attachment
 *
 * Revision 1.16  2005/10/14 09:04:19  ishtvan
 * updated rope climbing
 *
 * Revision 1.15  2005/10/12 14:52:52  domarius
 * Rope arrow - initial stage, just sticks you to the rope point of origin... permanently.
 *
 * Revision 1.14  2005/09/17 07:15:28  sophisticatedzombie
 * Added function that applies damage to the player when mantling at a high relative velocity. The damage amount is computed from minimum and scale constants in DarkModGlobals.
 *
 * Revision 1.13  2005/09/14 04:21:07  domarius
 * no message
 *
 * Revision 1.12  2005/09/09 19:56:02  ishtvan
 * removed water jump, allowed mantling out of water
 *
 * Revision 1.11  2005/09/08 04:42:34  sophisticatedzombie
 * Added mantle and lean states to the save/restore methods.
 *
 * Revision 1.10  2005/09/04 20:38:20  sophisticatedzombie
 * The collision/render model leaning of the player model is now accomplished by rotation of the waist joint of the model skeleton.
 *
 * Revision 1.9  2005/08/19 00:28:02  lloyd
 * *** empty log message ***
 *
 * Revision 1.8  2005/08/14 23:29:04  sophisticatedzombie
 * Broke methods into smaller more logical units.
 * Changed header documentation to use doxygen format and doxygen tags.
 * Leaning and Mantling constants moved to DarkModGlobals
 * Fixed infinite loop that could occur in mantling test due to floating point number precision.
 *
 * Revision 1.7  2005/08/04 05:16:43  sophisticatedzombie
 * Lean angle is now 12 degrees rather than 20.
 * Leaning now uses sinusoidal velocity rather than linear velocity.
 * Its still to bumpy, but pushing the player up is the only way to keep them out of the floor that I have come up with yet.
 *
 * Revision 1.6  2005/08/01 22:37:09  sophisticatedzombie
 * Added rotation test to the UpdateClipModelOrientation method which detects collisions due to changes in player yaw in between frames.  This can happen when leaning, leading to the clip model penetrating a nearby surface, thereby breaking collision "sidedness" calculations.  In order to get around the issue, if the rotation test detects that the change in player yaw between the last frame and this frame resulted in collision with another collision model, then the player snaps to the upright position.  It prevents the ability to rotate the view through objects.
 *
 * Revision 1.5  2005/07/30 01:31:28  sophisticatedzombie
 * Somewhat improved collision detection. Work needs to be done on handling collisions due to viewpoint rotation while in a leaned position.
 *
 * Revision 1.4  2005/07/27 20:48:09  sophisticatedzombie
 * Added leaning.  The clip model stuff still needs alot of work.
 *
 * Revision 1.3  2005/07/03 18:37:02  sophisticatedzombie
 * Added a time variable which accumulates the milliseconds that the jump button is held down.  If it gets greater than a constant (JUMP_HOLD_MANTLE_TRIGGER_MILLISECONDS) then a mantle attempt is initiated.  The timer is not reset until jump is released, so you can hold it in while falling and try to catch something.
 *
 * Revision 1.2  2005/07/01 21:21:23  sophisticatedzombie
 * This is my check in of the mantling code on July 1, 2005.  I've tested it agains the .3 sdk, but not the .2 one.  Any takers?
 *
 * Revision 1.1.1.1  2004/10/30 15:52:34  sparhawk
 * Initial release
 *
 ***************************************************************************/

// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __PHYSICS_PLAYER_H__
#define __PHYSICS_PLAYER_H__

/*
===================================================================================

	Player physics

	Simulates the motion of a player through the environment. Input from the
	player is used to allow a certain degree of control over the motion.

===================================================================================
*/

class idAFEntity_Base;

// movementType
typedef enum {
	PM_NORMAL,				// normal physics
	PM_DEAD,				// no acceleration or turning, but free falling
	PM_SPECTATOR,			// flying without gravity but with collision detection
	PM_FREEZE,				// stuck in place without control
	PM_NOCLIP				// flying without collision detection nor gravity
} pmtype_t;

#ifndef MOD_WATERPHYSICS

// waterLevel_t has been moved to Physics_Actor.h

typedef enum 
{
  WATERLEVEL_NONE,
  WATERLEVEL_FEET,
  WATERLEVEL_WAIST,
  WATERLEVEL_HEAD
} waterLevel_t;

#endif		// MOD_WATERPHYSICS


#define	MAXTOUCH					32

typedef struct playerPState_s {
	idVec3					origin;
	idVec3					velocity;
	idVec3					localOrigin;
	idVec3					pushVelocity;
	float					stepUp;
	int						movementType;
	int						movementFlags;
	int						movementTime;
} playerPState_t;


// This enumreation defines the phases of the mantling movement
typedef enum
{
	notMantling_DarkModMantlePhase	= 0x00,
	hang_DarkModMantlePhase			= 0x01,
	pull_DarkModMantlePhase			= 0x02,
	shiftHands_DarkModMantlePhase	= 0x03,
	push_DarkModMantlePhase			= 0x04

} EDarkMod_MantlePhase;


// The class itself
class idPhysics_Player : public idPhysics_Actor {

public:
	CLASS_PROTOTYPE( idPhysics_Player );

							idPhysics_Player( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

							// initialisation
	void					SetSpeed( const float newWalkSpeed, const float newCrouchSpeed );
	void					SetMaxStepHeight( const float newMaxStepHeight );
	float					GetMaxStepHeight( void ) const;
	void					SetMaxJumpHeight( const float newMaxJumpHeight );
	void					SetMovementType( const pmtype_t type );
	void					SetPlayerInput( const usercmd_t &cmd, const idAngles &newViewAngles );
	void					SetKnockBack( const int knockBackTime );
	void					SetDebugLevel( bool set );
							// feed back from last physics frame
#ifndef MOD_WATERPHYSICS

	waterLevel_t            GetWaterLevel( void ) const;

	int                     GetWaterType( void ) const;

#endif

	bool					HasJumped( void ) const;
	bool					HasSteppedUp( void ) const;
	float					GetStepUp( void ) const;
	bool					IsCrouching( void ) const;
    bool					OnRope( void ) const;
	bool					OnLadder( void ) const;
	const idVec3 &			PlayerGetOrigin( void ) const;	// != GetOrigin

	/**
	* Get the view yaw change between last frame and this frame
	* Useful for rotating items in response to yaw, rope arrow, etc
	* Returns the change in degrees
	**/
	float					GetDeltaViewYaw( void );					

public:	// common physics interface
	bool					Evaluate( int timeStepMSec, int endTimeMSec );
	void					UpdateTime( int endTimeMSec );
	int						GetTime( void ) const;

	void					GetImpactInfo( const int id, const idVec3 &point, impactInfo_t *info ) const;
	void					ApplyImpulse( const int id, const idVec3 &point, const idVec3 &impulse );
	bool					IsAtRest( void ) const;
	int						GetRestStartTime( void ) const;

	void					SaveState( void );
	void					RestoreState( void );

	void					SetOrigin( const idVec3 &newOrigin, int id = -1 );
	void					SetAxis( const idMat3 &newAxis, int id = -1 );

	void					Translate( const idVec3 &translation, int id = -1 );
	void					Rotate( const idRotation &rotation, int id = -1 );

	void					SetLinearVelocity( const idVec3 &newLinearVelocity, int id = 0 );

	const idVec3 &			GetLinearVelocity( int id = 0 ) const;

	void					SetPushed( int deltaTime );
	const idVec3 &			GetPushedLinearVelocity( const int id = 0 ) const;
	void					ClearPushedVelocity( void );

	void					SetMaster( idEntity *master, const bool orientated = true );

	void					WriteToSnapshot( idBitMsgDelta &msg ) const;
	void					ReadFromSnapshot( const idBitMsgDelta &msg );

/**
* Removes stale pointers when a rope entity is destroyed
* RopeEnt is the rope entity that's about to be destroyed.
**/
	void					RopeRemovalCleanup( idEntity *RopeEnt );

private:
	// player physics state
	playerPState_t			current;
	playerPState_t			saved;

	// properties
	float					walkSpeed;
	float					crouchSpeed;
	float					maxStepHeight;
	float					maxJumpHeight;
	int						debugLevel;				// if set, diagnostic output will be printed

	// player input
	usercmd_t				command;
	idAngles				viewAngles;

	// run-time variables
	int						framemsec;
	float					frametime;
	float					playerSpeed;
	idVec3					viewForward;
	idVec3					viewRight;

	// walk movement
	bool					walking;
	bool					groundPlane;
	trace_t					groundTrace;
	const idMaterial *		groundMaterial;

    // rope movement

	/**
	* Set to true if the player is moving and contacts a rope
	**/
    bool					m_bRopeContact;

	/**
	* The rope entity that the player last attached to
	**/
    idAFEntity_Base			*m_RopeEntity;

	/**
	* The rope entity that the player last touched (not necessarily attached to)
	* Used for the case where the player starts inside a rope and jumps up
	**/
	idAFEntity_Base			*m_RopeEntTouched;

	/**
	* The gametime since the last detachment (used for detach-reattach timer)
	**/
	int						m_RopeDetachTimer;
	
	/**
	* toggled based on whether the player should stay attached to rope
	**/
	bool					m_bRopeAttached;

	/**
	* toggled on in the frame that the player first attaches to the rope
	**/
	bool					m_bJustHitRope;

	// ladder movement
	bool					ladder;
	idVec3					ladderNormal;

	/**
	* View yaw change between this frame and last frame
	* In degrees.
	**/
	float					m_DeltaViewYaw;

	// results of last evaluate
#ifndef MOD_WATERPHYSICS

  waterLevel_t            waterLevel;

  int                     waterType;

#endif


private:
	float					CmdScale( const usercmd_t &cmd ) const;
	void					Accelerate( const idVec3 &wishdir, const float wishspeed, const float accel );
	bool					SlideMove( bool gravity, bool stepUp, bool stepDown, bool push );
	void					Friction( void );
	void					WaterMove( void );
	void					FlyMove( void );
	void					AirMove( void );
	void					WalkMove( void );
	void					DeadMove( void );
	void					NoclipMove( void );
	void					SpectatorMove( void );
	void					RopeMove( void );
	void					LadderMove( void );
	void					CorrectAllSolid( trace_t &trace, int contents );
	void					CheckGround( void );
	void					CheckDuck( void );
	void					CheckLadder( void );
	bool					CheckJump( void );
	bool					CheckRopeJump( void );
	void					RopeDetach( void );

#ifndef MOD_WATERPHYSICS
	void					SetWaterLevel( void );
#endif
	void					DropTimers( void );
	void					MovePlayer( int msec );

	//#####################################################
	// Mantling handler
	// by SophisticatedZombie (Damon Hill)
	//
	//#####################################################

public:


	// This method returns
	// true if the player is mantling, false otherwise
	__inline bool IsMantling (void) const;
	
	// This returns the current mantling phase
	__inline EDarkMod_MantlePhase GetMantlePhase(void) const;

	// Cancels any current mantle
	inline void CancelMantle();

	// Checks to see if there is a mantleable target within reach
	// of the player's view. If so, starts the mantle... 
	// If the player is already mantling, this does nothing.
	void PerformMantle();


protected:


	/*!
	* The current mantling phase
	*/
	EDarkMod_MantlePhase m_mantlePhase;

	/*!
	* Points along the mantle path
	*/
	idVec3 m_mantlePullStartPos;
	idVec3 m_mantlePullEndPos;
	idVec3 m_mantlePushEndPos;

	/*!
	* Pointer to the entity being mantled.
	* This is undefined if m_mantlePhase == notMantling_DarkModMantlePhase
	*/
	idEntity* m_p_mantledEntity;

	/*!
	* ID number of the entity being mantled
	* This is 0 if m_mantlePhase == notMantling_DarkModMantlePhase
	*/
	int m_mantledEntityID;

	/*!
	* The mantled entity name
	*/
	idStr m_mantledEntityName;

	/*!
	* How long will the current phase of the mantle operation take?
	* Uses milliseconds and counts down to 0.
	*/
	float m_mantleTime;

	/*!
	* Tracks, in milliseconds, how long jump button has been held down
	* Counts upwards from 0.
	*/ 
	float m_jumpHeldDownTime;

	/*!
	* This method determines the mantle time required for each phase of the mantle.
	* I made this a function so you could implement things such as carry-weight,
	* tiredness, length of lift....
	/* @param[in] mantlePhase The mantle phase for which the duration is to be retrieved
	*/
	float getMantleTimeForPhase 
	(
		EDarkMod_MantlePhase mantlePhase
	);

	/*!
	*
	* This method is used to determine the amount of damage conferred
	* to the player in starting the mantle.  It is based on the
	* velcity of the player relative to the mantle target.
	*
	* @param[in] p_mantledEntityRef  The entity the player is mantling.
	*  If this is null, then the player's velocity relative to the world frame
	*  is used.
	*
	* @param[in] mantlePos The position relative tot he world where
	* the mantle will start. (Not relative to the mantle target)
	*/
	int CalculateMantleCollisionDamage
	(
		idEntity* p_mantledEntityRef,
		idVec3 mantlePos
	);

	/*!
	*
	* Internal method to start the mantle operation
	*
	* @param[in] initialMantlePhase The mantle phase in which the mantle starts.
	* @param[in] eyePos The position of the player's eyes in the world
	* @param[in] startPos The position of the player's feet at the start of the mantle
	* @param[in] endPos The position of the player's feet at the end of the mantle
	*/
	void StartMantle
	(
		EDarkMod_MantlePhase initialMantlePhase,
		idVec3 eyePos,
		idVec3 startPos,
		idVec3 endPos
	);

	/*!
	* Internal method which determines the maximum vertical
	* and horizontal distances for mantling
	*
	* @param[out] out_maxVerticalReachDistance The distance that the player can reach vertically, from their current origin
	* @param[out] out_maxHorizontalReachDistance The distance that the player can reach horizontally, from their current origin
	* @param[out] out_maxMantleTraceDistance The maximum distance that the traces should look in front of the player for a mantle target
	*/
	void GetCurrentMantlingReachDistances
	(
		float& out_maxVerticalReachDistance,
		float& out_maxHorizontalReachDistance,
		float& out_maxMantleTraceDistance
	);

	/*!
	* This method runs the trace to find a mantle target
	* It first attempts to raycast along the player's gaze
	* direction to determine a target. If it doesn't find one,
	* then it tries a collision test along a vertical plane
	* from the players feet to their height, out in the direction
	* the player is facing.
	*
	* @param[in] maxMantleTraceDistance The maximum distance from the player that should be used in the traces
	* @param[in] eyePos The position of the player's eyes, used for the beginning of the gaze trace
	* @param[in] forwardVec The vector gives the direction that the player is facing
	* @param[out] out_trace This trace structure will hold the result of whichever trace succeeds. If both fail, the trace fraction will be 1.0
	*/
	void MantleTargetTrace
	(
		float maxMantleTraceDistance,
		idVec3 eyePos,
		idVec3 forwardVec,
		trace_t& out_trace
	);

	/*!
	* 
	* This function checks the collision target of the mantle 
	* trace to see if there is a surface within reach distance
	* upon which the player will fit.
	*
	* @param[in] maxVerticalReachDistance The maximum distance that the player can reach vertically from their current origin
	* @param[in] maxHorizontalReachDistance The maximum distance that the player can reach horizontally from their current origin
	* @param[in] in_targetTraceResult The trace which found the mantle target
	* @param[out] out_mantleEndPoint If the return code is true, this out paramter specifies the position of the player's origin at the end of the mantle move.
	*
	* @return the result of the test
	* @retval true if the mantle target has a mantleable surface
	* @retval false if the mantel target does not have a mantleable surface
	*
	*/
	bool DetermineIfMantleTargetHasMantleableSurface
	(
		float maxVerticalReachDistance,
		float maxHorizontalReachDistance,
		trace_t& in_targetTraceResult,
		idVec3& out_mantleEndPoint
	);		

	/*!
	* Call this method to test whether or not the path
	* along the mantle movement is free of obstructions.
	*
	* @param[in] maxVerticalReachDistance The maximum distance that the player can reach vertically from their current origin
	* @param[in] maxHorizontalReachDistance The maximum distance that the player can reach horizontally from their current origin
	* @param[in] eyePos The position of the player's eyes in the world
	* @param[in] mantleStartPoint The player's origin at the start of the mantle movement
	* @param[in] mantleEndPoint The player's origin at the end of the mantle movement
	*
	* @return the result of the test
	* @retval true if the path is clear
	* @retval false if the path is blocked
	*/
	bool DetermineIfPathToMantleSurfaceIsPossible
	(
		float maxVerticalReachDistance,
		float maxHorizontalReachDistance,
		idVec3 eyePos,
		idVec3 mantleStartPoint,
		idVec3 mantleEndPoint
	);

	/*!
	* Given a trace which resulted in a detected mantle
	* target, this method checks to see if the target
	* is mantleable.  It looks for a surface up from gravity
	* on which the player can fit while crouching. It also
	* checks that the distance does not violate horizontal
	* and vertical displacement rules for mantling. Finally
	* it checks that the path to the mantleable surface is
	* not blocked by obstructions.
	*
	* This method calls DetermineIfMantleTargetHasMantleableSurface and
	* also DetermineIfPathToMantleSurfaceIsPossible
	*
	* @param[in] maxVerticalReachDistance The maximum distance from the player's origin that the player can reach vertically
	* @param[in] maxHorizontalReachDistance The maximum distance from the player's origin that the player can reach horizontally
	* @param[in] eyePos The position of the player's eyes (camera point) in the world coordinate system
	* @param[in] in_targetTraceResult Pass in the trace result from MantleTargetTrace
	* @param[out] out_mantleEndPoint If the return value is true, this passes back out what the player's origin will be at the end of the mantle
	*
	* @returns the result of the test
	* @retval true if the mantle target can be mantled
	* @retval false if the mantle target cannot be mantled
	*
	*/	
	bool ComputeMantlePathForTarget
	(	
		float maxVerticalReachDistance,
		float maxHorizontalReachDistance,
		idVec3 eyePos,
		trace_t& in_targetTraceResult,
		idVec3& out_mantleEndPoint

	);
		

	/*!
	* This handles the reduction of the mantle timer by the
	* number of milliseconds between animation frames. It
	* uses the timer results to update the mantle timers.
	*/
	void UpdateMantleTimers();
    
	/*!
	* This handles the movement of the the player during
	* the phases of the mantle.  It performs the translation
	* of the player along the mantle path, the camera movement
	* that creates the visual effect, and the placing of the
	* player into a crouch during the end phase of the mantle.
	* 
	*/
	void MantleMove();

	// Tests if player is holding down jump while already jumping
	// (can be used to trigger mantle)
	bool CheckJumpHeldDown( void );

	//#################################################
	// End mantling handler
	//#################################################

	//#####################################################
	// Leaning handler
	// by SophisticatedZombie (Damon Hill)
	//
	//#####################################################

protected:

	/*!
	* An axis which is perpendicular to the gravity normal and
	* rotated by the given yaw angle clockwise (when looking down in direction of
	* gravity) around it. The player always leans to positive angles
	* around this axis.
	*/
	float m_leanYawAngleDegrees;

	/*! 
	* The current lean angle
	*/
	float m_currentLeanTiltDegrees;

	/**
	* Current lean stretch fraction.  When this is 1.0, the player is at full stretch, at 0.0, not stretched
	**/
	float m_CurrentLeanStretch;

	/*!
	* The start (roll) angle of this lean movement
	*/
	float m_leanMoveStartTilt;

	/*!
	* The end (roll) angle of this lean movement
	*/
	float m_leanMoveEndTilt;

	/*!
	* Is the lean finished
	*/
	bool m_b_leanFinished;

	/*!
	* How long will the current phase of the leaning operation take?
	* Uses milliseconds and counts down to 0
	*/
	float m_leanTime;

	/*!
	* The last recorded view angles
	* These are updated each animation frame and are used
	* to track player rotations to due rotation collision detection.
	* The original Doom3 code did not need to due collision tests
	* during player view rotation, but if the player is leaning, it is
	* necessary to prevent passing through solid objects
	*/
	idAngles m_lastPlayerViewAngles;

	float m_lastCommandViewYaw;
	
	/*!
	* The current resulting view lean angles
	*/
	idAngles m_viewLeanAngles;

	/*!
	* The current resulting view lean translation
	*/
	idVec3 m_viewLeanTranslation;


	/*! Lean the player model at the waist joint
	*/
	void LeanPlayerModelAtWaistJoint();

	/*!
	* This method is required to prevent collisions between the
	* player's torso and other objects due to rotation of the body
	* between physics frames, while leaning. or not a lean is taking place.
	*/
	void TestForViewRotationBasedCollisions();

	/*!
	* This method is used to update the view lean angles and view translation
	* that result from the lean.
	*
	* This is an internal method called by LeanPlayerModelAtWaistJoint.
	*
	* @param viewpointHeight
	*	The distance of the viewpoint height above the 
	*   player origin
	*	This is typically the player's pm_normalviewheight 
	*	or pm_crouchviewheightagainst the gravity normal from 
	*	the model origin.
	*
	* @param distanceFromWaistToViewpoint
	*	The distance from the waist joint height to the 
	*   viewpointHeight
	*
	*/
	void UpdateViewLeanAnglesAndTranslation
	(
		float viewpointHeight,
		float distanceFromWaistToViewpoint
	);

	/*!
	* This method updates the lean by as much of the delta amount given
	* that does not result in a clip model trace collision.
	*
	* This is an internal method called by LeanMove.
	*/
	void UpdateLeanAngle (float deltaLeanAngle, float deltaLeanStretch);

	/*!
	* This uses the other internal mtehods to coordiante the lean
	* lean movement.
	*/
	void LeanMove();

public:

	/*!
	* Starts or ends a lean around and axis which is perpendicular to the gravity normal and
	* rotated by the given yaw angle clockwise (when looking down in direction of
	* gravity) around it. 
	* 
	* @param[in] leanYawAngleDegrees The angle of the axis around which the player will
	* lean clockwise, itself rotated clockwise from straight forward.
	* 0.0 leans right, 180.0 leans left, 90.0 leans forward, 270.0 leans backward
	*
	*/
	void ToggleLean
	(
		float leanYawAngleDegrees
	);

	/*!
	* This method tests if the player is in the middle of a leaning
	* movement.
	*
	* @return the result of the test
	* @retval true if the player is changing lean
	* @retval false if the player is not changing lean
	*/
	__inline bool IsLeaning();

	/*!
	* This is called from idPlayer to adjust the camera before
	* rendering
	*/
	idAngles GetViewLeanAngles();

	/*
	* This is called from idPlayer to adjust the camera before
	* rendering
	*/
	idVec3 GetViewLeanTranslation();



};

#endif /* !__PHYSICS_PLAYER_H__ */
