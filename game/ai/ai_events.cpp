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

#include "../../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include "../game_local.h"
#include "../../DarkMod/Relations.h"
#include "../../DarkMod/DarkModGlobals.h"
#include "../../DarkMod/darkModAASFindHidingSpots.h"
#include "../../DarkMod/StimResponse/StimResponseCollection.h"
#include "../../DarkMod/AIComm_StimResponse.h"
#include "../../DarkMod/idAbsenceMarkerEntity.h"

class CRelations;


/***********************************************************************

	AI Events

***********************************************************************/

const idEventDef AI_FindEnemy( "findEnemy", "d", 'e' );
const idEventDef AI_FindEnemyAI( "findEnemyAI", "d", 'e' );
const idEventDef AI_FindEnemyInCombatNodes( "findEnemyInCombatNodes", NULL, 'e' );
const idEventDef AI_ClosestReachableEnemyOfEntity( "closestReachableEnemyOfEntity", "E", 'e' );
const idEventDef AI_HeardSound( "heardSound", "d", 'e' );
const idEventDef AI_SetEnemy( "setEnemy", "E" );
const idEventDef AI_ClearEnemy( "clearEnemy" );
const idEventDef AI_MuzzleFlash( "muzzleFlash", "s" );
const idEventDef AI_CreateMissile( "createMissile", "s", 'e' );
const idEventDef AI_AttackMissile( "attackMissile", "s", 'e' );
const idEventDef AI_FireMissileAtTarget( "fireMissileAtTarget", "ss", 'e' );
const idEventDef AI_LaunchMissile( "launchMissile", "vv", 'e' );
const idEventDef AI_AttackMelee( "attackMelee", "s", 'd' );
const idEventDef AI_DirectDamage( "directDamage", "es" );
const idEventDef AI_RadiusDamageFromJoint( "radiusDamageFromJoint", "ss" );
const idEventDef AI_BeginAttack( "attackBegin", "s" );
const idEventDef AI_EndAttack( "attackEnd" );
const idEventDef AI_MeleeAttackToJoint( "meleeAttackToJoint", "ss", 'd' );
const idEventDef AI_RandomPath( "randomPath", NULL, 'e' );
const idEventDef AI_CanBecomeSolid( "canBecomeSolid", NULL, 'f' );
const idEventDef AI_BecomeSolid( "becomeSolid" );
const idEventDef AI_BecomeRagdoll( "becomeRagdoll", NULL, 'd' );
const idEventDef AI_StopRagdoll( "stopRagdoll" );
const idEventDef AI_SetHealth( "setHealth", "f" );
const idEventDef AI_GetHealth( "getHealth", NULL, 'f' );
const idEventDef AI_AllowDamage( "allowDamage" );
const idEventDef AI_IgnoreDamage( "ignoreDamage" );
const idEventDef AI_GetCurrentYaw( "getCurrentYaw", NULL, 'f' );
const idEventDef AI_TurnTo( "turnTo", "f" );
const idEventDef AI_TurnToPos( "turnToPos", "v" );
const idEventDef AI_TurnToEntity( "turnToEntity", "E" );
const idEventDef AI_MoveStatus( "moveStatus", NULL, 'd' );
const idEventDef AI_StopMove( "stopMove" );
const idEventDef AI_MoveToCover( "moveToCover" );
const idEventDef AI_MoveToCoverFrom( "moveToCoverFrom", "E" );
const idEventDef AI_MoveToEnemy( "moveToEnemy" );
const idEventDef AI_MoveToEnemyHeight( "moveToEnemyHeight" );
const idEventDef AI_MoveOutOfRange( "moveOutOfRange", "ef" );
const idEventDef AI_MoveToAttackPosition( "moveToAttackPosition", "es" );
const idEventDef AI_Wander( "wander" );
const idEventDef AI_MoveToEntity( "moveToEntity", "e" );
const idEventDef AI_MoveToPosition( "moveToPosition", "v" );
const idEventDef AI_SlideTo( "slideTo", "vf" );
const idEventDef AI_FacingIdeal( "facingIdeal", NULL, 'd' );
const idEventDef AI_FaceEnemy( "faceEnemy" );
const idEventDef AI_FaceEntity( "faceEntity", "E" );
const idEventDef AI_GetCombatNode( "getCombatNode", NULL, 'e' );
const idEventDef AI_EnemyInCombatCone( "enemyInCombatCone", "Ed", 'd' );
const idEventDef AI_WaitMove( "waitMove" );
const idEventDef AI_GetJumpVelocity( "getJumpVelocity", "vff", 'v' );
const idEventDef AI_EntityInAttackCone( "entityInAttackCone", "E", 'd' );
const idEventDef AI_CanSeeEntity( "canSee", "E", 'd' );
const idEventDef AI_CanSeeEntityExt( "canSeeExt", "Edd", 'd' );
const idEventDef AI_CanSeePositionExt( "canSeePositionExt", "vdd", 'd' );
const idEventDef AI_SetTalkTarget( "setTalkTarget", "E" );
const idEventDef AI_GetTalkTarget( "getTalkTarget", NULL, 'e' );
const idEventDef AI_SetTalkState( "setTalkState", "d" );
const idEventDef AI_EnemyRange( "enemyRange", NULL, 'f' );
const idEventDef AI_EnemyRange2D( "enemyRange2D", NULL, 'f' );
const idEventDef AI_GetEnemy( "getEnemy", NULL, 'e' );
const idEventDef AI_GetEnemyPos( "getEnemyPos", NULL, 'v' );
const idEventDef AI_GetEnemyEyePos( "getEnemyEyePos", NULL, 'v' );
const idEventDef AI_PredictEnemyPos( "predictEnemyPos", "f", 'v' );
const idEventDef AI_CanHitEnemy( "canHitEnemy", NULL, 'd' );
const idEventDef AI_CanHitEnemyFromAnim( "canHitEnemyFromAnim", "s", 'd' );
const idEventDef AI_CanHitEnemyFromJoint( "canHitEnemyFromJoint", "s", 'd' );
const idEventDef AI_EnemyPositionValid( "enemyPositionValid", NULL, 'd' );
const idEventDef AI_ChargeAttack( "chargeAttack", "s" );
const idEventDef AI_TestChargeAttack( "testChargeAttack", NULL, 'f' );
const idEventDef AI_TestMoveToPosition( "testMoveToPosition", "v", 'd' );
const idEventDef AI_TestAnimMoveTowardEnemy( "testAnimMoveTowardEnemy", "s", 'd' );
const idEventDef AI_TestAnimMove( "testAnimMove", "s", 'd' );
const idEventDef AI_TestMeleeAttack( "testMeleeAttack", NULL, 'd' );
const idEventDef AI_TestAnimAttack( "testAnimAttack", "s", 'd' );
const idEventDef AI_Shrivel( "shrivel", "f" );
const idEventDef AI_Burn( "burn" );
const idEventDef AI_ClearBurn( "clearBurn" );
const idEventDef AI_PreBurn( "preBurn" );
const idEventDef AI_SetSmokeVisibility( "setSmokeVisibility", "dd" );
const idEventDef AI_NumSmokeEmitters( "numSmokeEmitters", NULL, 'd' );
const idEventDef AI_WaitAction( "waitAction", "s" );
const idEventDef AI_StopThinking( "stopThinking" );
const idEventDef AI_GetTurnDelta( "getTurnDelta", NULL, 'f' );
const idEventDef AI_GetMoveType( "getMoveType", NULL, 'd' );
const idEventDef AI_SetMoveType( "setMoveType", "d" );
const idEventDef AI_SaveMove( "saveMove" );
const idEventDef AI_RestoreMove( "restoreMove" );
const idEventDef AI_AllowMovement( "allowMovement", "f" );
const idEventDef AI_JumpFrame( "<jumpframe>" );
const idEventDef AI_EnableClip( "enableClip" );
const idEventDef AI_DisableClip( "disableClip" );
const idEventDef AI_EnableGravity( "enableGravity" );
const idEventDef AI_DisableGravity( "disableGravity" );
const idEventDef AI_EnableAFPush( "enableAFPush" );
const idEventDef AI_DisableAFPush( "disableAFPush" );
const idEventDef AI_SetFlySpeed( "setFlySpeed", "f" );
const idEventDef AI_SetFlyOffset( "setFlyOffset", "d" );
const idEventDef AI_ClearFlyOffset( "clearFlyOffset" );
const idEventDef AI_GetClosestHiddenTarget( "getClosestHiddenTarget", "s", 'e' );
const idEventDef AI_GetRandomTarget( "getRandomTarget", "s", 'e' );
const idEventDef AI_TravelDistanceToPoint( "travelDistanceToPoint", "v", 'f' );
const idEventDef AI_TravelDistanceToEntity( "travelDistanceToEntity", "e", 'f' );
const idEventDef AI_TravelDistanceBetweenPoints( "travelDistanceBetweenPoints", "vv", 'f' );
const idEventDef AI_TravelDistanceBetweenEntities( "travelDistanceBetweenEntities", "ee", 'f' );
const idEventDef AI_LookAtEntity( "lookAt", "Ef" );
const idEventDef AI_LookAtEnemy( "lookAtEnemy", "f" );
const idEventDef AI_SetJointMod( "setBoneMod", "d" );
const idEventDef AI_ThrowMoveable( "throwMoveable" );
const idEventDef AI_ThrowAF( "throwAF" );
const idEventDef AI_RealKill( "<kill>" );
const idEventDef AI_Kill( "kill" );
const idEventDef AI_WakeOnFlashlight( "wakeOnFlashlight", "d" );
const idEventDef AI_LocateEnemy( "locateEnemy" );
const idEventDef AI_KickObstacles( "kickObstacles", "Ef" );
const idEventDef AI_GetObstacle( "getObstacle", NULL, 'e' );
const idEventDef AI_PushPointIntoAAS( "pushPointIntoAAS", "v", 'v' );
const idEventDef AI_GetTurnRate( "getTurnRate", NULL, 'f' );
const idEventDef AI_SetTurnRate( "setTurnRate", "f" );
const idEventDef AI_AnimTurn( "animTurn", "f" );
const idEventDef AI_AllowHiddenMovement( "allowHiddenMovement", "d" );
const idEventDef AI_TriggerParticles( "triggerParticles", "s" );
const idEventDef AI_FindActorsInBounds( "findActorsInBounds", "vv", 'e' );
const idEventDef AI_CanReachPosition( "canReachPosition", "v", 'd' );
const idEventDef AI_CanReachEntity( "canReachEntity", "E", 'd' );
const idEventDef AI_CanReachEnemy( "canReachEnemy", NULL, 'd' );
const idEventDef AI_GetReachableEntityPosition( "getReachableEntityPosition", "e", 'v' );

// TDM
const idEventDef AI_PlayAndLipSync( "playAndLipSync", "ss" );
const idEventDef AI_RegisterKilledTask( "registerKilledTask", "sd" );
const idEventDef AI_RegisterKnockedOutTask( "registerKnockedOutTask", "sd" );

// DarkMod AI Relations Events
const idEventDef AI_GetRelationEnt( "getRelationEnt", "E", 'd' );
const idEventDef AI_IsEnemy( "isEnemy", "E", 'd' );
const idEventDef AI_IsFriend( "isFriend", "E", 'd' );
const idEventDef AI_IsNeutral( "isNeutral", "E", 'd' );

// Alert events
const idEventDef AI_SetAlertLevel( "setAlertLevel", "f" );
const idEventDef AI_Alert( "alert", "sf" );
const idEventDef AI_VisScan( "visScan", NULL, 'e' );
const idEventDef AI_GetSndDir( "getSndDir", NULL, 'v' );
const idEventDef AI_GetVisDir( "getVisDir", NULL, 'v' );
const idEventDef AI_GetTactEnt( "getTactEnt", NULL, 'e');
const idEventDef AI_SetAcuity( "setAcuity", "sf" );
const idEventDef AI_GetAcuity( "getAcuity", "s", 'f' );
const idEventDef AI_GetAudThresh( "getAudThresh", NULL, 'f' );
const idEventDef AI_SetAudThresh( "setAudThresh", "f" );
const idEventDef AI_GetAlertActor( "getAlertActor", NULL, 'e' );
const idEventDef AI_SetAlertGracePeriod( "setAlertGracePeriod", "fff" );

const idEventDef AI_ClosestReachableEnemy( "closestReachableEnemy", NULL, 'e' );

const idEventDef AI_IssueCommunication_IR_DOE ("issueCommunication_IR_DOE", "ffeev");
const idEventDef AI_IssueCommunication_IR ( "issueCommunication_IR", "ffev");
const idEventDef AI_IssueCommunication_DOE ( "issueCommunication_DOE", "ffev" );
const idEventDef AI_IssueCommunication ("issueCommunication", "ffv" );

/*!
* A look at event that just looks at a position in space
*
* @param lookAtWorldPosition The position in space to look at
*
* @param durationInSeconds The duration to look in seconds
*/
const idEventDef AI_LookAtPosition ("lookAtPosition", "vf");

/*!
* A look at event that just looks at a set of angles relative
* to the current body facing of the AI
*
* @param yawAngleClockwise Negative angles are to the left of the AIs body
*	and positive angles are to the right.
*
* @param pitchAngleUp Negative values are down and positive values are up
*	where down and up are defined by the body axis.
*
* @param rollAngle This is currently unused and does nothing
*
* @param durationInSeconds The duration to look in seconds
*
*/
const idEventDef AI_LookAtAngles ("lookAtAngles", "ffff");


/*!
* script callable: spawnThrowableProjectile
*
* @param pstr_projectileName The name of the prjectile to spawn
*	(as seen in a .def file) Must be descended from idProjectile
*
* @param pstr_attachJointName The name of the joint on the model
*	to which the particle should be attached for throwing. If this
*	is NULL or the empty string, then it is attached to the model center.
*
* @returns a pointer to a projectile entity that can be 
*   thrown by the AI. You can use AI_LaunchMissle (e* = launchMissle(v,v) )
*   to throw the stone.
*
*/
const idEventDef AI_SpawnThrowableProjectile ("spawnThrowableProjectile", "ss", 'e');

// DarkMod Hiding spot detection Events
/*!
* This event finds hiding spots in the bounds given by two vectors.
*
* The first paramter is a vector which gives the location of the
* eye from which hiding is desired.
*
* The second vector gives the minimums in each dimension for the
* search space.  
*
* The third and fourth vectors give the min and max bounds within which spots should be tested
*
* The fifth parameter gives the bit flags of the types of hiding spots
* for which the search should look.
*
* The sixth parameter indicates an entity that should be ignored in
* the visual occlusion checks.  This is usually the searcher itself but
* can be NULL.
*
* This method will only start the search, if it returns 1, you should call
* continueSearchForHidingSpots every frame to do more processing until that function
* returns 0.
*
* The return value is a 0 for failure, 1 for success.
*/
const idEventDef AI_StartSearchForHidingSpots ("startSearchForHidingSpots", "vvvdE", 'd');

/*!
* This event finds hiding spots in the bounds given by two vectors, and also excludes
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
const idEventDef AI_StartSearchForHidingSpotsWithExclusionArea ("startSearchForHidingSpotsWithExclusionArea", "vvvvvdE", 'd');

/*
* This method continues searching for hiding spots. It will only find so many before
* returning so as not to cause long delays.  Detected spots are added to the currently
* building hiding spot list.
*
* The return value is 0 if the end of the search was reached, or 1 if there
* is more processing to do (call this method again next AI frame)
*
*/
const idEventDef AI_ContinueSearchForHidingSpots ("continueSearchForHidingSpots", NULL, 'd');

/*!
* This should be called when the script is done with the hiding spot 
* search to free up memory.
*/
const idEventDef AI_CloseHidingSpotSearch ("closeHidingSpotSearch", "");

/*!
* This re-sorts an existing search based on a new search center and radius.
*
* @param newCenter The new center of the search
* 
* @param newRadius The new radius of the search. Points outside the original
*	search will not be added.
*
*/
const idEventDef AI_ResortHidingSpotSearch ("resortHidingSpotSearch", "vv");

/*!
* This event returns the number of hiding spots by the current
* hiding spot query. If there is no current query, this returns -1
*/
const idEventDef AI_GetNumHidingSpots ("getNumHidingSpots", "", 'd');

/*!
* This event returns the Nth hiding spot location. 
* Param is 0 based hiding spot index
*/
const idEventDef AI_GetNthHidingSpotLocation ("getNthHidingSpotLocation", "d", 'v');

/*!
* This event returns the Nth hiding spot type. 
* Param is 0 based hiding spot index
*/
const idEventDef AI_GetNthHidingSpotType ("getNthHidingSpotType", "d", 'd');

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
const idEventDef AI_GetSomeOfOtherEntitiesHidingSpotList ("getSomeOfOtherEntitiesHidingSpotList", "e", 'd');

/*!
* This event gets the alert number of another AI (AI_alertNum variable value)
*
* @param p_otherEntity The other AI entity who's alert number is being querried
*
* @return The alert number of the other AI, 0.0 if its not an AI or is NULL
*/
const idEventDef AI_GetAlertNumOfOtherAI ("getAlertNumOfOtherAI", "e", 'f');

