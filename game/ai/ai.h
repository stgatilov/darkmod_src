/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __AI_H__
#define __AI_H__

#include "../../DarkMod/Relations.h"
#include "../../DarkMod/AI/Mind.h"
#include "../../DarkMod/AI/Subsystem.h"
#include "../../DarkMod/HidingSpotSearchCollection.h"
#include "../../DarkMod/darkmodHidingSpotTree.h"
#include "MoveState.h"

#include <list>
#include <set>

/*
===============================================================================

	idAI

===============================================================================
*/

const float	SQUARE_ROOT_OF_2			= 1.414213562f;
const float	AI_TURN_PREDICTION			= 0.2f;
const float	AI_TURN_SCALE				= 60.0f;
const float	AI_SEEK_PREDICTION			= 0.3f;
const float	AI_FLY_DAMPENING			= 0.15f;
const float	AI_HEARING_RANGE			= 2048.0f;
const int	DEFAULT_FLY_OFFSET			= 68;


// used to declare the Dark Mod Acuity values array.
// THIS MUST BE CHANGED if you want more than 15 acuities
static const int s_MAXACUITIES = 15;

#define ATTACK_IGNORE			0
#define ATTACK_ON_DAMAGE		1
#define ATTACK_ON_ACTIVATE		2
#define ATTACK_ON_SIGHT			4


enum ECombatType
{
	COMBAT_NONE,
	COMBAT_MELEE, 
	COMBAT_RANGED,
	NUM_COMBAT_TYPES
};

typedef enum {
	TALK_NEVER,
	TALK_DEAD,
	TALK_OK,
	TALK_BUSY,
	NUM_TALK_STATES
} talkState_t;

#define	DI_NODIR	-1

// obstacle avoidance
typedef struct obstaclePath_s {
	idVec3				seekPos;					// seek position avoiding obstacles
	idEntity *			firstObstacle;				// if != NULL the first obstacle along the path
	idVec3				startPosOutsideObstacles;	// start position outside obstacles
	idEntity *			startPosObstacle;			// if != NULL the obstacle containing the start position 
	idVec3				seekPosOutsideObstacles;	// seek position outside obstacles
	idEntity *			seekPosObstacle;			// if != NULL the obstacle containing the seek position 
	CBinaryFrobMover*	frobMoverObstacle;			// greebo: if != NULL, this is the door in our way
} obstaclePath_t;

// path prediction
typedef enum {
	SE_BLOCKED			= BIT(0),
	SE_ENTER_LEDGE_AREA	= BIT(1),
	SE_ENTER_OBSTACLE	= BIT(2),
	SE_FALL				= BIT(3),
	SE_LAND				= BIT(4)
} stopEvent_t;

typedef struct predictedPath_s {
	idVec3				endPos;						// final position
	idVec3				endVelocity;				// velocity at end position
	idVec3				endNormal;					// normal of blocking surface
	int					endTime;					// time predicted
	int					endEvent;					// event that stopped the prediction
	const idEntity *	blockingEntity;				// entity that blocks the movement
} predictedPath_t;

//
// events
//
extern const idEventDef AI_BeginAttack;
extern const idEventDef AI_EndAttack;
extern const idEventDef AI_MuzzleFlash;
extern const idEventDef AI_CreateMissile;
extern const idEventDef AI_AttackMissile;
extern const idEventDef AI_FireMissileAtTarget;
extern const idEventDef AI_AttackMelee;
extern const idEventDef AI_DirectDamage;
extern const idEventDef AI_JumpFrame;
extern const idEventDef AI_EnableClip;
extern const idEventDef AI_DisableClip;
extern const idEventDef AI_EnableGravity;
extern const idEventDef AI_DisableGravity;
extern const idEventDef AI_TriggerParticles;
extern const idEventDef AI_RandomPath;

// DarkMod Events
extern const idEventDef AI_GetRelationEnt;
extern const idEventDef AI_IsEnemy;
extern const idEventDef AI_IsFriend;
extern const idEventDef AI_IsNeutral;
extern const idEventDef AI_GetSndDir;
extern const idEventDef AI_GetVisDir;
extern const idEventDef AI_GetTactEnt;
extern const idEventDef AI_VisScan;
extern const idEventDef AI_Alert;
extern const idEventDef AI_GetAcuity;
extern const idEventDef AI_SetAcuity;
extern const idEventDef AI_SetAudThresh;
extern const idEventDef AI_ClosestReachableEnemy;

// Darkmod: Glass Houses events
extern const idEventDef AI_SpawnThrowableProjectile;

// DarkMod hiding spot finding events
extern const idEventDef AI_StartSearchForHidingSpots;
extern const idEventDef AI_ContinueSearchForHidingSpots;
extern const idEventDef AI_CloseHidingSpotSearch;
extern const idEventDef AI_GetNumHidingSpots;
extern const idEventDef AI_GetNthHidingSpotLocation;
extern const idEventDef AI_GetNthHidingSpotType;
extern const idEventDef AI_GetSomeOfOtherEntitiesHidingSpotList;

// Darkmod AI additions
extern const idEventDef AI_GetVariableFromOtherAI;
extern const idEventDef AI_GetAlertLevelOfOtherAI;

// Set a grace period for alerts
extern const idEventDef AI_SetAlertGracePeriod;

// This event is used to get a position from which a given position can be observed
extern const idEventDef AI_GetObservationPosition;

// Darkmod communication issuing event
extern const idEventDef AI_IssueCommunication;

class idPathCorner;

typedef struct particleEmitter_s {
	particleEmitter_s() {
		particle = NULL;
		time = 0;
		joint = INVALID_JOINT;
	};
	const idDeclParticle *particle;
	int					time;
	jointHandle_t		joint;
} particleEmitter_t;

class idAASFindCover : public idAASCallback {
public:
						idAASFindCover( const idActor* hidingActor, const idEntity* hideFromEnt, const idVec3 &hideFromPos );
						~idAASFindCover();

	virtual bool		TestArea( const idAAS *aas, int areaNum );

private:
	const idActor*		hidingActor;
	const idEntity*		hideFromEnt;
	idVec3				hideFromPos;
};

class idAASFindAreaOutOfRange : public idAASCallback {
public:
						idAASFindAreaOutOfRange( const idVec3 &targetPos, float maxDist );

	virtual bool		TestArea( const idAAS *aas, int areaNum );

private:
	idVec3				targetPos;
	float				maxDistSqr;
};

class idAASFindAttackPosition : public idAASCallback {
public:
						idAASFindAttackPosition( const idAI *self, const idMat3 &gravityAxis, idEntity *target, const idVec3 &targetPos, const idVec3 &fireOffset );
						~idAASFindAttackPosition();

	virtual bool		TestArea( const idAAS *aas, int areaNum );

private:
	const idAI			*self;
	idEntity			*target;
	idBounds			excludeBounds;
	idVec3				targetPos;
	idVec3				fireOffset;
	idMat3				gravityAxis;
	pvsHandle_t			targetPVS;
	int					PVSAreas[ idEntity::MAX_PVS_AREAS ];
};

class idAASFindObservationPosition : public idAASCallback {
public:
						idAASFindObservationPosition( const idAI *self, const idMat3 &gravityAxis, const idVec3 &targetPos, const idVec3 &eyeOffset, float maxDistanceFromWhichToObserve );
						~idAASFindObservationPosition();

	virtual bool		TestArea( const idAAS *aas, int areaNum );

	// Gets the best goal result, even if it didn't meet the maximum distance
	bool getBestGoalResult
	(
		float& out_bestGoalDistance,
		aasGoal_t& out_bestGoal
	);

private:
	const idAI			*self;
	idBounds			excludeBounds;
	idVec3				targetPos;
	idVec3				eyeOffset;
	idMat3				gravityAxis;
	pvsHandle_t			targetPVS;
	int					PVSAreas[ idEntity::MAX_PVS_AREAS ];
	float				maxObservationDistance;

	// The best goal found, even if it was greater than the maxObservationDistance
	float bestGoalDistance;
	bool b_haveBestGoal;
	aasGoal_t		bestGoal; 
};

class idAI : public idActor {
public:
	CLASS_PROTOTYPE( idAI );

