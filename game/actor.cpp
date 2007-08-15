/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/


// Copyright (C) 2004 Id Software, Inc.
//

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include "game_local.h"
#include "../DarkMod/DarkModGlobals.h"
#include "../DarkMod/PlayerData.h"
// #include "logmgr.h"
/***********************************************************************

	idAnimState

***********************************************************************/

/*
=====================
idAnimState::idAnimState
=====================
*/
idAnimState::idAnimState() {
	self			= NULL;
	animator		= NULL;
	thread			= NULL;
	idleAnim		= true;
	disabled		= true;
	channel			= ANIMCHANNEL_ALL;
	animBlendFrames = 0;
	lastAnimBlendFrames = 0;
}

/*
=====================
idAnimState::~idAnimState
=====================
*/
idAnimState::~idAnimState() {
	delete thread;
}

/*
=====================
idAnimState::Save
=====================
*/
void idAnimState::Save( idSaveGame *savefile ) const {

	savefile->WriteObject( self );

	// Save the entity owner of the animator
	savefile->WriteObject( animator->GetEntity() );

	savefile->WriteObject( thread );

	savefile->WriteString( state );

	savefile->WriteInt( animBlendFrames );
	savefile->WriteInt( lastAnimBlendFrames );
	savefile->WriteInt( channel );
	savefile->WriteBool( idleAnim );
	savefile->WriteBool( disabled );
}

/*
=====================
idAnimState::Restore
=====================
*/
void idAnimState::Restore( idRestoreGame *savefile ) {
	savefile->ReadObject( reinterpret_cast<idClass *&>( self ) );

	idEntity *animowner;
	savefile->ReadObject( reinterpret_cast<idClass *&>( animowner ) );
	if ( animowner ) {
		animator = animowner->GetAnimator();
	}

	savefile->ReadObject( reinterpret_cast<idClass *&>( thread ) );

	savefile->ReadString( state );

	savefile->ReadInt( animBlendFrames );
	savefile->ReadInt( lastAnimBlendFrames );
	savefile->ReadInt( channel );
	savefile->ReadBool( idleAnim );
	savefile->ReadBool( disabled );
}

/*
=====================
idAnimState::Init
=====================
*/
void idAnimState::Init( idActor *owner, idAnimator *_animator, int animchannel ) {
	assert( owner );
	assert( _animator );
	self = owner;
	animator = _animator;
	channel = animchannel;

	if ( !thread ) {
		thread = new idThread();
		thread->ManualDelete();
	}
	thread->EndThread();
	thread->ManualControl();
}

/*
=====================
idAnimState::Shutdown
=====================
*/
void idAnimState::Shutdown( void ) {
	delete thread;
	thread = NULL;
}

/*
=====================
idAnimState::SetState
=====================
*/
void idAnimState::SetState( const char *statename, int blendFrames ) {
	const function_t *func;

	func = self->scriptObject.GetFunction( statename );
	if ( !func ) {
		assert( 0 );
		gameLocal.Error( "Can't find function '%s' in object '%s'", statename, self->scriptObject.GetTypeName() );
	}

	state = statename;
	disabled = false;
	animBlendFrames = blendFrames;
	lastAnimBlendFrames = blendFrames;
	thread->CallFunction( self, func, true );

	animBlendFrames = blendFrames;
	lastAnimBlendFrames = blendFrames;
	disabled = false;
	idleAnim = false;

	if ( ai_debugScript.GetInteger() == self->entityNumber ) {
		gameLocal.Printf( "%d: %s: Animstate: %s\n", gameLocal.time, self->name.c_str(), state.c_str() );
	}
}

/*
=====================
idAnimState::SetFrame
=====================
*/

void idAnimState::SetFrame( int anim, int frame )
{
	animator->SetFrame( channel, anim, frame, gameLocal.time, 0 );
}

/*
=====================
idAnimState::StopAnim
=====================
*/
void idAnimState::StopAnim( int frames ) {
	animBlendFrames = 0;
	animator->Clear( channel, gameLocal.time, FRAME2MS( frames ) );
}

/*
=====================
idAnimState::PlayAnim
=====================
*/
void idAnimState::PlayAnim( int anim ) {
	if ( anim ) {
		animator->PlayAnim( channel, anim, gameLocal.time, FRAME2MS( animBlendFrames ) );
	}
	animBlendFrames = 0;
}

/*
=====================
idAnimState::CycleAnim
=====================
*/
void idAnimState::CycleAnim( int anim ) {
	if ( anim ) {
		animator->CycleAnim( channel, anim, gameLocal.time, FRAME2MS( animBlendFrames ) );
	}
	animBlendFrames = 0;
}

/*
=====================
idAnimState::BecomeIdle
=====================
*/
void idAnimState::BecomeIdle( void ) {
	idleAnim = true;
}

/*
=====================
idAnimState::Disabled
=====================
*/
bool idAnimState::Disabled( void ) const {
	return disabled;
}

/*
=====================
idAnimState::AnimDone
=====================
*/
bool idAnimState::AnimDone( int blendFrames ) const {
	int animDoneTime;
	
	animDoneTime = animator->CurrentAnim( channel )->GetEndTime();
	if ( animDoneTime < 0 ) {
		// playing a cycle
		return false;
	} else if ( animDoneTime - FRAME2MS( blendFrames ) <= gameLocal.time ) {
		return true;
	} else {
		return false;
	}
}

/*
=====================
idAnimState::IsIdle
=====================
*/
bool idAnimState::IsIdle( void ) const {
	return disabled || idleAnim;
}

/*
=====================
idAnimState::GetAnimFlags
=====================
*/
animFlags_t idAnimState::GetAnimFlags( void ) const {
	animFlags_t flags;

	memset( &flags, 0, sizeof( flags ) );
	if ( !disabled && !AnimDone( 0 ) ) {
		flags = animator->GetAnimFlags( animator->CurrentAnim( channel )->AnimNum() );
	}

	return flags;
}

/*
=====================
idAnimState::Enable
=====================
*/
void idAnimState::Enable( int blendFrames ) {
	if ( disabled ) {
		disabled = false;
		animBlendFrames = blendFrames;
		lastAnimBlendFrames = blendFrames;
		if ( state.Length() ) {
			SetState( state.c_str(), blendFrames );
		}
	}
}

/*
=====================
idAnimState::Disable
=====================
*/
void idAnimState::Disable( void ) {
	disabled = true;
	idleAnim = false;
}

/*
=====================
idAnimState::UpdateState
=====================
*/
bool idAnimState::UpdateState( void ) {
	if ( disabled ) {
		return false;
	}

	if ( ai_debugScript.GetInteger() == self->entityNumber ) {
		thread->EnableDebugInfo();
	} else {
		thread->DisableDebugInfo();
	}

	thread->Execute();

	return true;
}

/***********************************************************************

	idActor

***********************************************************************/

const idEventDef AI_EnableEyeFocus( "enableEyeFocus" );
const idEventDef AI_DisableEyeFocus( "disableEyeFocus" );
const idEventDef EV_Footstep( "footstep" );
const idEventDef EV_FootstepLeft( "leftFoot" );
const idEventDef EV_FootstepRight( "rightFoot" );
const idEventDef EV_EnableWalkIK( "EnableWalkIK" );
const idEventDef EV_DisableWalkIK( "DisableWalkIK" );
const idEventDef EV_EnableLegIK( "EnableLegIK", "d" );
const idEventDef EV_DisableLegIK( "DisableLegIK", "d" );
const idEventDef AI_StopAnim( "stopAnim", "dd" );
const idEventDef AI_PlayAnim( "playAnim", "ds", 'd' );
const idEventDef AI_PlayCycle( "playCycle", "ds", 'd' );
const idEventDef AI_IdleAnim( "idleAnim", "ds", 'd' );
const idEventDef AI_SetSyncedAnimWeight( "setSyncedAnimWeight", "ddf" );
const idEventDef AI_SetBlendFrames( "setBlendFrames", "dd" );
const idEventDef AI_GetBlendFrames( "getBlendFrames", "d", 'd' );
const idEventDef AI_AnimState( "animState", "dsd" );
const idEventDef AI_GetAnimState( "getAnimState", "d", 's' );
const idEventDef AI_InAnimState( "inAnimState", "ds", 'd' );
const idEventDef AI_FinishAction( "finishAction", "s" );
const idEventDef AI_AnimDone( "animDone", "dd", 'd' );
const idEventDef AI_OverrideAnim( "overrideAnim", "d" );
const idEventDef AI_EnableAnim( "enableAnim", "dd" );
const idEventDef AI_PreventPain( "preventPain", "f" );
const idEventDef AI_DisablePain( "disablePain" );
const idEventDef AI_EnablePain( "enablePain" );
const idEventDef AI_GetPainAnim( "getPainAnim", NULL, 's' );
const idEventDef AI_SetAnimPrefix( "setAnimPrefix", "s" );
const idEventDef AI_HasAnim( "hasAnim", "ds", 'f' );
const idEventDef AI_CheckAnim( "checkAnim", "ds" );
const idEventDef AI_ChooseAnim( "chooseAnim", "ds", 's' );
const idEventDef AI_AnimLength( "animLength", "ds", 'f' );
const idEventDef AI_AnimDistance( "animDistance", "ds", 'f' );
const idEventDef AI_HasEnemies( "hasEnemies", NULL, 'd' );
const idEventDef AI_NextEnemy( "nextEnemy", "E", 'e' );
const idEventDef AI_ClosestEnemyToPoint( "closestEnemyToPoint", "v", 'e' );
const idEventDef AI_SetNextState( "setNextState", "s" );
const idEventDef AI_SetState( "setState", "s" );
const idEventDef AI_GetState( "getState", NULL, 's' );
const idEventDef AI_GetHead( "getHead", NULL, 'e' );
const idEventDef AI_GetEyePos( "getEyePos", NULL, 'v' );

// Attachment Related Events:
const idEventDef AI_Attach( "attach", "e" );
const idEventDef AI_ReAttach( "reAttach", "dsvv" );
const idEventDef AI_DropAttachment( "dropAttachment", "d" );
const idEventDef AI_ShowAttachment( "showAttachment", "dd" );
const idEventDef AI_GetAttachment( "getAttachment", "d", 'e' );
const idEventDef AI_GetNumAttachments( "getNumAttachments", NULL, 'd' );

// Task queue events
const idEventDef AI_AttachTaskQueue( "attachTaskQueue", "d" );
const idEventDef AI_DetachTaskQueue( "detachTaskQueue" );

CLASS_DECLARATION( idAFEntity_Gibbable, idActor )
	EVENT( AI_EnableEyeFocus,			idActor::Event_EnableEyeFocus )
	EVENT( AI_DisableEyeFocus,			idActor::Event_DisableEyeFocus )
	EVENT( EV_Footstep,					idActor::Event_Footstep )
	EVENT( EV_FootstepLeft,				idActor::Event_Footstep )
	EVENT( EV_FootstepRight,			idActor::Event_Footstep )
	EVENT( EV_EnableWalkIK,				idActor::Event_EnableWalkIK )
	EVENT( EV_DisableWalkIK,			idActor::Event_DisableWalkIK )
	EVENT( EV_EnableLegIK,				idActor::Event_EnableLegIK )
	EVENT( EV_DisableLegIK,				idActor::Event_DisableLegIK )
	EVENT( AI_PreventPain,				idActor::Event_PreventPain )
	EVENT( AI_DisablePain,				idActor::Event_DisablePain )
	EVENT( AI_EnablePain,				idActor::Event_EnablePain )
	EVENT( AI_GetPainAnim,				idActor::Event_GetPainAnim )
	EVENT( AI_SetAnimPrefix,			idActor::Event_SetAnimPrefix )
	EVENT( AI_StopAnim,					idActor::Event_StopAnim )
	EVENT( AI_PlayAnim,					idActor::Event_PlayAnim )
	EVENT( AI_PlayCycle,				idActor::Event_PlayCycle )
	EVENT( AI_IdleAnim,					idActor::Event_IdleAnim )
	EVENT( AI_SetSyncedAnimWeight,		idActor::Event_SetSyncedAnimWeight )
	EVENT( AI_SetBlendFrames,			idActor::Event_SetBlendFrames )
	EVENT( AI_GetBlendFrames,			idActor::Event_GetBlendFrames )
	EVENT( AI_AnimState,				idActor::Event_AnimState )
	EVENT( AI_GetAnimState,				idActor::Event_GetAnimState )
	EVENT( AI_InAnimState,				idActor::Event_InAnimState )
	EVENT( AI_FinishAction,				idActor::Event_FinishAction )
	EVENT( AI_AnimDone,					idActor::Event_AnimDone )
	EVENT( AI_OverrideAnim,				idActor::Event_OverrideAnim )
	EVENT( AI_EnableAnim,				idActor::Event_EnableAnim )
	EVENT( AI_HasAnim,					idActor::Event_HasAnim )
	EVENT( AI_CheckAnim,				idActor::Event_CheckAnim )
	EVENT( AI_ChooseAnim,				idActor::Event_ChooseAnim )
	EVENT( AI_AnimLength,				idActor::Event_AnimLength )
	EVENT( AI_AnimDistance,				idActor::Event_AnimDistance )
	EVENT( AI_HasEnemies,				idActor::Event_HasEnemies )
	EVENT( AI_NextEnemy,				idActor::Event_NextEnemy )
	EVENT( AI_ClosestEnemyToPoint,		idActor::Event_ClosestEnemyToPoint )
	EVENT( EV_StopSound,				idActor::Event_StopSound )
	EVENT( AI_SetNextState,				idActor::Event_SetNextState )
	EVENT( AI_SetState,					idActor::Event_SetState )
	EVENT( AI_GetState,					idActor::Event_GetState )
	EVENT( AI_GetHead,					idActor::Event_GetHead )
	EVENT( AI_GetEyePos,				idActor::Event_GetEyePos )
	
	EVENT ( AI_Attach,					idActor::Attach )
	EVENT ( AI_ReAttach,				idActor::ReAttach )
	EVENT ( AI_DropAttachment,			idActor::DropAttachment )
	EVENT ( AI_ShowAttachment,			idActor::ShowAttachment )
	EVENT ( AI_GetAttachment,			idActor::Event_GetAttachment )
	EVENT ( AI_GetNumAttachments,		idActor::Event_GetNumAttachments )

	EVENT ( AI_AttachTaskQueue,			idActor::Event_AttachTaskQueue )
	EVENT ( AI_DetachTaskQueue,			idActor::Event_DetachTaskQueue )
END_CLASS