/*!
* This event gets a script linked variable value from another AI
*
* @param otherEntity The entity who's script variable we are retrieving
*
* @param variableName The script name of the variable to be retrieved
*
* @return floating values are returned as is
* @return boolean values are 0.0 for false, 1.0 for true
* @return int values are returned as float casts
*
*/
const idEventDef AI_GetVariableFromOtherAI ("getVariableFromOtherAI", "es", 'f');


/*!
* This event is used to get a position that the AI can move to observe a 
* given position.  It is useful for looking at hiding spots that can't be reached,
* and performing other investigation functions.
*/
const idEventDef AI_GetObservationPosition ("getObservationPosition", "vf", 'v');


/**
* This event handles a knockout of the AI
**/
const idEventDef AI_Knockout( "knockout" );

/*
* This is the AI event table class for a generic NPC actor.
*
*/
CLASS_DECLARATION( idActor, idAI )
	EVENT( EV_Activate,							idAI::Event_Activate )
	EVENT( EV_Touch,							idAI::Event_Touch )
	EVENT( AI_FindEnemy,						idAI::Event_FindEnemy )
	EVENT( AI_FindEnemyAI,						idAI::Event_FindEnemyAI )
	EVENT( AI_FindEnemyInCombatNodes,			idAI::Event_FindEnemyInCombatNodes )
	EVENT( AI_ClosestReachableEnemyOfEntity,	idAI::Event_ClosestReachableEnemyOfEntity )
	EVENT( AI_HeardSound,						idAI::Event_HeardSound )
	EVENT( AI_SetEnemy,							idAI::Event_SetEnemy )
	EVENT( AI_ClearEnemy,						idAI::Event_ClearEnemy )
	EVENT( AI_MuzzleFlash,						idAI::Event_MuzzleFlash )
	EVENT( AI_CreateMissile,					idAI::Event_CreateMissile )
	EVENT( AI_AttackMissile,					idAI::Event_AttackMissile )
	EVENT( AI_FireMissileAtTarget,				idAI::Event_FireMissileAtTarget )
	EVENT( AI_LaunchMissile,					idAI::Event_LaunchMissile )
	EVENT( AI_AttackMelee,						idAI::Event_AttackMelee )
	EVENT( AI_DirectDamage,						idAI::Event_DirectDamage )
	EVENT( AI_RadiusDamageFromJoint,			idAI::Event_RadiusDamageFromJoint )
	EVENT( AI_BeginAttack,						idAI::Event_BeginAttack )
	EVENT( AI_EndAttack,						idAI::Event_EndAttack )
	EVENT( AI_MeleeAttackToJoint,				idAI::Event_MeleeAttackToJoint )
	EVENT( AI_RandomPath,						idAI::Event_RandomPath )
	EVENT( AI_CanBecomeSolid,					idAI::Event_CanBecomeSolid )
	EVENT( AI_BecomeSolid,						idAI::Event_BecomeSolid )
	EVENT( EV_BecomeNonSolid,					idAI::Event_BecomeNonSolid )
	EVENT( AI_BecomeRagdoll,					idAI::Event_BecomeRagdoll )
	EVENT( AI_StopRagdoll,						idAI::Event_StopRagdoll )
	EVENT( AI_SetHealth,						idAI::Event_SetHealth )
	EVENT( AI_GetHealth,						idAI::Event_GetHealth )
	EVENT( AI_AllowDamage,						idAI::Event_AllowDamage )
	EVENT( AI_IgnoreDamage,						idAI::Event_IgnoreDamage )
	EVENT( AI_GetCurrentYaw,					idAI::Event_GetCurrentYaw )
	EVENT( AI_TurnTo,							idAI::Event_TurnTo )
	EVENT( AI_TurnToPos,						idAI::Event_TurnToPos )
	EVENT( AI_TurnToEntity,						idAI::Event_TurnToEntity )
	EVENT( AI_MoveStatus,						idAI::Event_MoveStatus )
	EVENT( AI_StopMove,							idAI::Event_StopMove )
	EVENT( AI_MoveToCover,						idAI::Event_MoveToCover )
	EVENT( AI_MoveToCoverFrom,					idAI::Event_MoveToCoverFrom )
	EVENT( AI_MoveToEnemy,						idAI::Event_MoveToEnemy )
	EVENT( AI_MoveToEnemyHeight,				idAI::Event_MoveToEnemyHeight )
	EVENT( AI_MoveOutOfRange,					idAI::Event_MoveOutOfRange )
	EVENT( AI_MoveToAttackPosition,				idAI::Event_MoveToAttackPosition )
	EVENT( AI_Wander,							idAI::Event_Wander )
	EVENT( AI_MoveToEntity,						idAI::Event_MoveToEntity )
	EVENT( AI_MoveToPosition,					idAI::Event_MoveToPosition )
	EVENT( AI_SlideTo,							idAI::Event_SlideTo )
	EVENT( AI_FacingIdeal,						idAI::Event_FacingIdeal )
	EVENT( AI_FaceEnemy,						idAI::Event_FaceEnemy )
	EVENT( AI_FaceEntity,						idAI::Event_FaceEntity )
	EVENT( AI_WaitAction,						idAI::Event_WaitAction )
	EVENT( AI_GetCombatNode,					idAI::Event_GetCombatNode )
	EVENT( AI_EnemyInCombatCone,				idAI::Event_EnemyInCombatCone )
	EVENT( AI_WaitMove,							idAI::Event_WaitMove )
	EVENT( AI_GetJumpVelocity,					idAI::Event_GetJumpVelocity )
	EVENT( AI_EntityInAttackCone,				idAI::Event_EntityInAttackCone )
	EVENT( AI_CanSeeEntity,						idAI::Event_CanSeeEntity )
	EVENT( AI_CanSeeEntityExt,					idAI::Event_CanSeeEntityExt )
	EVENT( AI_CanSeePositionExt,				idAI::Event_CanSeePositionExt )
	EVENT( AI_SetTalkTarget,					idAI::Event_SetTalkTarget )
	EVENT( AI_GetTalkTarget,					idAI::Event_GetTalkTarget )
	EVENT( AI_SetTalkState,						idAI::Event_SetTalkState )
	EVENT( AI_EnemyRange,						idAI::Event_EnemyRange )
	EVENT( AI_EnemyRange2D,						idAI::Event_EnemyRange2D )
	EVENT( AI_GetEnemy,							idAI::Event_GetEnemy )
	EVENT( AI_GetEnemyPos,						idAI::Event_GetEnemyPos )
	EVENT( AI_GetEnemyEyePos,					idAI::Event_GetEnemyEyePos )
	EVENT( AI_PredictEnemyPos,					idAI::Event_PredictEnemyPos )
	EVENT( AI_CanHitEnemy,						idAI::Event_CanHitEnemy )
	EVENT( AI_CanHitEnemyFromAnim,				idAI::Event_CanHitEnemyFromAnim )
	EVENT( AI_CanHitEnemyFromJoint,				idAI::Event_CanHitEnemyFromJoint )
	EVENT( AI_EnemyPositionValid,				idAI::Event_EnemyPositionValid )
	EVENT( AI_ChargeAttack,						idAI::Event_ChargeAttack )
	EVENT( AI_TestChargeAttack,					idAI::Event_TestChargeAttack )
	EVENT( AI_TestAnimMoveTowardEnemy,			idAI::Event_TestAnimMoveTowardEnemy )
	EVENT( AI_TestAnimMove,						idAI::Event_TestAnimMove )
	EVENT( AI_TestMoveToPosition,				idAI::Event_TestMoveToPosition )
	EVENT( AI_TestMeleeAttack,					idAI::Event_TestMeleeAttack )
	EVENT( AI_TestAnimAttack,					idAI::Event_TestAnimAttack )
	EVENT( AI_Shrivel,							idAI::Event_Shrivel )
	EVENT( AI_Burn,								idAI::Event_Burn )
	EVENT( AI_PreBurn,							idAI::Event_PreBurn )
	EVENT( AI_SetSmokeVisibility,				idAI::Event_SetSmokeVisibility )
	EVENT( AI_NumSmokeEmitters,					idAI::Event_NumSmokeEmitters )
	EVENT( AI_ClearBurn,						idAI::Event_ClearBurn )
	EVENT( AI_StopThinking,						idAI::Event_StopThinking )
	EVENT( AI_GetTurnDelta,						idAI::Event_GetTurnDelta )
	EVENT( AI_GetMoveType,						idAI::Event_GetMoveType )
	EVENT( AI_SetMoveType,						idAI::Event_SetMoveType )
	EVENT( AI_SaveMove,							idAI::Event_SaveMove )
	EVENT( AI_RestoreMove,						idAI::Event_RestoreMove )
	EVENT( AI_AllowMovement,					idAI::Event_AllowMovement )	
	EVENT( AI_JumpFrame,						idAI::Event_JumpFrame )
	EVENT( AI_EnableClip,						idAI::Event_EnableClip )
	EVENT( AI_DisableClip,						idAI::Event_DisableClip )
	EVENT( AI_EnableGravity,					idAI::Event_EnableGravity )
	EVENT( AI_DisableGravity,					idAI::Event_DisableGravity )
	EVENT( AI_EnableAFPush,						idAI::Event_EnableAFPush )
	EVENT( AI_DisableAFPush,					idAI::Event_DisableAFPush )
	EVENT( AI_SetFlySpeed,						idAI::Event_SetFlySpeed )
	EVENT( AI_SetFlyOffset,						idAI::Event_SetFlyOffset )
	EVENT( AI_ClearFlyOffset,					idAI::Event_ClearFlyOffset )
	EVENT( AI_GetClosestHiddenTarget,			idAI::Event_GetClosestHiddenTarget )
	EVENT( AI_GetRandomTarget,					idAI::Event_GetRandomTarget )
	EVENT( AI_TravelDistanceToPoint,			idAI::Event_TravelDistanceToPoint )
	EVENT( AI_TravelDistanceToEntity,			idAI::Event_TravelDistanceToEntity )
	EVENT( AI_TravelDistanceBetweenPoints,		idAI::Event_TravelDistanceBetweenPoints )
	EVENT( AI_TravelDistanceBetweenEntities,	idAI::Event_TravelDistanceBetweenEntities )
	EVENT( AI_LookAtEntity,						idAI::Event_LookAtEntity )
	EVENT( AI_LookAtEnemy,						idAI::Event_LookAtEnemy )
	EVENT( AI_SetJointMod,						idAI::Event_SetJointMod )
	EVENT( AI_ThrowMoveable,					idAI::Event_ThrowMoveable )
	EVENT( AI_ThrowAF,							idAI::Event_ThrowAF )
	EVENT( EV_GetAngles,						idAI::Event_GetAngles )
	EVENT( EV_SetAngles,						idAI::Event_SetAngles )
	EVENT( AI_RealKill,							idAI::Event_RealKill )
	EVENT( AI_Kill,								idAI::Event_Kill )
	EVENT( AI_WakeOnFlashlight,					idAI::Event_WakeOnFlashlight )
	EVENT( AI_LocateEnemy,						idAI::Event_LocateEnemy )
	EVENT( AI_KickObstacles,					idAI::Event_KickObstacles )
	EVENT( AI_GetObstacle,						idAI::Event_GetObstacle )
	EVENT( AI_PushPointIntoAAS,					idAI::Event_PushPointIntoAAS )
	EVENT( AI_GetTurnRate,						idAI::Event_GetTurnRate )
	EVENT( AI_SetTurnRate,						idAI::Event_SetTurnRate )
	EVENT( AI_AnimTurn,							idAI::Event_AnimTurn )
	EVENT( AI_AllowHiddenMovement,				idAI::Event_AllowHiddenMovement )
	EVENT( AI_TriggerParticles,					idAI::Event_TriggerParticles )
	EVENT( AI_FindActorsInBounds,				idAI::Event_FindActorsInBounds )
	EVENT( AI_CanReachPosition,					idAI::Event_CanReachPosition )
	EVENT( AI_CanReachEntity,					idAI::Event_CanReachEntity )
	EVENT( AI_CanReachEnemy,					idAI::Event_CanReachEnemy )
	EVENT( AI_GetReachableEntityPosition,		idAI::Event_GetReachableEntityPosition )
	
	EVENT( AI_PlayAndLipSync,					idAI::Event_PlayAndLipSync )
	EVENT( AI_RegisterKilledTask,				idAI::Event_RegisterKilledTask )
	EVENT( AI_RegisterKnockedOutTask,			idAI::Event_RegisterKnockedOutTask )
	EVENT( AI_GetRelationEnt,					idAI::Event_GetRelationEnt )
	EVENT( AI_IsEnemy,							idAI::Event_IsEnemy )
	EVENT( AI_IsFriend,							idAI::Event_IsFriend )
	EVENT( AI_IsNeutral,						idAI::Event_IsNeutral )
	EVENT( AI_SetAlertLevel,					idAI::Event_SetAlertLevel )
	EVENT( AI_Alert,							idAI::Event_Alert )
	EVENT( AI_GetSndDir,						idAI::Event_GetSndDir )
	EVENT( AI_GetVisDir,						idAI::Event_GetVisDir )
	EVENT( AI_GetTactEnt,						idAI::Event_GetTactEnt )
	EVENT( AI_SetAcuity,						idAI::Event_SetAcuity )
	EVENT( AI_GetAcuity,						idAI::Event_GetAcuity )
	EVENT( AI_GetAudThresh,						idAI::Event_GetAudThresh )
	EVENT( AI_SetAudThresh,						idAI::Event_SetAudThresh )
	EVENT( AI_VisScan,							idAI::Event_VisScan )
	EVENT( AI_GetAlertActor,					idAI::Event_GetAlertActor )
	EVENT( AI_SetAlertGracePeriod,				idAI::Event_SetAlertGracePeriod )
	EVENT( AI_ClosestReachableEnemy,			idAI::Event_ClosestReachableEnemy )
	EVENT ( AI_StartSearchForHidingSpots,		idAI::Event_StartSearchForHidingSpots )
	EVENT ( AI_StartSearchForHidingSpotsWithExclusionArea,		idAI::Event_StartSearchForHidingSpotsWithExclusionArea )
	EVENT ( AI_ContinueSearchForHidingSpots,	idAI::Event_ContinueSearchForHidingSpots )
	EVENT ( AI_CloseHidingSpotSearch,			idAI::Event_CloseHidingSpotSearch )
	EVENT ( AI_ResortHidingSpotSearch,			idAI::Event_ResortHidingSpots )
	EVENT ( AI_GetNumHidingSpots,				idAI::Event_GetNumHidingSpots )
	EVENT ( AI_GetNthHidingSpotLocation,		idAI::Event_GetNthHidingSpotLocation )
	EVENT ( AI_GetNthHidingSpotType,			idAI::Event_GetNthHidingSpotType )
	EVENT ( AI_GetSomeOfOtherEntitiesHidingSpotList, idAI::Event_GetSomeOfOtherEntitiesHidingSpotList)
	EVENT ( AI_GetObservationPosition,			idAI::Event_GetObservationPosition)
	EVENT ( AI_LookAtPosition,					idAI::Event_LookAtPosition)
	EVENT ( AI_LookAtAngles,					idAI::Event_LookAtAngles)

	EVENT ( AI_GetAlertNumOfOtherAI,			idAI::Event_GetAlertNumOfOtherAI)
	EVENT ( AI_GetVariableFromOtherAI,			idAI::Event_GetVariableFromOtherAI)

	EVENT( AI_Knockout,							idAI::Knockout )
	EVENT ( AI_SpawnThrowableProjectile,		idAI::Event_SpawnThrowableProjectile)
	EVENT ( AI_IssueCommunication_IR_DOE,		idAI::Event_IssueCommunication_IR_DOE)
	EVENT ( AI_IssueCommunication_DOE,			idAI::Event_IssueCommunication_DOE)
	EVENT ( AI_IssueCommunication_IR,			idAI::Event_IssueCommunication_IR)
	EVENT ( AI_IssueCommunication,				idAI::Event_IssueCommunication)

END_CLASS

/*
=====================
idAI::Event_Activate
=====================
*/
void idAI::Event_Activate( idEntity *activator ) {
	Activate( activator );
}

/*
=====================
idAI::Event_Touch

DarkMod: Modified to issue a tactile alert.

Note: Event_Touch checks ReactionTo, which checks our DarkMod Relations
So it will only go off if the AI is bumped by an enemy that moves toward it.

AI bumping by inanimate objects is handled separately in idMoveable::Collide.
=====================
*/

void idAI::Event_Touch( idEntity *other, trace_t *trace ) 
{
	if ( !enemy.GetEntity() && !other->fl.notarget && ( ReactionTo( other ) & ATTACK_ON_ACTIVATE ) ) 
	{
		Activate( other );
	}
	AI_PUSHED = true;

	if( other && other->IsType(idActor::Type) )
	{
		HadTactile( static_cast<idActor *>(other) );
	}
}

/*
=====================
idAI::Event_FindEnemy
=====================
*/
void idAI::Event_FindEnemy( int useFOV ) 
{
	int			i;

	idEntity	*ent;

	idActor		*actor;



	if ( gameLocal.InPlayerPVS( this ) ) {

		for ( i = 0; i < gameLocal.numClients ; i++ ) {

			ent = gameLocal.entities[ i ];



			if ( !ent || !ent->IsType( idActor::Type ) ) {

				continue;

			}



			actor = static_cast<idActor *>( ent );

			if ( ( actor->health <= 0 ) || !( ReactionTo( actor ) & ATTACK_ON_SIGHT ) ) {

				continue;

			}



			if ( CanSee( actor, useFOV != 0 ) ) {

				idThread::ReturnEntity( actor );

				return;

			}

		}

	}



	idThread::ReturnEntity( NULL );

}