							idAI();
							~idAI();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn( void );
	void					HeardSound( idEntity *ent, const char *action );
	idActor					*GetEnemy( void ) const;
	void					TalkTo( idActor *actor );
	talkState_t				GetTalkState( void ) const;

	bool					GetAimDir( const idVec3 &firePos, idEntity *aimAtEnt, const idEntity *ignore, idVec3 &aimDir ) const;

	void					TouchedByFlashlight( idActor *flashlight_owner );

							// Outputs a list of all monsters to the console.
	static void				List_f( const idCmdArgs &args );

							// Finds a path around dynamic obstacles.
	static bool				FindPathAroundObstacles( const idPhysics *physics, const idAAS *aas, const idEntity *ignore, const idVec3 &startPos, const idVec3 &seekPos, obstaclePath_t &path );
							// Frees any nodes used for the dynamic obstacle avoidance.
	static void				FreeObstacleAvoidanceNodes( void );
							// Predicts movement, returns true if a stop event was triggered.
	static bool				PredictPath( const idEntity *ent, const idAAS *aas, const idVec3 &start, const idVec3 &velocity, int totalTime, int frameTime, int stopEvent, predictedPath_t &path );
							// Return true if the trajectory of the clip model is collision free.
	static bool				TestTrajectory( const idVec3 &start, const idVec3 &end, float zVel, float gravity, float time, float max_height, const idClipModel *clip, int clipmask, const idEntity *ignore, const idEntity *targetEntity, int drawtime );
							// Finds the best collision free trajectory for a clip model.
	static bool				PredictTrajectory( const idVec3 &firePos, const idVec3 &target, float projectileSpeed, const idVec3 &projGravity, const idClipModel *clip, int clipmask, float max_height, const idEntity *ignore, const idEntity *targetEntity, int drawtime, idVec3 &aimDir );

	// Begin Dark Mod Functions:
	
	/**
	* Checks with the global Relationship Manager to see if the
	* other entity is an enemy of this AI.
	**/
	bool IsEnemy( idEntity *other );
	// As above, but checks for Friend
	bool IsFriend( idEntity *other );
	// As above, but checks for Neutral
	bool IsNeutral( idEntity *other );
	
	/**
	* Interface with Dark Mod Sound Propagation
	**/

	/**
	* Convert Sound Pressure Level from sound propagation
	* to psychoacoustic Loudness for the given AI
	* propVol is read from propParms, and 
	* loudness is set in propParms for later use.
	**/
	void SPLtoLoudness( SSprParms *propParms );
							
	/**
	* CheckHearing returns "true" if the sound is above
	* AI hearing threshold, without taking env. noise into 
	* account.
	**/
	bool CheckHearing( SSprParms *propParms );

	/**
	* Called by soundprop when AI hears a sound Assumes that CheckHearing
	* has been called and that the sound is above threshold without
	* considering environmental noise masking.
	**/
	void HearSound( SSprParms *propParms, float noise, const idVec3& origin );

	/**
	* Return the last point at which the AI heard a sound
	* Returns (0,0,0) if the AI didn't hear a sound.
	* Check AI_HEARDSOUND to see if the vector is valid.
	**/
	idVec3 GetSndDir( void );

	/**
	* Return the last point at which an AI glimpsed something suspicious.
	* Returns (0,0,0) if the AI was not visually alerted.
	* Check AI_VISALERT to see if the vector is valid.
	**/
	idVec3 GetVisDir( void );

	/**
	* Returns the entity that the AI is in tactile contact with
	**/
	idEntity *GetTactEnt( void );

	/**
	* Visual Alerts
	**/

	/**
	* Do a visibility calculation based on 2 things:
	* The lightgem value, and the distance to entity
	* NYI: take velocity into account
	*
	* The visibility can also be integrated over a number
	* of frames if we need to do that for optimization later.
	**/
	float GetVisibility( idEntity *ent ) const;

	/**
	* angua: The uncorrected linear light values [1..DARKMOD_LG_MAX] 
	* didn't produce very believable results when used 
	* in GetVisibility(). This function takes the linear values
	* and corrects them with an "empirical" correction curve.
	* 
	* @returns: a float between [0...1].It is fairly high at 
	* values above 20, fairly low below 6 and increases linearly in between.
	**/
	float GetCalibratedLightgemValue() const;

	/**
	* Checks enemies in the AI's FOV and calls Alert( "vis", amount )
	* The amount is calculated based on distance and the lightgem
	*
	* For now the check is only done on the player
	**/
	void PerformVisualScan( float time = 1.0f/60.0f );

	/**
	* Checks to see if the AI is being blocked by an actor when it tries to move,
	* call HadTactile this AI and if this is the case.
	*
	* greebo: Note: Only works if the AI is not Dead, KO or already engaged in combat.
	**/
	void CheckTactile();

	/**
	* Tactile Alerts:
	*
	* If no amount is entered, of the alert is defined in the global AI 
	* settings def file, and it also gets multiplied by the AI's specific
	* "acuity_tact" key in the spawnargs (defaults to 1.0)
	*
	* The amount is in alert units, so as usual 1 = barely noticible, 
	*	10 = twice as noticable, etc.
	**/
	void TactileAlert(idEntity* tactEnt, float amount = -1);

	/**
	* This is called in the frame if the AI bumped into another actor.
	* Checks the relationship to the AI, and calls TactileAlert appropriately.
	*
	* If the bumped actor is an enemy of the AI, the AI calls TactileAlert on itself
	*
	* If the bumped actor is an AI, and if this AI is an enemy of the bumped AI,
	* it calls TactileAlert on the bumped AI as well.
	**/
	void HadTactile( idActor *actor );

	/**
	* Generalized alerts and acuities
	**/

	/**
	* Alert the AI.  The first parameter is the alert type (same as acuity type)
	* The second parameter is the alert amount.
	* NOTE: For "alert units," an alert of 1 corresponds to just barely
	* seeing something or just barely hearing a whisper of a sound.
	**/
	void AlertAI( const char *type, float amount );

	/**
	 * greebo: Sets the AI_AlertLevel of this AI and updates the AI_AlertIndex.
	 *
	 * This also updates the grace timers, alert times and checks for
	 * a valid agitatedsearching>combat transition.
	 *
	 * Additionally, the transition alert sounds ("snd_alertdown2") are played.
	 */
	void SetAlertLevel(float newAlertLevel);


	// angua: returns true if the current alert index is higher 
	// than the previous one, false otherwise
	bool AlertIndexIncreased();

	/**
	* Returns the float val of the specific AI's acuity
	* Acuity type is a char, from the same list as alert types
	* That list is defined in DarkModGlobals.cpp
	**/
	float GetAcuity( const char *type ) const;

	/**
	* Sets the AI acuity for a certain type of alert.
	**/
	void SetAcuity( const char *type, float acuity );

	/**
	* Calls objective system when the AI finds a body
	**/
	void FoundBody( idEntity *body );

	/**
	* Get the volume modifier for a given movement type
	* Use the same function as idPlayer::GetMovementVolMod.
	* Unfortunately this is exactly the same as idPlayer::GetMovementVolMod
	* It's bad coding, but that's how D3 wrote the movement vars.
	**/
	float GetMovementVolMod( void );

	/**
	* Returns true if AI is knocked out
	**/
	bool  IsKnockedOut( void ) { return (AI_KNOCKEDOUT!=0); };

public:
	/**
	* DarkMod AI Member Vars
	**/
	
	/**
	* Set to true if the AI has been alerted in this frame
	**/
	idScriptBool			AI_ALERTED;

	/**
	* Stores the actor ultimately responsible for the alert.
	* If they find a body, this gets set to whoever killed/KO'd the body.
	* If they get hit by an item, or hear it fall, this gets set to whoever 
	*	threw the item, and so on
	**/
	idEntityPtr<idActor>	m_AlertedByActor;

	int						alertTypeWeight[ai::EAlertTypeCount];
	/**
	* Set these flags so we can tell if the AI is running or creeping
	* for the purposes of playing audible sounds and propagated sounds.
	**/
	idScriptBool			AI_CROUCH;
	idScriptBool			AI_RUN;
	idScriptBool			AI_CREEP;

	/****************************************************************************************
	*
	*	Added By Rich
	*
	****************************************************************************************/
	//virtual bool Collide( const trace_t &collision, const idVec3 &velocity );