/*
=====================
idActor::idActor
=====================
*/
idActor::idActor( void ) {
	viewAxis.Identity();

	scriptThread		= NULL;		// initialized by ConstructScriptObject, which is called by idEntity::Spawn

	use_combat_bbox		= false;
	head				= NULL;

	team				= 0;
	m_AItype			= 0;
	m_Innocent			= false;
	rank				= 0;
	fovDot				= 0.0f;
	eyeOffset.Zero();
	pain_debounce_time	= 0;
	pain_delay			= 0;
	pain_threshold		= 0;

	state				= NULL;
	idealState			= NULL;

	task				= "";
	taskPriority        = 0;
	m_TaskQueue			= NULL;
	m_TaskQueueID		= -1;

	leftEyeJoint		= INVALID_JOINT;
	rightEyeJoint		= INVALID_JOINT;
	soundJoint			= INVALID_JOINT;

	modelOffset.Zero();
	deltaViewAngles.Zero();

	painTime			= 0;
	allowPain			= false;
	allowEyeFocus		= false;

	waitState			= "";
	
	blink_anim			= NULL;
	blink_time			= 0;
	blink_min			= 0;
	blink_max			= 0;

	finalBoss			= false;

	m_attachments.SetGranularity( 1 );

	enemyNode.SetOwner( this );
	enemyList.SetOwner( this );
}

/*
=====================
idActor::~idActor
=====================
*/
idActor::~idActor( void ) {
	int i;
	idEntity *ent;

	DeconstructScriptObject();
	scriptObject.Free();

	m_TaskQueue = NULL;

	StopSound( SND_CHANNEL_ANY, false );

	delete combatModel;
	combatModel = NULL;

	if ( head.GetEntity() ) {
		head.GetEntity()->ClearBody();
		head.GetEntity()->PostEventMS( &EV_Remove, 0 );
	}

	// remove any attached entities
	for( i = 0; i < m_attachments.Num(); i++ ) {
		ent = m_attachments[ i ].ent.GetEntity();
		if ( ent ) {
			ent->PostEventMS( &EV_SafeRemove, 0 );
		}
	}

	ShutdownThreads();
}

/*
=====================
idActor::Spawn
=====================
*/
void idActor::Spawn( void ) 
{
	idStr			jointName;
	float			fovDegrees;
	copyJoints_t	copyJoint;
	const idKeyValue *kv = NULL;

	animPrefix	= "";
	state		= NULL;
	idealState	= NULL;

	spawnArgs.GetFloat( "collision_delta_fatal", "86", m_delta_fatal ); // falling 3.5 stories 
	spawnArgs.GetFloat( "collision_delta_scale",  "1.0", m_delta_scale );
	spawnArgs.GetFloat( "collision_delta_min",    "31", m_delta_min ); // falling ~12 ft, g = 1066
	spawnArgs.GetInt( "rank", "0", rank );
	spawnArgs.GetInt( "team", "0", team );
	spawnArgs.GetInt( "type", "0", m_AItype );
	spawnArgs.GetBool( "innocent", "0", m_Innocent );
	spawnArgs.GetVector("offsetModel", "0 0 0", modelOffset);
	spawnArgs.GetVector("offsetHeadModel", "0 0 0", mHeadModelOffset);

	spawnArgs.GetBool( "use_combat_bbox", "0", use_combat_bbox );	

	viewAxis = GetPhysics()->GetAxis();

	spawnArgs.GetFloat( "fov", "90", fovDegrees );
	SetFOV( fovDegrees );

	pain_debounce_time	= 0;

	pain_delay		= SEC2MS( spawnArgs.GetFloat( "pain_delay" ) );
	pain_threshold	= spawnArgs.GetInt( "pain_threshold" );

	LoadAF();

	walkIK.Init( this, IK_ANIM, modelOffset );

	// the animation used to be set to the IK_ANIM at this point, but that was fixed, resulting in
	// attachments not binding correctly, so we're stuck setting the IK_ANIM before attaching things.
	animator.ClearAllAnims( gameLocal.time, 0 );
	animator.SetFrame( ANIMCHANNEL_ALL, animator.GetAnim( IK_ANIM ), 0, 0, 0 );

	SetupDamageGroups();
	SetupHead();

	// clear the bind anim
	animator.ClearAllAnims( gameLocal.time, 0 );

	idEntity *headEnt = head.GetEntity();
	idAnimator *headAnimator;
	if ( headEnt ) {
		headAnimator = headEnt->GetAnimator();
	} else {
		headAnimator = &animator;
	}

	if ( headEnt ) {
		// set up the list of joints to copy to the head
		for( kv = spawnArgs.MatchPrefix( "copy_joint", NULL ); kv != NULL; kv = spawnArgs.MatchPrefix( "copy_joint", kv ) ) {
			if ( kv->GetValue() == "" ) {
				// probably clearing out inherited key, so skip it
				continue;
			}

			jointName = kv->GetKey();
			if ( jointName.StripLeadingOnce( "copy_joint_world " ) ) {
				copyJoint.mod = JOINTMOD_WORLD_OVERRIDE;
			} else {
				jointName.StripLeadingOnce( "copy_joint " );
				copyJoint.mod = JOINTMOD_LOCAL_OVERRIDE;
			}

			copyJoint.from = animator.GetJointHandle( jointName );
			if ( copyJoint.from == INVALID_JOINT ) {
				gameLocal.Warning( "Unknown copy_joint '%s' on entity %s", jointName.c_str(), name.c_str() );
				continue;
			}

			jointName = kv->GetValue();
			copyJoint.to = headAnimator->GetJointHandle( jointName );
			if ( copyJoint.to == INVALID_JOINT ) {
				gameLocal.Warning( "Unknown copy_joint '%s' on head of entity %s", jointName.c_str(), name.c_str() );
				continue;
			}

			copyJoints.Append( copyJoint );
		}
	}

	// set up blinking
	blink_anim = headAnimator->GetAnim( "blink" );
	blink_time = 0;	// it's ok to blink right away
	blink_min = SEC2MS( spawnArgs.GetFloat( "blink_min", "0.5" ) );
	blink_max = SEC2MS( spawnArgs.GetFloat( "blink_max", "8" ) );

	// set up the head anim if necessary
	int headAnim = headAnimator->GetAnim( "def_head" );
	if ( headAnim ) {
		if ( headEnt ) {
            headAnimator->CycleAnim( ANIMCHANNEL_ALL, headAnim, gameLocal.time, 0 );
		} else {
			headAnimator->CycleAnim( ANIMCHANNEL_HEAD, headAnim, gameLocal.time, 0 );
		}
	}

	if ( spawnArgs.GetString( "sound_bone", "", jointName ) ) {
		soundJoint = animator.GetJointHandle( jointName );
		if ( soundJoint == INVALID_JOINT ) {
			gameLocal.Warning( "idAnimated '%s' at (%s): cannot find joint '%s' for sound playback", name.c_str(), GetPhysics()->GetOrigin().ToString(0), jointName.c_str() );
		}
	}

	finalBoss = spawnArgs.GetBool( "finalBoss" );

	FinishSetup();
}

/*
================
idActor::FinishSetup
================
*/
void idActor::FinishSetup( void ) {
	const char	*scriptObjectName;

	// setup script object
	if ( spawnArgs.GetString( "scriptobject", NULL, &scriptObjectName ) ) {
		if ( !scriptObject.SetType( scriptObjectName ) ) {
			gameLocal.Error( "Script object '%s' not found on entity '%s'.", scriptObjectName, name.c_str() );
		}

		ConstructScriptObject();
	}

	SetupBody();
}

/*
================
idActor::SetupHead
================
*/
void idActor::SetupHead( void ) {
	idAFAttachment		*headEnt;
	idStr				jointName;
	const char			*headModel;
	jointHandle_t		joint;
	jointHandle_t		damageJoint;
	int					i;
	const idKeyValue	*sndKV;
	idStr				oHeadOffsetName;
	idVec3				oHeadOffset;

	if(gameLocal.isClient)
		return;

	headModel = spawnArgs.GetString( "def_head", "" );
	if(headModel[0])
	{
		// We look if the head model is defined as a key to have a specific offset.
		// If that is not the case, then we use the defaul value, if it exists, 
		// otherwise there is no offset at all.
		if(spawnArgs.GetVector(headModel, "0 0 0", oHeadOffset) == true)
			mHeadModelOffset = oHeadOffset;

		jointName = spawnArgs.GetString( "head_joint" );
		joint = animator.GetJointHandle( jointName );
		if ( joint == INVALID_JOINT ) {
			gameLocal.Error( "Joint '%s' not found for 'head_joint' on '%s'", jointName.c_str(), name.c_str() );
		}

		// set the damage joint to be part of the head damage group (if possible)
		damageJoint = joint;

		for( i = 0; i < damageGroups.Num(); i++ ) {
			if ( damageGroups[ i ] == "head" ) {
				damageJoint = static_cast<jointHandle_t>( i );
				break;
			}
		}

		// copy any sounds in case we have frame commands on the head
		idDict	args;
		sndKV = spawnArgs.MatchPrefix( "snd_", NULL );
		while( sndKV ) {
			args.Set( sndKV->GetKey(), sndKV->GetValue() );
			sndKV = spawnArgs.MatchPrefix( "snd_", sndKV );
		}

		headEnt = static_cast<idAFAttachment *>( gameLocal.SpawnEntityType( idAFAttachment::Type, &args ) );
		headEnt->SetName( va( "%s_head", name.c_str() ) );
		headEnt->SetBody( this, headModel, damageJoint );
		headEnt->SetCombatModel();
		head = headEnt;

		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("SETBODY: Actor %s : damage joint %d for attached head is part of damage group %s\r", name.c_str(), (int) damageJoint, GetDamageGroup( damageJoint ) );

		idVec3		origin;
		idMat3		axis;
		idAttachInfo &attach = m_attachments.Alloc();
		attach.channel = animator.GetChannelForJoint( joint );
		animator.GetJointTransform( joint, gameLocal.time, origin, axis );
		origin = renderEntity.origin + ( origin + modelOffset + mHeadModelOffset ) * renderEntity.axis;
		attach.ent = headEnt;
		headEnt->SetOrigin( origin );
		headEnt->SetAxis( renderEntity.axis );
		headEnt->BindToJoint( this, joint, true );
	}
}

/*
================
idActor::CopyJointsFromBodyToHead
================
*/
void idActor::CopyJointsFromBodyToHead( void ) {
	idEntity	*headEnt = head.GetEntity();
	idAnimator	*headAnimator;
	int			i;
	idMat3		mat;
	idMat3		axis;
	idVec3		pos;

	if ( !headEnt ) {
		return;
	}

	headAnimator = headEnt->GetAnimator();

	// copy the animation from the body to the head
	for( i = 0; i < copyJoints.Num(); i++ ) {
		if ( copyJoints[ i ].mod == JOINTMOD_WORLD_OVERRIDE ) {
			mat = headEnt->GetPhysics()->GetAxis().Transpose();
			GetJointWorldTransform( copyJoints[ i ].from, gameLocal.time, pos, axis );
			pos -= headEnt->GetPhysics()->GetOrigin();
			headAnimator->SetJointPos( copyJoints[ i ].to, copyJoints[ i ].mod, pos * mat );
			headAnimator->SetJointAxis( copyJoints[ i ].to, copyJoints[ i ].mod, axis * mat );
		} else {
			animator.GetJointLocalTransform( copyJoints[ i ].from, gameLocal.time, pos, axis );
			headAnimator->SetJointPos( copyJoints[ i ].to, copyJoints[ i ].mod, pos );
			headAnimator->SetJointAxis( copyJoints[ i ].to, copyJoints[ i ].mod, axis );
		}
	}
}

/*
================
idActor::Restart
================
*/
void idActor::Restart( void ) {
	assert( !head.GetEntity() );
	SetupHead();
	FinishSetup();
}

/*
================
idActor::Save

archive object for savegame file
================
*/
void idActor::Save( idSaveGame *savefile ) const {
	idActor *ent;
	int i;

	savefile->WriteInt( team );
	savefile->WriteInt( rank );
	savefile->WriteInt( m_AItype );
	savefile->WriteBool( m_Innocent );
	savefile->WriteMat3( viewAxis );

	savefile->WriteInt( enemyList.Num() );
	for ( ent = enemyList.Next(); ent != NULL; ent = ent->enemyNode.Next() ) {
		savefile->WriteObject( ent );
	}

	savefile->WriteFloat( fovDot );
	savefile->WriteVec3( eyeOffset );
	savefile->WriteVec3( modelOffset );
	savefile->WriteAngles( deltaViewAngles );

	savefile->WriteInt( pain_debounce_time );
	savefile->WriteInt( pain_delay );
	savefile->WriteInt( pain_threshold );

	savefile->WriteInt( damageGroups.Num() );
	for( i = 0; i < damageGroups.Num(); i++ ) {
		savefile->WriteString( damageGroups[ i ] );
	}

	savefile->WriteInt( damageScale.Num() );
	for( i = 0; i < damageScale.Num(); i++ ) {
		savefile->WriteFloat( damageScale[ i ] );
	}

	savefile->WriteBool( use_combat_bbox );
	head.Save( savefile );

	savefile->WriteInt( copyJoints.Num() );
	for( i = 0; i < copyJoints.Num(); i++ ) {
		savefile->WriteInt( copyJoints[i].mod );
		savefile->WriteJoint( copyJoints[i].from );
		savefile->WriteJoint( copyJoints[i].to );
	}

	savefile->WriteJoint( leftEyeJoint );
	savefile->WriteJoint( rightEyeJoint );
	savefile->WriteJoint( soundJoint );

	walkIK.Save( savefile );

	savefile->WriteString( animPrefix );
	savefile->WriteString( painAnim );

	savefile->WriteInt( blink_anim );
	savefile->WriteInt( blink_time );
	savefile->WriteInt( blink_min );
	savefile->WriteInt( blink_max );

	// script variables
	savefile->WriteObject( scriptThread );

	savefile->WriteString( waitState );

	headAnim.Save( savefile );
	torsoAnim.Save( savefile );
	legsAnim.Save( savefile );

	savefile->WriteBool( allowPain );
	savefile->WriteBool( allowEyeFocus );

	savefile->WriteInt( painTime );

	savefile->WriteInt( m_attachments.Num() );
	for ( i = 0; i < m_attachments.Num(); i++ ) {
		m_attachments[i].ent.Save( savefile );
		savefile->WriteInt( m_attachments[i].channel );
	}

	savefile->WriteBool( finalBoss );
	savefile->WriteFloat( m_delta_fatal );
	savefile->WriteFloat( m_delta_scale );
	savefile->WriteFloat( m_delta_min );

	idToken token;

	//FIXME: this is unneccesary
	if ( state ) {
		idLexer src( state->Name(), idStr::Length( state->Name() ), "idAI::Save" );

		src.ReadTokenOnLine( &token );
		src.ExpectTokenString( "::" );
		src.ReadTokenOnLine( &token );

		savefile->WriteString( token );
	} else {
		savefile->WriteString( "" );
	}

	if ( idealState ) {
		idLexer src( idealState->Name(), idStr::Length( idealState->Name() ), "idAI::Save" );

		src.ReadTokenOnLine( &token );
		src.ExpectTokenString( "::" );
		src.ReadTokenOnLine( &token );

		savefile->WriteString( token );
	} else {
		savefile->WriteString( "" );
	}
	
	// Save task info
	savefile->WriteString(task.c_str());
	savefile->WriteInt(m_TaskQueueID);

	savefile->WriteFloat(m_stepvol_walk);
	savefile->WriteFloat(m_stepvol_run);
	savefile->WriteFloat(m_stepvol_creep);

	savefile->WriteFloat(m_stepvol_crouch_walk);
	savefile->WriteFloat(m_stepvol_crouch_creep);
	savefile->WriteFloat(m_stepvol_crouch_run);

	savefile->WriteDict(&m_replacementAnims);
}