/*
=====================
idAI::Event_FindEnemyAI
=====================
*/
void idAI::Event_FindEnemyAI( int useFOV ) {
	idEntity	*ent;
	idActor		*actor;
	idActor		*bestEnemy;
	float		bestDist;
	float		dist;
	idVec3		delta;
	pvsHandle_t pvs;

	pvs = gameLocal.pvs.SetupCurrentPVS( GetPVSAreas(), GetNumPVSAreas() );

	bestDist = idMath::INFINITY;
	bestEnemy = NULL;
	for ( ent = gameLocal.activeEntities.Next(); ent != NULL; ent = ent->activeNode.Next() ) {
		if ( ent->fl.hidden || ent->fl.isDormant || !ent->IsType( idActor::Type ) ) {
			continue;
		}

		actor = static_cast<idActor *>( ent );
		if ( ( actor->health <= 0 ) || !( ReactionTo( actor ) & ATTACK_ON_SIGHT ) ) {
			continue;
		}

		if ( !gameLocal.pvs.InCurrentPVS( pvs, actor->GetPVSAreas(), actor->GetNumPVSAreas() ) ) {
			continue;
		}

		delta = physicsObj.GetOrigin() - actor->GetPhysics()->GetOrigin();
		dist = delta.LengthSqr();
		if ( ( dist < bestDist ) && CanSee( actor, useFOV != 0 ) ) {
			bestDist = dist;
			bestEnemy = actor;
		}
	}

	gameLocal.pvs.FreeCurrentPVS( pvs );
	idThread::ReturnEntity( bestEnemy );
}

/*
=====================
idAI::Event_FindEnemyInCombatNodes
=====================
*/
void idAI::Event_FindEnemyInCombatNodes( void ) {
	int				i, j;
	idCombatNode	*node;
	idEntity		*ent;
	idEntity		*targetEnt;
	idActor			*actor;

	if ( !gameLocal.InPlayerPVS( this ) ) {
		// don't locate the player when we're not in his PVS
		idThread::ReturnEntity( NULL );
		return;
	}

	for ( i = 0; i < gameLocal.numClients ; i++ ) {
		ent = gameLocal.entities[ i ];

		if ( !ent || !ent->IsType( idActor::Type ) ) {
			continue;
		}

		actor = static_cast<idActor *>( ent );
		if ( ( actor->health <= 0 ) || !( ReactionTo( actor ) & ATTACK_ON_SIGHT ) ) {
			continue;
		}

		for( j = 0; j < targets.Num(); j++ ) {
			targetEnt = targets[ j ].GetEntity();
			if ( !targetEnt || !targetEnt->IsType( idCombatNode::Type ) ) {
				continue;
			}

			node = static_cast<idCombatNode *>( targetEnt );
			if ( !node->IsDisabled() && node->EntityInView( actor, actor->GetPhysics()->GetOrigin() ) ) {
				idThread::ReturnEntity( actor );
				return;
			}
		}
	}

	idThread::ReturnEntity( NULL );
}

/*
=====================
idAI::Event_ClosestReachableEnemyOfEntity
=====================
*/
void idAI::Event_ClosestReachableEnemyOfEntity( idEntity *team_mate ) {
	idActor *actor;
	idActor *ent;
	idActor	*bestEnt;
	float	bestDistSquared;
	float	distSquared;
	idVec3	delta;
	int		areaNum;
	int		enemyAreaNum;
	aasPath_t path;
	
	if ( !team_mate->IsType( idActor::Type ) ) {
		gameLocal.Error( "Entity '%s' is not an AI character or player", team_mate->GetName() );
	}

	actor = static_cast<idActor *>( team_mate );

	const idVec3 &origin = physicsObj.GetOrigin();
	areaNum = PointReachableAreaNum( origin );

	bestDistSquared = idMath::INFINITY;
	bestEnt = NULL;
	for( ent = actor->enemyList.Next(); ent != NULL; ent = ent->enemyNode.Next() ) {
		if ( ent->fl.hidden ) {
			continue;
		}
		delta = ent->GetPhysics()->GetOrigin() - origin;
		distSquared = delta.LengthSqr();
		if ( distSquared < bestDistSquared ) {
			const idVec3 &enemyPos = ent->GetPhysics()->GetOrigin();
			enemyAreaNum = PointReachableAreaNum( enemyPos );
			if ( ( areaNum != 0 ) && PathToGoal( path, areaNum, origin, enemyAreaNum, enemyPos ) ) {
				bestEnt = ent;
				bestDistSquared = distSquared;
			}
		}
	}

	idThread::ReturnEntity( bestEnt );
}

/*
=====================
idAI::Event_HeardSound
=====================
*/
void idAI::Event_HeardSound( int ignore_team ) {

	// check if we heard any sounds in the last frame

	idActor	*actor = gameLocal.GetAlertEntity();

	if ( actor && ( !ignore_team || ( ReactionTo( actor ) & ATTACK_ON_SIGHT ) ) && gameLocal.InPlayerPVS( this ) ) {

		idVec3 pos = actor->GetPhysics()->GetOrigin();

		idVec3 org = physicsObj.GetOrigin();

		float dist = ( pos - org ).LengthSqr();

		if ( dist < Square( AI_HEARING_RANGE ) ) {

			idThread::ReturnEntity( actor );

			return;

		}

	}



	idThread::ReturnEntity( NULL );

}


/*
=====================
idAI::Event_SetEnemy
=====================
*/
void idAI::Event_SetEnemy( idEntity *ent ) {
	if ( !ent ) {
		ClearEnemy();
	} else if ( !ent->IsType( idActor::Type ) ) {
		gameLocal.Error( "'%s' is not an idActor (player or ai controlled character)", ent->name.c_str() );
	} else {
		SetEnemy( static_cast<idActor *>( ent ) );
	}
}

/*
=====================
idAI::Event_ClearEnemy
=====================
*/
void idAI::Event_ClearEnemy( void ) {
	ClearEnemy();
}

/*
=====================
idAI::Event_MuzzleFlash
=====================
*/
void idAI::Event_MuzzleFlash( const char *jointname ) 
{
	idVec3	muzzle;
	idMat3	axis;

	GetMuzzle( jointname, muzzle, axis );
	TriggerWeaponEffects( muzzle );
}


/*
=====================
idAI::Event_IssueCommunication
=====================
*/

void idAI::Event_IssueCommunication_IR_DOE ( float messageType, float maxRadius, idEntity* intendedRecipientEntity, idEntity* directObjectEntity, const idVec3& directObjectLocation)
{
	IssueCommunication_Internal (messageType, maxRadius, intendedRecipientEntity, directObjectEntity, directObjectLocation);
}

/*---------------------------------------------------------------------------------------------------*/

void idAI::Event_IssueCommunication_IR ( float messageType, float maxRadius, idEntity* intendedRecipientEntity, const idVec3& directObjectLocation)
{
	IssueCommunication_Internal (messageType, maxRadius, intendedRecipientEntity, NULL, directObjectLocation);
}

/*---------------------------------------------------------------------------------------------------*/

void idAI::Event_IssueCommunication_DOE ( float messageType, float maxRadius, idEntity* directObjectEntity, const idVec3& directObjectLocation)
{
	IssueCommunication_Internal (messageType, maxRadius, NULL, directObjectEntity, directObjectLocation);
}

/*---------------------------------------------------------------------------------------------------*/

void idAI::Event_IssueCommunication ( float messageType, float maxRadius, const idVec3& directObjectLocation)
{
	IssueCommunication_Internal (messageType, maxRadius, NULL, NULL, directObjectLocation);
}

/*---------------------------------------------------------------------------------------------------*/

void idAI::IssueCommunication_Internal
( 
	float messageType, 
	float maxRadius, 
	idEntity* intendedRecipientEntity, 
	idEntity* directObjectEntity, 
	const idVec3& directObjectLocation
)
{
	// Get the communication stim (outbound messgaes)
	CStim* p_stim;
	p_stim = NULL;
    
	p_stim = m_StimResponseColl->AddStim (this, ST_COMMUNICATION, g_Global.m_AICommStimRadius, true, false);
	p_stim->EnableSR(true);
	
	if (p_stim != NULL)
	{
		CAIComm_Stim* p_commStim = (CAIComm_Stim*) p_stim;
		CAIComm_Message::TCommType messageTypeEnumVal = (CAIComm_Message::TCommType) (unsigned long) messageType;

		if (!p_commStim->addMessage ( messageTypeEnumVal, maxRadius, this, intendedRecipientEntity, directObjectEntity, directObjectLocation ))
		{
			DM_LOG(LC_AI, LT_WARNING).LogString ("Failed to add message to communication stim");
		}
	}
	else
	{
		DM_LOG(LC_AI, LT_WARNING).LogString ("Failed to make or get communication stim");
	}


}

/*
=====================
idAI::Event_SpawnThrowableProjectile
=====================
*/
void idAI::Event_SpawnThrowableProjectile
(
	const char* pstr_projectileName,
	const char* pstr_jointName
)
{
	// Load definition from movable.def
	projectileDef = gameLocal.FindEntityDefDict( pstr_projectileName );
	if (!projectileDef)
	{
		DM_LOG(LC_AI, LT_WARNING).LogString ("Projectile with name '%s' was not found\n", pstr_projectileName);
		idThread::ReturnEntity (NULL);
	}

	// Create the projectile
	idVec3 projectileDir = viewAxis[ 0 ] * physicsObj.GetGravityAxis();
	idVec3 projectileOrigin = physicsObj.GetOrigin();
	CreateProjectile (projectileOrigin, projectileDir);

	// Create a clip model for the projectile
	if (projectile.GetEntity() == NULL)
	{
		idThread::ReturnEntity (NULL);
	}

	// Create clip model
	if ( projectileClipModel == NULL ) 
	{
		CreateProjectileClipModel();
	}

	// Bind to joint
	if ( !pstr_jointName || !pstr_jointName[ 0 ] ) 
	{
		projectile.GetEntity()->Bind( this, true );
	}	
	else
	{
		projectile.GetEntity()->BindToJoint( this, pstr_jointName, true );
	}

	// Return it
	idThread::ReturnEntity (projectile.GetEntity());

}

/*
=====================
idAI::Event_CreateMissile
=====================
*/
void idAI::Event_CreateMissile( const char *jointname ) {
	idVec3 muzzle;
	idMat3 axis;

	if ( !projectileDef ) {
		gameLocal.Warning( "%s (%s) doesn't have a projectile specified", name.c_str(), GetEntityDefName() );
		idThread::ReturnEntity( NULL );

		return;
	}

	GetMuzzle( jointname, muzzle, axis );
	CreateProjectile( muzzle, viewAxis[ 0 ] * physicsObj.GetGravityAxis() );
	if ( projectile.GetEntity() ) {
		if ( !jointname || !jointname[ 0 ] ) {
			projectile.GetEntity()->Bind( this, true );
		} else {
			projectile.GetEntity()->BindToJoint( this, jointname, true );
		}
	}
	idThread::ReturnEntity( projectile.GetEntity() );
}

/*
=====================
idAI::Event_AttackMissile
=====================
*/
void idAI::Event_AttackMissile( const char *jointname ) {
	idProjectile *proj;

	proj = LaunchProjectile( jointname, enemy.GetEntity(), true );
	idThread::ReturnEntity( proj );
}

/*
=====================
idAI::Event_FireMissileAtTarget
=====================
*/
void idAI::Event_FireMissileAtTarget( const char *jointname, const char *targetname ) {
	idEntity		*aent;
	idProjectile	*proj;

	aent = gameLocal.FindEntity( targetname );
	if ( !aent ) {
		gameLocal.Warning( "Entity '%s' not found for 'fireMissileAtTarget'", targetname );
	}

	proj = LaunchProjectile( jointname, aent, false );
	idThread::ReturnEntity( proj );
}

/*
=====================
idAI::Event_LaunchMissile
=====================
*/
void idAI::Event_LaunchMissile( const idVec3 &org, const idAngles &ang ) {
	idVec3		start;
	trace_t		tr;
	idBounds	projBounds;
	const idClipModel *projClip;
	idMat3		axis;
	float		distance;

	if ( !projectileDef ) {
		gameLocal.Warning( "%s (%s) doesn't have a projectile specified", name.c_str(), GetEntityDefName() );
		idThread::ReturnEntity( NULL );
		return;
	}

	axis = ang.ToMat3();
	if ( !projectile.GetEntity() ) {
		CreateProjectile( org, axis[ 0 ] );
	}

	// make sure the projectile starts inside the monster bounding box
	const idBounds &ownerBounds = physicsObj.GetAbsBounds();
	projClip = projectile.GetEntity()->GetPhysics()->GetClipModel();
	projBounds = projClip->GetBounds().Rotate( projClip->GetAxis() );

	// check if the owner bounds is bigger than the projectile bounds
	if ( ( ( ownerBounds[1][0] - ownerBounds[0][0] ) > ( projBounds[1][0] - projBounds[0][0] ) ) &&
		( ( ownerBounds[1][1] - ownerBounds[0][1] ) > ( projBounds[1][1] - projBounds[0][1] ) ) &&
		( ( ownerBounds[1][2] - ownerBounds[0][2] ) > ( projBounds[1][2] - projBounds[0][2] ) ) ) {
		if ( (ownerBounds - projBounds).RayIntersection( org, viewAxis[ 0 ], distance ) ) {
			start = org + distance * viewAxis[ 0 ];
		} else {
			start = ownerBounds.GetCenter();
		}
	} else {
		// projectile bounds bigger than the owner bounds, so just start it from the center
		start = ownerBounds.GetCenter();
	}

	gameLocal.clip.Translation( tr, start, org, projClip, projClip->GetAxis(), MASK_SHOT_RENDERMODEL, this );

	// launch the projectile
	idThread::ReturnEntity( projectile.GetEntity() );
	projectile.GetEntity()->Launch( tr.endpos, axis[ 0 ], vec3_origin );
	projectile = NULL;

	TriggerWeaponEffects( tr.endpos );

	lastAttackTime = gameLocal.time;
}

/*
=====================
idAI::Event_AttackMelee
=====================
*/
void idAI::Event_AttackMelee( const char *meleeDefName ) {
	bool hit;
	
	hit = AttackMelee( meleeDefName );
	idThread::ReturnInt( hit );
}

/*
=====================
idAI::Event_DirectDamage
=====================
*/
void idAI::Event_DirectDamage( idEntity *damageTarget, const char *damageDefName ) {
	DirectDamage( damageDefName, damageTarget );
}

/*
=====================
idAI::Event_RadiusDamageFromJoint
=====================
*/
void idAI::Event_RadiusDamageFromJoint( const char *jointname, const char *damageDefName ) {
	jointHandle_t joint;
	idVec3 org;
	idMat3 axis;

	if ( !jointname || !jointname[ 0 ] ) {
		org = physicsObj.GetOrigin();
	} else {
		joint = animator.GetJointHandle( jointname );
		if ( joint == INVALID_JOINT ) {
			gameLocal.Error( "Unknown joint '%s' on %s", jointname, GetEntityDefName() );
		}
		GetJointWorldTransform( joint, gameLocal.time, org, axis );
	}

	gameLocal.RadiusDamage( org, this, this, this, this, damageDefName );
}

/*
=====================
idAI::Event_RandomPath
=====================
*/
void idAI::Event_RandomPath( void ) {
	idPathCorner *path;

	path = idPathCorner::RandomPath( this, NULL );
	idThread::ReturnEntity( path );
}

/*
=====================
idAI::Event_BeginAttack
=====================
*/
void idAI::Event_BeginAttack( const char *name ) {
	BeginAttack( name );
}

/*
=====================
idAI::Event_EndAttack
=====================
*/
void idAI::Event_EndAttack( void ) {
	EndAttack();
}

/*
=====================
idAI::Event_MeleeAttackToJoint
=====================
*/
void idAI::Event_MeleeAttackToJoint( const char *jointname, const char *meleeDefName ) {
	jointHandle_t	joint;
	idVec3			start;
	idVec3			end;
	idMat3			axis;
	trace_t			trace;
	idEntity		*hitEnt;

	joint = animator.GetJointHandle( jointname );
	if ( joint == INVALID_JOINT ) {
		gameLocal.Error( "Unknown joint '%s' on %s", jointname, GetEntityDefName() );
	}
	animator.GetJointTransform( joint, gameLocal.time, end, axis );
	end = physicsObj.GetOrigin() + ( end + modelOffset ) * viewAxis * physicsObj.GetGravityAxis();
	start = GetEyePosition();
	
	if ( ai_debugMove.GetBool() ) {
		gameRenderWorld->DebugLine( colorYellow, start, end, gameLocal.msec );
	}

	gameLocal.clip.TranslationEntities( trace, start, end, NULL, mat3_identity, MASK_SHOT_BOUNDINGBOX, this );
	if ( trace.fraction < 1.0f ) {
		hitEnt = gameLocal.GetTraceEntity( trace );
		if ( hitEnt && hitEnt->IsType( idActor::Type ) ) {
			DirectDamage( meleeDefName, hitEnt );
			idThread::ReturnInt( true );
			return;
		}
	}

	idThread::ReturnInt( false );
}