	/****************************************************************************************
	*
	*	Added By Rich to implement AI Falling damage
	*	Called from Think with saved origin and velocity from before moving
	*   
	****************************************************************************************/
	void CrashLand( const idVec3 &oldOrigin, const idVec3 &oldVelocity );

	/**
	* greebo: Accessor methods for the airTicks member variable. 
	*/
	int		getAirTicks() const;
	void	setAirTicks(int airTicks);

	// greebo: Accessor methods for the array of subsystems
	ID_INLINE const ai::SubsystemPtr& GetSubsystem(ai::SubsystemId id)
	{
		return subsystems[id];
	}

	/**
	 * greebo: Replaces an AI subsystem with the given one. This removes the old
	 *         subsystem from the array (which usually triggers a shared_ptr destruction).
	 */ 
	ID_INLINE void InstallSubsystem(ai::SubsystemId id, const ai::SubsystemPtr& subsystem)
	{
		subsystems[id] = subsystem;
	}

	ID_INLINE ai::MindPtr& GetMind()
	{
		return mind;
	}

	ID_INLINE ai::Memory& GetMemory()
	{
		return mind->GetMemory();
	}

	ID_INLINE float GetMeleeRange() const
	{
		return melee_range;
	}

	ID_INLINE idAAS* GetAAS() const
	{
		return aas;
	}

	float GetArmReachLength();

	// Virtual override of idActor method, routes the call into the current Mind State
	virtual void NeedToUseElevator(const eas::RouteInfoPtr& routeInfo);


protected:
	// navigation
	idAAS *					aas;
	int						travelFlags;

	// greebo: When this AI has last re-evaluated a forbidden area (game time)
	int						lastAreaReevaluationTime;
	// The minimum time that needs to pass by before the AI re-evaluates a forbidden area (msec)
	int						maxAreaReevaluationInterval;
	// The time that needs to pass before locked doors are enabled for another try (msec)
	int						doorRetryTime;

	idMoveState				move;
	idMoveState				savedMove;

	// A stack of movestates, used for saving and restoring moves in PushMove() and PopMove()
	std::list<idMoveState>	moveStack;

	float					kickForce;
	bool					ignore_obstacles;
	float					blockedRadius;
	int						blockedMoveTime;
	int						blockedAttackTime;

	// turning
	float					ideal_yaw;
	float					current_yaw;
	float					turnRate;
	float					turnVel;
	float					anim_turn_yaw;
	float					anim_turn_amount;
	float					anim_turn_angles;

	// This expands the AABB a bit when the AI is checking for reached positions.
	float					reachedpos_bbox_expansion;

	// physics
	idPhysics_Monster		physicsObj;

	// flying
	jointHandle_t			flyTiltJoint;
	float					fly_speed;
	float					fly_bob_strength;
	float					fly_bob_vert;
	float					fly_bob_horz;
	int						fly_offset;					// prefered offset from player's view
	float					fly_seek_scale;
	float					fly_roll_scale;
	float					fly_roll_max;
	float					fly_roll;
	float					fly_pitch_scale;
	float					fly_pitch_max;
	float					fly_pitch;

	bool					allowMove;					// disables any animation movement
	bool					allowHiddenMovement;		// allows character to still move around while hidden
	bool					disableGravity;				// disables gravity and allows vertical movement by the animation
	
	// weapon/attack vars
	bool					lastHitCheckResult;
	int						lastHitCheckTime;
	int						lastAttackTime;
	float					melee_range;
	float					fire_range;
	float					projectile_height_to_distance_ratio;	// calculates the maximum height a projectile can be thrown
	idList<idVec3>			missileLaunchOffset;

	const idDict *			projectileDef;
	mutable idClipModel		*projectileClipModel;
	float					projectileRadius;
	float					projectileSpeed;
	idVec3					projectileVelocity;
	idVec3					projectileGravity;
	idEntityPtr<idProjectile> projectile;
	idStr					attack;

	// chatter/talking
	talkState_t				talk_state;
	idEntityPtr<idActor>	talkTarget;

	// cinematics
	int						num_cinematics;
	int						current_cinematic;

	bool					allowJointMod;
	idEntityPtr<idEntity>	focusEntity;
	idVec3					currentFocusPos;
	int						focusTime;
	int						alignHeadTime;
	int						forceAlignHeadTime;
	idAngles				eyeAng;
	idAngles				lookAng;
	idAngles				destLookAng;
	idAngles				lookMin;
	idAngles				lookMax;
	idList<jointHandle_t>	lookJoints;
	idList<idAngles>		lookJointAngles;
	float					eyeVerticalOffset;
	float					eyeHorizontalOffset;
	float					eyeFocusRate;
	float					headFocusRate;
	int						focusAlignTime;

	// special fx
	float					shrivel_rate;
	int						shrivel_start;
	
	bool					restartParticles;			// should smoke emissions restart
	bool					useBoneAxis;				// use the bone vs the model axis
	idList<particleEmitter_t> particles;				// particle data

	renderLight_t			worldMuzzleFlash;			// positioned on world weapon bone
	int						worldMuzzleFlashHandle;
	jointHandle_t			flashJointWorld;
	int						muzzleFlashEnd;
	int						flashTime;

	// joint controllers
	idAngles				eyeMin;
	idAngles				eyeMax;
	jointHandle_t			focusJoint;
	jointHandle_t			orientationJoint;

	typedef std::set<CBinaryFrobMover*> FrobMoverList;
	FrobMoverList			unlockableDoors;

public: // greebo: Made these public
	// enemy variables
	idEntityPtr<idActor>	enemy;
	idVec3					lastVisibleEnemyPos;
	idVec3					lastVisibleEnemyEyeOffset;
	idVec3					lastVisibleReachableEnemyPos;
	idVec3					lastReachableEnemyPos;
	bool					enemyReachable;
	bool					wakeOnFlashlight;

public: // greebo: Made these public for now, I didn't want to write an accessor for EVERYTHING
	// script variables
	idScriptBool			AI_TALK;
	idScriptBool			AI_DAMAGE;
	idScriptBool			AI_PAIN;
	idScriptFloat			AI_SPECIAL_DAMAGE;
	idScriptBool			AI_DEAD;
	idScriptBool			AI_KNOCKEDOUT;
	idScriptBool			AI_ENEMY_VISIBLE;
	idScriptBool			AI_ENEMY_IN_FOV;
	idScriptBool			AI_ENEMY_DEAD;
	idScriptBool			AI_MOVE_DONE;
	idScriptBool			AI_ONGROUND;
	idScriptBool			AI_ACTIVATED;
	idScriptBool			AI_FORWARD;
	idScriptBool			AI_JUMP;
	idScriptBool			AI_BLOCKED;
	idScriptBool			AI_OBSTACLE_IN_PATH;
	idScriptBool			AI_DEST_UNREACHABLE;
	idScriptBool			AI_HIT_ENEMY;
	idScriptBool			AI_PUSHED;

	/**
	* The following variables are set as soon as the AI
	* gets a certain type of alert, and never unset by the
	* game code.  They are only unset in scripting.  This is
	* to facilitate different script reactions to different kinds
	* of alerts.
	*
	* It's also done this way so that the AI will know if it has been
	* alerted even if it happened in a frame that the script did not check.
	*
	* This is to facilitate optimization by having the AI check for alerts
	* every N frames rather than every frame.
	**/


	/**
	* Set to true if the AI heard a suspicious sound.
	**/
	idScriptBool			AI_HEARDSOUND;

	/**
	* Set to true if the AI saw something suspicious.
	**/
	idScriptBool			AI_VISALERT;

	/**
	* Set to true if the AI was pushed by or bumped into an enemy.
	**/
	idScriptBool			AI_TACTALERT;

	/**
	* The current alert number of the AI.
	* This is checked to see if the AI should
	* change alert indices.  This var is very important!
	* NOTE: Don't change this directly. Instead, call Event_SetAlertLevel
	* to change it.
	**/
	idScriptFloat			AI_AlertLevel;
	
	/**
	* Current alert index of the AI. Is set based on AI_AlertLevel and the alert threshold values:
	* 	0 if AI_AlertLevel < thresh_2
	* 	1 if thresh_2 <= AI_AlertLevel < thresh_3
	* 	2 if thresh_3 <= AI_AlertLevel < thresh_4
	* 	3 if thresh_4 <= AI_AlertLevel < thresh_5
	* 	4 if thresh_5 <= AI_AlertLevel
	**/
	idScriptFloat			AI_AlertIndex;
	