/*
================
idActor::Restore

unarchives object from save game file
================
*/
void idActor::Restore( idRestoreGame *savefile ) {
	int i, num;
	idActor *ent;

	savefile->ReadInt( team );
	savefile->ReadInt( rank );
	savefile->ReadInt( m_AItype );
	savefile->ReadBool( m_Innocent );
	savefile->ReadMat3( viewAxis );

	savefile->ReadInt( num );
	for ( i = 0; i < num; i++ ) {
		savefile->ReadObject( reinterpret_cast<idClass *&>( ent ) );
		assert( ent );
		if ( ent ) {
			ent->enemyNode.AddToEnd( enemyList );
		}
	}

	savefile->ReadFloat( fovDot );
	savefile->ReadVec3( eyeOffset );
	savefile->ReadVec3( modelOffset );
	savefile->ReadAngles( deltaViewAngles );

	savefile->ReadInt( pain_debounce_time );
	savefile->ReadInt( pain_delay );
	savefile->ReadInt( pain_threshold );

	savefile->ReadInt( num );
	damageGroups.SetGranularity( 1 );
	damageGroups.SetNum( num );
	for( i = 0; i < num; i++ ) {
		savefile->ReadString( damageGroups[ i ] );
	}

	savefile->ReadInt( num );
	damageScale.SetNum( num );
	for( i = 0; i < num; i++ ) {
		savefile->ReadFloat( damageScale[ i ] );
	}

	savefile->ReadBool( use_combat_bbox );
	head.Restore( savefile );

	savefile->ReadInt( num );
	copyJoints.SetNum( num );
	for( i = 0; i < num; i++ ) {
		int val;
		savefile->ReadInt( val );
		copyJoints[i].mod = static_cast<jointModTransform_t>( val );
		savefile->ReadJoint( copyJoints[i].from );
		savefile->ReadJoint( copyJoints[i].to );
	}

	savefile->ReadJoint( leftEyeJoint );
	savefile->ReadJoint( rightEyeJoint );
	savefile->ReadJoint( soundJoint );

	walkIK.Restore( savefile );

	savefile->ReadString( animPrefix );
	savefile->ReadString( painAnim );

	savefile->ReadInt( blink_anim );
	savefile->ReadInt( blink_time );
	savefile->ReadInt( blink_min );
	savefile->ReadInt( blink_max );

	savefile->ReadObject( reinterpret_cast<idClass *&>( scriptThread ) );

	savefile->ReadString( waitState );

	headAnim.Restore( savefile );
	torsoAnim.Restore( savefile );
	legsAnim.Restore( savefile );

	savefile->ReadBool( allowPain );
	savefile->ReadBool( allowEyeFocus );

	savefile->ReadInt( painTime );

	savefile->ReadInt( num );
	for ( i = 0; i < num; i++ ) {
		idAttachInfo &attach = m_attachments.Alloc();
		attach.ent.Restore( savefile );
		savefile->ReadInt( attach.channel );
	}

	savefile->ReadBool( finalBoss );
	savefile->ReadFloat( m_delta_fatal );
	savefile->ReadFloat( m_delta_scale );
	savefile->ReadFloat( m_delta_min );

	idStr statename;

	savefile->ReadString( statename );
	if ( statename.Length() > 0 ) {
		state = GetScriptFunction( statename );
	}

	savefile->ReadString( statename );
	if ( statename.Length() > 0 ) {
		idealState = GetScriptFunction( statename );
	}
	
	// Restore task info
	savefile->ReadString(task);
	savefile->ReadInt(m_TaskQueueID);
	if (m_TaskQueueID != -1)
	{
		m_TaskQueue = gameLocal.GetPriorityQueue(m_TaskQueueID);
	}

	savefile->ReadFloat(m_stepvol_walk);
	savefile->ReadFloat(m_stepvol_run);
	savefile->ReadFloat(m_stepvol_creep);

	savefile->ReadFloat(m_stepvol_crouch_walk);
	savefile->ReadFloat(m_stepvol_crouch_creep);
	savefile->ReadFloat(m_stepvol_crouch_run);

	savefile->ReadDict(&m_replacementAnims);
}

/*
================
idActor::Hide
================
*/
void idActor::Hide( void ) {
	idEntity *ent;
	idEntity *next;

	idAFEntity_Base::Hide();
	if ( head.GetEntity() ) {
		head.GetEntity()->Hide();
	}

	for( ent = GetNextTeamEntity(); ent != NULL; ent = next ) {
		next = ent->GetNextTeamEntity();
		if ( ent->GetBindMaster() == this ) {
			ent->Hide();
			if ( ent->IsType( idLight::Type ) ) {
				static_cast<idLight *>( ent )->Off();
			}
		}
	}
	UnlinkCombat();
}

/*
================
idActor::Show
================
*/
void idActor::Show( void ) {
	idEntity *ent;
	idEntity *next;

	idAFEntity_Base::Show();
	if ( head.GetEntity() ) {
		head.GetEntity()->Show();
	}
	for( ent = GetNextTeamEntity(); ent != NULL; ent = next ) {
		next = ent->GetNextTeamEntity();
		if ( ent->GetBindMaster() == this ) {
			ent->Show();
			if ( ent->IsType( idLight::Type ) ) {
				static_cast<idLight *>( ent )->On();
			}
		}
	}
	LinkCombat();
}

/*
==============
idActor::GetDefaultSurfaceType
==============
*/
int	idActor::GetDefaultSurfaceType( void ) const {
	return SURFTYPE_FLESH;
}

/*
================
idActor::ProjectOverlay
================
*/
void idActor::ProjectOverlay( const idVec3 &origin, const idVec3 &dir, float size, const char *material ) {
	idEntity *ent;
	idEntity *next;

	idEntity::ProjectOverlay( origin, dir, size, material );

	for( ent = GetNextTeamEntity(); ent != NULL; ent = next ) {
		next = ent->GetNextTeamEntity();
		if ( ent->GetBindMaster() == this ) {
			if ( ent->fl.takedamage && ent->spawnArgs.GetBool( "bleed" ) ) {
				ent->ProjectOverlay( origin, dir, size, material );
			}
		}
	}
}

/*
================
idActor::LoadAF
================
*/
bool idActor::LoadAF( void ) 
{
	bool bReturnVal = false;
	idStr fileName;

	if ( !spawnArgs.GetString( "ragdoll", "*unknown*", fileName ) || !fileName.Length() ) 
	{
		goto Quit;
	}
	af.SetAnimator( GetAnimator() );
	bReturnVal =  af.Load( this, fileName );
	SetUpGroundingVars();

Quit:
	return bReturnVal;
}

/*
=====================
idActor::SetupBody
=====================
*/
void idActor::SetupBody( void ) {
	const char *jointname;

	animator.ClearAllAnims( gameLocal.time, 0 );
	animator.ClearAllJoints();

	idEntity *headEnt = head.GetEntity();
	if ( headEnt ) {
		jointname = spawnArgs.GetString( "bone_leftEye" );
		leftEyeJoint = headEnt->GetAnimator()->GetJointHandle( jointname );

		jointname = spawnArgs.GetString( "bone_rightEye" );
		rightEyeJoint = headEnt->GetAnimator()->GetJointHandle( jointname );

		// set up the eye height.  check if it's specified in the def.
		if ( !spawnArgs.GetFloat( "eye_height", "0", eyeOffset.z ) ) {
			// if not in the def, then try to base it off the idle animation
			int anim = headEnt->GetAnimator()->GetAnim( "idle" );
			if ( anim && ( leftEyeJoint != INVALID_JOINT ) ) {
				idVec3 pos;
				idMat3 axis;
				headEnt->GetAnimator()->PlayAnim( ANIMCHANNEL_ALL, anim, gameLocal.time, 0 );
				headEnt->GetAnimator()->GetJointTransform( leftEyeJoint, gameLocal.time, pos, axis );
				headEnt->GetAnimator()->ClearAllAnims( gameLocal.time, 0 );
				headEnt->GetAnimator()->ForceUpdate();
				pos += headEnt->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin();
				eyeOffset = pos + modelOffset;
			} else {
				// just base it off the bounding box size
				eyeOffset.z = GetPhysics()->GetBounds()[ 1 ].z - 6;
			}
		}
		headAnim.Init( this, headEnt->GetAnimator(), ANIMCHANNEL_ALL );
	} else {
		jointname = spawnArgs.GetString( "bone_leftEye" );
		leftEyeJoint = animator.GetJointHandle( jointname );

		jointname = spawnArgs.GetString( "bone_rightEye" );
		rightEyeJoint = animator.GetJointHandle( jointname );

		// set up the eye height.  check if it's specified in the def.
		if ( !spawnArgs.GetFloat( "eye_height", "0", eyeOffset.z ) ) {
			// if not in the def, then try to base it off the idle animation
			int anim = animator.GetAnim( "idle" );
			if ( anim && ( leftEyeJoint != INVALID_JOINT ) ) {
				idVec3 pos;
				idMat3 axis;
				animator.PlayAnim( ANIMCHANNEL_ALL, anim, gameLocal.time, 0 );
				animator.GetJointTransform( leftEyeJoint, gameLocal.time, pos, axis );
				animator.ClearAllAnims( gameLocal.time, 0 );
				animator.ForceUpdate();
				eyeOffset = pos + modelOffset;
			} else {
				// just base it off the bounding box size
				eyeOffset.z = GetPhysics()->GetBounds()[ 1 ].z - 6;
			}
		}
		headAnim.Init( this, &animator, ANIMCHANNEL_HEAD );
	}

	waitState = "";

	torsoAnim.Init( this, &animator, ANIMCHANNEL_TORSO );
	legsAnim.Init( this, &animator, ANIMCHANNEL_LEGS );
}

/*
=====================
idActor::CheckBlink
=====================
*/
void idActor::CheckBlink( void ) {
	// check if it's time to blink
	if ( !blink_anim || ( health <= 0 ) || !allowEyeFocus || ( blink_time > gameLocal.time ) ) {
		return;
	}

	idEntity *headEnt = head.GetEntity();
	if ( headEnt ) {
		headEnt->GetAnimator()->PlayAnim( ANIMCHANNEL_EYELIDS, blink_anim, gameLocal.time, 1 );
	} else {
		animator.PlayAnim( ANIMCHANNEL_EYELIDS, blink_anim, gameLocal.time, 1 );
	}

	// set the next blink time
	blink_time = gameLocal.time + blink_min + gameLocal.random.RandomFloat() * ( blink_max - blink_min );
}

/*
================
idActor::GetPhysicsToVisualTransform
================
*/
bool idActor::GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis ) {
	if ( af.IsActive() ) {
		af.GetPhysicsToVisualTransform( origin, axis );
		return true;
	}
	origin = modelOffset;
	axis = viewAxis;
	return true;
}

/*
================
idActor::GetPhysicsToSoundTransform
================
*/
bool idActor::GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis ) {
	if ( soundJoint != INVALID_JOINT ) {
		animator.GetJointTransform( soundJoint, gameLocal.time, origin, axis );
		origin += modelOffset;
		axis = viewAxis;
	} else {
		origin = GetPhysics()->GetGravityNormal() * -eyeOffset.z;
		axis.Identity();
	}
	return true;
}

/***********************************************************************

	script state management

***********************************************************************/

/*
================
idActor::ShutdownThreads
================
*/
void idActor::ShutdownThreads( void ) {
	headAnim.Shutdown();
	torsoAnim.Shutdown();
	legsAnim.Shutdown();

	if ( scriptThread ) {
		scriptThread->EndThread();
		scriptThread->PostEventMS( &EV_Remove, 0 );
		delete scriptThread;
		scriptThread = NULL;
	}
}

/*
================
idActor::ShouldConstructScriptObjectAtSpawn

Called during idEntity::Spawn to see if it should construct the script object or not.
Overridden by subclasses that need to spawn the script object themselves.
================
*/
bool idActor::ShouldConstructScriptObjectAtSpawn( void ) const {
	return false;
}

/*
================
idActor::ConstructScriptObject

Called during idEntity::Spawn.  Calls the constructor on the script object.
Can be overridden by subclasses when a thread doesn't need to be allocated.
================
*/
idThread *idActor::ConstructScriptObject( void ) {
	const function_t *constructor;

	// make sure we have a scriptObject
	if ( !scriptObject.HasObject() ) {
		gameLocal.Error( "No scriptobject set on '%s'.  Check the '%s' entityDef.", name.c_str(), GetEntityDefName() );
	}

	if ( !scriptThread ) {
		// create script thread
		scriptThread = new idThread();
		scriptThread->ManualDelete();
		scriptThread->ManualControl();
		scriptThread->SetThreadName( name.c_str() );
	} else {
		scriptThread->EndThread();
	}
	
	// call script object's constructor
	constructor = scriptObject.GetConstructor();
	if ( !constructor ) {
		gameLocal.Error( "Missing constructor on '%s' for entity '%s'", scriptObject.GetTypeName(), name.c_str() );
	}

	// init the script object's data
	scriptObject.ClearObject();

	// just set the current function on the script.  we'll execute in the subclasses.
	scriptThread->CallFunction( this, constructor, true );

	return scriptThread;
}

/*
=====================
idActor::GetScriptFunction
=====================
*/
const function_t *idActor::GetScriptFunction( const char *funcname ) {
	const function_t *func;

	func = scriptObject.GetFunction( funcname );
	if ( !func ) {
		scriptThread->Error( "Unknown function '%s' in '%s'", funcname, scriptObject.GetTypeName() );
	}

	return func;
}

/*
=====================
idActor::SetState
=====================
*/
void idActor::SetState( const function_t *newState ) {
	if ( !newState ) {
		gameLocal.Error( "idActor::SetState: Null state" );
	}

	if ( ai_debugScript.GetInteger() == entityNumber ) {
		gameLocal.Printf( "%d: %s: State: %s\n", gameLocal.time, name.c_str(), newState->Name() );
	}

	state = newState;
	idealState = state;
	scriptThread->CallFunction( this, state, true );
}