/*
=====================
idAI::Event_CanBecomeSolid
=====================
*/
void idAI::Event_CanBecomeSolid( void ) {
	int			i;
	int			num;
	idEntity *	hit;
	idClipModel *cm;
	idClipModel *clipModels[ MAX_GENTITIES ];

	num = gameLocal.clip.ClipModelsTouchingBounds( physicsObj.GetAbsBounds(), MASK_MONSTERSOLID, clipModels, MAX_GENTITIES );
	for ( i = 0; i < num; i++ ) {
		cm = clipModels[ i ];

		// don't check render entities
		if ( cm->IsRenderModel() ) {
			continue;
		}

		hit = cm->GetEntity();
		if ( ( hit == this ) || !hit->fl.takedamage ) {
			continue;
		}

		if ( physicsObj.ClipContents( cm ) ) {
			idThread::ReturnFloat( false );
			return;
		}
	}

	idThread::ReturnFloat( true );
}

/*
=====================
idAI::Event_BecomeSolid
=====================
*/
void idAI::Event_BecomeSolid( void ) {
	physicsObj.EnableClip();
	if ( spawnArgs.GetBool( "big_monster" ) ) {
		physicsObj.SetContents( 0 );
	} else if ( use_combat_bbox ) {
		physicsObj.SetContents( CONTENTS_BODY|CONTENTS_SOLID );
	} else {
		physicsObj.SetContents( CONTENTS_BODY );
	}

	// SR CONTENTS_RESONSE FIX
	if( m_StimResponseColl->HasResponse() )
		physicsObj.SetContents( physicsObj.GetContents() | CONTENTS_RESPONSE );

	physicsObj.GetClipModel()->Link( gameLocal.clip );
	fl.takedamage = !spawnArgs.GetBool( "noDamage" );
}

/*
=====================
idAI::Event_BecomeNonSolid
=====================
*/
void idAI::Event_BecomeNonSolid( void ) {
	fl.takedamage = false;
	physicsObj.SetContents( 0 );
	physicsObj.GetClipModel()->Unlink();
}

/*
=====================
idAI::Event_BecomeRagdoll
=====================
*/
void idAI::Event_BecomeRagdoll( void ) {
	bool result;

	result = StartRagdoll();
	idThread::ReturnInt( result );
}

/*
=====================
idAI::Event_StopRagdoll
=====================
*/
void idAI::Event_StopRagdoll( void ) {
	StopRagdoll();

	// set back the monster physics
	SetPhysics( &physicsObj );
}

/*
=====================
idAI::Event_SetHealth
=====================
*/
void idAI::Event_SetHealth( float newHealth ) {
	health = newHealth;
	fl.takedamage = true;
	if ( health > 0 ) {
		AI_DEAD = false;
	} else {
		AI_DEAD = true;
	}
}

/*
=====================
idAI::Event_GetHealth
=====================
*/
void idAI::Event_GetHealth( void ) {
	idThread::ReturnFloat( health );
}

/*
=====================
idAI::Event_AllowDamage
=====================
*/
void idAI::Event_AllowDamage( void ) {
	fl.takedamage = true;
}

/*
=====================
idAI::Event_IgnoreDamage
=====================
*/
void idAI::Event_IgnoreDamage( void ) {
	fl.takedamage = false;
}

/*
=====================
idAI::Event_GetCurrentYaw
=====================
*/
void idAI::Event_GetCurrentYaw( void ) {
	idThread::ReturnFloat( current_yaw );
}

/*
=====================
idAI::Event_TurnTo
=====================
*/
void idAI::Event_TurnTo( float angle ) {
	TurnToward( angle );
}

/*
=====================
idAI::Event_TurnToPos
=====================
*/
void idAI::Event_TurnToPos( const idVec3 &pos ) {
	TurnToward( pos );
}

/*
=====================
idAI::Event_TurnToEntity
=====================
*/
void idAI::Event_TurnToEntity( idEntity *ent ) {
	if ( ent ) {
		TurnToward( ent->GetPhysics()->GetOrigin() );
	}
}

/*
=====================
idAI::Event_MoveStatus
=====================
*/
void idAI::Event_MoveStatus( void ) {
	idThread::ReturnInt( move.moveStatus );
}

/*
=====================
idAI::Event_StopMove
=====================
*/
void idAI::Event_StopMove( void ) {
	StopMove( MOVE_STATUS_DONE );
}

/*
=====================
idAI::Event_MoveToCover
=====================
*/
void idAI::Event_MoveToCover( void ) {
	idActor *enemyEnt = enemy.GetEntity();
	if (!enemyEnt) common->Printf("Warning: Entity is null\n");

	StopMove( MOVE_STATUS_DEST_NOT_FOUND );
	if ( !enemyEnt || !MoveToCover( enemyEnt, lastVisibleEnemyPos ) ) {
		return;
	}
}

/*
=====================
idAI::Event_MoveToCoverFrom
=====================
*/
void idAI::Event_MoveToCoverFrom( idEntity* enemyEnt ) {
	if (!enemyEnt) enemyEnt = enemy.GetEntity();
	if (!enemyEnt) { common->Printf("Warning: Entity is null\n"); return; }
	StopMove( MOVE_STATUS_DEST_NOT_FOUND );

	// Hide from the eye position of the enemy, if the enemy is an actor;
	// otherwise we have to make do with its origin plus an offset.
	idVec3 hideFrom;
	if (dynamic_cast <idActor*>(enemyEnt)) {
		hideFrom = static_cast<idActor*> (enemyEnt)->GetEyePosition();
	} else {
		hideFrom = enemyEnt->GetPhysics()->GetOrigin();
		hideFrom.z += 96.0f; // about 6 feet
	}
	
	if (!MoveToCover( enemyEnt, hideFrom )) {
		// failed
		return;
	}
}

/*
=====================
idAI::Event_MoveToEnemy
=====================
*/
void idAI::Event_MoveToEnemy( void ) {
	StopMove( MOVE_STATUS_DEST_NOT_FOUND );
	if ( !enemy.GetEntity() || !MoveToEnemy() ) {
		return;
	}
}

/*
=====================
idAI::Event_MoveToEnemyHeight
=====================
*/
void idAI::Event_MoveToEnemyHeight( void ) {
	StopMove( MOVE_STATUS_DEST_NOT_FOUND );
	MoveToEnemyHeight();
}

/*
=====================
idAI::Event_MoveOutOfRange
=====================
*/
void idAI::Event_MoveOutOfRange( idEntity *entity, float range ) {
	StopMove( MOVE_STATUS_DEST_NOT_FOUND );
	MoveOutOfRange( entity, range );
}

/*
=====================
idAI::Event_GetObservationPosition
by SophisticatedZobmie for The Dark Mod
This is an adaptation of the find attack position
query that is within MoveToAttackPosition
=====================
*/
void idAI::Event_GetObservationPosition (const idVec3& pointToObserve, const float visualAcuityZeroToOne)
{
	int				areaNum;
	aasObstacle_t	obstacle;
	aasGoal_t		goal;
	idBounds		bounds;
	idVec3			observeFromPos;
	aasPath_t	path;

	if ( !aas ) 
	{	
		observeFromPos = GetPhysics()->GetOrigin();
		idThread::ReturnVector (observeFromPos);
		AI_DEST_UNREACHABLE = true;
		return;
	}

	const idVec3 &org = physicsObj.GetOrigin();
	areaNum	= PointReachableAreaNum( org );

	// Raise point up just a bit so it isn't on the floor of the aas
	idVec3 pointToObserve2 = pointToObserve;
	pointToObserve2.z += 45.0;

	// What is the lighting along the line where the thing to be observed
	// might be.
	float maxDistanceToObserve = getMaximumObservationDistance
	(
		pointToObserve,
		pointToObserve2,
		NULL
	);
		
	idAASFindObservationPosition findGoal
	(
		this, 
		physicsObj.GetGravityAxis(), 
		pointToObserve2, 
		GetEyePosition() - org,  // Offset of eye from origin
		maxDistanceToObserve // Maximum distance from which we can observe
	);

	if (!aas->FindNearestGoal
	(
		goal, 
		areaNum, 
		org,
		pointToObserve2, // It is also the goal target
		travelFlags, 
		NULL, 
		0, 
		findGoal 
	) ) 
	{
		float bestDistance;

		// See if we can get to the point itself since noplace was good enough
		// for just looking from a distance due to lighting/occlusion/reachability.
		if (PathToGoal( path, areaNum, physicsObj.GetOrigin(), areaNum, org ) ) 
		{

			// Can reach the point itself, so walk right up to it
			observeFromPos = pointToObserve; 

			// Draw the AI Debug Graphics
			if (cv_ai_search_show.GetInteger() >= 1.0)
			{
				idVec4 markerColor (0.0, 1.0, 1.0, 1.0);
				idVec3 arrowLength (0.0, 0.0, 50.0);

				gameRenderWorld->DebugArrow
				(
					markerColor,
					observeFromPos + arrowLength,
					observeFromPos,
					2,
					cv_ai_search_show.GetInteger()
				);
			}

		}
		else if (findGoal.getBestGoalResult
		(
			bestDistance,
			goal
		))
		{

			// Use closest reachable observation point that we found
			observeFromPos = goal.origin;

			// Draw the AI Debug Graphics
			if (cv_ai_search_show.GetInteger() >= 1.0)
			{
				idVec4 markerColor (1.0, 1.0, 0.0, 1.0);
				idVec3 arrowLength (0.0, 0.0, 50.0);

				gameRenderWorld->DebugArrow
				(
					markerColor,
					observeFromPos,
					pointToObserve,
					2,
					cv_ai_search_show.GetInteger()
				);
			}

		}
		else
		{
			// No choice but to try to walk up to it as much as we can
			observeFromPos = pointToObserve; 

			// Draw the AI Debug Graphics
			if (cv_ai_search_show.GetInteger() >= 1.0)
			{
				idVec4 markerColor (1.0, 0.0, 0.0, 1.0);
				idVec3 arrowLength (0.0, 0.0, 50.0);

				gameRenderWorld->DebugArrow
				(
					markerColor,
					observeFromPos + arrowLength,
					observeFromPos,
					2,
					cv_ai_search_show.GetInteger()
				);
			}

		}
		
		
		idThread::ReturnVector (observeFromPos);

		
		return;
	}
	else
	{
		observeFromPos = goal.origin;
		AI_DEST_UNREACHABLE = false;

		// Draw the AI Debug Graphics
		if (cv_ai_search_show.GetInteger() >= 1.0)
		{
			idVec4 markerColor (0.0, 1.0, 0.0, 1.0);
			idVec3 arrowLength (0.0, 0.0, 50.0);

			gameRenderWorld->DebugArrow
			(
				markerColor,
				observeFromPos,
				pointToObserve,
				2,
				cv_ai_search_show.GetInteger()
			);
		}

		idThread::ReturnVector (observeFromPos);
		return;
	}
	
}

/*
=====================
idAI::Event_MoveToAttackPosition
=====================
*/
void idAI::Event_MoveToAttackPosition( idEntity *entity, const char *attack_anim ) {
	int anim;

	StopMove( MOVE_STATUS_DEST_NOT_FOUND );

	anim = GetAnim( ANIMCHANNEL_LEGS, attack_anim );
	if ( !anim ) {
		gameLocal.Error( "Unknown anim '%s'", attack_anim );
	}

	MoveToAttackPosition( entity, anim );
}

/*
=====================
idAI::Event_MoveToEntity
=====================
*/
void idAI::Event_MoveToEntity( idEntity *ent ) {
	StopMove( MOVE_STATUS_DEST_NOT_FOUND );
	if ( ent ) {
		MoveToEntity( ent );
	}
}

/*
=====================
idAI::Event_MoveToPosition
=====================
*/
void idAI::Event_MoveToPosition( const idVec3 &pos ) {
	StopMove( MOVE_STATUS_DONE );
	MoveToPosition( pos );
}

/*
=====================
idAI::Event_SlideTo
=====================
*/
void idAI::Event_SlideTo( const idVec3 &pos, float time ) {
	SlideToPosition( pos, time );
}
/*
=====================
idAI::Event_Wander
=====================
*/
void idAI::Event_Wander( void ) {
	WanderAround();
}

/*
=====================
idAI::Event_FacingIdeal
=====================
*/
void idAI::Event_FacingIdeal( void ) {
	bool facing = FacingIdeal();
	idThread::ReturnInt( facing );
}

/*
=====================
idAI::Event_FaceEnemy
=====================
*/
void idAI::Event_FaceEnemy( void ) {
	FaceEnemy();
}

/*
=====================
idAI::Event_FaceEntity
=====================
*/
void idAI::Event_FaceEntity( idEntity *ent ) {
	FaceEntity( ent );
}

/*
=====================
idAI::Event_WaitAction
=====================
*/
void idAI::Event_WaitAction( const char *waitForState ) {
	if ( idThread::BeginMultiFrameEvent( this, &AI_WaitAction ) ) {
		SetWaitState( waitForState );
	}

	if ( !WaitState() ) {
		idThread::EndMultiFrameEvent( this, &AI_WaitAction );
	}
}

/*
=====================
idAI::Event_GetCombatNode
=====================
*/
void idAI::Event_GetCombatNode( void ) {
	int				i;
	float			dist;
	idEntity		*targetEnt;
	idCombatNode	*node;
	float			bestDist;
	idCombatNode	*bestNode;
	idActor			*enemyEnt = enemy.GetEntity();

	if ( !targets.Num() ) {
		// no combat nodes
		idThread::ReturnEntity( NULL );
		return;
	}

	if ( !enemyEnt || !EnemyPositionValid() ) {
		// don't return a combat node if we don't have an enemy or
		// if we can see he's not in the last place we saw him
		idThread::ReturnEntity( NULL );
		return;
	}

	// find the closest attack node that can see our enemy and is closer than our enemy
	bestNode = NULL;
	const idVec3 &myPos = physicsObj.GetOrigin();
	bestDist = ( myPos - lastVisibleEnemyPos ).LengthSqr();
	for( i = 0; i < targets.Num(); i++ ) {
		targetEnt = targets[ i ].GetEntity();
		if ( !targetEnt || !targetEnt->IsType( idCombatNode::Type ) ) {
			continue;
		}

		node = static_cast<idCombatNode *>( targetEnt );
		if ( !node->IsDisabled() && node->EntityInView( enemyEnt, lastVisibleEnemyPos ) ) {
			idVec3 org = node->GetPhysics()->GetOrigin();
			dist = ( myPos - org ).LengthSqr();
			if ( dist < bestDist ) {
				bestNode = node;
				bestDist = dist;
			}
		}
	}

	idThread::ReturnEntity( bestNode );
}

/*
=====================
idAI::Event_EnemyInCombatCone
=====================
*/
void idAI::Event_EnemyInCombatCone( idEntity *ent, int use_current_enemy_location ) {
	idCombatNode	*node;
	bool			result;
	idActor			*enemyEnt = enemy.GetEntity();

	if ( !targets.Num() ) {
		// no combat nodes
		idThread::ReturnInt( false );
		return;
	}

	if ( !enemyEnt ) {
		// have to have an enemy
		idThread::ReturnInt( false );
		return;
	}

	if ( !ent || !ent->IsType( idCombatNode::Type ) ) {
		// not a combat node
		idThread::ReturnInt( false );
		return;
	}

	node = static_cast<idCombatNode *>( ent );
	if ( use_current_enemy_location ) {
		const idVec3 &pos = enemyEnt->GetPhysics()->GetOrigin();
		result = node->EntityInView( enemyEnt, pos );
	} else {
		result = node->EntityInView( enemyEnt, lastVisibleEnemyPos );
	}

	idThread::ReturnInt( result );
}

/*
=====================
idAI::Event_WaitMove
=====================
*/
void idAI::Event_WaitMove( void ) {
	idThread::BeginMultiFrameEvent( this, &AI_WaitMove );

	if ( MoveDone() ) {
		idThread::EndMultiFrameEvent( this, &AI_WaitMove );
	}
}

/*
=====================
idAI::Event_GetJumpVelocity
=====================
*/
void idAI::Event_GetJumpVelocity( const idVec3 &pos, float speed, float max_height ) {
	idVec3 start;
	idVec3 end;
	idVec3 dir;
	float dist;
	bool result;
	idEntity *enemyEnt = enemy.GetEntity();

	if ( !enemyEnt ) {
		idThread::ReturnVector( vec3_zero );
		return;
	}

	if ( speed <= 0.0f ) {
		gameLocal.Error( "Invalid speed.  speed must be > 0." );
	}

	start = physicsObj.GetOrigin();
	end = pos;
	dir = end - start;
	dist = dir.Normalize();
	if ( dist > 16.0f ) {
		dist -= 16.0f;
		end -= dir * 16.0f;
	}

	result = PredictTrajectory( start, end, speed, physicsObj.GetGravity(), physicsObj.GetClipModel(), MASK_MONSTERSOLID, max_height, this, enemyEnt, ai_debugMove.GetBool() ? 4000 : 0, dir );
	if ( result ) {
		idThread::ReturnVector( dir * speed );
	} else {
		idThread::ReturnVector( vec3_zero );
	}
}