	/* Additional scriptvars, imported from scripting. TODO: Document properly (for now, see script for docs) */
	idScriptVector			AI_lastAlertPosSearched; // greebo: TODO: Remove this, idVec3 doesn't work?
	idScriptFloat			AI_timeOfLastStimulusBark;
	idScriptFloat			AI_currentAlertLevelDuration;
	idScriptFloat			AI_currentAlertLevelStartTime;

	/**
	* Boolean scriptvars set to true if either ranged or melee weapons are drawn
	**/
	idScriptBool			AI_bMeleeWeapDrawn;
	idScriptBool			AI_bRangedWeapDrawn;

	/**
	* Stores the amount alerted in this frame
	* Used to compare simultaneous alerts, the smaller one is ignored
	* Should be cleared at the start of each frame.
	**/
	float					m_AlertLevelThisFrame;


	// angua: stores the previous alert index at alert index changes
	int						m_prevAlertIndex;

	// angua: the highest alert level/index the AI reached already 
	float						m_maxAlertLevel;
	int							m_maxAlertIndex;

	// angua: the alert level the AI had after the last alert level increase
	float						m_lastAlertLevel;

	/**
	* If true, the AI ignores alerts during all actions
	**/
	bool					m_bIgnoreAlerts;

	/**
	* Array containing the various AI acuities (visual, aural, tactile, etc)
	**/
	idList<float>			m_Acuities;

	/**
	* Audio detection threshold (in dB of Sound Pressure Level)
	* Sounds heard below this volume will be ignored (default is 20 dB)
	* Soundprop only goes down to 15 dB, although setting it lower than
	* this will still have some effect, since alert = volume - threshold
	**/
	float					m_AudThreshold;

	/**
	* Static visual distance cutoff that is calculated dynamically
	* from the other visual acuity settings.
	**/
	float					m_VisDistMax;

	/**
	* The loudest direction for the last suspicious sound the AI heard
	* is set to NULL if the AI has not yet heard a suspicious sound
	* Note suspicious sounds that are omnidirectional do not set this.
	* If no sound has been propagated it will be (0,0,0).
	**/
	idVec3					m_SoundDir;

	/**
	* Position of the last visual alert
	**/
	idVec3					m_LastSight;

	/**
	* The entity that last issued a tactile alert
	**/
	idEntityPtr<idEntity>	m_TactAlertEnt;

	/**
	* Alert Grace period variables :
	* Actor that the alert grace period applies to:
	**/
	idEntityPtr<idActor>	m_AlertGraceActor;

	/**
	* Time of the grace period start [ms]
	**/
	int						m_AlertGraceStart;

	/**
	* Duration of the grace period [ms]
	**/
	int						m_AlertGraceTime;

	/**
	* Alert number below which alerts are ignored during the grace period
	**/
	float					m_AlertGraceThresh;

	/**
	* Number of alerts ignored in this grace period
	**/
	int						m_AlertGraceCount;

	/**
	* Number of alerts it takes to override the grace period via sheer number
	* (This is needed to make AI rapidly come up to alert due to visual alert,
	* because visual alerts do not increase in magnitude but just come in more rapidly
	**/
	int						m_AlertGraceCountLimit;

	/**
	* The current mod hiding spot search of this AI, usually -1
	*/
	int						m_HidingSpotSearchHandle;

	/**
	* The spots resulting from the current search or gotten from
	* another AI.
	*/
	CDarkmodHidingSpotTree m_hidingSpots;


	/**
	* Used for drowning
	**/
	int						m_AirCheckTimer;

	bool					m_bCanDrown;

	/**
	* Head body ID on the AF, used by drowning
	**/
	int						m_HeadBodyID;

	/**
	* Head joint ID on the living AI (used by FOV and KOing)
	**/
	jointHandle_t			m_HeadJointID;

	/**
	* Current number of air ticks left for drowning
	**/
	int						m_AirTics;

	/**
	* Max number of air ticks for drowning
	**/
	int						m_AirTicksMax;

	/**
	* number of seconds between air checks
	**/
	int						m_AirCheckInterval;

	/**
	* Offset relative to the eye position, used to locate the mouth
	**/
	idVec3					m_MouthOffset;

	/**
	* Set to true if the AI can be KO'd (defaults to true)
	**/
	bool					m_bCanBeKnockedOut;

	/**
	 * greebo: Is set to TRUE if the AI is able to open/close doors at all.
	 */
	bool					m_bCanOperateDoors;

	/**
	 * angua: is set true while the AI is handling the door.
	 */
	bool					m_HandlingDoor;

	/**
	 * angua: is set true while the AI is handling an elevator.
	 */
	bool					m_HandlingElevator;

	/**
	* Head center offset in head joint coordinates, relative to head joint
	* When this offset is added to the head joint, we should be at the head center
	**/
	idVec3					m_HeadCenterOffset;
	
	// AI_AlertLevel thresholds for each alert level
	// Alert levels are: 1=slightly suspicious, 2=aroused, 3=investigating, 4=agitated investigating, 5=hunting
	float thresh_1, thresh_2, thresh_3, thresh_4, thresh_5;
	// Grace period info for each alert level
	float m_gracetime_1, m_gracetime_2, m_gracetime_3, m_gracetime_4;
	float m_gracefrac_1, m_gracefrac_2, m_gracefrac_3, m_gracefrac_4;
	int m_gracecount_1, m_gracecount_2, m_gracecount_3, m_gracecount_4;
	// De-alert times for each alert level
	float atime1, atime2, atime3, atime4;

	float atime1_fuzzyness, atime2_fuzzyness, atime3_fuzzyness, atime4_fuzzyness;

	// angua: Random head turning
	int m_timeBetweenHeadTurnChecks;
	float m_headTurnChanceIdle;
	float m_headTurnFactorAlerted;
	float m_headTurnMaxYaw;
	float m_headTurnMaxPitch;
	int m_headTurnMinDuration;
	int m_headTurnMaxDuration;

	// The mind of this AI
	ai::MindPtr mind;

	// The array of subsystems of this AI
	ai::SubsystemPtr subsystems[ai::SubsystemCount];

	//
	// ai/ai.cpp
	//


	/**
	* This internal method destroys the current hiding spot search
	* if it is not null.
	*/
	void destroyCurrentHidingSpotSearch();

	/*!
	* This method finds hiding spots in the bounds given by two vectors, and also excludes
	* any points contained within a different pair of vectors.
	*
	* The first paramter is a vector which gives the location of the
	* eye from which hiding is desired.
	*
	* The second vector gives the minimums in each dimension for the
	* search space.  
	*
	* The third and fourth vectors give the min and max bounds within which spots should be tested
	*
	* The fifth and sixth vectors give the min and max bounds of an area where
	*	spots should NOT be tested. This overrides the third and fourth parameters where they overlap
	*	(producing a dead zone where points are not tested)
	*
	* The seventh parameter gives the bit flags of the types of hiding spots
	* for which the search should look.
	*
	* The eighth parameter indicates an entity that should be ignored in
	* the visual occlusion checks.  This is usually the searcher itself but
	* can be NULL.
	*
	* This method will only start the search, if it returns 1, you should call
	* continueSearchForHidingSpots every frame to do more processing until that function
	* returns 0.
	*
	* The return value is a 0 for failure, 1 for success.
	*/
	int StartSearchForHidingSpotsWithExclusionArea
	(
		const idVec3& hideFromLocation,
		const idVec3& minBounds, 
		const idVec3& maxBounds, 
		const idVec3& exclusionMinBounds, 
		const idVec3& exclusionMaxBounds, 
		int hidingSpotTypesAllowed, 
		idEntity* p_ignoreEntity
	);

	/*
	* This method continues searching for hiding spots. It will only find so many before
	* returning so as not to cause long delays.  Detected spots are added to the currently
	* building hiding spot list.
	*
	* The return value is 0 if the end of the search was reached, or 1 if there
	* is more processing to do (call this method again next AI frame)
	*
	*/
	int ContinueSearchForHidingSpots();