/*
=====================
idActor::SetState
=====================
*/
void idActor::SetState( const char *statename ) {
	const function_t *newState;

	newState = GetScriptFunction( statename );
	SetState( newState );
}
/*
=====================
idActor::SetTask
=====================
*/
void idActor::SetTask(const idStr& newTask, int newTaskPriority) {
	task = newTask.c_str();
	taskPriority = newTaskPriority;
	if (newTask.Length())
	{
		scriptThread->CallFunction(this, GetScriptFunction( newTask.c_str() ), true);
	}
}

/*
=====================
idActor::UpdateScript
=====================
*/
void idActor::UpdateScript( void ) {
	int	i;

	if ( ai_debugScript.GetInteger() == entityNumber ) {
		scriptThread->EnableDebugInfo();
	} else {
		scriptThread->DisableDebugInfo();
	}

	// a series of state changes can happen in a single frame.
	// this loop limits them in case we've entered an infinite loop.
	for( i = 0; i < 20; i++ ) {
		if ( idealState != state ) {
			SetState( idealState );
		}

		// don't call script until it's done waiting
		if ( scriptThread->IsWaiting() ) {
			break;
		}
        
		scriptThread->Execute();
		if ( idealState == state ) {
			break;
		}
	}

	if ( i == 20 ) {
		scriptThread->Warning( "idActor::UpdateScript: exited loop to prevent lockup" );
	}
	
	// TDM: Task management
	if (m_TaskQueue != NULL)
	{
		for( i = 0; i < 20; i++ ) // Permit multiple task changes per frame, but avoid infinite loops
		{
			//gameLocal.Printf("Task management for queue %d, iteration %d\n", gameLocal.m_PriorityQueues.FindIndex(m_TaskQueue), i);
			//gameLocal.Printf("Queue contents: %s\n", m_TaskQueue->DebuggingInfo().c_str());
			idStr topTask = idStr(m_TaskQueue->Peek());
			int topTaskPriority = m_TaskQueue->PeekPriority();
			if (topTask.Length() && (topTaskPriority > taskPriority || !task.Length()))
			{
				//// There's a more important task to do, so do it
				
				// Do a cleanup task first if we need to
				// Note that a cleanup task should not have its own cleanup task. The resulting behaviour
				// would be undefined, since it'd be dependent on the implementation of the priority queue.
				idStr cleanupTask = task+"_cleanup";
				const function_t *func = scriptObject.GetFunction(cleanupTask.c_str());
				if (func)
				{
					// There's a cleanup task available, so add it as the highest priority task and do it
					topTask = cleanupTask;
					topTaskPriority = PQUEUE_HIGHEST_PRIORITY;
				}
				else
				{
					// There's no cleanup task to take care of first, so go ahead
					// and pop the new task off the queue.
					// (Once we start a task, it should be removed from the queue so that
					// it doesn't get re-entered later.)
					m_TaskQueue->Pop();
				}
				SetTask(topTask, topTaskPriority);
			}
			
			// don't call script until it's done waiting
			if ( scriptThread->IsWaiting() ) break;
	        
			scriptThread->Execute();
			
			// If the function returned, the task is done, so look for another task
			if (scriptThread->IsDying() && task.Length())
			{
				task = ""; // Erase current task so that we get a new one
			}
			else
			{
				// If we're still doing the highest priority task,
				// or there are no more tasks,
				// then we don't need to change tasks any further
				if ( m_TaskQueue->PeekPriority() <= taskPriority || !task.Length() ) break;
			}
		}

		if ( i == 20 )
		{
			scriptThread->Warning( "idActor::UpdateScript: exited task loop to prevent lockup" );
		}
	}
}

/***********************************************************************

	vision

***********************************************************************/

/*
=====================
idActor::setFov
=====================
*/
void idActor::SetFOV( float fov ) {
	fovDot = (float)cos( DEG2RAD( fov * 0.5f ) );
}

/*
=====================
idActor::SetEyeHeight
=====================
*/
void idActor::SetEyeHeight( float height ) {
	eyeOffset.z = height;
}

/*
=====================
idActor::EyeHeight
=====================
*/
float idActor::EyeHeight( void ) const {
	return eyeOffset.z;
}

/*
=====================
idActor::EyeOffset
=====================
*/
idVec3 idActor::EyeOffset( void ) const {
	return GetPhysics()->GetGravityNormal() * -eyeOffset.z;
}

/*
=====================
idActor::GetEyePosition
=====================
*/
idVec3 idActor::GetEyePosition( void ) const {
	return GetPhysics()->GetOrigin() + ( GetPhysics()->GetGravityNormal() * -eyeOffset.z );
}

/*
=====================
idActor::GetViewPos
=====================
*/
void idActor::GetViewPos( idVec3 &origin, idMat3 &axis ) const {
	origin = GetEyePosition();
	axis = viewAxis;
}

/*
=====================
idActor::CheckFOV
=====================
*/
bool idActor::CheckFOV( const idVec3 &pos ) const 
{
	//DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("idActor::CheckFOV was called\r");

	if ( fovDot == 1.0f ) {
		return true;
	}

	float	dot;
	idVec3	delta;
	
	delta = pos - GetEyePosition();

	// get our gravity normal
	const idVec3 &gravityDir = GetPhysics()->GetGravityNormal();

	// infinite vertical vision, so project it onto our orientation plane
	delta -= gravityDir * ( gravityDir * delta );

	delta.Normalize();
	dot = viewAxis[ 0 ] * delta;

	return ( dot >= fovDot );
}

/*
=====================
idActor::CanSee
=====================
*/
bool idActor::CanSee( idEntity *ent, bool useFov ) const {
	trace_t		tr;
	idVec3		eye;
	idVec3		toPos;

	// TDM: We need to be able to see lights that are off and hence hidden
	/*
	if ( ent->IsHidden() ) 
	{
		return false;
	}
	*/

	// greebo: If the target entity is an idActor, use its eyeposition,
	// otherwise just use the origin (for general entities).
	if ( ent->IsType( idActor::Type ) ) {
		toPos = ( ( idActor * )ent )->GetEyePosition();
	} else {
		toPos = ent->GetPhysics()->GetOrigin();
	}

	// Check the field of view if specified
	if ( useFov && !CheckFOV( toPos ) ) {
		return false;
	}

	eye = GetEyePosition();

	// Perform a trace from the eye position to the target entity
	gameLocal.clip.TracePoint( tr, eye, toPos, MASK_OPAQUE, this );
	if ( tr.fraction >= 1.0f || ( gameLocal.GetTraceEntity( tr ) == ent ) ) {
		return true;
	}

	return false;
}

/*
=====================
idActor::PointVisible
=====================
*/
bool idActor::PointVisible( const idVec3 &point ) const {
	trace_t results;
	idVec3 start, end;

	start = GetEyePosition();
	end = point;
	end[2] += 1.0f;

	gameLocal.clip.TracePoint( results, start, end, MASK_OPAQUE, this );
	return ( results.fraction >= 1.0f );
}

/*
=====================
idActor::GetAIAimTargets

Returns positions for the AI to aim at.
=====================
*/
void idActor::GetAIAimTargets( const idVec3 &lastSightPos, idVec3 &headPos, idVec3 &chestPos ) {
	headPos = lastSightPos + EyeOffset();
	chestPos = ( headPos + lastSightPos + GetPhysics()->GetBounds().GetCenter() ) * 0.5f;
}

/*
=====================
idActor::GetRenderView
=====================
*/
renderView_t *idActor::GetRenderView() {
	renderView_t *rv = idEntity::GetRenderView();
	rv->viewaxis = viewAxis;
	rv->vieworg = GetEyePosition();
	return rv;
}

/***********************************************************************

	Model/Ragdoll

***********************************************************************/

/*
================
idActor::SetCombatModel
================
*/
void idActor::SetCombatModel( void ) {
	idAFAttachment *headEnt;

	if ( !use_combat_bbox ) {
		if ( combatModel ) {
			combatModel->Unlink();
			combatModel->LoadModel( modelDefHandle );
		} else {
			combatModel = new idClipModel( modelDefHandle );
		}

		headEnt = head.GetEntity();
		if ( headEnt ) {
			headEnt->SetCombatModel();
		}
	}
}

/*
================
idActor::GetCombatModel
================
*/
idClipModel *idActor::GetCombatModel( void ) const {
	return combatModel;
}

/*
================
idActor::LinkCombat
================
*/
void idActor::LinkCombat( void ) {
	idAFAttachment *headEnt;

	if ( fl.hidden || use_combat_bbox ) {
		return;
	}

	if ( combatModel ) {
		combatModel->Link( gameLocal.clip, this, 0, renderEntity.origin, renderEntity.axis, modelDefHandle );
	}
	headEnt = head.GetEntity();
	if ( headEnt ) {
		headEnt->LinkCombat();
	}
}

/*
================
idActor::UnlinkCombat
================
*/
void idActor::UnlinkCombat( void ) {
	idAFAttachment *headEnt;

	if ( combatModel ) {
		combatModel->Unlink();
	}
	headEnt = head.GetEntity();
	if ( headEnt ) {
		headEnt->UnlinkCombat();
	}
}

/*
================
idActor::StartRagdoll
================
*/
bool idActor::StartRagdoll( void ) {
	float slomoStart, slomoEnd;
	float jointFrictionDent, jointFrictionDentStart, jointFrictionDentEnd;
	float contactFrictionDent, contactFrictionDentStart, contactFrictionDentEnd;

	// if no AF loaded
	if ( !af.IsLoaded() ) {
		return false;
	}

	// if the AF is already active
	if ( af.IsActive() ) {
		return true;
	}

	// disable the monster bounding box
	GetPhysics()->DisableClip();

	// start using the AF
	af.StartFromCurrentPose( spawnArgs.GetInt( "velocityTime", "0" ) );

	slomoStart = MS2SEC( gameLocal.time ) + spawnArgs.GetFloat( "ragdoll_slomoStart", "-1.6" );
	slomoEnd = MS2SEC( gameLocal.time ) + spawnArgs.GetFloat( "ragdoll_slomoEnd", "0.8" );

	// do the first part of the death in slow motion
	af.GetPhysics()->SetTimeScaleRamp( slomoStart, slomoEnd );

	jointFrictionDent = spawnArgs.GetFloat( "ragdoll_jointFrictionDent", "0.1" );
	jointFrictionDentStart = MS2SEC( gameLocal.time ) + spawnArgs.GetFloat( "ragdoll_jointFrictionStart", "0.2" );
	jointFrictionDentEnd = MS2SEC( gameLocal.time ) + spawnArgs.GetFloat( "ragdoll_jointFrictionEnd", "1.2" );

	// set joint friction dent
	af.GetPhysics()->SetJointFrictionDent( jointFrictionDent, jointFrictionDentStart, jointFrictionDentEnd );

	contactFrictionDent = spawnArgs.GetFloat( "ragdoll_contactFrictionDent", "0.1" );
	contactFrictionDentStart = MS2SEC( gameLocal.time ) + spawnArgs.GetFloat( "ragdoll_contactFrictionStart", "1.0" );
	contactFrictionDentEnd = MS2SEC( gameLocal.time ) + spawnArgs.GetFloat( "ragdoll_contactFrictionEnd", "2.0" );

	// set contact friction dent
	af.GetPhysics()->SetContactFrictionDent( contactFrictionDent, contactFrictionDentStart, contactFrictionDentEnd );

	// drop any items the actor is holding
	idMoveableItem::DropItems( this, "death", NULL );

	// drop any articulated figures the actor is holding
	idAFEntity_Base::DropAFs( this, "death", NULL );

	RemoveAttachments();

	return true;
}

/*
================
idActor::StopRagdoll
================
*/
void idActor::StopRagdoll( void ) {
	if ( af.IsActive() ) {
		af.Stop();
	}
}

/*
================
idActor::UpdateAnimationControllers
================
*/
bool idActor::UpdateAnimationControllers( void ) {

	if ( af.IsActive() ) {
		return idAFEntity_Base::UpdateAnimationControllers();
	} else {
		animator.ClearAFPose();
	}

	if ( walkIK.IsInitialized() ) {
		walkIK.Evaluate();
		return true;
	}

	return false;
}

/*
================
idActor::RemoveAttachments
================
*/
void idActor::RemoveAttachments( void ) {
	int i;
	idEntity *ent;

	// remove any attached entities
	for( i = 0; i < m_attachments.Num(); i++ ) {
		ent = m_attachments[ i ].ent.GetEntity();
		if ( ent && ent->spawnArgs.GetBool( "remove" ) ) {
			ent->PostEventMS( &EV_SafeRemove, 0 );
		}
	}
}

/*
================
idActor::Attach
================
*/
void idActor::Attach( idEntity *ent ) 
{
	idVec3			origin;
	idMat3			axis, rotate, newAxis;
	jointHandle_t	joint;
	idStr			jointName;
	idAttachInfo	&attach = m_attachments.Alloc();
	idAngles		angleOffset;
	idVec3			originOffset;
	idStr			nm;
	idStr			ClassName;

	jointName = ent->spawnArgs.GetString( "joint" );
	joint = animator.GetJointHandle( jointName );
	if ( joint == INVALID_JOINT ) {
		gameLocal.Error( "Joint '%s' not found for attaching '%s' on '%s'", jointName.c_str(), ent->GetClassname(), name.c_str() );
	}

	spawnArgs.GetString("classname", "", ClassName);
	sprintf(nm, "angles_%s", ClassName.c_str());
	if(ent->spawnArgs.GetAngles(nm.c_str(), "0 0 0", angleOffset) == false)
		angleOffset = ent->spawnArgs.GetAngles( "angles" );

	sprintf(nm, "origin_%s", ClassName.c_str());
	if(ent->spawnArgs.GetVector(nm.c_str(), "0 0 0", originOffset) == false)
		originOffset = ent->spawnArgs.GetVector( "origin" );

	attach.channel = animator.GetChannelForJoint( joint );
	GetJointWorldTransform( joint, gameLocal.time, origin, axis );
	attach.ent = ent;

	rotate = angleOffset.ToMat3();
	newAxis = rotate * axis;
	ent->SetAxis( newAxis );
	// Use the local joint axis instead of the overall AI axis
	ent->SetOrigin( origin + originOffset * axis );

	ent->BindToJoint( this, joint, true );
	ent->cinematic = cinematic;

	// If the ent we're attaching is an AFEntity, call SetBody to set up damage propagation, physics propagation, etc.
	if( ent->IsType(idAFAttachment::Type) )
	{
		// TODO: Is this line correct?  Won't know until we test.
		idStr modelName = ent->spawnArgs.GetString("model","");
		static_cast<idAFAttachment *>(ent)->SetBody( this, modelName.c_str(), joint );
	}
}

/*
================
idActor::UnbindNotify
================
*/
void idActor::BindNotify( idEntity *ent )
{
	idAFEntity_Base::UnbindNotify( ent );

	// Override our animations based on the bound entity's replace_anim_* spawnargs
	const idKeyValue *KeyVal = ent->spawnArgs.MatchPrefix( "replace_anim_", NULL );
	while ( KeyVal )
	{
		idStr key = KeyVal->GetKey();
		key.StripLeadingOnce("replace_anim_");

		m_replacementAnims.Set( key, KeyVal->GetValue() );

		KeyVal = ent->spawnArgs.MatchPrefix( "replace_anim_", KeyVal );
	}
}