/*
=====================
idAI::Event_EntityInAttackCone
=====================
*/
void idAI::Event_EntityInAttackCone( idEntity *ent ) {
	float	attack_cone;
	idVec3	delta;
	float	yaw;
	float	relYaw;
	
	if ( !ent ) {
		idThread::ReturnInt( false );
		return;
	}

	delta = ent->GetPhysics()->GetOrigin() - GetEyePosition();

	// get our gravity normal
	const idVec3 &gravityDir = GetPhysics()->GetGravityNormal();

	// infinite vertical vision, so project it onto our orientation plane
	delta -= gravityDir * ( gravityDir * delta );

	delta.Normalize();
	yaw = delta.ToYaw();

	attack_cone = spawnArgs.GetFloat( "attack_cone", "70" );
	relYaw = idMath::AngleNormalize180( ideal_yaw - yaw );
	if ( idMath::Fabs( relYaw ) < ( attack_cone * 0.5f ) ) {
		idThread::ReturnInt( true );
	} else {
		idThread::ReturnInt( false );
	}
}

/*
=====================
idAI::Event_CanSeeEntity
=====================
*/
void idAI::Event_CanSeeEntity( idEntity *ent ) {
	if ( !ent ) {
		idThread::ReturnInt( false );
		return;
	}

	// Test if it is occluded, and use field of vision in the check (true as second parameter)
	bool cansee = CanSee( ent, true );
	
	idThread::ReturnInt( cansee );
}

/*
=====================
idAI::Event_CanSeeEntityExt
=====================
*/
void idAI::Event_CanSeeEntityExt( idEntity *ent, int bool_useFOV, int bool_useLighting ) 
{

	if ( !ent ) 
	{
		idThread::ReturnInt( false );
		return;
	}

	// Test if it is visible
	bool cansee = CanSeeExt( ent, (bool_useFOV != 0), (bool_useLighting != 0) );
	
	idThread::ReturnInt( cansee );
}

/*
=====================
idAI::Event_CanSeePositionExt
=====================
*/
void idAI::Event_CanSeePositionExt( const idVec3& position, int bool_useFOV, int bool_useLighting ) 
{
	bool cansee = CanSeePositionExt( position, (bool_useFOV != 0), (bool_useLighting != 0) );
	idThread::ReturnInt( cansee );
}


/*
=====================
idAI::Event_SetTalkTarget
=====================
*/
void idAI::Event_SetTalkTarget( idEntity *target ) {
	if ( target && !target->IsType( idActor::Type ) ) {
		gameLocal.Error( "Cannot set talk target to '%s'.  Not a character or player.", target->GetName() );
	}
	talkTarget = static_cast<idActor *>( target );
	if ( target ) {
		AI_TALK = true;
	} else {
		AI_TALK = false;
	}
}

/*
=====================
idAI::Event_GetTalkTarget
=====================
*/
void idAI::Event_GetTalkTarget( void ) {
	idThread::ReturnEntity( talkTarget.GetEntity() );
}

/*
================
idAI::Event_SetTalkState
================
*/
void idAI::Event_SetTalkState( int state ) {
	if ( ( state < 0 ) || ( state >= NUM_TALK_STATES ) ) {
		gameLocal.Error( "Invalid talk state (%d)", state );
	}

	talk_state = static_cast<talkState_t>( state );
}

/*
=====================
idAI::Event_EnemyRange
=====================
*/
void idAI::Event_EnemyRange( void ) {
	float dist;
	idActor *enemyEnt = enemy.GetEntity();

	if ( enemyEnt ) {
		dist = ( enemyEnt->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin() ).Length();
	} else {
		// Just some really high number
		dist = idMath::INFINITY;
	}

	idThread::ReturnFloat( dist );
}

/*
=====================
idAI::Event_EnemyRange2D
=====================
*/
void idAI::Event_EnemyRange2D( void ) {
	float dist;
	idActor *enemyEnt = enemy.GetEntity();

	if ( enemyEnt ) {
		dist = ( enemyEnt->GetPhysics()->GetOrigin().ToVec2() - GetPhysics()->GetOrigin().ToVec2() ).Length();
	} else {
		// Just some really high number
		dist = idMath::INFINITY;
	}

	idThread::ReturnFloat( dist );
}

/*
=====================
idAI::Event_GetEnemy
=====================
*/
void idAI::Event_GetEnemy( void ) {
	idThread::ReturnEntity( enemy.GetEntity() );
}

/*
=====================
idAI::Event_GetEnemyPos
=====================
*/
void idAI::Event_GetEnemyPos( void ) {
	idThread::ReturnVector( lastVisibleEnemyPos );
}

/*
=====================
idAI::Event_GetEnemyEyePos
=====================
*/
void idAI::Event_GetEnemyEyePos( void ) {
	idThread::ReturnVector( lastVisibleEnemyPos + lastVisibleEnemyEyeOffset );
}

/*
=====================
idAI::Event_PredictEnemyPos
=====================
*/
void idAI::Event_PredictEnemyPos( float time ) {
	predictedPath_t path;
	idActor *enemyEnt = enemy.GetEntity();

	// if no enemy set
	if ( !enemyEnt ) {
		idThread::ReturnVector( physicsObj.GetOrigin() );
		return;
	}

	// predict the enemy movement
	idAI::PredictPath( enemyEnt, aas, lastVisibleEnemyPos, enemyEnt->GetPhysics()->GetLinearVelocity(), SEC2MS( time ), SEC2MS( time ), ( move.moveType == MOVETYPE_FLY ) ? SE_BLOCKED : ( SE_BLOCKED | SE_ENTER_LEDGE_AREA ), path );

	idThread::ReturnVector( path.endPos );
}

/*
=====================
idAI::Event_CanHitEnemy
=====================
*/
void idAI::Event_CanHitEnemy( void ) {
	trace_t	tr;
	idEntity *hit;

	idActor *enemyEnt = enemy.GetEntity();
	if ( !AI_ENEMY_VISIBLE || !enemyEnt ) {
		idThread::ReturnInt( false );
		return;
	}

	// don't check twice per frame
	if ( gameLocal.time == lastHitCheckTime ) {
		idThread::ReturnInt( lastHitCheckResult );
		return;
	}

	lastHitCheckTime = gameLocal.time;

	idVec3 toPos = enemyEnt->GetEyePosition();
	idVec3 eye = GetEyePosition();
	idVec3 dir;

	// expand the ray out as far as possible so we can detect anything behind the enemy
	dir = toPos - eye;
	dir.Normalize();
	toPos = eye + dir * MAX_WORLD_SIZE;
	gameLocal.clip.TracePoint( tr, eye, toPos, MASK_SHOT_BOUNDINGBOX, this );
	hit = gameLocal.GetTraceEntity( tr );
	if ( tr.fraction >= 1.0f || ( hit == enemyEnt ) ) {
		lastHitCheckResult = true;
	} else if ( ( tr.fraction < 1.0f ) && ( hit->IsType( idAI::Type ) ) && 
		( static_cast<idAI *>( hit )->team != team ) ) {
		lastHitCheckResult = true;
	} else {
		lastHitCheckResult = false;
	}

	idThread::ReturnInt( lastHitCheckResult );
}

/*
=====================
idAI::Event_CanHitEnemyFromAnim
=====================
*/
void idAI::Event_CanHitEnemyFromAnim( const char *animname ) {
	int		anim;
	idVec3	dir;
	idVec3	local_dir;
	idVec3	fromPos;
	idMat3	axis;
	idVec3	start;
	trace_t	tr;
	float	distance;

	idActor *enemyEnt = enemy.GetEntity();
	if ( !AI_ENEMY_VISIBLE || !enemyEnt ) {
		idThread::ReturnInt( false );
		return;
	}

	anim = GetAnim( ANIMCHANNEL_LEGS, animname );
	if ( !anim ) {
		idThread::ReturnInt( false );
		return;
	}

	// just do a ray test if close enough
	if ( enemyEnt->GetPhysics()->GetAbsBounds().IntersectsBounds( physicsObj.GetAbsBounds().Expand( 16.0f ) ) ) {
		Event_CanHitEnemy();
		return;
	}

	// calculate the world transform of the launch position
	const idVec3 &org = physicsObj.GetOrigin();
	dir = lastVisibleEnemyPos - org;
	physicsObj.GetGravityAxis().ProjectVector( dir, local_dir );
	local_dir.z = 0.0f;
	local_dir.ToVec2().Normalize();
	axis = local_dir.ToMat3();
	fromPos = physicsObj.GetOrigin() + missileLaunchOffset[ anim ] * axis;

	if ( projectileClipModel == NULL ) {
		CreateProjectileClipModel();
	}

	// check if the owner bounds is bigger than the projectile bounds
	const idBounds &ownerBounds = physicsObj.GetAbsBounds();
	const idBounds &projBounds = projectileClipModel->GetBounds();
	if ( ( ( ownerBounds[1][0] - ownerBounds[0][0] ) > ( projBounds[1][0] - projBounds[0][0] ) ) &&
		( ( ownerBounds[1][1] - ownerBounds[0][1] ) > ( projBounds[1][1] - projBounds[0][1] ) ) &&
		( ( ownerBounds[1][2] - ownerBounds[0][2] ) > ( projBounds[1][2] - projBounds[0][2] ) ) ) {
		if ( (ownerBounds - projBounds).RayIntersection( org, viewAxis[ 0 ], distance ) ) {
			start = org + distance * viewAxis[ 0 ];
		} else {
			start = ownerBounds.GetCenter();
		}
	} else {
		// projectile bounds bigger than the owner bounds, so just start it from the center
		start = ownerBounds.GetCenter();
	}

	gameLocal.clip.Translation( tr, start, fromPos, projectileClipModel, mat3_identity, MASK_SHOT_RENDERMODEL, this );
	fromPos = tr.endpos;

	if ( GetAimDir( fromPos, enemy.GetEntity(), this, dir ) ) {
		idThread::ReturnInt( true );
	} else {
		idThread::ReturnInt( false );
	}
}

/*
=====================
idAI::Event_CanHitEnemyFromJoint
=====================
*/
void idAI::Event_CanHitEnemyFromJoint( const char *jointname ) {
	trace_t	tr;
	idVec3	muzzle;
	idMat3	axis;
	idVec3	start;
	float	distance;

	idActor *enemyEnt = enemy.GetEntity();
	if ( !AI_ENEMY_VISIBLE || !enemyEnt ) {
		idThread::ReturnInt( false );
		return;
	}

	// don't check twice per frame
	if ( gameLocal.time == lastHitCheckTime ) {
		idThread::ReturnInt( lastHitCheckResult );
		return;
	}

	lastHitCheckTime = gameLocal.time;

	const idVec3 &org = physicsObj.GetOrigin();
	idVec3 toPos = enemyEnt->GetEyePosition();
	jointHandle_t joint = animator.GetJointHandle( jointname );
	if ( joint == INVALID_JOINT ) {
		gameLocal.Error( "Unknown joint '%s' on %s", jointname, GetEntityDefName() );
	}
	animator.GetJointTransform( joint, gameLocal.time, muzzle, axis );
	muzzle = org + ( muzzle + modelOffset ) * viewAxis * physicsObj.GetGravityAxis();

	if ( projectileClipModel == NULL ) {
		CreateProjectileClipModel();
	}

	// check if the owner bounds is bigger than the projectile bounds
	const idBounds &ownerBounds = physicsObj.GetAbsBounds();
	const idBounds &projBounds = projectileClipModel->GetBounds();
	if ( ( ( ownerBounds[1][0] - ownerBounds[0][0] ) > ( projBounds[1][0] - projBounds[0][0] ) ) &&
		( ( ownerBounds[1][1] - ownerBounds[0][1] ) > ( projBounds[1][1] - projBounds[0][1] ) ) &&
		( ( ownerBounds[1][2] - ownerBounds[0][2] ) > ( projBounds[1][2] - projBounds[0][2] ) ) ) {
		if ( (ownerBounds - projBounds).RayIntersection( org, viewAxis[ 0 ], distance ) ) {
			start = org + distance * viewAxis[ 0 ];
		} else {
			start = ownerBounds.GetCenter();
		}
	} else {
		// projectile bounds bigger than the owner bounds, so just start it from the center
		start = ownerBounds.GetCenter();
	}

	gameLocal.clip.Translation( tr, start, muzzle, projectileClipModel, mat3_identity, MASK_SHOT_BOUNDINGBOX, this );
	muzzle = tr.endpos;

	gameLocal.clip.Translation( tr, muzzle, toPos, projectileClipModel, mat3_identity, MASK_SHOT_BOUNDINGBOX, this );
	if ( tr.fraction >= 1.0f || ( gameLocal.GetTraceEntity( tr ) == enemyEnt ) ) {
		lastHitCheckResult = true;
	} else {
		lastHitCheckResult = false;
	}

	idThread::ReturnInt( lastHitCheckResult );
}

/*
=====================
idAI::Event_EnemyPositionValid
=====================
*/
void idAI::Event_EnemyPositionValid( void ) {
	bool result;

	result = EnemyPositionValid();
	idThread::ReturnInt( result );
}

/*
=====================
idAI::Event_ChargeAttack
=====================
*/
void idAI::Event_ChargeAttack( const char *damageDef ) {
	idActor *enemyEnt = enemy.GetEntity();

	StopMove( MOVE_STATUS_DEST_NOT_FOUND );
	if ( enemyEnt ) {
		idVec3 enemyOrg;

		if ( move.moveType == MOVETYPE_FLY ) {
			// position destination so that we're in the enemy's view
			enemyOrg = enemyEnt->GetEyePosition();
			enemyOrg -= enemyEnt->GetPhysics()->GetGravityNormal() * fly_offset;
		} else {
			enemyOrg = enemyEnt->GetPhysics()->GetOrigin();
		}

		BeginAttack( damageDef );
		DirectMoveToPosition( enemyOrg );
		TurnToward( enemyOrg );
	}
}

/*
=====================
idAI::Event_TestChargeAttack
=====================
*/
void idAI::Event_TestChargeAttack( void ) {
	trace_t trace;
	idActor *enemyEnt = enemy.GetEntity();
	predictedPath_t path;
	idVec3 end;

	if ( !enemyEnt ) {
		idThread::ReturnFloat( 0.0f );
		return;
	}

	if ( move.moveType == MOVETYPE_FLY ) {
		// position destination so that we're in the enemy's view
		end = enemyEnt->GetEyePosition();
		end -= enemyEnt->GetPhysics()->GetGravityNormal() * fly_offset;
	} else {
		end = enemyEnt->GetPhysics()->GetOrigin();
	}

	idAI::PredictPath( this, aas, physicsObj.GetOrigin(), end - physicsObj.GetOrigin(), 1000, 1000, ( move.moveType == MOVETYPE_FLY ) ? SE_BLOCKED : ( SE_ENTER_OBSTACLE | SE_BLOCKED | SE_ENTER_LEDGE_AREA ), path );

	if ( ai_debugMove.GetBool() ) {
		gameRenderWorld->DebugLine( colorGreen, physicsObj.GetOrigin(), end, gameLocal.msec );
		gameRenderWorld->DebugBounds( path.endEvent == 0 ? colorYellow : colorRed, physicsObj.GetBounds(), end, gameLocal.msec );
	}

	if ( ( path.endEvent == 0 ) || ( path.blockingEntity == enemyEnt ) ) {
		idVec3 delta = end - physicsObj.GetOrigin();
		float time = delta.LengthFast();
		idThread::ReturnFloat( time );
	} else {
		idThread::ReturnFloat( 0.0f );
	}
}

/*
=====================
idAI::Event_TestAnimMoveTowardEnemy
=====================
*/
void idAI::Event_TestAnimMoveTowardEnemy( const char *animname ) {
	int				anim;
	predictedPath_t path;
	idVec3			moveVec;
	float			yaw;
	idVec3			delta;
	idActor			*enemyEnt;
	
	enemyEnt = enemy.GetEntity();
	if ( !enemyEnt ) {
		idThread::ReturnInt( false );
		return;
	}

	anim = GetAnim( ANIMCHANNEL_LEGS, animname );
	if ( !anim ) {
		
#ifndef SUPPRESS_CONSOLE_WARNINGS
		gameLocal.DWarning( "missing '%s' animation on '%s' (%s)", animname, name.c_str(), GetEntityDefName() );
#endif

		idThread::ReturnInt( false );
		return;
	}

	delta = enemyEnt->GetPhysics()->GetOrigin() - physicsObj.GetOrigin();
    yaw = delta.ToYaw();

	moveVec = animator.TotalMovementDelta( anim ) * idAngles( 0.0f, yaw, 0.0f ).ToMat3() * physicsObj.GetGravityAxis();
	idAI::PredictPath( this, aas, physicsObj.GetOrigin(), moveVec, 1000, 1000, ( move.moveType == MOVETYPE_FLY ) ? SE_BLOCKED : ( SE_ENTER_OBSTACLE | SE_BLOCKED | SE_ENTER_LEDGE_AREA ), path );

	if ( ai_debugMove.GetBool() ) {
		gameRenderWorld->DebugLine( colorGreen, physicsObj.GetOrigin(), physicsObj.GetOrigin() + moveVec, gameLocal.msec );
		gameRenderWorld->DebugBounds( path.endEvent == 0 ? colorYellow : colorRed, physicsObj.GetBounds(), physicsObj.GetOrigin() + moveVec, gameLocal.msec );
	}

	idThread::ReturnInt( path.endEvent == 0 );
}