	/*!
	* This method returns the Nth hiding spot location. 
	* Param is 0-based hiding spot index.
	*/
	idVec3 GetNthHidingSpotLocation(int hidingSpotIndex);

	/*!
	* This event splits off half of the hiding spot list of another entity
	* and sets our hiding spot list to the "taken" points.
	*
	* As such, it is useful for getting hiding spots from a seraching AI that this
	* AI is trying to assist.
	*
	* @param p_otherEntity The other entity who's hiding spots we are taking
	* 
	* @return The number of points in the list gotten
	*/
	int GetSomeOfOtherEntitiesHidingSpotList(idEntity* p_ownerOfSearch);

	void					SetAAS( void );
	virtual	void			DormantBegin( void );	// called when entity becomes dormant
	virtual	void			DormantEnd( void );		// called when entity wakes from being dormant
	void					Think( void );
	void					Activate( idEntity *activator );
	int						ReactionTo( const idEntity *ent );
	bool					CheckForEnemy( void );
	void					EnemyDead( void );

	
	/** 
	 * angua: Interleaved thinking optimization
	 * AI will only think once in a certain number of frames
	 * depending on player distance and whether the AI is in the player view
	 * (this includes movement, pathing, physics and the states and tasks)
	 */

	// This checks whether the AI should think in this frame
	bool					ThinkingIsAllowed();

	// Sets the frame number when the AI should think next time
	void					SetNextThinkFrame();

	// returns interleave think frames
	// the AI will only think once in this number of frames
	int						GetThinkInterleave();
	int						m_nextThinkFrame;

	// Below min dist, the AI thinks normally every frame.
	// Above max dist, the thinking frequency is given by max interleave think frames.
	// The thinking frequency increases linearly between min and max dist.
	int						m_maxInterleaveThinkFrames;
	float					m_minInterleaveThinkDist;
	float					m_maxInterleaveThinkDist;

	// the last time where the AI did its thinking (used for physics)
	int						m_lastThinkTime;

	virtual void			Hide( void );
	virtual void			Show( void );
	virtual bool			CanBecomeSolid();
	idVec3					FirstVisiblePointOnPath( const idVec3 origin, const idVec3 &target, int travelFlags );
	void					CalculateAttackOffsets( void );
	void					PlayCinematic( void );