/*
================
idActor::UnbindNotify
================
*/
void idActor::UnbindNotify( idEntity *ent )
{
	// Remove animation overrides
	const idKeyValue *KeyVal = ent->spawnArgs.MatchPrefix( "replace_anim_", NULL );
	while ( KeyVal )
	{
		idStr key = KeyVal->GetKey();
		key.StripLeadingOnce("replace_anim_");

		if (strcmp(m_replacementAnims.GetString( key ), KeyVal->GetValue().c_str()) == 0 )
		{
			// This animation override is present, so remove it
			m_replacementAnims.Delete( key );
		}

		KeyVal = ent->spawnArgs.MatchPrefix( "replace_anim_", KeyVal );
	}
}

/*
================
idActor::Teleport
================
*/
void idActor::Teleport( const idVec3 &origin, const idAngles &angles, idEntity *destination ) {
	GetPhysics()->SetOrigin( origin + idVec3( 0, 0, CM_CLIP_EPSILON ) );
	GetPhysics()->SetLinearVelocity( vec3_origin );

	viewAxis = angles.ToMat3();

	UpdateVisuals();

	if ( !IsHidden() ) {
		// kill anything at the new position
		gameLocal.KillBox( this );
	}
}

/*
================
idActor::GetDeltaViewAngles
================
*/
const idAngles &idActor::GetDeltaViewAngles( void ) const {
	return deltaViewAngles;
}

/*
================
idActor::SetDeltaViewAngles
================
*/
void idActor::SetDeltaViewAngles( const idAngles &delta ) {
	deltaViewAngles = delta;
}

/*
================
idActor::HasEnemies
================
*/
bool idActor::HasEnemies( void ) const {
	idActor *ent;

	for( ent = enemyList.Next(); ent != NULL; ent = ent->enemyNode.Next() ) {
		if ( !ent->fl.hidden ) {
			return true;
		}
	}

	return false;
}

/*
================
idActor::ClosestEnemyToPoint
================
*/
idActor *idActor::ClosestEnemyToPoint( const idVec3 &pos ) {
	idActor		*ent;
	idActor		*bestEnt;
	float		bestDistSquared;
	float		distSquared;
	idVec3		delta;

	bestDistSquared = idMath::INFINITY;
	bestEnt = NULL;
	for( ent = enemyList.Next(); ent != NULL; ent = ent->enemyNode.Next() ) {
		if ( ent->fl.hidden ) {
			continue;
		}
		delta = ent->GetPhysics()->GetOrigin() - pos;
		distSquared = delta.LengthSqr();
		if ( distSquared < bestDistSquared ) {
			bestEnt = ent;
			bestDistSquared = distSquared;
		}
	}

	return bestEnt;
}

/*
================
idActor::EnemyWithMostHealth
================
*/
idActor *idActor::EnemyWithMostHealth() {
	idActor		*ent;
	idActor		*bestEnt;

	int most = -9999;
	bestEnt = NULL;
	for( ent = enemyList.Next(); ent != NULL; ent = ent->enemyNode.Next() ) {
		if ( !ent->fl.hidden && ( ent->health > most ) ) {
			bestEnt = ent;
			most = ent->health;
		}
	}
	return bestEnt;
}

/*
================
idActor::OnLadder
================
*/
bool idActor::OnLadder( void ) const {
	return false;
}

/*
==============
idActor::GetAASLocation
==============
*/
void idActor::GetAASLocation( idAAS *aas, idVec3 &pos, int &areaNum ) const {
	idVec3		size;
	idBounds	bounds;

	GetFloorPos( 64.0f, pos );
	if ( !aas ) {
		areaNum = 0;
		return;
	}
	
	size = aas->GetSettings()->boundingBoxes[0][1];
	bounds[0] = -size;
	size.z = 32.0f;
	bounds[1] = size;

	areaNum = aas->PointReachableAreaNum( pos, bounds, AREA_REACHABLE_WALK );
	if ( areaNum ) {
		aas->PushPointIntoAreaNum( areaNum, pos );
	}
}

/***********************************************************************

	animation state

***********************************************************************/

/*
=====================
idActor::SetAnimState
=====================
*/
void idActor::SetAnimState( int channel, const char *statename, int blendFrames ) {
	const function_t *func;

	func = scriptObject.GetFunction( statename );
	if ( !func ) {
		assert( 0 );
		gameLocal.Error( "Can't find function '%s' in object '%s'", statename, scriptObject.GetTypeName() );
	}

	switch( channel ) {
	case ANIMCHANNEL_HEAD :
		headAnim.SetState( statename, blendFrames );
		allowEyeFocus = true;
		break;
		
	case ANIMCHANNEL_TORSO :
		torsoAnim.SetState( statename, blendFrames );
		legsAnim.Enable( blendFrames );
		allowPain = true;
		allowEyeFocus = true;
		break;

	case ANIMCHANNEL_LEGS :
		legsAnim.SetState( statename, blendFrames );
		torsoAnim.Enable( blendFrames );
		allowPain = true;
		allowEyeFocus = true;
		break;

	default:
		gameLocal.Error( "idActor::SetAnimState: Unknown anim group" );
		break;
	}
}

/*
=====================
idActor::GetAnimState
=====================
*/
const char *idActor::GetAnimState( int channel ) const {
	switch( channel ) {
	case ANIMCHANNEL_HEAD :
		return headAnim.state;
		break;

	case ANIMCHANNEL_TORSO :
		return torsoAnim.state;
		break;

	case ANIMCHANNEL_LEGS :
		return legsAnim.state;
		break;

	default:
		gameLocal.Error( "idActor::GetAnimState: Unknown anim group" );
		return NULL;
		break;
	}
}

/*
=====================
idActor::InAnimState
=====================
*/
bool idActor::InAnimState( int channel, const char *statename ) const {
	switch( channel ) {
	case ANIMCHANNEL_HEAD :
		if ( headAnim.state == statename ) {
			return true;
		}
		break;

	case ANIMCHANNEL_TORSO :
		if ( torsoAnim.state == statename ) {
			return true;
		}
		break;

	case ANIMCHANNEL_LEGS :
		if ( legsAnim.state == statename ) {
			return true;
		}
		break;

	default:
		gameLocal.Error( "idActor::InAnimState: Unknown anim group" );
		break;
	}

	return false;
}

/*
=====================
idActor::WaitState
=====================
*/
const char *idActor::WaitState( void ) const {
	if ( waitState.Length() ) {
		return waitState;
	} else {
		return NULL;
	}
}

/*
=====================
idActor::SetWaitState
=====================
*/
void idActor::SetWaitState( const char *_waitstate ) {
	waitState = _waitstate;
}

/*
=====================
idActor::UpdateAnimState
=====================
*/
void idActor::UpdateAnimState( void ) {
	headAnim.UpdateState();
	torsoAnim.UpdateState();
	legsAnim.UpdateState();
}

/*
=====================
idActor::GetAnim
=====================
*/
int idActor::GetAnim( int channel, const char *animname ) {
	int			anim;
	const char *temp;
	idAnimator *animatorPtr;

	if ( channel == ANIMCHANNEL_HEAD ) {
		if ( !head.GetEntity() ) {
			return 0;
		}
		animatorPtr = head.GetEntity()->GetAnimator();
	} else {
		animatorPtr = &animator;
	}

	if ( animPrefix.Length() ) {
		temp = LookupReplacementAnim( va( "%s_%s", animPrefix.c_str(), animname ) );
		anim = animatorPtr->GetAnim( temp );
		if ( anim ) {
			return anim;
		}
	}

	anim = animatorPtr->GetAnim( LookupReplacementAnim( animname ) );

	return anim;
}

/*
=====================
idActor::LookupReplacementAnim
(TDM)
=====================
*/
const char* idActor::LookupReplacementAnim( const char *animname )
{
	// Recursively lookup the animation to find its replacement animation
	const char* replacement = animname;
	int tries = 0; // Infinite loop prevention counter

	while ( m_replacementAnims.FindKey( replacement ) )
	{
		replacement = m_replacementAnims.GetString( replacement );
		
		// Avoid infinite loops
		tries++;
		if (tries > 500)
		{
			gameLocal.Warning("Infinite loop detected in replacements for animation '%s' applied to actor '%s'\n",
				animname, this->name.c_str());
			break;
		}
	}

	return replacement;
}

/*
===============
idActor::SyncAnimChannels
===============
*/
void idActor::SyncAnimChannels( int channel, int syncToChannel, int blendFrames ) {
	idAnimator		*headAnimator;
	idAFAttachment	*headEnt;
	int				anim;
	idAnimBlend		*syncAnim;
	int				starttime;
	int				blendTime;
	int				cycle;

	blendTime = FRAME2MS( blendFrames );
	if ( channel == ANIMCHANNEL_HEAD ) {
		headEnt = head.GetEntity();
		if ( headEnt ) {
			headAnimator = headEnt->GetAnimator();
			syncAnim = animator.CurrentAnim( syncToChannel );
			if ( syncAnim ) {
				anim = headAnimator->GetAnim( syncAnim->AnimFullName() );
				if ( !anim ) {
					anim = headAnimator->GetAnim( syncAnim->AnimName() );
				}
				if ( anim ) {
					cycle = animator.CurrentAnim( syncToChannel )->GetCycleCount();
					starttime = animator.CurrentAnim( syncToChannel )->GetStartTime();
					headAnimator->PlayAnim( ANIMCHANNEL_ALL, anim, gameLocal.time, blendTime );
					headAnimator->CurrentAnim( ANIMCHANNEL_ALL )->SetCycleCount( cycle );
					headAnimator->CurrentAnim( ANIMCHANNEL_ALL )->SetStartTime( starttime );
				} else {
					headEnt->PlayIdleAnim( blendTime );
				}
			}
		}
	} else if ( syncToChannel == ANIMCHANNEL_HEAD ) {
		headEnt = head.GetEntity();
		if ( headEnt ) {
			headAnimator = headEnt->GetAnimator();
			syncAnim = headAnimator->CurrentAnim( ANIMCHANNEL_ALL );
			if ( syncAnim ) {
				anim = GetAnim( channel, syncAnim->AnimFullName() );
				if ( !anim ) {
					anim = GetAnim( channel, syncAnim->AnimName() );
				}
				if ( anim ) {
					cycle = headAnimator->CurrentAnim( ANIMCHANNEL_ALL )->GetCycleCount();
					starttime = headAnimator->CurrentAnim( ANIMCHANNEL_ALL )->GetStartTime();
					animator.PlayAnim( channel, anim, gameLocal.time, blendTime );
					animator.CurrentAnim( channel )->SetCycleCount( cycle );
					animator.CurrentAnim( channel )->SetStartTime( starttime );
				}
			}
		}
	} else {
		animator.SyncAnimChannels( channel, syncToChannel, gameLocal.time, blendTime );
	}
}

/***********************************************************************

	Damage

***********************************************************************/

/*
============
idActor::Gib
============
*/
void idActor::Gib( const idVec3 &dir, const char *damageDefName ) {
	// no gibbing in multiplayer - by self damage or by moving objects
	if ( gameLocal.isMultiplayer ) {
		return;
	}
	// only gib once
	if ( gibbed ) {
		return;
	}
	idAFEntity_Gibbable::Gib( dir, damageDefName );
	if ( head.GetEntity() ) {
		head.GetEntity()->Hide();
	}
	StopSound( SND_CHANNEL_VOICE, false );
}


/*
============
idActor::Damage

this		entity that is being damaged
inflictor	entity that is causing the damage
attacker	entity that caused the inflictor to damage targ
	example: this=monster, inflictor=rocket, attacker=player

dir			direction of the attack for knockback in global space
point		point at which the damage is being inflicted, used for headshots
damage		amount of damage being inflicted

collision	trace info for the collision that caused the damage.  Defaults to NULL.

inflictor, attacker, dir, and point can be NULL for environmental effects

Bleeding wounds and surface overlays are applied in the collision code that
calls Damage()
============
*/
void idActor::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, 
					  const char *damageDefName, const float damageScale, const int location,
					  trace_t *collision ) 
{
	bool bKO(false), bKOPowerBlow(false);
	int bodID(0);
	idEntity *reroute = NULL;
	idAFBody *StruckBody = NULL;

	if( collision )
	{
		bodID = BodyForClipModelId( collision->c.id );
		StruckBody = GetAFPhysics()->GetBody( bodID );
		
		if( StruckBody != NULL )
			reroute = StruckBody->GetRerouteEnt();
	}

	if( reroute != NULL )
	{
		reroute->Damage( inflictor, attacker, dir, damageDefName, damageScale, location, collision );
		goto Quit;
	}
	
	if ( !fl.takedamage ) {
		return;
	}

	if ( !inflictor ) {
		inflictor = gameLocal.world;
	}
	if ( !attacker ) {
		attacker = gameLocal.world;
	}

	if ( finalBoss && !inflictor->IsType( idSoulCubeMissile::Type ) ) {
		return;
	}

	const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName );
	if ( !damageDef ) {
		gameLocal.Error( "Unknown damageDef '%s'", damageDefName );
	}

	int	damage = damageDef->GetInt( "damage" ) * damageScale;

	damage = GetDamageForLocation( damage, location );

	DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("Actor %s received damage %d at joint %d, corresponding to damage group %s\r", name.c_str(), damage, (int) location, GetDamageGroup(location) );

	// inform the attacker that they hit someone
	attacker->DamageFeedback( this, inflictor, damage );

	// DarkMod: check for KO damage type and knockout AI if appropriate
	bKO = damageDef->GetBool( "knockout" );
	bKOPowerBlow = damageDef->GetBool( "knockout_power" );
	
	if( (bKO || bKOPowerBlow) && collision )
	{
			if( TestKnockoutBlow( dir, collision, bKOPowerBlow ) )
			{
				if ( (attacker && attacker->IsType( idPlayer::Type ) ) ) 
				{
					// TODO: Add a KO to the player stats (not yet implemented)
					// static_cast< idPlayer* >( attacker )->AddAIKO();	
				}

				// For now, first KO blow does no health damage
				goto Quit;
			}
	}


	if ( damage > 0 )
	{
		health -= damage;
		if ( health <= 0 ) 
		{
			if ( health < -999 ) 
			{
				health = -999;
			}
			Killed( inflictor, attacker, damage, dir, location );
			if ( ( health < -20 ) && spawnArgs.GetBool( "gib" ) && damageDef->GetBool( "gib" ) ) 
			{
				Gib( dir, damageDefName );
			}
		} 
		else
		{
			Pain( inflictor, attacker, damage, dir, location );

			// FIX: if drowning, stop pain SFX and play drown SFX on voice channel
			if ( damageDef->GetBool( "no_air" ) ) 
			{
				StopSound( SND_CHANNEL_VOICE, false );
				StartSound( "snd_airGasp", SND_CHANNEL_VOICE, 0, false, NULL );
			}
		}
	} else 
	{
		// don't accumulate knockback
		if ( af.IsLoaded() ) 
		{
			// clear impacts
			af.Rest();

			// physics is turned off by calling af.Rest()
			BecomeActive( TH_PHYSICS );
		}
	}