/*
=====================
idAI::Event_TestAnimMove
=====================
*/
void idAI::Event_TestAnimMove( const char *animname ) {
	int				anim;
	predictedPath_t path;
	idVec3			moveVec;

	anim = GetAnim( ANIMCHANNEL_LEGS, animname );
	if ( !anim ) {

#ifndef SUPPRESS_CONSOLE_WARNINGS
		gameLocal.DWarning( "missing '%s' animation on '%s' (%s)", animname, name.c_str(), GetEntityDefName() );
#endif

		idThread::ReturnInt( false );
		return;
	}

	moveVec = animator.TotalMovementDelta( anim ) * idAngles( 0.0f, ideal_yaw, 0.0f ).ToMat3() * physicsObj.GetGravityAxis();
	idAI::PredictPath( this, aas, physicsObj.GetOrigin(), moveVec, 1000, 1000, ( move.moveType == MOVETYPE_FLY ) ? SE_BLOCKED : ( SE_ENTER_OBSTACLE | SE_BLOCKED | SE_ENTER_LEDGE_AREA ), path );

	if ( ai_debugMove.GetBool() ) {
		gameRenderWorld->DebugLine( colorGreen, physicsObj.GetOrigin(), physicsObj.GetOrigin() + moveVec, gameLocal.msec );
		gameRenderWorld->DebugBounds( path.endEvent == 0 ? colorYellow : colorRed, physicsObj.GetBounds(), physicsObj.GetOrigin() + moveVec, gameLocal.msec );
	}

	idThread::ReturnInt( path.endEvent == 0 );
}

/*
=====================
idAI::Event_TestMoveToPosition
=====================
*/
void idAI::Event_TestMoveToPosition( const idVec3 &position ) {
	predictedPath_t path;

	idAI::PredictPath( this, aas, physicsObj.GetOrigin(), position - physicsObj.GetOrigin(), 1000, 1000, ( move.moveType == MOVETYPE_FLY ) ? SE_BLOCKED : ( SE_ENTER_OBSTACLE | SE_BLOCKED | SE_ENTER_LEDGE_AREA ), path );

	if ( ai_debugMove.GetBool() ) {
		gameRenderWorld->DebugLine( colorGreen, physicsObj.GetOrigin(), position, gameLocal.msec );
		gameRenderWorld->DebugBounds( colorYellow, physicsObj.GetBounds(), position, gameLocal.msec );
		if ( path.endEvent ) {
			gameRenderWorld->DebugBounds( colorRed, physicsObj.GetBounds(), path.endPos, gameLocal.msec );
		}
	}

	idThread::ReturnInt( path.endEvent == 0 );
}

/*
=====================
idAI::Event_TestMeleeAttack
=====================
*/
void idAI::Event_TestMeleeAttack( void ) {
	bool result = TestMelee();
	idThread::ReturnInt( result );
}

/*
=====================
idAI::Event_TestAnimAttack
=====================
*/
void idAI::Event_TestAnimAttack( const char *animname ) {
	int				anim;
	predictedPath_t path;

	anim = GetAnim( ANIMCHANNEL_LEGS, animname );
	if ( !anim ) {

#ifndef SUPPRESS_CONSOLE_WARNINGS
		gameLocal.DWarning( "missing '%s' animation on '%s' (%s)", animname, name.c_str(), GetEntityDefName() );
#endif

		idThread::ReturnInt( false );
		return;
	}

	idAI::PredictPath( this, aas, physicsObj.GetOrigin(), animator.TotalMovementDelta( anim ), 1000, 1000, ( move.moveType == MOVETYPE_FLY ) ? SE_BLOCKED : ( SE_ENTER_OBSTACLE | SE_BLOCKED | SE_ENTER_LEDGE_AREA ), path );

	idThread::ReturnInt( path.blockingEntity && ( path.blockingEntity == enemy.GetEntity() ) );
}

/*
=====================
idAI::Event_Shrivel
=====================
*/
void idAI::Event_Shrivel( float shrivel_time ) {
	float t;

	if ( idThread::BeginMultiFrameEvent( this, &AI_Shrivel ) ) {
		if ( shrivel_time <= 0.0f ) {
			idThread::EndMultiFrameEvent( this, &AI_Shrivel );
			return;
		}

		shrivel_rate = 0.001f / shrivel_time;
		shrivel_start = gameLocal.time;
	}

	t = ( gameLocal.time - shrivel_start ) * shrivel_rate;
	if ( t > 0.25f ) {
		renderEntity.noShadow = true;
	}
	if ( t > 1.0f ) {
		t = 1.0f;
		idThread::EndMultiFrameEvent( this, &AI_Shrivel );
	}

	renderEntity.shaderParms[ SHADERPARM_MD5_SKINSCALE ] = 1.0f - t * 0.5f;
	UpdateVisuals();
}

/*
=====================
idAI::Event_PreBurn
=====================
*/
void idAI::Event_PreBurn( void ) {
	// for now this just turns shadows off
	renderEntity.noShadow = true;
}

/*
=====================
idAI::Event_Burn
=====================
*/
void idAI::Event_Burn( void ) {
	renderEntity.shaderParms[ SHADERPARM_TIME_OF_DEATH ] = gameLocal.time * 0.001f;
	SpawnParticles( "smoke_burnParticleSystem" );
	UpdateVisuals();
}

/*
=====================
idAI::Event_ClearBurn
=====================
*/
void idAI::Event_ClearBurn( void ) {
	renderEntity.noShadow = spawnArgs.GetBool( "noshadows" );
	renderEntity.shaderParms[ SHADERPARM_TIME_OF_DEATH ] = 0.0f;
	UpdateVisuals();
}

/*
=====================
idAI::Event_SetSmokeVisibility
=====================
*/
void idAI::Event_SetSmokeVisibility( int num, int on ) {
	int i;
	int time;

	if ( num >= particles.Num() ) {
		gameLocal.Warning( "Particle #%d out of range (%d particles) on entity '%s'", num, particles.Num(), name.c_str() );
		return;
	}

	if ( on != 0 ) {
		time = gameLocal.time;
		BecomeActive( TH_UPDATEPARTICLES );
	} else {
		time = 0;
	}

	if ( num >= 0 ) {
		particles[ num ].time = time;
	} else {
		for ( i = 0; i < particles.Num(); i++ ) {
			particles[ i ].time = time;
		}
	}

	UpdateVisuals();
}

/*
=====================
idAI::Event_NumSmokeEmitters
=====================
*/
void idAI::Event_NumSmokeEmitters( void ) {
	idThread::ReturnInt( particles.Num() );
}

/*
=====================
idAI::Event_StopThinking
=====================
*/
void idAI::Event_StopThinking( void ) {
	BecomeInactive( TH_THINK );
	idThread *thread = idThread::CurrentThread();
	if ( thread ) {
		thread->DoneProcessing();
	}
}

/*
=====================
idAI::Event_GetTurnDelta
=====================
*/
void idAI::Event_GetTurnDelta( void ) {
	float amount;

	if ( turnRate ) {
		amount = idMath::AngleNormalize180( ideal_yaw - current_yaw );
		idThread::ReturnFloat( amount );
	} else {
		idThread::ReturnFloat( 0.0f );
	}
}

/*
=====================
idAI::Event_GetMoveType
=====================
*/
void idAI::Event_GetMoveType( void ) {
	idThread::ReturnInt( move.moveType );
}

/*
=====================
idAI::Event_SetMoveTypes
=====================
*/
void idAI::Event_SetMoveType( int moveType ) {
	if ( ( moveType < 0 ) || ( moveType >= NUM_MOVETYPES ) ) {
		gameLocal.Error( "Invalid movetype %d", moveType );
	}

	move.moveType = static_cast<moveType_t>( moveType );
	if ( move.moveType == MOVETYPE_FLY ) {
		travelFlags = TFL_WALK|TFL_AIR|TFL_FLY|TFL_DOOR;
	} else {
		travelFlags = TFL_WALK|TFL_AIR|TFL_DOOR;
	}
}

/*
=====================
idAI::Event_SaveMove
=====================
*/
void idAI::Event_SaveMove( void ) {
	savedMove = move;
}

/*
=====================
idAI::Event_RestoreMove
=====================
*/
void idAI::Event_RestoreMove( void ) {
	idVec3 goalPos;
	idVec3 dest;

	switch( savedMove.moveCommand ) {
	case MOVE_NONE :
		StopMove( savedMove.moveStatus );
		break;

	case MOVE_FACE_ENEMY :
		FaceEnemy();
		break;

	case MOVE_FACE_ENTITY :
		FaceEntity( savedMove.goalEntity.GetEntity() );
		break;

	case MOVE_TO_ENEMY :
		MoveToEnemy();
		break;

	case MOVE_TO_ENEMYHEIGHT :
		MoveToEnemyHeight();
		break;

	case MOVE_TO_ENTITY :
		MoveToEntity( savedMove.goalEntity.GetEntity() );
		break;

	case MOVE_OUT_OF_RANGE :
		MoveOutOfRange( savedMove.goalEntity.GetEntity(), savedMove.range );
		break;

	case MOVE_TO_ATTACK_POSITION :
		MoveToAttackPosition( savedMove.goalEntity.GetEntity(), savedMove.anim );
		break;

	case MOVE_TO_COVER :
		MoveToCover( savedMove.goalEntity.GetEntity(), lastVisibleEnemyPos );
		break;

	case MOVE_TO_POSITION :
		MoveToPosition( savedMove.moveDest );
		break;

	case MOVE_TO_POSITION_DIRECT :
		DirectMoveToPosition( savedMove.moveDest );
		break;

	case MOVE_SLIDE_TO_POSITION :
		SlideToPosition( savedMove.moveDest, savedMove.duration );
		break;

	case MOVE_WANDER :
		WanderAround();
		break;
	}

	if ( GetMovePos( goalPos ) ) {
		CheckObstacleAvoidance( goalPos, dest );
	}
}

/*
=====================
idAI::Event_AllowMovement
=====================
*/
void idAI::Event_AllowMovement( float flag ) {
	allowMove = ( flag != 0.0f );
}

/*
=====================
idAI::Event_JumpFrame
=====================
*/
void idAI::Event_JumpFrame( void ) {
	AI_JUMP = true;
}

/*
=====================
idAI::Event_EnableClip
=====================
*/
void idAI::Event_EnableClip( void ) {
	physicsObj.SetClipMask( MASK_MONSTERSOLID );
	disableGravity = false;
}

/*
=====================
idAI::Event_DisableClip
=====================
*/
void idAI::Event_DisableClip( void ) {
	physicsObj.SetClipMask( 0 );
	disableGravity = true;
}

/*
=====================
idAI::Event_EnableGravity
=====================
*/
void idAI::Event_EnableGravity( void ) {
	disableGravity = false;
}

/*
=====================
idAI::Event_DisableGravity
=====================
*/
void idAI::Event_DisableGravity( void ) {
	disableGravity = true;
}

/*
=====================
idAI::Event_EnableAFPush
=====================
*/
void idAI::Event_EnableAFPush( void ) {
	af_push_moveables = true;
}

/*
=====================
idAI::Event_DisableAFPush
=====================
*/
void idAI::Event_DisableAFPush( void ) {
	af_push_moveables = false;
}

/*
=====================
idAI::Event_SetFlySpeed
=====================
*/
void idAI::Event_SetFlySpeed( float speed ) {
	if ( move.speed == fly_speed ) {
		move.speed = speed;
	}
	fly_speed = speed;
}

/*
================
idAI::Event_SetFlyOffset
================
*/
void idAI::Event_SetFlyOffset( int offset ) {
	fly_offset = offset;
}

/*
================
idAI::Event_ClearFlyOffset
================
*/
void idAI::Event_ClearFlyOffset( void ) {
	spawnArgs.GetInt( "fly_offset",	"0", fly_offset );
}

/*
=====================
idAI::Event_GetClosestHiddenTarget
=====================
*/
void idAI::Event_GetClosestHiddenTarget( const char *type ) {
	int	i;
	idEntity *ent;
	idEntity *bestEnt;
	float time;
	float bestTime;
	const idVec3 &org = physicsObj.GetOrigin();
	idActor *enemyEnt = enemy.GetEntity();

	if ( !enemyEnt ) {
		// no enemy to hide from
		idThread::ReturnEntity( NULL );
		return;
	}

	if ( targets.Num() == 1 ) {
		ent = targets[ 0 ].GetEntity();
		if ( ent && idStr::Cmp( ent->GetEntityDefName(), type ) == 0 ) {
			if ( !EntityCanSeePos( enemyEnt, lastVisibleEnemyPos, ent->GetPhysics()->GetOrigin() ) ) {
				idThread::ReturnEntity( ent );
				return;
			}
		}
		idThread::ReturnEntity( NULL );
		return;
	}

	bestEnt = NULL;
	bestTime = idMath::INFINITY;
	for( i = 0; i < targets.Num(); i++ ) {
		ent = targets[ i ].GetEntity();
		if ( ent && idStr::Cmp( ent->GetEntityDefName(), type ) == 0 ) {
			const idVec3 &destOrg = ent->GetPhysics()->GetOrigin();
			time = TravelDistance( org, destOrg );
			if ( ( time >= 0.0f ) && ( time < bestTime ) ) {
				if ( !EntityCanSeePos( enemyEnt, lastVisibleEnemyPos, destOrg ) ) {
					bestEnt = ent;
					bestTime = time;
				}
			}
		}
	}
	idThread::ReturnEntity( bestEnt );
}

/*
=====================
idAI::Event_GetRandomTarget
=====================
*/
void idAI::Event_GetRandomTarget( const char *type ) {
	int	i;
	int	num;
	int which;
	idEntity *ent;
	idEntity *ents[ MAX_GENTITIES ];

	num = 0;
	for( i = 0; i < targets.Num(); i++ ) {
		ent = targets[ i ].GetEntity();
		if ( ent && idStr::Cmp( ent->GetEntityDefName(), type ) == 0 ) {
			ents[ num++ ] = ent;
			if ( num >= MAX_GENTITIES ) {
				break;
			}
		}
	}

	if ( !num ) {
		idThread::ReturnEntity( NULL );
		return;
	}

	which = gameLocal.random.RandomInt( num );
	idThread::ReturnEntity( ents[ which ] );
}

/*
================
idAI::Event_TravelDistanceToPoint
================
*/
void idAI::Event_TravelDistanceToPoint( const idVec3 &pos ) {
	float time;

	time = TravelDistance( physicsObj.GetOrigin(), pos );
	idThread::ReturnFloat( time );
}

/*
================
idAI::Event_TravelDistanceToEntity
================
*/
void idAI::Event_TravelDistanceToEntity( idEntity *ent ) {
	float time;

	time = TravelDistance( physicsObj.GetOrigin(), ent->GetPhysics()->GetOrigin() );
	idThread::ReturnFloat( time );
}

/*
================
idAI::Event_TravelDistanceBetweenPoints
================
*/
void idAI::Event_TravelDistanceBetweenPoints( const idVec3 &source, const idVec3 &dest ) {
	float time;

	time = TravelDistance( source, dest );
	idThread::ReturnFloat( time );
}

/*
================
idAI::Event_TravelDistanceBetweenEntities
================
*/
void idAI::Event_TravelDistanceBetweenEntities( idEntity *source, idEntity *dest ) {
	float time;

	assert( source );
	assert( dest );
	time = TravelDistance( source->GetPhysics()->GetOrigin(), dest->GetPhysics()->GetOrigin() );
	idThread::ReturnFloat( time );
}

/*
=====================
idAI::Event_LookAtPosition
=====================
*/
void idAI::Event_LookAtPosition (const idVec3& lookAtWorldPosition, float duration)
{
	if ( ( focusEntity.GetEntity() != NULL ) || ( currentFocusPos != lookAtWorldPosition) || (gameLocal.time ) ) 
	{
		focusEntity	= NULL;
		currentFocusPos = lookAtWorldPosition;
		alignHeadTime = gameLocal.time;
		forceAlignHeadTime = gameLocal.time + SEC2MS( 1 );
		blink_time = 0;
	}

	focusTime = gameLocal.time + SEC2MS( duration );

}


void idAI::Event_LookAtAngles (float yawAngleClockwise, float pitchAngleUp, float rollAngle, float durationInSeconds)
{
	// Get our physical axis
	idMat3 physicalAxis = GetPhysics()->GetAxis();
	idAngles physicalAngles = physicalAxis.ToAngles ();

	// FIX (Ishtvan)
	physicalAngles.yaw = current_yaw;

	// Now rotate it by the given angles
	idAngles lookAngles = idAngles(pitchAngleUp, yawAngleClockwise, rollAngle);

	lookAngles += physicalAngles;
	lookAngles.Normalize180();

	// Determine the look at world position
	idVec3 lookAtPositionDelta = lookAngles.ToForward() * 15.0;
	idVec3 lookAtWorldPosition = GetEyePosition() + lookAtPositionDelta;

	//gameRenderWorld->DebugArrow (idVec4(1.0,0.0,0.0,1.0), GetEyePosition(), lookAtWorldPosition, 1, 5000);

	// Update focus position
	if ( ( focusEntity.GetEntity() != NULL ) || ( currentFocusPos != lookAtWorldPosition) || (gameLocal.time ) ) 
	{
		focusEntity	= NULL;
		currentFocusPos = lookAtWorldPosition;
		alignHeadTime = gameLocal.time;
		forceAlignHeadTime = gameLocal.time + SEC2MS( 1 );
		blink_time = 0;
	}

	focusTime = gameLocal.time + SEC2MS( durationInSeconds );

}