	// movement
	virtual void			ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse );
	void					GetMoveDelta( const idMat3 &oldaxis, const idMat3 &axis, idVec3 &delta );
	void					CheckObstacleAvoidance( const idVec3 &goalPos, idVec3 &newPos );
	void					DeadMove( void );
	void					AnimMove( void );
	void					SlideMove( void );
	void					SittingMove();
	void					AdjustFlyingAngles( void );
	void					AddFlyBob( idVec3 &vel );
	void					AdjustFlyHeight( idVec3 &vel, const idVec3 &goalPos );
	void					FlySeekGoal( idVec3 &vel, idVec3 &goalPos );
	void					AdjustFlySpeed( idVec3 &vel );
	void					FlyTurn( void );
	void					FlyMove( void );
	void					StaticMove( void );

	// greebo: Overrides idActor::PlayFootStepSound()
	virtual void			PlayFootStepSound();

	// greebo: Plays the given bark sound (will override any sounds currently playing)
	virtual void			Bark(const idStr& soundName);

	// damage
	virtual bool			Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	virtual void			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );

	void					PostDeath();

	/**
	* TestKnockOutBlow is called when the AI is hit with a weapon with knockout capability.
	* This function tests the location hit, angle of the blow, and alert state of the AI.
	*
	* The "dir" vector is from the knockback of the weapon.  It is not used for now.
	*
	* tr is the trace from the weapon collision, bIsPowerBlow is set if the BJ was powered up
	*
	* Returns false if BJ attempt failed, or if already knocked out
	**/
	bool					TestKnockoutBlow( idEntity* attacker, idVec3 dir, trace_t *tr, bool bIsPowerBlow );  
	
	/**
	* Tells the AI to go unconscious.  Called by TestKnockoutBlow if successful,
	* also can be called by itself and is called by scriptevent Event_Knockout.
	*
	* @inflictor: This is the entity causing the knockout, can be NULL for "unknown originator".
	**/
	void					Knockout( idEntity* inflictor );

	/**
	 * greebo: Does a few things after the knockout animation has finished.
	 *         This includes setting the model, dropping attachments and starting ragdoll mode.
	 *         Note: Gets called by the Mind's KnockOutState.
	 */
	void					PostKnockOut();

	/**
	* Drop certain attachments and def_drop items when transitioning into a ragdoll state
	* Called by idActor::Killed and idActor::KnockedOut
	**/
	void					DropOnRagdoll( void );

	// navigation
	void					KickObstacles( const idVec3 &dir, float force, idEntity *alwaysKick );

	// greebo: For Documentation, see idActor class (this is an override).
	virtual bool			ReEvaluateArea(int areaNum);

	/**
	 * greebo: ReachedPos checks whether we the AI has reached the given target position <pos>.
	 *         The check is a bounds check ("bounds contain point?"), the bounds radius is 
	 *         depending on the given <moveCommand>.
	 *
	 * @returns: TRUE when the position has been reached, FALSE otherwise.
	 */
	bool					ReachedPos( const idVec3 &pos, const moveCommand_t moveCommand ) const;

	float					TravelDistance( const idVec3 &start, const idVec3 &end );

	/**
	 * greebo: Returns the number of the nearest reachable area for the given point. 
	 *         Depending on the move type, the call is routed to the AAS->PointReachableAreaNum 
	 *         function. The bounds are modified before submission to the AAS function.
	 *
	 * @returns: the areanumber of the given point, 0 == no area found.
	 */
	int						PointReachableAreaNum(const idVec3 &pos, const float boundsScale = 2.0f, const idVec3& offset = idVec3(0,0,0)) const;

	bool					PathToGoal( aasPath_t &path, int areaNum, const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin, idActor* actor ) const;
	void					DrawRoute( void ) const;
	bool					GetMovePos( idVec3 &seekPos );
	bool					MoveDone( void ) const;

	ID_INLINE moveType_t GetMoveType()
	{
		return move.moveType;
	}

	void					SetMoveType( int moveType );

	
	/**
	* This is a virtual override of the idActor method.  It takes lighting levels into consideration
	* additional to the ordinary FOV/trace check in idActor.
	*/
	virtual bool			CanSee( idEntity *ent, bool useFOV ) const;


	/**
	* This version can optionally use or not use lighting and fov
	*/
	virtual bool			CanSeeExt (idEntity* ent, bool useFOV, bool useLighting ) const;

	/**
	* This tests if a position is visible.  it can optionally use lighting and fov.
	*/
	virtual bool			CanSeePositionExt( idVec3 position, bool useFOV, bool useLighting );

	bool					EntityCanSeePos( idActor *actor, const idVec3 &actorOrigin, const idVec3 &pos );
	void					BlockedFailSafe( void );
	/**
	* Overloaded idActor::CheckFOV with FOV check that depends on head joint orientation
	**/
	virtual bool			CheckFOV( const idVec3 &pos ) const;

	/**
	* Darkmod enemy tracking: Is an entity shrouded in darkness?
	* @author: SophisticatedZombie
	*
	* @return true if the entity is in darkness
	* @return false if not
	*/
	bool IsEntityHiddenByDarkness (idEntity* p_entity) const;

	/**
	 * greebo: Returns TRUE if the entity is within the "attack_cone".
	 */
	bool EntityInAttackCone(idEntity* entity);

	/**
	 * Returns TRUE or FALSE, depending on the distance to the 
	 * given entity and the weapons attached to this AI.
	 * Melee AI normally perform a bounding box expansion check,
	 * ranged AI implement a visual test incl. lighting.
	 */
	bool CanHitEntity(idActor* entity, ECombatType combatType = COMBAT_NONE);


	// movement control
	void					StopMove( moveStatus_t status );
	bool					FaceEnemy( void );
	bool					FaceEntity( idEntity *ent );
	bool					DirectMoveToPosition( const idVec3 &pos );
	bool					MoveToEnemyHeight( void );
	bool					MoveOutOfRange( idEntity *entity, float range );
	const idVec3&			GetMoveDest() const;
	/**
	 * greebo: Flee from the given entity. Pass the maximum distance this AI should search escape areas in.
	 */
	bool					Flee(idEntity* entityToFleeFrom, int algorithm, int distanceOption);
	aasGoal_t				GetPositionWithinRange(const idVec3& targetPos);
	idVec3					GetObservationPosition (const idVec3& pointToObserve, const float visualAcuityZeroToOne);
	bool					MoveToAttackPosition( idEntity *ent, int attack_anim );
	bool					MoveToEnemy( void );
	bool					MoveToEntity( idEntity *ent );

	/**
	 * greebo: This moves the entity to the given point.
	 *
	 * @returns: FALSE, if the position is not reachable (AI_DEST_UNREACHABLE && AI_MOVE_DONE == true)
	 * @returns: TRUE, if the position is reachable and the AI is moving (AI_MOVE_DONE == false) 
	 *                 OR the position is already reached (AI_MOVE_DONE == true).
	 */
	bool					MoveToPosition( const idVec3 &pos );

	/**
	 * angua: This looks for a suitable position for taking cover
	 *
	 * @returns: FALSE, if no suitable position is found
	 * @returns: TRUE, if the position is reachable 
	 * The position is stored in <hideGoal>.           
	 */	
	bool					LookForCover(aasGoal_t& hideGoal, idEntity *entity, const idVec3 &pos );
	bool					MoveToCover( idEntity *entity, const idVec3 &pos );
	bool					SlideToPosition( const idVec3 &pos, float time );
	bool					WanderAround( void );
	/**
	* Ish : Move AI along a vector without worrying about AAS or obstacles
	* Can be used for direct control of an AI
	* Applies finite turn speed toward the direction
	**/
	bool					MoveAlongVector( float yaw );
	bool					StepDirection( float dir );
	bool					NewWanderDir( const idVec3 &dest );

	// greebo: These two Push/Pop commands can be used to save the current move state in a stack and restore it later on
	void					PushMove();
	void					PopMove();

	// Local helper function, will restore the current movestate from the given saved one
	void					RestoreMove(const idMoveState& saved);

	// effects
	const idDeclParticle	*SpawnParticlesOnJoint( particleEmitter_t &pe, const char *particleName, const char *jointName );
	void					SpawnParticles( const char *keyName );
	bool					ParticlesActive( void );

	// turning
	bool					FacingIdeal( void );
	void					Turn( void );
	bool					TurnToward( float yaw );
	bool					TurnToward( const idVec3 &pos );
	ID_INLINE float			GetCurrentYaw() { return current_yaw; }
	ID_INLINE float			GetTurnRate() { return turnRate; }
	ID_INLINE void			SetTurnRate(float newTurnRate) { turnRate = newTurnRate; }

	// enemy management
	void					ClearEnemy( void );
	bool					EnemyPositionValid( void ) const;

	/**
	 * greebo: SetEnemyPos() tries to determine the enemy location and to setup a path
	 *         to the enemy, depending on the AI's move type (FLY/WALK).
	 *
	 * If this method succeeds in setting up a path to the enemy, the following members are
	 * set: lastVisibleReachableEnemyPos, lastVisibleReachableEnemyAreaNum
	 * and the movecommands move.toAreaNum and move.dest are updated, but the latter two 
	 * ONLY if the movecommand is set to MOVE_TO_ENEMY beforehand.
	 *
	 * The AI_DEST_UNREACHABLE is updated if the movecommand is currently set to MOVE_TO_ENEMY. 
	 *
	 * It is TRUE (enemy unreachable) in the following cases:
	 * - Enemy is not on ground (OnLadder) for non-flying AI.
	 * - The entity area number could not be determined.
	 * - PathToGoal failed, no path to the enemy could be found.
	 * 
	 * Note: This overwrites a few lastVisibleReachableEnemyPos IN ANY CASE with the 
	 * lastReachableEnemyPos, so this is kind of cheating if the enemy is not visible before calling this.
	 * Also, lastVisibleEnemyPos is overwritten by the enemy's origin IN ANY CASE.
	 *
	 * Basically, this method relies on "lastReachableEnemyPos" being set beforehand.
	 */
	void					SetEnemyPosition();

	/**
	 * greebo: This is pretty similar to SetEnemyPosition, but not the same.
	 *
	 * First, this tries to locate the current enemy position (disregards visibility!) and
	 * to set up a path to the entity's origin/floorposition. If this succeeds,
	 * the "lastReachableEnemyPos" member is updated, but ONLY if the enemy is on ground.
	 *
	 * Second, if the enemy is visible or heard, SetEnemyPosition is called, which updates
	 * the lastVisibleReachableEnemyPosition.
	 */
	void					UpdateEnemyPosition();

	/**
	 * greebo: Updates the enemy pointer and tries to set up a path to the enemy by
	 *         using SetEnemyPosition(), but ONLY if the enemy has actually changed.
	 *
	 * AI_ENEMY_DEAD is updated at any rate.
	 *
	 * @returns: TRUE if the enemy has been set and is non-NULL, FALSE if the enemy
	 *           is dead or has been cleared by anything else.
	 */
	bool					SetEnemy(idActor *newEnemy);

	/**
	* DarkMod: Ishtvan note:
	* Before I added this, this code was only called in
	* Event_FindEnemy, so it could be used by scripting, but
	* not by the SDK.  I just moved the code to a new function,
	* and made Event_FindEnemy call this.
	*
	* This was because I needed to use FindEnemy in the visibility
	* calculation.
	**/
	idActor * FindEnemy( bool useFOV ) ;

	idActor* FindEnemyAI(bool useFOV);

	idActor* FindFriendlyAI(int requiredTeam);


	/**
	* Similarly to FindEnemy, this was previously only an Event_ scripting
	* function.  I moved it over to a new SDK function and had the Event_ 
	* call it, in case we want to use this later.  It returns the closest
	* AI or Player enemy.
	*
	* It was originally used to get tactile alerts, but is no longer used for that
	* IMO we should leave it in though, as we might use it for something later,
	* like determining what targets to engage with ranged weapons.
	**/	
	idActor * FindNearestEnemy( bool useFOV = true );


	/**
	 * angua: this returns if the AI has seen evidence of an intruder already 
	 * (the enemy, a body, missing loot...)
	 */
	bool HasSeenEvidence();

	/**
	* Draw the debug cone representing valid knockout area
	* Called every frame when cvar cv_ai_ko_show is set to true.
	**/
	void KnockoutDebugDraw( void );

	/**
	* Draw the debug cone representing the FOV
	* Called every frame when cvar cv_ai_fov_show is set to true.
	**/
	void FOVDebugDraw( void );

	/**
	* This method calculates the maximum distance froma given
	* line segment that the segment is visible due to current light conditions
	* at the segment
	*/
	float GetMaximumObservationDistance (idVec3 bottomPoint, idVec3 topPoint, idEntity* p_ignoreEntity) const;

	/**
	* The point of this function is to determine the visual stimulus level caused
	* by addition of the CVAR tdm_ai_sight_mag. The current alert level is taken 
	* as reference value and the difference in logarithmic alert units is returned.
	*/
	float GetPlayerVisualStimulusAmount() const;

	// attacks
	void					CreateProjectileClipModel( void ) const;
	idProjectile			*CreateProjectile( const idVec3 &pos, const idVec3 &dir );
	void					RemoveProjectile( void );
	idProjectile			*LaunchProjectile( const char *jointname, idEntity *target, bool clampToAttackCone );
	virtual void			DamageFeedback( idEntity *victim, idEntity *inflictor, int &damage );
	void					DirectDamage( const char *meleeDefName, idEntity *ent );
	bool					TestMelee( void ) const;
	bool					TestRanged( void ) const;
	bool					AttackMelee( const char *meleeDefName );
	void					BeginAttack( const char *name );
	void					EndAttack( void );
	void					PushWithAF( void );

	// special effects
	void					GetMuzzle( const char *jointname, idVec3 &muzzle, idMat3 &axis );
	void					InitMuzzleFlash( void );
	void					TriggerWeaponEffects( const idVec3 &muzzle );
	void					UpdateMuzzleFlash( void );
	virtual bool			UpdateAnimationControllers( void );
	void					UpdateParticles( void );
	void					TriggerParticles( const char *jointName );

	// AI script state management
	void					LinkScriptVariables( void );
	virtual void			UpdateScript(); // overrides idActor::UpdateScript

	// Returns true if the current enemy can be reached
	bool					CanReachEnemy();

	// Returns the current move status (MOVE_STATUS_MOVING, for instance).
	moveStatus_t			GetMoveStatus() const;

	/**
	* Returns true if AI's mouth is underwater
	**/
	bool MouthIsUnderwater( void );

	/**
	* Checks for drowning, damages if drowning.
	*
	* greebo: Only enabled if the entity is able to drown and the 
	* interleave timer is elapsed (is not checked each frame).
	**/
	void					UpdateAir();

	/**
	* Halts lipsync
	**/
	void					StopLipSync();

	/**
	 * Plays and lipsyncs the given sound name, returns the duration in msecs.
	 */
	int						PlayAndLipSync(const char *soundName, const char *animName);

	// Lip sync stuff
	bool					m_lipSyncActive; /// True iff we're currently lip syncing
	int						m_lipSyncAnim; /// The number of the animation that we are lipsyncing to
	int						m_lipSyncEndTimer; /// Time at which to stop lip syncing
	
	/** Call the script function DrawWeapon (in a new thread) if it exists */
	void					DrawWeapon();
	/** Call the script function SheathWeapon (in a new thread) if it exists */
	void					SheathWeapon();

	// angua: this is used to check whether the AI is able to unlock a specific door
	bool					CanUnlock(CBinaryFrobMover *frobMover);

	// angua: this checks whether the AI should close the door after passing through
	bool					ShouldCloseDoor(CBinaryFrobMover *frobMover);

	
	//
	// ai/ai_events.cpp
	//
	// The post-spawn event parses the spawnargs which refer to other entities
	// that might not be available at spawn time.
	void					Event_PostSpawn();
	void					Event_Activate( idEntity *activator );