Quit:
	return;
}

/*
=====================
idActor::ClearPain
=====================
*/
void idActor::ClearPain( void ) {
	pain_debounce_time = 0;
}

/*
=====================
idActor::Pain
=====================
*/
bool idActor::Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	if ( af.IsLoaded() && !IsKnockedOut() ) {
		// clear impacts
		af.Rest();

		// physics is turned off by calling af.Rest()
		BecomeActive( TH_PHYSICS );
	}

	if ( gameLocal.time < pain_debounce_time ) {
		return false;
	}

	// don't play pain sounds more than necessary
	pain_debounce_time = gameLocal.time + pain_delay;

	if ( health > 75  ) {
		StartSound( "snd_pain_small", SND_CHANNEL_VOICE, 0, false, NULL );
	} else if ( health > 50 ) {
		StartSound( "snd_pain_medium", SND_CHANNEL_VOICE, 0, false, NULL );
	} else if ( health > 25 ) {
		StartSound( "snd_pain_large", SND_CHANNEL_VOICE, 0, false, NULL );
	} else {
		StartSound( "snd_pain_huge", SND_CHANNEL_VOICE, 0, false, NULL );
	}

	if ( !allowPain || ( gameLocal.time < painTime ) ) {
		// don't play a pain anim
		return false;
	}

	if ( pain_threshold && ( damage < pain_threshold ) ) {
		return false;
	}

	// set the pain anim
	idStr damageGroup = GetDamageGroup( location );

	painAnim = "";
	if ( animPrefix.Length() ) {
		if ( damageGroup.Length() && ( damageGroup != "legs" ) ) {
			sprintf( painAnim, "%s_pain_%s", animPrefix.c_str(), damageGroup.c_str() );
			if ( !animator.HasAnim( painAnim ) ) {
				sprintf( painAnim, "pain_%s", damageGroup.c_str() );
				if ( !animator.HasAnim( painAnim ) ) {
					painAnim = "";
				}
			}
		}

		if ( !painAnim.Length() ) {
			sprintf( painAnim, "%s_pain", animPrefix.c_str() );
			if ( !animator.HasAnim( painAnim ) ) {
				painAnim = "";
			}
		}
	} else if ( damageGroup.Length() && ( damageGroup != "legs" ) ) {
		sprintf( painAnim, "pain_%s", damageGroup.c_str() );
		if ( !animator.HasAnim( painAnim ) ) {
			sprintf( painAnim, "pain_%s", damageGroup.c_str() );
			if ( !animator.HasAnim( painAnim ) ) {
				painAnim = "";
			}
		}
	}

	if ( !painAnim.Length() ) {
		painAnim = "pain";
	}

	if ( g_debugDamage.GetBool() ) {
		gameLocal.Printf( "Damage: joint: '%s', zone '%s', anim '%s'\n", animator.GetJointName( ( jointHandle_t )location ), 
			damageGroup.c_str(), painAnim.c_str() );
	}

	return true;
}

/*
=====================
idActor::SpawnGibs
=====================
*/
void idActor::SpawnGibs( const idVec3 &dir, const char *damageDefName ) {
	idAFEntity_Gibbable::SpawnGibs( dir, damageDefName );
	RemoveAttachments();
}

/*
=====================
idActor::SetupDamageGroups

FIXME: only store group names once and store an index for each joint
=====================
*/
void idActor::SetupDamageGroups( void ) {
	int						i;
	const idKeyValue		*arg;
	idStr					groupname;
	idList<jointHandle_t>	jointList;
	int						jointnum;
	float					scale;

	// create damage zones
	damageGroups.SetNum( animator.NumJoints() );
	arg = spawnArgs.MatchPrefix( "damage_zone ", NULL );
	while ( arg ) {
		groupname = arg->GetKey();
		groupname.Strip( "damage_zone " );
		animator.GetJointList( arg->GetValue(), jointList );
		for( i = 0; i < jointList.Num(); i++ ) {
			jointnum = jointList[ i ];
			damageGroups[ jointnum ] = groupname;
		}
		jointList.Clear();
		arg = spawnArgs.MatchPrefix( "damage_zone ", arg );
	}

	// initilize the damage zones to normal damage
	damageScale.SetNum( animator.NumJoints() );
	for( i = 0; i < damageScale.Num(); i++ ) {
		damageScale[ i ] = 1.0f;
	}

	// set the percentage on damage zones
	arg = spawnArgs.MatchPrefix( "damage_scale ", NULL );
	while ( arg ) {
		scale = atof( arg->GetValue() );
		groupname = arg->GetKey();
		groupname.Strip( "damage_scale " );
		for( i = 0; i < damageScale.Num(); i++ ) {
			if ( damageGroups[ i ] == groupname ) {
				damageScale[ i ] = scale;
			}
		}
		arg = spawnArgs.MatchPrefix( "damage_scale ", arg );
	}
}

/*
=====================
idActor::GetDamageForLocation
=====================
*/
int idActor::GetDamageForLocation( int damage, int location ) {
	if ( ( location < 0 ) || ( location >= damageScale.Num() ) ) {
		return damage;
	}

	return (int)ceil( damage * damageScale[ location ] );
}

/*
=====================
idActor::GetDamageGroup
=====================
*/
const char *idActor::GetDamageGroup( int location ) {
	if ( ( location < 0 ) || ( location >= damageGroups.Num() ) ) {
		return "";
	}

	return damageGroups[ location ];
}

/*
=====================
idActor::PlayFootStepSound
=====================
*/
void idActor::PlayFootStepSound( void ) 
{
	idStr				moveType, localSound, sound;
	idMaterial			*material = NULL;
	idPlayer			*thisPlayer(NULL);
	idAI				*thisAI(NULL);
	const idSoundShader	*sndShader = NULL;
	
	if ( !GetPhysics()->HasGroundContacts() ) {
		return;
	}

	// DarkMod: make the string to identify the movement speed (crouch_run, creep, etc)
	// Currently only players have movement flags set up this way, not AI.  We could change that later.
	if ( IsType( idPlayer::Type ) )
	{
		thisPlayer =  static_cast<idPlayer *>(this);
		moveType.Clear();

		UpdateMoveVolumes();

		if( thisPlayer->AI_CROUCH )
			moveType = "_crouch";

		if( thisPlayer->AI_RUN )
			moveType += "_run";
		else if ( thisPlayer->AI_CREEP )
			moveType += "_creep";
		else
			moveType += "_walk";
	}

	else if ( IsType( idAI::Type ) )
	{
		thisAI =  static_cast<idAI *>(this);
		moveType.Clear();

		if( thisAI->AI_CROUCH )
			moveType = "_crouch";

		if( thisAI->AI_RUN )
			moveType += "_run";
		else if ( thisAI->AI_CREEP )
			moveType += "_creep";
		else
			moveType += "_walk";
	}


	// start footstep sound based on material type
	material = const_cast<idMaterial *>( GetPhysics()->GetContact( 0 ).material );
	if ( material != NULL ) 
	{
		DM_LOG(LC_SOUND,LT_DEBUG)LOGSTRING("Actor %s stepped on entity %s, material %s \r", name.c_str(), gameLocal.entities[GetPhysics()->GetContact( 0 ).entityNum]->name.c_str(), material->GetName() );  
		g_Global.GetSurfName(material, localSound);
		localSound = "snd_footstep_" + localSound;

		DM_LOG(LC_SOUND,LT_DEBUG)LOGSTRING("Found surface type sound: %s\r", localSound.c_str() ); 
		sound = spawnArgs.GetString( localSound.c_str() );
	}

	waterLevel_t waterLevel = static_cast<idPhysics_Actor *>(GetPhysics())->GetWaterLevel();
	// If player is walking in liquid, replace the bottom surface sound with water sounds
	if (waterLevel == WATERLEVEL_FEET )
	{
		localSound = "snd_footstep_puddle";
		sound = spawnArgs.GetString( localSound.c_str() );
	}
	else if (waterLevel == WATERLEVEL_WAIST)
	{
		localSound = "snd_footstep_wading";
		sound = spawnArgs.GetString( localSound.c_str() );
	}
	// greebo: Added this to disable the walking sound when completely underwater
	// this should be replaced by snd_
	else if (waterLevel == WATERLEVEL_HEAD)
	{
		localSound = "snd_footstep_swim";
		sound = spawnArgs.GetString( localSound.c_str() );
	}

	if ( sound.IsEmpty() && waterLevel != WATERLEVEL_HEAD ) 
	{
		localSound = "snd_footstep";
	}
	
	sound = spawnArgs.GetString( localSound.c_str() );

	// if a sound was not found for that specific material, use default
	if( sound.IsEmpty() && waterLevel != WATERLEVEL_HEAD )
	{
		sound = spawnArgs.GetString( "snd_footstep" );
		localSound = "snd_footstep";
	}

	// The player always considers the movement type when propagating
	if( thisPlayer )
	{
		localSound += moveType;
	}
/***
* AI footsteps always propagate as snd_footstep for now
* If we want to add in AI soundprop based on movement speed later,
*	here is the place to do it.
**/
/*
	localSound += moveType;
	if( !gameLocal.m_sndProp->CheckSound( localSound.c_str(), false ) )
		localSound -= moveType;
*/

	if ( !sound.IsEmpty() ) 
	{
		// apply the movement type modifier to the volume
		sndShader = declManager->FindSound( sound.c_str() );
		SetSoundVolume( sndShader->GetParms()->volume + GetMovementVolMod() );
		StartSoundShader( sndShader, SND_CHANNEL_BODY, 0, false, NULL );
		SetSoundVolume( 0.0f );

		// propagate the suspicious sound to other AI
		PropSoundDirect( static_cast<const char *>( localSound.c_str() ), true, false );
	}
}

/*
========================
idActor::ReAttach
========================
*/
void idActor::ReAttach( int ind, idStr jointName, idVec3 offset, idAngles angles  ) 
{
	idEntity		*ent( NULL );
	idVec3			origin;
	idMat3			axis, rotate, newAxis;
	jointHandle_t	joint;
	idAttachInfo	*attachment;

	ind--;
	if( ind < 0 || ind >= m_attachments.Num() )
	{
		// TODO: log invalid index error
		goto Quit;
	}

	attachment = &m_attachments[ind];
	ent = attachment->ent.GetEntity();

	if( !ent || !attachment->ent.IsValid() )
	{
		// TODO: log bad attachment entity error
		goto Quit;
	}

	joint = animator.GetJointHandle( jointName );
	if ( joint == INVALID_JOINT )
	{
		// TODO: log error
		gameLocal.Warning( "Joint '%s' not found for attaching '%s' on '%s'", jointName.c_str(), ent->GetClassname(), name.c_str() );
		goto Quit;
	}

	attachment->channel = animator.GetChannelForJoint( joint );
	GetJointWorldTransform( joint, gameLocal.time, origin, axis );

	rotate = angles.ToMat3();
	newAxis = rotate * axis;

	ent->Unbind();
	ent->SetAxis( newAxis );
	// Use the local joint axis instead of the overall AI axis
	ent->SetOrigin( origin + offset * axis );

	ent->BindToJoint( this, joint, true );
	ent->cinematic = cinematic;
	
	// set the spawnargs for later retrieval as well
	ent->spawnArgs.Set( "joint", jointName.c_str() );
	ent->spawnArgs.SetVector( "origin", offset );
	ent->spawnArgs.SetAngles( "angles", angles );

Quit:
	return;
}

void idActor::ShowAttachment( int ind, bool bShow )
{
	idEntity *ent( NULL );

	ind--;
	if( ind < 0 || ind >= m_attachments.Num() )
	{
		// TODO: log invalid index error
		goto Quit;
	}

	ent = m_attachments[ind].ent.GetEntity();

	if( !ent || !m_attachments[ind].ent.IsValid() )
	{
		// TODO: log bad attachment entity error
		goto Quit;
	}

	if( bShow )
		ent->Show();
	else
		ent->Hide();

Quit:
	return;
}

void idActor::DropAttachment( int ind )
{
	idEntity *ent = NULL;

	ind--;
	if( ind < 0 || ind >= m_attachments.Num() )
	{
		// TODO: log invalid index error
		goto Quit;
	}

	ent = m_attachments[ind].ent.GetEntity();

	if( !ent || !m_attachments[ind].ent.IsValid() )
	{
		// TODO: log bad attachment entity error
		goto Quit;
	}

	ent->Unbind();
	m_attachments[ind].ent = NULL;

Quit:
	return;
}

bool idActor::GetAttachInfo( int ind, idStr &jointName, idVec3 &offset, 
							idAngles &angles )
{
	bool bReturnVal = false;
	idEntity *ent = NULL;

	ind--;
	if( ind < 0 || ind >= m_attachments.Num() )
	{
		// TODO: log invalid index error
		goto Quit;
	}

	ent = m_attachments[ind].ent.GetEntity();

	if( !ent || !m_attachments[ind].ent.IsValid() )
	{
		// TODO: log bad attachment entity error
		goto Quit;
	}

	jointName = ent->spawnArgs.GetString( "joint", "" );
	offset = ent->spawnArgs.GetVector( "origin" );
	angles = ent->spawnArgs.GetAngles( "angles" );

	bReturnVal = true;

Quit:
	return bReturnVal;
}

idEntity *idActor::GetAttachedEnt( int ind )
{
	idEntity *ent = NULL;

	ind--;
	if( ind < 0 || ind >= m_attachments.Num() )
	{
		// TODO: log invalid index error
		goto Quit;
	}

	ent = m_attachments[ind].ent.GetEntity();

	if( !ent || !m_attachments[ind].ent.IsValid() )
	{
		// TODO: log bad attachment entity error
		ent = NULL;
		goto Quit;
	}

Quit:
	return ent;
}


/***********************************************************************

	Events

***********************************************************************/

/*
=====================
idActor::Event_EnableEyeFocus
=====================
*/
void idActor::Event_EnableEyeFocus( void ) {
	allowEyeFocus = true;
	blink_time = gameLocal.time + blink_min + gameLocal.random.RandomFloat() * ( blink_max - blink_min );
}

/*
=====================
idActor::Event_DisableEyeFocus
=====================
*/
void idActor::Event_DisableEyeFocus( void ) {
	allowEyeFocus = false;
	
	idEntity *headEnt = head.GetEntity();
	if ( headEnt ) {
		headEnt->GetAnimator()->Clear( ANIMCHANNEL_EYELIDS, gameLocal.time, FRAME2MS( 2 ) );
	} else {
		animator.Clear( ANIMCHANNEL_EYELIDS, gameLocal.time, FRAME2MS( 2 ) );
	}
}