/*
=====================
idAI::Event_LookAtEntity
=====================
*/
void idAI::Event_LookAtEntity( idEntity *ent, float duration ) {
	if ( ent == this ) {
		ent = NULL;
	}

	if ( ( ent != focusEntity.GetEntity() ) || ( focusTime < gameLocal.time ) ) {
		focusEntity	= ent;
		alignHeadTime = gameLocal.time;
		forceAlignHeadTime = gameLocal.time + SEC2MS( 1 );
		blink_time = 0;
	}

	focusTime = gameLocal.time + SEC2MS( duration );
}

/*
=====================
idAI::Event_LookAtEnemy
=====================
*/
void idAI::Event_LookAtEnemy( float duration ) {
	idActor *enemyEnt;

	enemyEnt = enemy.GetEntity();
	if ( ( enemyEnt != focusEntity.GetEntity() ) || ( focusTime < gameLocal.time ) ) {
		focusEntity	= enemyEnt;
		alignHeadTime = gameLocal.time;
		forceAlignHeadTime = gameLocal.time + SEC2MS( 1 );
		blink_time = 0;
	}

	focusTime = gameLocal.time + SEC2MS( duration );
}

/*
===============
idAI::Event_SetJointMod
===============
*/
void idAI::Event_SetJointMod( int allow ) {
	allowJointMod = ( allow != 0 );
}

/*
================
idAI::Event_ThrowMoveable
================
*/
void idAI::Event_ThrowMoveable( void ) {
	idEntity *ent;
	idEntity *moveable = NULL;

	for ( ent = GetNextTeamEntity(); ent != NULL; ent = ent->GetNextTeamEntity() ) {
		if ( ent->GetBindMaster() == this && ent->IsType( idMoveable::Type ) ) {
			moveable = ent;
			break;
		}
	}
	if ( moveable ) {
		moveable->Unbind();
		moveable->PostEventMS( &EV_SetOwner, 200, NULL );
	}
}

/*
================
idAI::Event_ThrowAF
================
*/
void idAI::Event_ThrowAF( void ) {
	idEntity *ent;
	idEntity *af = NULL;

	for ( ent = GetNextTeamEntity(); ent != NULL; ent = ent->GetNextTeamEntity() ) {
		if ( ent->GetBindMaster() == this && ent->IsType( idAFEntity_Base::Type ) ) {
			af = ent;
			break;
		}
	}
	if ( af ) {
		af->Unbind();
		af->PostEventMS( &EV_SetOwner, 200, NULL );
	}
}

/*
================
idAI::Event_SetAngles
================
*/
void idAI::Event_SetAngles( idAngles const &ang ) {
	current_yaw = ang.yaw;
	viewAxis = idAngles( 0, current_yaw, 0 ).ToMat3();
}

/*
================
idAI::Event_GetAngles
================
*/
void idAI::Event_GetAngles( void ) {
	idThread::ReturnVector( idVec3( 0.0f, current_yaw, 0.0f ) );
}

/*
================
idAI::Event_RealKill
================
*/
void idAI::Event_RealKill( void ) {
	health = 0;

	if ( af.IsLoaded() ) {
		// clear impacts
		af.Rest();

		// physics is turned off by calling af.Rest()
		BecomeActive( TH_PHYSICS );
	}

	Killed( this, this, 0, vec3_zero, INVALID_JOINT );
}

/*
================
idAI::Event_Kill
================
*/
void idAI::Event_Kill( void ) {
	PostEventMS( &AI_RealKill, 0 );
}

/*
================
idAI::Event_WakeOnFlashlight
================
*/
void idAI::Event_WakeOnFlashlight( int enable ) {
	wakeOnFlashlight = ( enable != 0 );
}

/*
================
idAI::Event_LocateEnemy
================
*/
void idAI::Event_LocateEnemy( void ) {
	idActor *enemyEnt;
	int areaNum;
	
	enemyEnt = enemy.GetEntity();
	if ( !enemyEnt ) {
		return;
	}

	enemyEnt->GetAASLocation( aas, lastReachableEnemyPos, areaNum );

	// SZ: Why is this in here if we are unsure of where the enemy is. We have to update it first
	// Update was already after SetEnemyPosition so I'm just commenting out SetEnemyPosition (which
	// is called form in UpdateEnemyPosition if we can see them)
	//SetEnemyPosition();
	UpdateEnemyPosition();
}

/*
================
idAI::Event_KickObstacles
================
*/
void idAI::Event_KickObstacles( idEntity *kickEnt, float force ) {
	idVec3 dir;
	idEntity *obEnt;
	
	if ( kickEnt ) {
		obEnt = kickEnt;
	} else {
		obEnt = move.obstacle.GetEntity();
	}

	if ( obEnt ) {
		dir = obEnt->GetPhysics()->GetOrigin() - physicsObj.GetOrigin();
		dir.Normalize();
	} else {
		dir = viewAxis[ 0 ];
	}
	KickObstacles( dir, force, obEnt );
}

/*
================
idAI::Event_GetObstacle
================
*/
void idAI::Event_GetObstacle( void ) {
	idThread::ReturnEntity( move.obstacle.GetEntity() );
}

/*
================
idAI::Event_PushPointIntoAAS
================
*/
void idAI::Event_PushPointIntoAAS( const idVec3 &pos ) {
	int		areaNum;
	idVec3	newPos;

	areaNum = PointReachableAreaNum( pos );
	if ( areaNum ) {
		newPos = pos;
		aas->PushPointIntoAreaNum( areaNum, newPos );
		idThread::ReturnVector( newPos );
	} else {
		idThread::ReturnVector( pos );
	}
}


/*
================
idAI::Event_GetTurnRate
================
*/
void idAI::Event_GetTurnRate( void ) {
	idThread::ReturnFloat( turnRate );
}

/*
================
idAI::Event_SetTurnRate
================
*/
void idAI::Event_SetTurnRate( float rate ) {
	turnRate = rate;
}

/*
================
idAI::Event_AnimTurn
================
*/
void idAI::Event_AnimTurn( float angles ) {
	turnVel = 0.0f;
	anim_turn_angles = angles;
	if ( angles ) {
		anim_turn_yaw = current_yaw;
		anim_turn_amount = idMath::Fabs( idMath::AngleNormalize180( current_yaw - ideal_yaw ) );
		if ( anim_turn_amount > anim_turn_angles ) {
			anim_turn_amount = anim_turn_angles;
		}
	} else {
		anim_turn_amount = 0.0f;
		animator.CurrentAnim( ANIMCHANNEL_LEGS )->SetSyncedAnimWeight( 0, 1.0f );
		animator.CurrentAnim( ANIMCHANNEL_LEGS )->SetSyncedAnimWeight( 1, 0.0f );
		animator.CurrentAnim( ANIMCHANNEL_TORSO )->SetSyncedAnimWeight( 0, 1.0f );
		animator.CurrentAnim( ANIMCHANNEL_TORSO )->SetSyncedAnimWeight( 1, 0.0f );
	}
}

/*
================
idAI::Event_AllowHiddenMovement
================
*/
void idAI::Event_AllowHiddenMovement( int enable ) {
	allowHiddenMovement = ( enable != 0 );
}

/*
================
idAI::Event_TriggerParticles
================
*/
void idAI::Event_TriggerParticles( const char *jointName ) {
	TriggerParticles( jointName );
}

/*
=====================
idAI::Event_FindActorsInBounds
=====================
*/
void idAI::Event_FindActorsInBounds( const idVec3 &mins, const idVec3 &maxs ) {
	idEntity *	ent;
	idEntity *	entityList[ MAX_GENTITIES ];
	int			numListedEntities;
	int			i;

	numListedEntities = gameLocal.clip.EntitiesTouchingBounds( idBounds( mins, maxs ), CONTENTS_BODY, entityList, MAX_GENTITIES );
	for( i = 0; i < numListedEntities; i++ ) {
		ent = entityList[ i ];
		if ( ent != this && !ent->IsHidden() && ( ent->health > 0 ) && ent->IsType( idActor::Type ) ) {
			idThread::ReturnEntity( ent );
			return;
		}
	}

	idThread::ReturnEntity( NULL );
}

/*
================
idAI::Event_CanReachPosition
================
*/
void idAI::Event_CanReachPosition( const idVec3 &pos ) {
	aasPath_t	path;
	int			toAreaNum;
	int			areaNum;

	toAreaNum = PointReachableAreaNum( pos );
	areaNum	= PointReachableAreaNum( physicsObj.GetOrigin() );
	if ( !toAreaNum || !PathToGoal( path, areaNum, physicsObj.GetOrigin(), toAreaNum, pos ) ) {
		idThread::ReturnInt( false );
	} else {
		idThread::ReturnInt( true );
	}
}

/*
================
idAI::Event_CanReachEntity
================
*/
void idAI::Event_CanReachEntity( idEntity *ent ) {
	aasPath_t	path;
	int			toAreaNum;
	int			areaNum;
	idVec3		pos;

	if ( !ent ) {
		idThread::ReturnInt( false );
		return;
	}

	if ( move.moveType != MOVETYPE_FLY ) {
		if ( !ent->GetFloorPos( 64.0f, pos ) ) {
			idThread::ReturnInt( false );
			return;
		}
		if ( ent->IsType( idActor::Type ) && static_cast<idActor *>( ent )->OnLadder() ) {
			idThread::ReturnInt( false );
			return;
		}
	} else {
		pos = ent->GetPhysics()->GetOrigin();
	}

	toAreaNum = PointReachableAreaNum( pos );
	if ( !toAreaNum ) {
		idThread::ReturnInt( false );
		return;
	}

	const idVec3 &org = physicsObj.GetOrigin();
	areaNum	= PointReachableAreaNum( org );
	if ( !toAreaNum || !PathToGoal( path, areaNum, org, toAreaNum, pos ) ) {
		idThread::ReturnInt( false );
	} else {
		idThread::ReturnInt( true );
	}
}

/*
================
idAI::Event_CanReachEnemy
================
*/
void idAI::Event_CanReachEnemy( void ) {
	aasPath_t	path;
	int			toAreaNum;
	int			areaNum;
	idVec3		pos;
	idActor		*enemyEnt;

	enemyEnt = enemy.GetEntity();
	if ( !enemyEnt ) {
		idThread::ReturnInt( false );
		return;
	}

	if ( move.moveType != MOVETYPE_FLY ) {
		if ( enemyEnt->OnLadder() ) {
			idThread::ReturnInt( false );
			return;
		}
		enemyEnt->GetAASLocation( aas, pos, toAreaNum );
	}  else {
		pos = enemyEnt->GetPhysics()->GetOrigin();
		toAreaNum = PointReachableAreaNum( pos );
	}

	if ( !toAreaNum ) {
		idThread::ReturnInt( false );
		return;
	}

	const idVec3 &org = physicsObj.GetOrigin();
	areaNum	= PointReachableAreaNum( org );
	if ( !PathToGoal( path, areaNum, org, toAreaNum, pos ) ) {
		idThread::ReturnInt( false );
	} else {
		idThread::ReturnInt( true );
	}
}

/*
================
idAI::Event_GetReachableEntityPosition
================
*/
void idAI::Event_GetReachableEntityPosition( idEntity *ent ) {
	int		toAreaNum;

	idVec3	pos;



	if ( move.moveType != MOVETYPE_FLY ) {

		if ( !ent->GetFloorPos( 64.0f, pos ) ) {

			// NOTE: not a good way to return 'false'

			idThread::ReturnVector( vec3_zero );

			return;

		}

		if ( ent->IsType( idActor::Type ) && static_cast<idActor *>( ent )->OnLadder() ) {

			// NOTE: not a good way to return 'false'

			idThread::ReturnVector( vec3_zero );

			return;

		}

	} else {

		pos = ent->GetPhysics()->GetOrigin();

	}



	if ( aas ) {

		toAreaNum = PointReachableAreaNum( pos );

		aas->PushPointIntoAreaNum( toAreaNum, pos );

	}



	idThread::ReturnVector( pos );

}

void idAI::Event_RegisterKilledTask(const char* taskName, int priority)
{
	m_killedTask = taskName;
	m_killedTaskPriority = priority;
}

void idAI::Event_RegisterKnockedOutTask(const char* taskName, int priority)
{
	m_knockedOutTask = taskName;
	m_knockedOutTaskPriority = priority;
}

/**
* DarkMod: Begin Team Relationship Events.  See the definitions on CRelations
* for descriptions of the Relations functions that are called.
**/

void idAI::Event_GetRelationEnt( idEntity *ent )
{
	idActor *actor;

	if ( !ent->IsType( idActor::Type ) ) 
	{
		// inanimate objects are neutral to everyone
		idThread::ReturnInt( 0 );
	}

	actor = static_cast<idActor *>( ent );
	idThread::ReturnInt( gameLocal.m_RelationsManager->GetRelNum( team, actor->team ) );
}

void idAI::Event_IsEnemy( idEntity *ent )
{
	if (!ent)
	{
		/* The NULL pointer is not your enemy! As long as you remember to check for it to avoid crashes. */
		idThread::ReturnInt(0);
	}
	else if (ent->IsType (idAbsenceMarkerEntity::Type))
	{
		idAbsenceMarkerEntity* marker;
		marker = static_cast<idAbsenceMarkerEntity*>( ent );
		idThread::ReturnInt( gameLocal.m_RelationsManager->IsEnemy( team, marker->ownerTeam ) );
	}
	else
	{
		idThread::ReturnInt( (int) IsEnemy( ent ) );
	}
}

void idAI::Event_IsFriend( idEntity *ent )
{

	if (ent->IsType (idAbsenceMarkerEntity::Type))
	{
		idAbsenceMarkerEntity* marker;
		marker = static_cast<idAbsenceMarkerEntity*>( ent );
		idThread::ReturnInt( gameLocal.m_RelationsManager->IsFriend( team, marker->ownerTeam ) );
	}
	else if ( ent->IsType( idActor::Type ) ) 
	{
		idActor *actor;
		actor = static_cast<idActor *>( ent );
		idThread::ReturnInt( gameLocal.m_RelationsManager->IsFriend( team, actor->team ) );
	}
	else
	{
		idThread::ReturnInt( 0 );
	}
}

void idAI::Event_IsNeutral( idEntity *ent )
{
	idActor *actor;


	if (ent->IsType (idAbsenceMarkerEntity::Type))
	{
		idAbsenceMarkerEntity* marker;
		marker = static_cast<idAbsenceMarkerEntity*>( ent );
		idThread::ReturnInt( gameLocal.m_RelationsManager->IsNeutral( team, marker->ownerTeam ) );
	}
	else if ( !ent->IsType( idActor::Type ) ) 
	{
		// inanimate objects are neutral to everyone
		idThread::ReturnInt( 1 );
	}

	actor = static_cast<idActor *>( ent );
	idThread::ReturnInt( gameLocal.m_RelationsManager->IsNeutral( team, actor->team ) );
}

void idAI::Event_GetAcuity( const char *type )
{
	idThread::ReturnFloat( GetAcuity( type ) );
}

void idAI::Event_SetAcuity( const char *type, float val )
{
	SetAcuity( type, val );
}

void idAI::Event_GetAudThresh( void )
{
	idThread::ReturnFloat( m_AudThreshold );
}

void idAI::Event_SetAudThresh( float val )
{
	m_AudThreshold = val;
}

void idAI::Event_SetAlertLevel( float newAlertLevel)
{
	bool bool_alertRising = false;
	if (newAlertLevel > AI_AlertNum) bool_alertRising = true;
	AI_AlertNum = newAlertLevel;
	
	// grace period vars
	float grace_time;
	float grace_frac;
	int grace_count;

	// If alert level is less than 3, sheathe weapon (if appropriate), otherwise draw it
	if (newAlertLevel < thresh_3) SheathWeapon();
	else DrawWeapon();

	// How long should this alert level last?
	if (newAlertLevel >= thresh_3)
	{
		AI_currentAlertLevelDuration = atime3;
		grace_time = m_gracetime_3;
		grace_frac = m_gracefrac_3;
		grace_count = m_gracecount_3;
	}
	else if (newAlertLevel >= thresh_2)
	{
		AI_currentAlertLevelDuration = atime2;
		grace_time = m_gracetime_2;
		grace_frac = m_gracefrac_2;
		grace_count = m_gracecount_2;
	}
	else if (newAlertLevel >= thresh_1)
	{
		AI_currentAlertLevelDuration = atime1;
		grace_time = m_gracetime_1;
		grace_frac = m_gracefrac_1;
		grace_count = m_gracecount_1;
	}
	else
	{
		AI_currentAlertLevelDuration = 0.0;
		grace_time = 0.0;
		grace_frac = 0.0;
		grace_count = 0;
	}
	
	// Add random variance to alert level duration
	AI_currentAlertLevelDuration = AI_currentAlertLevelDuration*(1.0 + gameLocal.random.RandomFloat()*0.50);
	
	//DEBUG_PRINT ("Alert level duration set to " + AI_currentAlertLevelDuration + " due to alert factor " + AI_AlertNum);
	
	// Set time of start of new alert level
	AI_currentAlertLevelStartTime = gameLocal.realClientTime;

	// Begin the grace period
	Event_SetAlertGracePeriod( grace_frac, grace_time, grace_count );

	// Only bark if we haven't barked too recently
	if (( gameLocal.realClientTime - AI_timeOfLastStimulusBark) > MINIMUM_SECONDS_BETWEEN_STIMULUS_BARKS)
	{
		// Note: Alert rising sounds are played based on the type of stimulus before we ever reach this function/
		// We only have to do alert-down sounds here
		if (!bool_alertRising)
		{
			if (newAlertLevel > thresh_3)
			{
				AI_timeOfLastStimulusBark = gameLocal.realClientTime;
				// TODO: Shouldn't hard-code the animation name, talk1, here (and below)
				Event_PlayAndLipSync( "snd_alert3s", "talk1" );
			}
			else if (newAlertLevel > thresh_2)
			{
			
				AI_timeOfLastStimulusBark = gameLocal.realClientTime;
				Event_PlayAndLipSync( "snd_alertdown2", "talk1" );
			}	
			else if (newAlertLevel > thresh_1) 
			{
				AI_timeOfLastStimulusBark = gameLocal.realClientTime;
				Event_PlayAndLipSync( "snd_alertdown1", "talk1" );
			}
		}
	}
	
	// If less than alert 1, all new stimuli should be considered new
	if (newAlertLevel < thresh_1)
	{
		//DEBUG_PRINT ("Clearing last searched alert position");
		AI_lastAlertPosSearched = idVec3(0,0,0);
	}
	
	// If somewhat alerted, have double chance of looking around while idle
	if (newAlertLevel > 0)
	{
		//DEBUG_PRINT ("Chance of looking around while idle is higher due to agitation");
		AI_chancePerSecond_RandomLookAroundWhileIdle = IDLE_RANDOM_HEAD_TURN_CHANCE_PER_SECOND * SLIGHTLY_AGITATED_HEAD_TURN_CHANCE_MULTIPLIER;
	}
}

