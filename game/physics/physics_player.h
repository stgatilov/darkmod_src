/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
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

// movementType
typedef enum {
	PM_NORMAL,				// normal physics
	PM_DEAD,				// no acceleration or turning, but free falling
	PM_SPECTATOR,			// flying without gravity but with collision detection
	PM_FREEZE,				// stuck in place without control
	PM_NOCLIP				// flying without collision detection nor gravity
} pmtype_t;

typedef enum {
	WATERLEVEL_NONE,
	WATERLEVEL_FEET,
	WATERLEVEL_WAIST,
	WATERLEVEL_HEAD
} waterLevel_t;

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
	waterLevel_t			GetWaterLevel( void ) const;
	int						GetWaterType( void ) const;
	bool					HasJumped( void ) const;
	bool					HasSteppedUp( void ) const;
	float					GetStepUp( void ) const;
	bool					IsCrouching( void ) const;
	bool					OnLadder( void ) const;
	const idVec3 &			PlayerGetOrigin( void ) const;	// != GetOrigin

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

	// ladder movement
	bool					ladder;
	idVec3					ladderNormal;

	// results of last evaluate
	waterLevel_t			waterLevel;
	int						waterType;

private:
	float					CmdScale( const usercmd_t &cmd ) const;
	void					Accelerate( const idVec3 &wishdir, const float wishspeed, const float accel );
	bool					SlideMove( bool gravity, bool stepUp, bool stepDown, bool push );
	void					Friction( void );
	void					WaterJumpMove( void );
	void					WaterMove( void );
	void					FlyMove( void );
	void					AirMove( void );
	void					WalkMove( void );
	void					DeadMove( void );
	void					NoclipMove( void );
	void					SpectatorMove( void );
	void					LadderMove( void );
	void					CorrectAllSolid( trace_t &trace, int contents );
	void					CheckGround( void );
	void					CheckDuck( void );
	void					CheckLadder( void );
	bool					CheckJump( void );
	bool					CheckWaterJump( void );
	void					SetWaterLevel( void );
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


	// The current mantling phase
	EDarkMod_MantlePhase m_mantlePhase;

	// Point being mantled to...
	idVec3 m_mantlePullStartPos;
	idVec3 m_mantlePullEndPos;
	idVec3 m_mantlePushEndPos;

	// Pointer to the entity being mantled.
	// This is undefined if m_mantlePhase == notMantling_DarkModMantlePhase
	idEntity* p_mantledEntity;
	int mantledEntityID;

	// How long will the current phase of the mantle operation take?
	// Uses the same time unit as other movement times.
	float m_mantleTime;

	// Jump held down timer
	// Times how long jump has been held down
	float m_jumpHeldDownTime;

	// This method determines the mantle time required for each phase of the mantle.
	// I made this a function so you could implement things such as carry-weight,
	// tiredness, length of lift....
	float getMantleTimeForPhase (EDarkMod_MantlePhase mantlePhase);

	// Internal method to start the mantle operation
	void StartMantle
	(
		EDarkMod_MantlePhase initialMantlePhase,
		idVec3 eyePos,
		idVec3 startPos,
		idVec3 endPos
	);

	// Timer change methods
	void UpdateMantleTimers();
    
	// Movement methods
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

	// Starts or ends a lean around and axis which is perpendicular to the gravity normal and
	// rotated by the given yaw angle clockwise (when looking down in direction of
	// gravity) around it. 
	float m_leanYawAngleDegrees;

	// The current lean angle
	float m_currentLeanTiltDegrees;

	// Is the player trying to lean
	bool m_b_tryingToLean;

	// Is the lean finished
	bool m_b_leanFinished;
	
	// The current resulting view lean angles
	idAngles m_viewLeanAngles;

	// The current resulting view lean translation
	idVec3 m_viewLeanTranslation;

	// This method handles the change to the player view angles
	// and bounding box during a lean
	void LeanMove();

	// This method is required for leaning to work right.
	// By default, Doom 3 does not orient the player's clipmodel at all.
	// (Physics_Player::Rotate is never caled).  So the player's clipmodel
	// always faces north.
	// Leaning is orientation sensitive, so this method is used to set
	// the clip model orientation to match the portions of the 
	// view direction perpendicular to the gravity normal. (appropriate for mouse look)
	// This is called within LeanMove whether or not a lean is taking place.
	void UpdateClipModelOrientation();

	// This method is used to update the lean angles and translation
	// It needs to be called every MovePlayer cycle, because if the player
	// is leaning and then looks around, these values change.
	// This is called within LeanMove whether or not a lean is taking place.
	void UpdateViewLeanAnglesAndTranslation();

	// This method updates the lean by as much of the delta amount given
	// that does not result in a clip model trace collision.
	// This is an internal method called by LeanMove
	void UpdateLeanAngle (float deltaLeanAngle);

public:

	// Starts or ends a lean around and axis which is perpendicular to the gravity normal and
	// rotated by the given yaw angle clockwise (when looking down in direction of
	// gravity) around it. 
	// 0.0 leans right, 180.0 leans left, 90.0 leans forward, 270.0 leans backward
	void ToggleLean
	(
		float leanYawAngleDegrees
	);

	// This method returns true if a lean movement is being executed
	// or is still cancelling.
	// It only returns false if the lean is fully off.
	__inline bool IsLeaning();

	// These are called from idPlayer to adjust the camera viewpoint before
	// rendering
	idAngles GetViewLeanAngles();
	idVec3 GetViewLeanTranslation();



};

#endif /* !__PHYSICS_PLAYER_H__ */