/*
===============
idActor::Event_Footstep
===============
*/
void idActor::Event_Footstep( void ) {
	PlayFootStepSound();
}

/*
=====================
idActor::Event_EnableWalkIK
=====================
*/
void idActor::Event_EnableWalkIK( void ) {
	walkIK.EnableAll();
}

/*
=====================
idActor::Event_DisableWalkIK
=====================
*/
void idActor::Event_DisableWalkIK( void ) {
	walkIK.DisableAll();
}

/*
=====================
idActor::Event_EnableLegIK
=====================
*/
void idActor::Event_EnableLegIK( int num ) {
	walkIK.EnableLeg( num );
}

/*
=====================
idActor::Event_DisableLegIK
=====================
*/
void idActor::Event_DisableLegIK( int num ) {
	walkIK.DisableLeg( num );
}

/*
=====================
idActor::Event_PreventPain
=====================
*/
void idActor::Event_PreventPain( float duration ) {
	painTime = gameLocal.time + SEC2MS( duration );
}

/*
===============
idActor::Event_DisablePain
===============
*/
void idActor::Event_DisablePain( void ) {
	allowPain = false;
}

/*
===============
idActor::Event_EnablePain
===============
*/
void idActor::Event_EnablePain( void ) {
	allowPain = true;
}

/*
=====================
idActor::Event_GetPainAnim
=====================
*/
void idActor::Event_GetPainAnim( void ) {
	if ( !painAnim.Length() ) {
		idThread::ReturnString( "pain" );
	} else {
		idThread::ReturnString( painAnim );
	}
}

/*
=====================
idActor::Event_SetAnimPrefix
=====================
*/
void idActor::Event_SetAnimPrefix( const char *prefix ) {
	animPrefix = prefix;
}

/*
===============
idActor::Event_StopAnim
===============
*/
void idActor::Event_StopAnim( int channel, int frames ) {
	switch( channel ) {
	case ANIMCHANNEL_HEAD :
		headAnim.StopAnim( frames );
		break;

	case ANIMCHANNEL_TORSO :
		torsoAnim.StopAnim( frames );
		break;

	case ANIMCHANNEL_LEGS :
		legsAnim.StopAnim( frames );
		break;

	default:
		gameLocal.Error( "Unknown anim group" );
		break;
	}
}

/*
===============
idActor::Event_PlayAnim
===============
*/
void idActor::Event_PlayAnim( int channel, const char *animname ) {
	animFlags_t	flags;
	idEntity *headEnt;
	int	anim;
	
	anim = GetAnim( channel, animname );
	if ( !anim ) {
		
#ifndef SUPPRESS_CONSOLE_WARNINGS
		if ( ( channel == ANIMCHANNEL_HEAD ) && head.GetEntity() ) {
			gameLocal.DPrintf( "missing '%s' animation on '%s' (%s)\n", animname, name.c_str(), spawnArgs.GetString( "def_head", "" ) );
		} else {
			gameLocal.DPrintf( "missing '%s' animation on '%s' (%s)\n", animname, name.c_str(), GetEntityDefName() );
		}
#endif

		idThread::ReturnInt( 0 );
		return;
	}

	switch( channel ) {
	case ANIMCHANNEL_HEAD :
		headEnt = head.GetEntity();
		if ( headEnt ) {
			headAnim.idleAnim = false;
			headAnim.PlayAnim( anim );
			flags = headAnim.GetAnimFlags();
			if ( !flags.prevent_idle_override ) {
				if ( torsoAnim.IsIdle() ) {
					torsoAnim.animBlendFrames = headAnim.lastAnimBlendFrames;
					SyncAnimChannels( ANIMCHANNEL_TORSO, ANIMCHANNEL_HEAD, headAnim.lastAnimBlendFrames );
					if ( legsAnim.IsIdle() ) {
						legsAnim.animBlendFrames = headAnim.lastAnimBlendFrames;
						SyncAnimChannels( ANIMCHANNEL_LEGS, ANIMCHANNEL_HEAD, headAnim.lastAnimBlendFrames );
					}
				}
			}
		}
		break;

	case ANIMCHANNEL_TORSO :
		torsoAnim.idleAnim = false;
		torsoAnim.PlayAnim( anim );
		flags = torsoAnim.GetAnimFlags();
		if ( !flags.prevent_idle_override ) {
			if ( headAnim.IsIdle() ) {
				headAnim.animBlendFrames = torsoAnim.lastAnimBlendFrames;
				SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_TORSO, torsoAnim.lastAnimBlendFrames );
			}
			if ( legsAnim.IsIdle() ) {
				legsAnim.animBlendFrames = torsoAnim.lastAnimBlendFrames;
				SyncAnimChannels( ANIMCHANNEL_LEGS, ANIMCHANNEL_TORSO, torsoAnim.lastAnimBlendFrames );
			}
		}
		break;

	case ANIMCHANNEL_LEGS :
		legsAnim.idleAnim = false;
		legsAnim.PlayAnim( anim );
		flags = legsAnim.GetAnimFlags();
		if ( !flags.prevent_idle_override ) {
			if ( torsoAnim.IsIdle() ) {
				torsoAnim.animBlendFrames = legsAnim.lastAnimBlendFrames;
				SyncAnimChannels( ANIMCHANNEL_TORSO, ANIMCHANNEL_LEGS, legsAnim.lastAnimBlendFrames );
				if ( headAnim.IsIdle() ) {
					headAnim.animBlendFrames = legsAnim.lastAnimBlendFrames;
					SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_LEGS, legsAnim.lastAnimBlendFrames );
				}
			}
		}
		break;

	default :
		gameLocal.Error( "Unknown anim group" );
		break;
	}
	idThread::ReturnInt( 1 );
}

/*
===============
idActor::Event_PlayCycle
===============
*/
void idActor::Event_PlayCycle( int channel, const char *animname ) {
	animFlags_t	flags;
	int			anim;
	
	anim = GetAnim( channel, animname );
	if ( !anim ) {
		
#ifndef SUPPRESS_CONSOLE_WARNINGS
		if ( ( channel == ANIMCHANNEL_HEAD ) && head.GetEntity() ) {
			gameLocal.DPrintf( "missing '%s' animation on '%s' (%s)\n", animname, name.c_str(), spawnArgs.GetString( "def_head", "" ) );
		} else {
			gameLocal.DPrintf( "missing '%s' animation on '%s' (%s)\n", animname, name.c_str(), GetEntityDefName() );
		}
#endif

		idThread::ReturnInt( false );
		return;
	}

	switch( channel ) {
	case ANIMCHANNEL_HEAD :
		headAnim.idleAnim = false;
		headAnim.CycleAnim( anim );
		flags = headAnim.GetAnimFlags();
		if ( !flags.prevent_idle_override ) {
			if ( torsoAnim.IsIdle() && legsAnim.IsIdle() ) {
				torsoAnim.animBlendFrames = headAnim.lastAnimBlendFrames;
				SyncAnimChannels( ANIMCHANNEL_TORSO, ANIMCHANNEL_HEAD, headAnim.lastAnimBlendFrames );
				legsAnim.animBlendFrames = headAnim.lastAnimBlendFrames;
				SyncAnimChannels( ANIMCHANNEL_LEGS, ANIMCHANNEL_HEAD, headAnim.lastAnimBlendFrames );
			}
		}
		break;

	case ANIMCHANNEL_TORSO :
		torsoAnim.idleAnim = false;
		torsoAnim.CycleAnim( anim );
		flags = torsoAnim.GetAnimFlags();
		if ( !flags.prevent_idle_override ) {
			if ( headAnim.IsIdle() ) {
				headAnim.animBlendFrames = torsoAnim.lastAnimBlendFrames;
				SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_TORSO, torsoAnim.lastAnimBlendFrames );
			}
			if ( legsAnim.IsIdle() ) {
				legsAnim.animBlendFrames = torsoAnim.lastAnimBlendFrames;
				SyncAnimChannels( ANIMCHANNEL_LEGS, ANIMCHANNEL_TORSO, torsoAnim.lastAnimBlendFrames );
			}
		}
		break;

	case ANIMCHANNEL_LEGS :
		legsAnim.idleAnim = false;
		legsAnim.CycleAnim( anim );
		flags = legsAnim.GetAnimFlags();
		if ( !flags.prevent_idle_override ) {
			if ( torsoAnim.IsIdle() ) {
				torsoAnim.animBlendFrames = legsAnim.lastAnimBlendFrames;
				SyncAnimChannels( ANIMCHANNEL_TORSO, ANIMCHANNEL_LEGS, legsAnim.lastAnimBlendFrames );
				if ( headAnim.IsIdle() ) {
					headAnim.animBlendFrames = legsAnim.lastAnimBlendFrames;
					SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_LEGS, legsAnim.lastAnimBlendFrames );
				}
			}
		}
		break;

	default:
		gameLocal.Error( "Unknown anim group" );
	}

	idThread::ReturnInt( true );
}

/*
===============
idActor::Event_IdleAnim
===============
*/
void idActor::Event_IdleAnim( int channel, const char *animname ) {
	int anim;
	
	anim = GetAnim( channel, animname );	
	if ( !anim ) {

#ifndef SUPPRESS_CONSOLE_WARNINGS
		if ( ( channel == ANIMCHANNEL_HEAD ) && head.GetEntity() ) {
			gameLocal.DPrintf( "missing '%s' animation on '%s' (%s)\n", animname, name.c_str(), spawnArgs.GetString( "def_head", "" ) );
		} else {
			gameLocal.DPrintf( "missing '%s' animation on '%s' (%s)\n", animname, name.c_str(), GetEntityDefName() );
		}
#endif

		switch( channel ) {
		case ANIMCHANNEL_HEAD :
			headAnim.BecomeIdle();
			break;

		case ANIMCHANNEL_TORSO :
			torsoAnim.BecomeIdle();
			break;

		case ANIMCHANNEL_LEGS :
			legsAnim.BecomeIdle();
			break;

		default:
			gameLocal.Error( "Unknown anim group" );
		}

		idThread::ReturnInt( false );
		return;
	}

	switch( channel ) {
	case ANIMCHANNEL_HEAD :
		headAnim.BecomeIdle();
		if ( torsoAnim.GetAnimFlags().prevent_idle_override ) {
			// don't sync to torso body if it doesn't override idle anims
			headAnim.CycleAnim( anim );
		} else if ( torsoAnim.IsIdle() && legsAnim.IsIdle() ) {
			// everything is idle, so play the anim on the head and copy it to the torso and legs
			headAnim.CycleAnim( anim );
			torsoAnim.animBlendFrames = headAnim.lastAnimBlendFrames;
			SyncAnimChannels( ANIMCHANNEL_TORSO, ANIMCHANNEL_HEAD, headAnim.lastAnimBlendFrames );
			legsAnim.animBlendFrames = headAnim.lastAnimBlendFrames;
			SyncAnimChannels( ANIMCHANNEL_LEGS, ANIMCHANNEL_HEAD, headAnim.lastAnimBlendFrames );
		} else if ( torsoAnim.IsIdle() ) {
			// sync the head and torso to the legs
			SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_LEGS, headAnim.animBlendFrames );
			torsoAnim.animBlendFrames = headAnim.lastAnimBlendFrames;
			SyncAnimChannels( ANIMCHANNEL_TORSO, ANIMCHANNEL_LEGS, torsoAnim.animBlendFrames );
		} else {
			// sync the head to the torso
			SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_TORSO, headAnim.animBlendFrames );
		}
		break;

	case ANIMCHANNEL_TORSO :
		torsoAnim.BecomeIdle();
		if ( legsAnim.GetAnimFlags().prevent_idle_override ) {
			// don't sync to legs if legs anim doesn't override idle anims
			torsoAnim.CycleAnim( anim );
		} else if ( legsAnim.IsIdle() ) {
			// play the anim in both legs and torso
			torsoAnim.CycleAnim( anim );
			legsAnim.animBlendFrames = torsoAnim.lastAnimBlendFrames;
			SyncAnimChannels( ANIMCHANNEL_LEGS, ANIMCHANNEL_TORSO, torsoAnim.lastAnimBlendFrames );
		} else {
			// sync the anim to the legs
			SyncAnimChannels( ANIMCHANNEL_TORSO, ANIMCHANNEL_LEGS, torsoAnim.animBlendFrames );
		}

		if ( headAnim.IsIdle() ) {
			SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_TORSO, torsoAnim.lastAnimBlendFrames );
		}
		break;

	case ANIMCHANNEL_LEGS :
		legsAnim.BecomeIdle();
		if ( torsoAnim.GetAnimFlags().prevent_idle_override ) {
			// don't sync to torso if torso anim doesn't override idle anims
			legsAnim.CycleAnim( anim );
		} else if ( torsoAnim.IsIdle() ) {
			// play the anim in both legs and torso
			legsAnim.CycleAnim( anim );
			torsoAnim.animBlendFrames = legsAnim.lastAnimBlendFrames;
			SyncAnimChannels( ANIMCHANNEL_TORSO, ANIMCHANNEL_LEGS, legsAnim.lastAnimBlendFrames );
			if ( headAnim.IsIdle() ) {
				SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_LEGS, legsAnim.lastAnimBlendFrames );
			}
		} else {
			// sync the anim to the torso
			SyncAnimChannels( ANIMCHANNEL_LEGS, ANIMCHANNEL_TORSO, legsAnim.animBlendFrames );
		}
		break;

	default:
		gameLocal.Error( "Unknown anim group" );
	}

	idThread::ReturnInt( true );
}

/*
================
idActor::Event_SetSyncedAnimWeight
================
*/
void idActor::Event_SetSyncedAnimWeight( int channel, int anim, float weight ) {
	idEntity *headEnt;

	headEnt = head.GetEntity();
	switch( channel ) {
	case ANIMCHANNEL_HEAD :
		if ( headEnt ) {
			animator.CurrentAnim( ANIMCHANNEL_ALL )->SetSyncedAnimWeight( anim, weight );
		} else {
			animator.CurrentAnim( ANIMCHANNEL_HEAD )->SetSyncedAnimWeight( anim, weight );
		}
		if ( torsoAnim.IsIdle() ) {
			animator.CurrentAnim( ANIMCHANNEL_TORSO )->SetSyncedAnimWeight( anim, weight );
			if ( legsAnim.IsIdle() ) {
				animator.CurrentAnim( ANIMCHANNEL_LEGS )->SetSyncedAnimWeight( anim, weight );
			}
		}
		break;

	case ANIMCHANNEL_TORSO :
		animator.CurrentAnim( ANIMCHANNEL_TORSO )->SetSyncedAnimWeight( anim, weight );
		if ( legsAnim.IsIdle() ) {
			animator.CurrentAnim( ANIMCHANNEL_LEGS )->SetSyncedAnimWeight( anim, weight );
		}
		if ( headEnt && headAnim.IsIdle() ) {
			animator.CurrentAnim( ANIMCHANNEL_ALL )->SetSyncedAnimWeight( anim, weight );
		}
		break;

	case ANIMCHANNEL_LEGS :
		animator.CurrentAnim( ANIMCHANNEL_LEGS )->SetSyncedAnimWeight( anim, weight );
		if ( torsoAnim.IsIdle() ) {
			animator.CurrentAnim( ANIMCHANNEL_TORSO )->SetSyncedAnimWeight( anim, weight );
			if ( headEnt && headAnim.IsIdle() ) {
				animator.CurrentAnim( ANIMCHANNEL_ALL )->SetSyncedAnimWeight( anim, weight );
			}
		}
		break;

	default:
		gameLocal.Error( "Unknown anim group" );
	}
}