void idAI::Event_Alert( const char *type, float amount )
{
	AlertAI( type, amount );
}

void idAI::Event_GetSndDir( void )
{
	idThread::ReturnVector( m_SoundDir );
}

void idAI::Event_GetVisDir( void )
{
	idThread::ReturnVector( m_LastSight );
}

void idAI::Event_GetTactEnt( void )
{
	idEntity *ent = GetTactEnt();

	if(!ent)
		idThread::ReturnEntity( NULL );		
	else
	idThread::ReturnEntity( ent );
}


void idAI::Event_VisScan( void )
{
	idActor *actor;
	float time;
	
	// assume we are checking over one frame
	time = 1.0f/60.0f;

	actor = VisualScan( time );
	
	if (!actor)
		idThread::ReturnEntity( NULL );
	else
		idThread::ReturnEntity( actor );
}

void idAI::Event_ClosestReachableEnemy( void ) 
{
	Event_ClosestReachableEnemyOfEntity( static_cast<idEntity *>(this) );
}

//-----------------------------------------------------------------------------------------------------

void idAI::destroyCurrentHidingSpotSearch()
{

	// Check to see if there is one
	if (m_HidingSpotSearchHandle != NULL_HIDING_SPOT_SEARCH_HANDLE)
	{
		// Dereference current search
		HidingSpotSearchCollection.dereference (m_HidingSpotSearchHandle);
		m_HidingSpotSearchHandle = NULL_HIDING_SPOT_SEARCH_HANDLE;

		DM_LOG(LC_AI, LT_DEBUG).LogString ("Hiding spot search dereferenced\n");
	}

	// No hiding spots
	m_hidingSpots.clear();

	
}

//-----------------------------------------------------------------------------------------------------

// TODO: Parameterize these as darkmod globals
#define HIDING_OBJECT_HEIGHT 0.35f
#define MAX_SPOTS_PER_SEARCH_CALL 100

void idAI::Event_StartSearchForHidingSpots
(
	const idVec3& hideFromLocation,
	const idVec3& minBounds, 
	const idVec3& maxBounds, 
	int hidingSpotTypesAllowed, 
	idEntity* p_ignoreEntity
)
{
	DM_LOG(LC_AI, LT_DEBUG).LogString ("Event_StartSearchForHidingSpots called.\n");

	// Destroy any current search
	destroyCurrentHidingSpotSearch();

	// Make caller's search bounds
	idBounds searchBounds (minBounds, maxBounds);
	idBounds searchExclusionBounds;
	searchExclusionBounds.Clear(); // no exclusion bounds

	// SZ: Must use same AAS that LAS used during setup

	// Get aas
	if (aas != NULL)
	{
		// Allocate object that handles the search
		DM_LOG(LC_AI, LT_DEBUG).LogString ("Making finder\n");
		bool b_searchCompleted = false;
		m_HidingSpotSearchHandle = HidingSpotSearchCollection.getOrCreateSearch
		(
			hideFromLocation, 
			aas, 
			HIDING_OBJECT_HEIGHT,
			searchBounds,
			searchExclusionBounds,
			hidingSpotTypesAllowed,
			p_ignoreEntity,
			gameLocal.framenum,
			b_searchCompleted
		);

		// Wait at least one frame for other AIs to indicate they want to share
		// this search. Return result indicating search is not done yet.
		idThread::ReturnInt(1);

	}
	else
	{
		DM_LOG(LC_AI, LT_ERROR).LogString ("Cannot perform Event_StartSearchForHidingSpots if no AAS is set for the AI\n");
	
		// Search is done since there is no search
		idThread::ReturnInt(0);
	}


}

//-----------------------------------------------------------------------------------------------------

void idAI::Event_StartSearchForHidingSpotsWithExclusionArea
(
	const idVec3& hideFromLocation,
	const idVec3& minBounds, 
	const idVec3& maxBounds, 
	const idVec3& exclusionMinBounds, 
	const idVec3& exclusionMaxBounds, 
	int hidingSpotTypesAllowed, 
	idEntity* p_ignoreEntity
)
{
	DM_LOG(LC_AI, LT_DEBUG).LogString ("Event_StartSearchForHidingSpots called.\n");

	// Destroy any current search
	destroyCurrentHidingSpotSearch();

	// Make caller's search bounds
	idBounds searchBounds (minBounds, maxBounds);
	idBounds searchExclusionBounds (exclusionMinBounds, exclusionMaxBounds);

	// Get aas
	if (aas != NULL)
	{
		// Allocate object that handles the search
		DM_LOG(LC_AI, LT_DEBUG).LogString ("Making finder\n");
		bool b_searchCompleted = false;
		m_HidingSpotSearchHandle = HidingSpotSearchCollection.getOrCreateSearch
		(
			hideFromLocation, 
			aas, 
			HIDING_OBJECT_HEIGHT,
			searchBounds,
			searchExclusionBounds,
			hidingSpotTypesAllowed,
			p_ignoreEntity,
			gameLocal.framenum,
			b_searchCompleted
		);

		// Wait at least one frame for other AIs to indicate they want to share
		// this search. Return result indicating search is not done yet.
		idThread::ReturnInt(1);

	}
	else
	{
		DM_LOG(LC_AI, LT_ERROR).LogString ("Cannot perform Event_StartSearchForHidingSpotsWithExclusionArea if no AAS is set for the AI\n");
	
		// Search is done since there is no search
		idThread::ReturnInt(0);
	}


}


//-----------------------------------------------------------------------------------------------------

void idAI::Event_ContinueSearchForHidingSpots()
{
	DM_LOG(LC_AI, LT_DEBUG).LogString ("Event_ContinueSearchForHidingSpots called.\n");

	// Get hiding spot search instance from handle
	darkModAASFindHidingSpots* p_hidingSpotFinder = NULL;
	if (m_HidingSpotSearchHandle != NULL_HIDING_SPOT_SEARCH_HANDLE)
	{
		p_hidingSpotFinder = HidingSpotSearchCollection.getSearchByHandle
		(
			m_HidingSpotSearchHandle
		);
	}

	// Make sure search still around
	if (p_hidingSpotFinder == NULL)
	{
		// No hiding spot search to continue
		DM_LOG(LC_AI, LT_DEBUG).LogString ("No current hiding spot search to continue\n");
		idThread::ReturnInt(0);
	}
	else
	{
		// Call finder method to continue search
		bool b_moreProcessingToDo = p_hidingSpotFinder->continueSearchForHidingSpots
		(
			p_hidingSpotFinder->hidingSpotList,
			g_Global.m_maxNumHidingSpotPointTestsPerAIFrame,
			gameLocal.framenum
		);


		// Return result
		if (b_moreProcessingToDo)
		{
			idThread::ReturnInt(1);
		}
		else
		{
			unsigned int refCount;

			// Get finder we just referenced
			darkModAASFindHidingSpots* p_hidingSpotFinder = 
				HidingSpotSearchCollection.getSearchAndReferenceCountByHandle 
				(
					m_HidingSpotSearchHandle,
					refCount
				);

			m_hidingSpots.clear();
			p_hidingSpotFinder->hidingSpotList.getOneNth
			(
				refCount,
				&m_hidingSpots
			);

			// Done with search object, dereference so other AIs know how many
			// AIs will still be retrieving points from the search
			HidingSpotSearchCollection.dereference (m_HidingSpotSearchHandle);
			m_HidingSpotSearchHandle = NULL_HIDING_SPOT_SEARCH_HANDLE;


			// DEBUGGING
			if (cv_ai_search_show.GetInteger() >= 1.0)
			{
				// Clear the debug draw list and then fill with our results
				p_hidingSpotFinder->debugClearHidingSpotDrawList();
				p_hidingSpotFinder->debugAppendHidingSpotsToDraw (m_hidingSpots);
				p_hidingSpotFinder->debugDrawHidingSpots (cv_ai_search_show.GetInteger());
			}

			DM_LOG(LC_AI, LT_DEBUG).LogString ("Hiding spot search completed\n");
			idThread::ReturnInt(0);
		}
	}
}


//-----------------------------------------------------------------------------------------------------

void idAI::Event_CloseHidingSpotSearch ()
{
       
	// Destroy current hiding spot search
	DM_LOG(LC_AI, LT_DEBUG).LogString ("Closing hiding spot search\n");
	destroyCurrentHidingSpotSearch();
}

//-----------------------------------------------------------------------------------------------------

void idAI::Event_ResortHidingSpots
(
	const idVec3& searchCenter,
	const idVec3& searchRadius
)
{
	DM_LOG(LC_AI, LT_DEBUG).LogString ("Resorting hiding spots for new search center\n");
	m_hidingSpots.sortForNewCenter
	(
		searchCenter,
		searchRadius.Length()
	);

}


//-----------------------------------------------------------------------------------------------------

void idAI::Event_GetNumHidingSpots ()
{


	// Get the number of hiding spots
	int numSpots = m_hidingSpots.getNumSpots();

	// Return count
	idThread::ReturnInt (numSpots);
}

/*------------------------------------------------------------------------------*/

void idAI::Event_GetNthHidingSpotLocation (int hidingSpotIndex)
{
	idVec3 outLocation (0.0f, 0.0f, 0.0f);

	int numSpots = m_hidingSpots.getNumSpots();

	// In bounds?
	if ((hidingSpotIndex >= 0) && (hidingSpotIndex < numSpots))
	{
		idBounds areaNodeBounds;
		darkModHidingSpot_t* p_spot = m_hidingSpots.getNthSpotWithAreaNodeBounds(hidingSpotIndex, areaNodeBounds);
		if (p_spot == NULL)
		{
			outLocation.x = 0;
			outLocation.y = 0;
			outLocation.z = 0;
		}
		else
		{
			outLocation = p_spot->goal.origin;
		}

		if (cv_ai_search_show.GetInteger() >= 1.0)
		{
			idVec4 markerColor (1.0, 1.0, 1.0, 1.0);
			idVec3 arrowLength (0.0, 0.0, 50.0);

			// Debug draw the point to be searched
			gameRenderWorld->DebugArrow
			(
				markerColor,
				outLocation + arrowLength,
				outLocation,
				2,
				cv_ai_search_show.GetInteger()
			);

			// Debug draw the bounds of the area node containing the hiding spot point
			// This may be smaller than the containing AAS area due to octant subdivision.
			gameRenderWorld->DebugBounds
			(
				markerColor,
				areaNodeBounds,
				vec3_origin,
				cv_ai_search_show.GetInteger()
			);


		}

    }
	else
	{
		DM_LOG(LC_AI, LT_ERROR).LogString ("Index %d is out of bounds, there are %d hiding spots\n", hidingSpotIndex, numSpots);
	}


	// Return the location
	idThread::ReturnVector (outLocation);

}

/*------------------------------------------------------------------------------*/

void idAI::Event_GetNthHidingSpotType (int hidingSpotIndex)
{
	int outTypeFlags = 0;

	int numSpots = m_hidingSpots.getNumSpots();

	// In bounds?
	if ((hidingSpotIndex >= 0) && (hidingSpotIndex < numSpots))
	{
		darkModHidingSpot_t* p_spot = m_hidingSpots.getNthSpot(hidingSpotIndex);
		if (p_spot == NULL)
		{
			outTypeFlags = 0;
		}
		else
		{
			outTypeFlags = p_spot->hidingSpotTypes;
		}
	}
	else
	{
		DM_LOG(LC_AI, LT_ERROR).LogString ("Index %d is out of bounds, there are %d hiding spots\n", hidingSpotIndex, numSpots);
	}

	// Return the type
	idThread::ReturnInt (outTypeFlags);

}

//-----------------------------------------------------------------------------------------

void idAI::Event_GetVariableFromOtherAI (idEntity* p_otherEntity, const char* pstr_variableName)
{
	// Test parameters
	if (p_otherEntity == NULL) 
	{
		idThread::ReturnFloat (0.0);
		return;
	}


	// The other entity must be an AI
	idAI* p_otherAI = dynamic_cast<idAI*>(p_otherEntity);
	if (p_otherAI == NULL)
	{
		// Not an AI
		idThread::ReturnFloat (0.0);
		return;
	}

	// Get the variable from the script link map and convert to float
	float value;
	if (!strcmp (pstr_variableName, "stateOfMind_b_enemiesHaveBeenSeen") )
	{
		value = (float) (p_otherAI->stateOfMind_b_enemiesHaveBeenSeen);
	}
	else if (!strcmp (pstr_variableName, "stateOfMind_b_itemsHaveBeenStolen") )
	{
		value = (float) (p_otherAI->stateOfMind_b_itemsHaveBeenStolen);
	}
	else if (!strcmp (pstr_variableName, "stateOfMind_count_evidenceOfIntruders") )
	{
		value = p_otherAI->stateOfMind_count_evidenceOfIntruders;
	}
	else if (!strcmp (pstr_variableName, "AI_AlertNum") )
	{
		value = p_otherAI->AI_AlertNum;
	}
	else
	{
		DM_LOG(LC_AI, LT_ERROR).LogString ("Unexpected AI variable name '%s' requested, value 0.0 returned\n", pstr_variableName);
		value = 0.0;
	}

	// TODO: Add others as needed


	// Return the value
	idThread::ReturnFloat (value);

}

//-----------------------------------------------------------------------------------------

void idAI::Event_GetAlertNumOfOtherAI (idEntity* p_otherEntity)
{
	// Test parameters
	if (p_otherEntity == NULL) 
	{
		idThread::ReturnFloat (0.0);
		return;
	}


	// The other entity must be an AI
	idAI* p_otherAI = dynamic_cast<idAI*>(p_otherEntity);
	if (p_otherAI == NULL)
	{
		// Not an AI
		idThread::ReturnFloat (0.0);
		return;
	}

	// Return the other AI's alert num
	idThread::ReturnFloat (p_otherAI->AI_AlertNum);

}

/*---------------------------------------------------------------------------------*/

void idAI::Event_GetSomeOfOtherEntitiesHidingSpotList (idEntity* p_ownerOfSearch)
{
	// Test parameters
	if (p_ownerOfSearch == NULL) 
	{
		idThread::ReturnInt (0);
		return;
	}


	// The other entity must be an AI
	idAI* p_otherAI = dynamic_cast<idAI*>(p_ownerOfSearch);
	if (p_otherAI == NULL)
	{
		// Not an AI
		idThread::ReturnInt (0);
		return;
	}


	CDarkmodHidingSpotTree* p_othersTree = &(p_otherAI->m_hidingSpots);
	if (p_othersTree->getNumSpots() <= 1)
	{
		// No points
		idThread::ReturnInt (0);
		return;
	}

	// We must clear our current hiding spot search
	destroyCurrentHidingSpotSearch();

	// Move points from their tree to ours
	p_othersTree->getOneNth
	(
		2,
		&m_hidingSpots
	);

	// Done
	idThread::ReturnInt (m_hidingSpots.getNumSpots());

}

//--------------------------------------------------------------------------------

void idAI::Event_GetAlertActor( void )
{
	idThread::ReturnEntity( m_AlertedByActor.GetEntity() );
}

//--------------------------------------------------------------------------------

void idAI::Event_SetAlertGracePeriod( float frac, float duration, int count )
{
	// set the parameters
	m_AlertGraceActor = m_AlertedByActor.GetEntity();
	m_AlertGraceStart = gameLocal.time;
	m_AlertGraceTime = SEC2MS( duration );
	m_AlertGraceThresh = m_AlertNumThisFrame * frac;
	m_AlertGraceCountLimit = count;
	m_AlertGraceCount = 0;
}