/*****
* DarkMod: Event_Touch was modified to issue a tactile alert.
*
* Note: Event_Touch checks ReactionTo, which checks our DarkMod Relations.
* So it will only go off if the AI is bumped by an enemy that moves into it.
* This is NOT called when an AI moves into an enemy.
*
* AI bumping by inanimate objects is handled separately by CheckTactile.
****/
	void					Event_Touch( idEntity *other, trace_t *trace );

	void					Event_FindEnemy( int useFOV );
	void					Event_FindEnemyAI( int useFOV );
	void					Event_FindEnemyInCombatNodes( void );

	/**
	 * greebo: Finds the nearest friendly and visible AI. Used to look for allies.
	 *         The <team> argument is optional and can be used to limit the search to a given team.
	 *         Set <team> to -1 to disable the team search.
	 *
	 *         Returns the best candidate (can be the nullentity) to the script thread.
	 */
	void					Event_FindFriendlyAI(int requiredTeam);

	void					Event_ClosestReachableEnemyOfEntity( idEntity *team_mate );
	void					Event_HeardSound( int ignore_team );
	void					Event_SetEnemy( idEntity *ent );
	void					Event_ClearEnemy( void );
	void					Event_FoundBody( idEntity *body );
	void					Event_MuzzleFlash( const char *jointname );
	void					Event_CreateMissile( const char *jointname );
	void					Event_AttackMissile( const char *jointname );
	void					Event_FireMissileAtTarget( const char *jointname, const char *targetname );
	void					Event_LaunchMissile( const idVec3 &muzzle, const idAngles &ang );
	void					Event_AttackMelee( const char *meleeDefName );
	void					Event_DirectDamage( idEntity *damageTarget, const char *damageDefName );
	void					Event_RadiusDamageFromJoint( const char *jointname, const char *damageDefName );
	void					Event_BeginAttack( const char *name );
	void					Event_EndAttack( void );
	void					Event_MeleeAttackToJoint( const char *jointname, const char *meleeDefName );
	void					Event_RandomPath( void );
	void					Event_CanBecomeSolid( void );
	void					Event_BecomeSolid( void );
	void					Event_BecomeNonSolid( void );
	void					Event_BecomeRagdoll( void );
	void					Event_StopRagdoll( void );
	void					Event_SetHealth( float newHealth );
	void					Event_GetHealth( void );
	void					Event_AllowDamage( void );
	void					Event_IgnoreDamage( void );
	void					Event_GetCurrentYaw( void );
	void					Event_TurnTo( float angle );
	void					Event_TurnToPos( const idVec3 &pos );
	void					Event_TurnToEntity( idEntity *ent );
	void					Event_MoveStatus( void );
	void					Event_StopMove( void );
	void					Event_MoveToCover( void );
	void					Event_MoveToCoverFrom( idEntity *enemyEnt );
	void					Event_MoveToEnemy( void );
	void					Event_MoveToEnemyHeight( void );
	void					Event_MoveOutOfRange( idEntity *entity, float range );
	void					Event_Flee(idEntity* entityToFleeFrom, int algorithm, int distanceOption);
	void					Event_MoveToAttackPosition( idEntity *entity, const char *attack_anim );
	void					Event_MoveToEntity( idEntity *ent );
	void					Event_MoveToPosition( const idVec3 &pos );
	void					Event_SlideTo( const idVec3 &pos, float time );
	void					Event_Wander( void );
	void					Event_FacingIdeal( void );
	void					Event_FaceEnemy( void );
	void					Event_FaceEntity( idEntity *ent );
	void					Event_WaitAction( const char *waitForState );
	void					Event_GetCombatNode( void );
	void					Event_EnemyInCombatCone( idEntity *ent, int use_current_enemy_location );
	void					Event_WaitMove( void );
	void					Event_GetJumpVelocity( const idVec3 &pos, float speed, float max_height );
	void					Event_EntityInAttackCone( idEntity *ent );
	void					Event_CanSeeEntity( idEntity *ent );
	void					Event_CanSeeEntityExt( idEntity *ent, int useFOV, int useLighting);
	void					Event_CanSeePositionExt( const idVec3& position, int useFOV, int useLighting);
	void					Event_SetTalkTarget( idEntity *target );
	void					Event_GetTalkTarget( void );
	void					Event_SetTalkState( int state );
	void					Event_EnemyRange( void );
	void					Event_EnemyRange2D( void );
	void					Event_GetEnemy( void );
	void					Event_GetEnemyPos( void );
	void					Event_GetEnemyEyePos( void );
	void					Event_PredictEnemyPos( float time );
	void					Event_CanHitEnemy( void );
	void					Event_CanHitEnemyFromAnim( const char *animname );
	void					Event_CanHitEnemyFromJoint( const char *jointname );
	void					Event_EnemyPositionValid( void );
	void					Event_ChargeAttack( const char *damageDef );
	void					Event_TestChargeAttack( void );
	void					Event_TestAnimMoveTowardEnemy( const char *animname );
	void					Event_TestAnimMove( const char *animname );
	void					Event_TestMoveToPosition( const idVec3 &position );
	void					Event_TestMeleeAttack( void );
	void					Event_TestAnimAttack( const char *animname );
	void					Event_Shrivel( float shirvel_time );
	void					Event_Burn( void );
	void					Event_PreBurn( void );
	void					Event_ClearBurn( void );
	void					Event_SetSmokeVisibility( int num, int on );
	void					Event_NumSmokeEmitters( void );
	void					Event_StopThinking( void );
	void					Event_GetTurnDelta( void );
	void					Event_GetMoveType( void );
	void					Event_SetMoveType( int moveType );
	void					Event_SaveMove( void );
	void					Event_RestoreMove( void );
	void					Event_AllowMovement( float flag );
	void					Event_JumpFrame( void );
	void					Event_EnableClip( void );
	void					Event_DisableClip( void );
	void					Event_EnableGravity( void );
	void					Event_DisableGravity( void );
	void					Event_EnableAFPush( void );
	void					Event_DisableAFPush( void );
	void					Event_SetFlySpeed( float speed );
	void					Event_SetFlyOffset( int offset );
	void					Event_ClearFlyOffset( void );
	void					Event_GetClosestHiddenTarget( const char *type );
	void					Event_GetRandomTarget( const char *type );
	void					Event_TravelDistanceToPoint( const idVec3 &pos );
	void					Event_TravelDistanceToEntity( idEntity *ent );
	void					Event_TravelDistanceBetweenPoints( const idVec3 &source, const idVec3 &dest );
	void					Event_TravelDistanceBetweenEntities( idEntity *source, idEntity *dest );
	void					Event_LookAtEntity( idEntity *ent, float duration );
	void					Event_LookAtEnemy( float duration );
	void					Event_LookAtPosition (const idVec3& lookAtWorldPosition, float duration);
	void					Event_LookAtAngles (float yawAngleClockwise, float pitchAngleUp, float rollAngle, float durationInSeconds);
	void					Event_SetJointMod( int allowJointMod );
	void					Event_ThrowMoveable( void );
	void					Event_ThrowAF( void );
	void					Event_SetAngles( idAngles const &ang );
	void					Event_GetAngles( void );
	void					Event_RealKill( void );
	void					Event_Kill( void );
	void					Event_WakeOnFlashlight( int enable );
	void					Event_LocateEnemy( void );
	void					Event_KickObstacles( idEntity *kickEnt, float force );
	void					Event_GetObstacle( void );
	void					Event_PushPointIntoAAS( const idVec3 &pos );
	void					Event_GetTurnRate( void );
	void					Event_SetTurnRate( float rate );
	void					Event_AnimTurn( float angles );
	void					Event_AllowHiddenMovement( int enable );
	void					Event_TriggerParticles( const char *jointName );
	void					Event_FindActorsInBounds( const idVec3 &mins, const idVec3 &maxs );
	void 					Event_CanReachPosition( const idVec3 &pos );
	void 					Event_CanReachEntity( idEntity *ent );
	void					Event_CanReachEnemy( void );
	void					Event_GetReachableEntityPosition( idEntity *ent );

	// Script interface for state manipulation
	void					Event_PushState(const char* state);
	void					Event_SwitchState(const char* state);
	void					Event_EndState();

	void					Event_PlayAndLipSync( const char *soundName, const char *animName );
	
	/**
	* Frontend scripting functions for Dark Mod Relations Manager
	* See CRelations class definition for descriptions
	**/
	void					Event_GetRelationEnt( idEntity *ent );
	void					Event_IsEnemy( idEntity *ent );
	void					Event_IsFriend( idEntity *ent );
	void					Event_IsNeutral( idEntity *ent );
	
	void					Event_SetAlertLevel( float newAlertLevel );

	/**
	* Script frontend for idAI::GetAcuity and idAI::SetAcuity
	* and idAI::AlertAI
	**/
	void Event_Alert( const char *type, float amount );
	void Event_GetAcuity( const char *type );
	void Event_SetAcuity( const char *type, float val );
	void Event_GetAudThresh( void );
	void Event_SetAudThresh( float val );

	/**
	* Get the actor that alerted the AI this frame
	**/
	void Event_GetAlertActor( void );

	/**
	* Set an alert grace period
	* First argument is the fraction of the current alert this frame to ignore
	* Second argument is the number of SECONDS the grace period lasts.
	* Third argument is the number of events it takes to override the grace period
	**/
	void Event_SetAlertGracePeriod( float frac, float duration, int count );

    /**
	* Script frontend for DarkMod hiding spot detection functions
	**/
	void Event_StartSearchForHidingSpots (const idVec3& hideFromLocation, const idVec3 &minBounds, const idVec3 &maxBounds, int hidingSpotTypesAllowed, idEntity* p_ignoreEntity); 
	void Event_StartSearchForHidingSpotsWithExclusionArea (const idVec3& hideFromLocation, const idVec3 &minBounds, const idVec3 &maxBounds, const idVec3 &exclusionMinBounds, const idVec3 &exclusionMaxBounds, int hidingSpotTypesAllowed, idEntity* p_ignoreEntity); 
	void Event_ContinueSearchForHidingSpots(); 
	void Event_CloseHidingSpotSearch ();
	void Event_ResortHidingSpots ( const idVec3& searchCenter, const idVec3& searchRadius);
	void Event_GetNumHidingSpots ();
	void Event_GetNthHidingSpotLocation (int hidingSpotIndex);
	void Event_GetNthHidingSpotType (int hidingSpotIndex);
	void Event_GetObservationPosition (const idVec3& pointToObserve, const float visualAcuityZeroToOne );
	void Event_GetSomeOfOtherEntitiesHidingSpotList (idEntity* p_ownerOfSearch);

	/**
	* Gets the alert number of an entity that is another AI.
	* Will return 0.0 to script if other entity is NULL or not an AI.
	*/
	void Event_GetAlertLevelOfOtherAI (idEntity* p_otherEntity);

	/*!
	* This event is used by an AI script to issue a message to other AI's through
	* the communication stim/response mechanism.  The message is added to the
	* caller's Communication Stim in their Stim/Response Collection. Because
	* there is no way to pass a NULL entity, there are several forms of the
	* functions. Here are the naming conventions:
	*	IR = intended recipient included
	*	DOE = direct object entity included
	* 
	* @param a message type enumeration value
	*
	* @param The maximum distance of the communications stim
	*
	* @param Pointer to the intended recipient entity. This can be NULL if there
	*		is no specific intended recipient.
	*
	* @param directObjectEntity Pointer to the entity this communication is about, 
	*		this can be null if it doesn't apply to the message type
	*
	* @param directObjectLocation World position that the communication is aobut,
	*		this can be null if it doesn't apply to the message type
	*
	*/
	void IssueCommunication_Internal (float messageType, float maxRadius, idEntity* intendedRecipientEntity, idEntity* directObjectEntity, const idVec3& directObjectLocation);
	void Event_IssueCommunication_IR_DOE ( float messageType, float maxRadius, idEntity* intendedRecipientEntity, idEntity* directObjectEntity, const idVec3& directObjectLocation);
	void Event_IssueCommunication_IR ( float messageType, float maxRadius, idEntity* intendedRecipientEntity, const idVec3& directObjectLocation);
	void Event_IssueCommunication_DOE ( float messageType, float maxRadius, idEntity* directObjectEntity, const idVec3& directObjectLocation);
	void Event_IssueCommunication ( float messageType, float maxRadius, const idVec3& directObjectLocation);


	void Event_ProcessBlindStim(idEntity* stimSource, int skipVisibilityCheck);
	/**
	 * greebo: Script event for processing a visual stim coming from the entity <stimSource>
	 */
	void Event_ProcessVisualStim(idEntity* stimSource);

	/*!
	* Spawns a new stone projectile that the AI can throw
	*
	* @param pstr_projectileName Name of the projectile type to
	*	be spawned (as given in .def file)
	*
	* @param pstr_jointName Name of the model joint to which the
	*	stone projectile will be bound until thrown.
	*/
	void Event_SpawnThrowableProjectile ( const char* pstr_projectileName, const char* pstr_jointName );

	/**
	* Scan for the player in FOV, and cause a visual alert if found
	* Currently only checks the player.
	* Will return the entity that caused the visual alert for 
	* scripting purposes.
	**/
	void Event_VisScan( void );
	
	/**
	* Return the last suspicious sound direction
	**/
	void Event_GetSndDir( void );

	/**
	* Return the last visual alert position
	**/
	void Event_GetVisDir( void );

	/**
	* Return the entity that the AI is in tactile contact with
	**/
	void Event_GetTactEnt( void );

	/**
	* This is needed for accurate AI-AI combat
	* It just calls the vanilla D3 event:
	* Event_ClosestReachableEnemyToEntity( idEntity *ent )
	* with the "this" pointer.
	*
	* For some reason this was left out of D3.
	**/
	void Event_ClosestReachableEnemy( void );

#ifdef TIMING_BUILD
private:
	int aiThinkTimer;
	int aiMindTimer;
#endif
};

class idCombatNode : public idEntity {
public:
	CLASS_PROTOTYPE( idCombatNode );

						idCombatNode();

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	void				Spawn( void );
	bool				IsDisabled( void ) const;
	bool				EntityInView( idActor *actor, const idVec3 &pos );
	static void			DrawDebugInfo( void );

private:
	float				min_dist;
	float				max_dist;
	float				cone_dist;
	float				min_height;
	float				max_height;
	idVec3				cone_left;
	idVec3				cone_right;
	idVec3				offset;
	bool				disabled;

	void				Event_Activate( idEntity *activator );
	void				Event_MarkUsed( void );
};

#endif /* !__AI_H__ */