/*
===============
idActor::Event_OverrideAnim
===============
*/
void idActor::Event_OverrideAnim( int channel ) {
	switch( channel ) {
	case ANIMCHANNEL_HEAD :
		headAnim.Disable();
		if ( !torsoAnim.IsIdle() ) {
			SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_TORSO, torsoAnim.lastAnimBlendFrames );
		} else {
			SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_LEGS, legsAnim.lastAnimBlendFrames );
		}
		break;

	case ANIMCHANNEL_TORSO :
		torsoAnim.Disable();
		SyncAnimChannels( ANIMCHANNEL_TORSO, ANIMCHANNEL_LEGS, legsAnim.lastAnimBlendFrames );
		if ( headAnim.IsIdle() ) {
			SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_TORSO, torsoAnim.lastAnimBlendFrames );
		}
		break;

	case ANIMCHANNEL_LEGS :
		legsAnim.Disable();
		SyncAnimChannels( ANIMCHANNEL_LEGS, ANIMCHANNEL_TORSO, torsoAnim.lastAnimBlendFrames );
		break;

	default:
		gameLocal.Error( "Unknown anim group" );
		break;
	}
}

/*
===============
idActor::Event_EnableAnim
===============
*/
void idActor::Event_EnableAnim( int channel, int blendFrames ) {
	switch( channel ) {
	case ANIMCHANNEL_HEAD :
		headAnim.Enable( blendFrames );
		break;

	case ANIMCHANNEL_TORSO :
		torsoAnim.Enable( blendFrames );
		break;

	case ANIMCHANNEL_LEGS :
		legsAnim.Enable( blendFrames );
		break;

	default:
		gameLocal.Error( "Unknown anim group" );
		break;
	}
}

/*
===============
idActor::Event_SetBlendFrames
===============
*/
void idActor::Event_SetBlendFrames( int channel, int blendFrames ) {
	switch( channel ) {
	case ANIMCHANNEL_HEAD :
		headAnim.animBlendFrames = blendFrames;
		headAnim.lastAnimBlendFrames = blendFrames;
		break;

	case ANIMCHANNEL_TORSO :
		torsoAnim.animBlendFrames = blendFrames;
		torsoAnim.lastAnimBlendFrames = blendFrames;
		break;

	case ANIMCHANNEL_LEGS :
		legsAnim.animBlendFrames = blendFrames;
		legsAnim.lastAnimBlendFrames = blendFrames;
		break;

	default:
		gameLocal.Error( "Unknown anim group" );
		break;
	}
}

/*
===============
idActor::Event_GetBlendFrames
===============
*/
void idActor::Event_GetBlendFrames( int channel ) {
	switch( channel ) {
	case ANIMCHANNEL_HEAD :
		idThread::ReturnInt( headAnim.animBlendFrames );
		break;

	case ANIMCHANNEL_TORSO :
		idThread::ReturnInt( torsoAnim.animBlendFrames );
		break;

	case ANIMCHANNEL_LEGS :
		idThread::ReturnInt( legsAnim.animBlendFrames );
		break;

	default:
		gameLocal.Error( "Unknown anim group" );
		break;
	}
}

/*
===============
idActor::Event_AnimState
===============
*/
void idActor::Event_AnimState( int channel, const char *statename, int blendFrames ) {
	SetAnimState( channel, statename, blendFrames );
}

/*
===============
idActor::Event_GetAnimState
===============
*/
void idActor::Event_GetAnimState( int channel ) {
	const char *state;

	state = GetAnimState( channel );
	idThread::ReturnString( state );
}

/*
===============
idActor::Event_InAnimState
===============
*/
void idActor::Event_InAnimState( int channel, const char *statename ) {
	bool instate;

	instate = InAnimState( channel, statename );
	idThread::ReturnInt( instate );
}

/*
===============
idActor::Event_FinishAction
===============
*/
void idActor::Event_FinishAction( const char *actionname ) {
	if ( waitState == actionname ) {
		SetWaitState( "" );
	}
}

/*
===============
idActor::Event_AnimDone
===============
*/
void idActor::Event_AnimDone( int channel, int blendFrames ) {
	bool result;

	switch( channel ) {
	case ANIMCHANNEL_HEAD :
		result = headAnim.AnimDone( blendFrames );
		idThread::ReturnInt( result );
		break;

	case ANIMCHANNEL_TORSO :
		result = torsoAnim.AnimDone( blendFrames );
		idThread::ReturnInt( result );
		break;

	case ANIMCHANNEL_LEGS :
		result = legsAnim.AnimDone( blendFrames );
		idThread::ReturnInt( result );
		break;

	default:
		gameLocal.Error( "Unknown anim group" );
	}
}

/*
================
idActor::Event_HasAnim
================
*/
void idActor::Event_HasAnim( int channel, const char *animname ) {
	if ( GetAnim( channel, animname ) != NULL ) {
		idThread::ReturnFloat( 1.0f );
	} else {
		idThread::ReturnFloat( 0.0f );
	}
}

/*
================
idActor::Event_CheckAnim
================
*/
void idActor::Event_CheckAnim( int channel, const char *animname ) {
	if ( !GetAnim( channel, animname ) ) {
		if ( animPrefix.Length() ) {
			gameLocal.Error( "Can't find anim '%s_%s' for '%s'", animPrefix.c_str(), animname, name.c_str() );
		} else {
			gameLocal.Error( "Can't find anim '%s' for '%s'", animname, name.c_str() );
		}
	}
}

/*
================
idActor::Event_ChooseAnim
================
*/
void idActor::Event_ChooseAnim( int channel, const char *animname ) {
	int anim;

	anim = GetAnim( channel, animname );
	if ( anim ) {
		if ( channel == ANIMCHANNEL_HEAD ) {
			if ( head.GetEntity() ) {
				idThread::ReturnString( head.GetEntity()->GetAnimator()->AnimFullName( anim ) );
				return;
			}
		} else {
			idThread::ReturnString( animator.AnimFullName( anim ) );
			return;
		}
	}

	idThread::ReturnString( "" );
}

/*
================
idActor::Event_AnimLength
================
*/
void idActor::Event_AnimLength( int channel, const char *animname ) {
	int anim;

	anim = GetAnim( channel, animname );
	if ( anim ) {
		if ( channel == ANIMCHANNEL_HEAD ) {
			if ( head.GetEntity() ) {
				idThread::ReturnFloat( MS2SEC( head.GetEntity()->GetAnimator()->AnimLength( anim ) ) );
				return;
			}
		} else {
			idThread::ReturnFloat( MS2SEC( animator.AnimLength( anim ) ) );
			return;
		}		
	}
	
	idThread::ReturnFloat( 0.0f );
}

/*
================
idActor::Event_AnimDistance
================
*/
void idActor::Event_AnimDistance( int channel, const char *animname ) {
	int anim;

	anim = GetAnim( channel, animname );
	if ( anim ) {
		if ( channel == ANIMCHANNEL_HEAD ) {
			if ( head.GetEntity() ) {
				idThread::ReturnFloat( head.GetEntity()->GetAnimator()->TotalMovementDelta( anim ).Length() );
				return;
			}
		} else {
			idThread::ReturnFloat( animator.TotalMovementDelta( anim ).Length() );
			return;
		}
	}
	
	idThread::ReturnFloat( 0.0f );
}

/*
================
idActor::Event_HasEnemies
================
*/
void idActor::Event_HasEnemies( void ) {
	bool hasEnemy;

	hasEnemy = HasEnemies();
	idThread::ReturnInt( hasEnemy );
}

/*
================
idActor::Event_NextEnemy
================
*/
void idActor::Event_NextEnemy( idEntity *ent ) {
	idActor *actor;

	if ( !ent || ( ent == this ) ) {
		actor = enemyList.Next();
	} else {
		if ( !ent->IsType( idActor::Type ) ) {
			gameLocal.Error( "'%s' cannot be an enemy", ent->name.c_str() );
		}

		actor = static_cast<idActor *>( ent );
		if ( actor->enemyNode.ListHead() != &enemyList ) {
			gameLocal.Error( "'%s' is not in '%s' enemy list", actor->name.c_str(), name.c_str() );
		}
	}

	for( ; actor != NULL; actor = actor->enemyNode.Next() ) {
		if ( !actor->fl.hidden ) {
			idThread::ReturnEntity( actor );
			return;
		}
	}

    idThread::ReturnEntity( NULL );
}

/*
================
idActor::Event_ClosestEnemyToPoint
================
*/
void idActor::Event_ClosestEnemyToPoint( const idVec3 &pos ) {
	idActor *bestEnt = ClosestEnemyToPoint( pos );
	idThread::ReturnEntity( bestEnt );
}

/*
================
idActor::Event_StopSound
================
*/
void idActor::Event_StopSound( int channel, int netSync ) {
	if ( channel == SND_CHANNEL_VOICE ) {
		idEntity *headEnt = head.GetEntity();
		if ( headEnt ) {
			headEnt->StopSound( channel, ( netSync != 0 ) );
		}
	}
	StopSound( channel, ( netSync != 0 ) );
}

/*
=====================
idActor::Event_SetNextState
=====================
*/
void idActor::Event_SetNextState( const char *name ) {
	idealState = GetScriptFunction( name );
	if ( idealState == state ) {
		state = NULL;
	}
}

/*
=====================
idActor::Event_SetState
=====================
*/
void idActor::Event_SetState( const char *name ) {
	idealState = GetScriptFunction( name );
	if ( idealState == state ) {
		state = NULL;
	}
	scriptThread->DoneProcessing();
}

/*
=====================
idActor::Event_GetState
=====================
*/
void idActor::Event_GetState( void ) {
	if ( state ) {
		idThread::ReturnString( state->Name() );
	} else {
		idThread::ReturnString( "" );
	}
}

/*
=====================
idActor::Event_GetHead
=====================
*/
void idActor::Event_GetHead( void ) {
	idThread::ReturnEntity( head.GetEntity() );
}

/*
=====================
idActor::Event_GetEyePos
=====================
*/
void idActor::Event_GetEyePos( void )
{
	idThread::ReturnVector( GetEyePosition() );
}

/*
=====================
idActor::Event_GetAttachment
=====================
*/
void idActor::Event_GetAttachment( int ind )
{
	idEntity *ent = GetAttachedEnt( ind );
	idThread::ReturnEntity( ent );
}

/*
=====================
idActor::Event_GetAttachment
=====================
*/
void idActor::Event_GetNumAttachments( void )
{
	idThread::ReturnInt( m_attachments.Num() );
}

/****************************************************************************************
	=====================
	idActor::CrashLand
	handle collision(Falling) damage to AI/Players
	Added by Richard Day
	=====================
****************************************************************************************/
float idActor::CrashLand( const idPhysics_Actor& physicsObj, const idVec3 &savedOrigin, const idVec3 &savedVelocity )
{
	bool bNoDamage = false;
	float delta = 0.0f;

	// no falling damage if touching a nodamage surface
	// We do this here since the sound wont be played otherwise
	// as we do no damage if this is true.
	for( int i = 0; i < physicsObj.GetNumContacts(); i++ )
	{
		const contactInfo_t &contact = physicsObj.GetContact( i );
		if ( contact.material->GetSurfaceFlags() & SURF_NODAMAGE )
		{
			bNoDamage = true;
			StartSound( "snd_land_hard", SND_CHANNEL_ANY, 0, false, NULL );
			break;
		}
	}
	if( !bNoDamage )
	{
		idVec3 vGravNorm = GetPhysics()->GetGravityNormal();
		idVec3 deltaVec = (savedVelocity - GetPhysics()->GetLinearVelocity());
		idVec3 deltaVecHoriz = deltaVec - (deltaVec * vGravNorm) * vGravNorm;
		float deltaHoriz = deltaVecHoriz.LengthSqr();
		float deltaVert = deltaVec.LengthSqr() - deltaHoriz;
		
		// conversion factor to 10s of MJ/kg, horizontal and vertical weighted differently
		delta = cv_collision_damage_scale_vert.GetFloat() * deltaVert;
		delta += cv_collision_damage_scale_horiz.GetFloat() * deltaHoriz;

		// damage scale per AI:
		delta *= m_delta_scale;

		waterLevel_t waterLevel = physicsObj.GetWaterLevel();

		// reduce falling damage if there is standing water
		if( waterLevel == WATERLEVEL_HEAD )
		{
			delta *= 0.25f;
		}
		else if( waterLevel == WATERLEVEL_WAIST )
		{
			delta *= 0.5f;
		}
		else if( waterLevel == WATERLEVEL_FEET )
		{
			delta *= 0.8f;
		}
		if( delta >= m_delta_min )
		{
			pain_debounce_time = gameLocal.time + pain_delay + 1;  // ignore pain since we'll play our landing anim
			if( delta > m_delta_fatal )
			{
				Damage( NULL, NULL, idVec3( 0, 0, -1 ), "damage_fatalfall", 10.0f, 0 );
			}
			else
			{
				// damage scales linearly with delta up to fatal
				// assume a max health of 100, player-relative, damage_softfall of 10
				float dScale = cv_collision_damage_min.GetFloat() / 10.0f;
				dScale += 10.0f * (delta - m_delta_min) / m_delta_fatal;
				Damage( NULL, NULL, idVec3( 0, 0, -1 ), "damage_softfall", dScale, 0 );
			} // if( delta > fatalDelta ) else
		}// if ( delta >= 1.0f )
	} // if( !noDamage )
	return delta;
}

/* Task queue attachment and detachment */

void idActor::Event_AttachTaskQueue(int queueID)
{
	// Request the queue with the given ID from gameLocal
	CPriorityQueue* taskQueue = gameLocal.GetPriorityQueue(queueID);

	if (taskQueue != NULL)
	{
		m_TaskQueueID = queueID;
		m_TaskQueue = taskQueue;
	}
	else 
	{
		scriptThread->Error("attachTaskQueue: Priority queue #%d does not exist");
	}
}

void idActor::Event_DetachTaskQueue()
{
	m_TaskQueue = NULL;
	m_TaskQueueID = -1;
}
