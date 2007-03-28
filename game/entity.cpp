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

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#pragma warning(disable : 4533 4800)

#include "Game_local.h"
#include "../DarkMod/DarkmodGlobals.h"
#include "../DarkMod/declxdata.h"
#include "../DarkMod/PlayerData.h"
#include "../DarkMod/sndProp.h"
#include "../DarkMod/StimResponse/StimResponseCollection.h"
#include "../DarkMod/tdmInventory.h"

/*
===============================================================================

	idEntity

===============================================================================
*/

// overridable events
const idEventDef EV_PostSpawn( "<postspawn>", NULL );
const idEventDef EV_FindTargets( "<findTargets>", NULL );
const idEventDef EV_Touch( "<touch>", "et" );
const idEventDef EV_GetName( "getName", NULL, 's' );
const idEventDef EV_SetName( "setName", "s" );
const idEventDef EV_IsType ( "isType", "s" , 'd' );
const idEventDef EV_Activate( "activate", "e" );
const idEventDef EV_ActivateTargets( "activateTargets", "e" );
const idEventDef EV_NumTargets( "numTargets", NULL, 'f' );
const idEventDef EV_GetTarget( "getTarget", "f", 'e' );
const idEventDef EV_RandomTarget( "randomTarget", "s", 'e' );
const idEventDef EV_Bind( "bind", "e" );
const idEventDef EV_BindPosition( "bindPosition", "e" );
const idEventDef EV_BindToJoint( "bindToJoint", "esf" );
const idEventDef EV_GetBindMaster( "getBindMaster", NULL, 'e' );
const idEventDef EV_NumBindChildren( "numBindChildren", NULL, 'd' );
const idEventDef EV_GetBindChild( "getBindChild", "d", 'e' );
const idEventDef EV_Unbind( "unbind", NULL );
const idEventDef EV_RemoveBinds( "removeBinds" );
const idEventDef EV_SpawnBind( "<spawnbind>", NULL );
const idEventDef EV_SetOwner( "setOwner", "e" );
const idEventDef EV_SetModel( "setModel", "s" );
const idEventDef EV_SetSkin( "setSkin", "s" );
const idEventDef EV_GetWorldOrigin( "getWorldOrigin", NULL, 'v' );
const idEventDef EV_SetWorldOrigin( "setWorldOrigin", "v" );
const idEventDef EV_GetOrigin( "getOrigin", NULL, 'v' );
const idEventDef EV_SetOrigin( "setOrigin", "v" );
const idEventDef EV_GetAngles( "getAngles", NULL, 'v' );
const idEventDef EV_SetAngles( "setAngles", "v" );
const idEventDef EV_GetLinearVelocity( "getLinearVelocity", NULL, 'v' );
const idEventDef EV_SetLinearVelocity( "setLinearVelocity", "v" );
const idEventDef EV_GetAngularVelocity( "getAngularVelocity", NULL, 'v' );
const idEventDef EV_SetAngularVelocity( "setAngularVelocity", "v" );
const idEventDef EV_GetSize( "getSize", NULL, 'v' );
const idEventDef EV_SetSize( "setSize", "vv" );
const idEventDef EV_GetMins( "getMins", NULL, 'v' );
const idEventDef EV_GetMaxs( "getMaxs", NULL, 'v' );
const idEventDef EV_IsHidden( "isHidden", NULL, 'd' );
const idEventDef EV_Hide( "hide", NULL );
const idEventDef EV_Show( "show", NULL );
const idEventDef EV_Touches( "touches", "E", 'd' );
const idEventDef EV_ClearSignal( "clearSignal", "d" );
const idEventDef EV_GetShaderParm( "getShaderParm", "d", 'f' );
const idEventDef EV_SetShaderParm( "setShaderParm", "df" );
const idEventDef EV_SetShaderParms( "setShaderParms", "ffff" );
const idEventDef EV_SetColor( "setColor", "fff" );
const idEventDef EV_GetColor( "getColor", NULL, 'v' );
const idEventDef EV_CacheSoundShader( "cacheSoundShader", "s" );
const idEventDef EV_StartSoundShader( "startSoundShader", "sd", 'f' );
const idEventDef EV_StartSound( "startSound", "sdd", 'f' );
const idEventDef EV_StopSound( "stopSound", "dd" );
const idEventDef EV_FadeSound( "fadeSound", "dff" );
const idEventDef EV_GetNextKey( "getNextKey", "ss", 's' );
const idEventDef EV_SetKey( "setKey", "ss" );
const idEventDef EV_GetKey( "getKey", "s", 's' );
const idEventDef EV_GetIntKey( "getIntKey", "s", 'f' );
const idEventDef EV_GetFloatKey( "getFloatKey", "s", 'f' );
const idEventDef EV_GetVectorKey( "getVectorKey", "s", 'v' );
const idEventDef EV_GetEntityKey( "getEntityKey", "s", 'e' );
const idEventDef EV_RemoveKey( "removeKey", "s" );
const idEventDef EV_RestorePosition( "restorePosition" );
const idEventDef EV_UpdateCameraTarget( "<updateCameraTarget>", NULL );
const idEventDef EV_DistanceTo( "distanceTo", "E", 'f' );
const idEventDef EV_DistanceToPoint( "distanceToPoint", "v", 'f' );
const idEventDef EV_StartFx( "startFx", "s" );
const idEventDef EV_HasFunction( "hasFunction", "s", 'd' );
const idEventDef EV_CallFunction( "callFunction", "s" );
const idEventDef EV_SetNeverDormant( "setNeverDormant", "d" );

const idEventDef EV_InPVS( "inPVS", NULL, 'd' );
// greebo: Script event definition for dealing damage
const idEventDef EV_Damage("damage", "EEvsf");
// greebo: Script event definition for healing 
// Takes the name of the healing entity and the healing scale, returns an integer
const idEventDef EV_Heal("heal", "sf", 'd');

//===============================================================
//                   TDM GUI interface
//===============================================================
const idEventDef EV_SetGui( "setGui", "ds" );
const idEventDef EV_GetGui( "getGui", "d", 's' );
const idEventDef EV_SetGuiString( "setGuiString", "dss" );
const idEventDef EV_GetGuiString( "getGuiString", "ds", 's' );
const idEventDef EV_SetGuiFloat( "setGuiFloat", "dsf" );
const idEventDef EV_GetGuiFloat( "getGuiFloat", "ds", 'f' );
const idEventDef EV_SetGuiInt( "setGuiInt", "dsd" );
const idEventDef EV_GetGuiInt( "getGuiInt", "ds", 'f' );
const idEventDef EV_SetGuiStringFromKey( "setGuiStringFromKey", "dses" );
const idEventDef EV_CallGui( "callGui", "ds" );
const idEventDef EV_CreateOverlay( "createOverlay", "sd", 'd' );
const idEventDef EV_DestroyOverlay( "destroyOverlay", "d" );
const idEventDef EV_LoadExternalData( "loadExternalData", "ss", 'd' );


//===============================================================
//                   TDM Inventory
//===============================================================
const idEventDef EV_ReplaceItem("replaceItem", "ee", 'd');					// olditem, newitem -> 1 if succeeded
const idEventDef EV_GetNextItem("getNextItem", "d", 'e');
const idEventDef EV_GetPrevItem("getPrevItem", "d", 'e');
const idEventDef EV_SetCursorGroup("setCursorGroup", "s", 'd');				// groupname -> 1 = success
const idEventDef EV_SetCursorGroupItem("setCursorGroupItem", "ss", 'd');	// itemname, groupname -> 1 = success
const idEventDef EV_SetCursorItem("setCursorItem", "s", 'd');				// itemname -> 1 = success
const idEventDef EV_GetCursorGroup("getCursorGroup", NULL, 's');
const idEventDef EV_GetCursorItem("getCursorItem", NULL, 'e');
const idEventDef EV_AddItem("addItem", "e");								// entityitem
const idEventDef EV_GetGroupItem("getGroupItem", "ss", 'e');				// itemname, groupname -> NULL not in group
const idEventDef EV_GetItem("getItem", "s", 'e');							// itemname -> NULL not in any group
const idEventDef EV_GetLoot("getLoot", "d", 'd');							// returns the current value for the given group
const idEventDef EV_AddToInventory("addToInventory", "e");					// Adds an item to the inventory


// The Dark Mod Stim/Response interface functions for scripting
// Normally I don't like names, which are "the other way around"
// but I think in this case it would be ok, because the interface
// for stims and responses are pretty much the same.
const idEventDef EV_StimAdd( "StimAdd", "df" );
const idEventDef EV_StimRemove( "StimRemove", "d" );
const idEventDef EV_StimEnable( "StimEnable", "dd" );
const idEventDef EV_StimClearIgnoreList( "StimClearIgnoreList", "d" );
const idEventDef EV_ResponseEnable( "ResponseEnable", "dd" );
const idEventDef EV_ResponseAdd( "ResponseAdd", "d" );
const idEventDef EV_ResponseRemove( "ResponseRemove", "d" );
const idEventDef EV_ResponseIgnore( "ResponseIgnore", "de" );
const idEventDef EV_ResponseAllow( "ResponseAllow", "de" );
const idEventDef EV_ResponseSetAction( "ResponseSetAction", "ds" );

// StimType, Hours, minutes, seconds, miliseconds(?)
const idEventDef EV_TimerCreate( "CreateTimer", "ddddd" );
const idEventDef EV_TimerStop( "StopTimer", "d" );
const idEventDef EV_TimerStart( "StartTimer", "d" );
const idEventDef EV_TimerRestart( "RestartTimer", "d" );
const idEventDef EV_TimerReset( "ResetTimer", "d" );
const idEventDef EV_TimerSetState( "SetTimerState", "dd" );

// soundprop event: Propagate sound directly from scripting
const idEventDef EV_TDM_PropSoundMod( "propSoundMod", "sf" );
// I don't think scripting supports optional argument, so I must do this
const idEventDef EV_TDM_PropSound( "propSound", "s" );

const idEventDef EV_TDM_SDKSignal( "SDKSignal", "dd" );

// For detecting ranged enemies. Returns nonzero if this entity could
// potentially attack the given entity (first parameter) at range.
const idEventDef EV_TDM_RangedThreatTo( "rangedThreatTo", "e", 'f' );


#ifdef MOD_WATERPHYSICS

const idEventDef EV_GetMass( "getMass", "d" , 'f' );

const idEventDef EV_IsInLiquid( "isInLiquid", NULL, 'd' );

#endif      // MOD_WATERPHYSICS

const idEventDef EV_CopyBind( "copyBind", "e" );
const idEventDef EV_IsFrobable( "isFrobable", NULL, 'd' );
const idEventDef EV_SetFrobable( "setFrobable", "d" );
const idEventDef EV_IsHilighted( "isHilighted", NULL, 'd' );


ABSTRACT_DECLARATION( idClass, idEntity )
	EVENT( EV_Thread_SetRenderCallback,	idEntity::Event_WaitForRender )

	EVENT( EV_GetName,				idEntity::Event_GetName )
	EVENT( EV_SetName,				idEntity::Event_SetName )
	EVENT (EV_IsType,				idEntity::Event_IsType )
	EVENT( EV_FindTargets,			idEntity::Event_FindTargets )
	EVENT( EV_ActivateTargets,		idEntity::Event_ActivateTargets )
	EVENT( EV_NumTargets,			idEntity::Event_NumTargets )
	EVENT( EV_GetTarget,			idEntity::Event_GetTarget )
	EVENT( EV_RandomTarget,			idEntity::Event_RandomTarget )
	EVENT( EV_BindToJoint,			idEntity::Event_BindToJoint )
	EVENT( EV_RemoveBinds,			idEntity::Event_RemoveBinds )
	EVENT( EV_Bind,					idEntity::Event_Bind )
	EVENT( EV_BindPosition,			idEntity::Event_BindPosition )
	EVENT( EV_Unbind,				idEntity::Event_Unbind )
	EVENT( EV_SpawnBind,			idEntity::Event_SpawnBind )
	EVENT( EV_GetBindMaster,		idEntity::Event_GetBindMaster )
	EVENT( EV_NumBindChildren,		idEntity::Event_NumBindChildren )
	EVENT( EV_GetBindChild,			idEntity::Event_GetBindChild )
	EVENT( EV_SetOwner,				idEntity::Event_SetOwner )
	EVENT( EV_SetModel,				idEntity::Event_SetModel )
	EVENT( EV_SetSkin,				idEntity::Event_SetSkin )
	EVENT( EV_GetShaderParm,		idEntity::Event_GetShaderParm )
	EVENT( EV_SetShaderParm,		idEntity::Event_SetShaderParm )
	EVENT( EV_SetShaderParms,		idEntity::Event_SetShaderParms )
	EVENT( EV_SetColor,				idEntity::Event_SetColor )
	EVENT( EV_GetColor,				idEntity::Event_GetColor )
	EVENT( EV_IsHidden,				idEntity::Event_IsHidden )
	EVENT( EV_Hide,					idEntity::Event_Hide )
	EVENT( EV_Show,					idEntity::Event_Show )
	EVENT( EV_CacheSoundShader,		idEntity::Event_CacheSoundShader )
	EVENT( EV_StartSoundShader,		idEntity::Event_StartSoundShader )
	EVENT( EV_StartSound,			idEntity::Event_StartSound )
	EVENT( EV_StopSound,			idEntity::Event_StopSound )
	EVENT( EV_FadeSound,			idEntity::Event_FadeSound )
	EVENT( EV_GetWorldOrigin,		idEntity::Event_GetWorldOrigin )
	EVENT( EV_SetWorldOrigin,		idEntity::Event_SetWorldOrigin )
	EVENT( EV_GetOrigin,			idEntity::Event_GetOrigin )
	EVENT( EV_SetOrigin,			idEntity::Event_SetOrigin )
	EVENT( EV_GetAngles,			idEntity::Event_GetAngles )
	EVENT( EV_SetAngles,			idEntity::Event_SetAngles )
	EVENT( EV_GetLinearVelocity,	idEntity::Event_GetLinearVelocity )
	EVENT( EV_SetLinearVelocity,	idEntity::Event_SetLinearVelocity )
	EVENT( EV_GetAngularVelocity,	idEntity::Event_GetAngularVelocity )
	EVENT( EV_SetAngularVelocity,	idEntity::Event_SetAngularVelocity )
	EVENT( EV_GetSize,				idEntity::Event_GetSize )
	EVENT( EV_SetSize,				idEntity::Event_SetSize )
	EVENT( EV_GetMins,				idEntity::Event_GetMins)
	EVENT( EV_GetMaxs,				idEntity::Event_GetMaxs )
	EVENT( EV_Touches,				idEntity::Event_Touches )
	EVENT( EV_GetNextKey,			idEntity::Event_GetNextKey )
	EVENT( EV_SetKey,				idEntity::Event_SetKey )
	EVENT( EV_GetKey,				idEntity::Event_GetKey )
	EVENT( EV_GetIntKey,			idEntity::Event_GetIntKey )
	EVENT( EV_GetFloatKey,			idEntity::Event_GetFloatKey )
	EVENT( EV_GetVectorKey,			idEntity::Event_GetVectorKey )
	EVENT( EV_GetEntityKey,			idEntity::Event_GetEntityKey )
	EVENT( EV_RemoveKey,			idEntity::Event_RemoveKey )
	EVENT( EV_RestorePosition,		idEntity::Event_RestorePosition )
	EVENT( EV_UpdateCameraTarget,	idEntity::Event_UpdateCameraTarget )
	EVENT( EV_DistanceTo,			idEntity::Event_DistanceTo )
	EVENT( EV_DistanceToPoint,		idEntity::Event_DistanceToPoint )
	EVENT( EV_StartFx,				idEntity::Event_StartFx )
	EVENT( EV_Thread_WaitFrame,		idEntity::Event_WaitFrame )
	EVENT( EV_Thread_Wait,			idEntity::Event_Wait )
	EVENT( EV_HasFunction,			idEntity::Event_HasFunction )
	EVENT( EV_CallFunction,			idEntity::Event_CallFunction )
	EVENT( EV_SetNeverDormant,		idEntity::Event_SetNeverDormant )

	EVENT( EV_InPVS,				idEntity::Event_InPVS )
	EVENT( EV_Damage,				idEntity::Event_Damage )
	EVENT( EV_Heal,					idEntity::Event_Heal )

	EVENT( EV_SetGui,				idEntity::Event_SetGui )
	EVENT( EV_GetGui,				idEntity::Event_GetGui )
	EVENT( EV_SetGuiString,			idEntity::Event_SetGuiString )
	EVENT( EV_GetGuiString,			idEntity::Event_GetGuiString )
	EVENT( EV_SetGuiFloat,			idEntity::Event_SetGuiFloat )
	EVENT( EV_GetGuiFloat,			idEntity::Event_GetGuiFloat )
	EVENT( EV_SetGuiInt,			idEntity::Event_SetGuiInt )
	EVENT( EV_GetGuiInt,			idEntity::Event_GetGuiInt )
	EVENT( EV_SetGuiStringFromKey,	idEntity::Event_SetGuiStringFromKey )
	EVENT( EV_CallGui,				idEntity::Event_CallGui )
	EVENT( EV_CreateOverlay,		idEntity::Event_CreateOverlay )
	EVENT( EV_DestroyOverlay,		idEntity::Event_DestroyOverlay )

	EVENT( EV_LoadExternalData,		idEntity::Event_LoadExternalData )

	EVENT( EV_ReplaceItem,			idEntity::Event_ReplaceItem )
	EVENT( EV_GetNextItem,			idEntity::Event_GetNextItem )
	EVENT( EV_GetPrevItem,			idEntity::Event_GetPrevItem )
	EVENT( EV_SetCursorGroup,		idEntity::Event_SetCursorGroup )
	EVENT( EV_SetCursorGroupItem,	idEntity::Event_SetCursorGroupItem )
	EVENT( EV_SetCursorItem,		idEntity::Event_SetCursorItem )
	EVENT( EV_GetCursorGroup,		idEntity::Event_GetCursorGroup )
	EVENT( EV_GetCursorItem,		idEntity::Event_GetCursorItem )
	EVENT( EV_AddItem,				idEntity::Event_AddItem )
	EVENT( EV_GetGroupItem,			idEntity::Event_GetGroupItem )
	EVENT( EV_GetItem,				idEntity::Event_GetItem )
	EVENT( EV_GetLoot,				idEntity::Event_GetLoot )
	EVENT( EV_AddToInventory,		idEntity::AddToInventory )

	EVENT( EV_StimAdd,				idEntity::StimAdd)
	EVENT( EV_StimRemove,			idEntity::StimRemove)
	EVENT( EV_StimEnable,			idEntity::StimEnable)
	EVENT( EV_StimClearIgnoreList,	idEntity::StimClearIgnoreList)
	EVENT( EV_ResponseEnable,		idEntity::ResponseEnable)
	EVENT( EV_ResponseAdd,			idEntity::ResponseAdd)
	EVENT( EV_ResponseRemove,		idEntity::ResponseRemove)
	EVENT( EV_ResponseIgnore,		idEntity::ResponseIgnore)
	EVENT( EV_ResponseAllow,		idEntity::ResponseAllow)
	EVENT( EV_ResponseSetAction,	idEntity::ResponseSetAction)

	EVENT( EV_TimerCreate,			idEntity::Event_TimerCreate )
	EVENT( EV_TimerStop,			idEntity::Event_TimerStop )
	EVENT( EV_TimerStart,			idEntity::Event_TimerStart )
	EVENT( EV_TimerRestart,			idEntity::Event_TimerRestart )
	EVENT( EV_TimerReset,			idEntity::Event_TimerReset )
	EVENT( EV_TimerSetState,		idEntity::Event_TimerSetState )

	EVENT( EV_TDM_PropSound,		idEntity::Event_PropSound )
	EVENT( EV_TDM_PropSoundMod,		idEntity::Event_PropSoundMod )

	EVENT( EV_TDM_SDKSignal,		idEntity::SDKSignal )

	EVENT( EV_TDM_RangedThreatTo,	idEntity::Event_RangedThreatTo )

#ifdef MOD_WATERPHYSICS

	EVENT( EV_GetMass,              idEntity::Event_GetMass )

	EVENT( EV_IsInLiquid,           idEntity::Event_IsInLiquid )

#endif		// MOD_WATERPHYSICS

	EVENT( EV_CopyBind,				idEntity::Event_CopyBind )
	EVENT( EV_IsFrobable,			idEntity::Event_IsFrobable )
	EVENT( EV_SetFrobable,			idEntity::Event_SetFrobable )
	EVENT( EV_IsHilighted,			idEntity::Event_IsHilighted )

END_CLASS

/*
================
UpdateGuiParms
================
*/
void UpdateGuiParms( idUserInterface *gui, const idDict *args ) {
	if ( gui == NULL || args == NULL ) {
		return;
	}
	const idKeyValue *kv = args->MatchPrefix( "gui_parm", NULL );
	while( kv ) {
		gui->SetStateString( kv->GetKey(), kv->GetValue() );
		kv = args->MatchPrefix( "gui_parm", kv );
	}
	gui->SetStateBool( "noninteractive",  args->GetBool( "gui_noninteractive" ) ) ;
	gui->StateChanged( gameLocal.time );
}

/*
================
AddRenderGui
================
*/
void AddRenderGui( const char *name, idUserInterface **gui, const idDict *args ) {
	const idKeyValue *kv = args->MatchPrefix( "gui_parm", NULL );
	*gui = uiManager->FindGui( name, true, ( kv != NULL ) );
	UpdateGuiParms( *gui, args );
}

/*
================
idGameEdit::ParseSpawnArgsToRenderEntity

parse the static model parameters
this is the canonical renderEntity parm parsing,
which should be used by dmap and the editor
================
*/
void idGameEdit::ParseSpawnArgsToRenderEntity( const idDict *args, renderEntity_t *renderEntity ) {
	int			i;
	const char	*temp;
	idVec3		color;
	float		angle;
	const idDeclModelDef *modelDef;

	memset( renderEntity, 0, sizeof( *renderEntity ) );

	temp = args->GetString( "model" );

	modelDef = NULL;
	if ( temp[0] != '\0' ) {
		if ( !strstr( temp, "." ) ) {

			// FIXME: temp hack to replace obsolete ips particles with prt particles
			if ( !strstr( temp, ".ips" ) ) {
				idStr str = temp;
				str.Replace( ".ips", ".prt" );
				modelDef = static_cast<const idDeclModelDef *>( declManager->FindType( DECL_MODELDEF, str, false ) );
			} else {
				modelDef = static_cast<const idDeclModelDef *>( declManager->FindType( DECL_MODELDEF, temp, false ) );
			}
			if ( modelDef ) {
				renderEntity->hModel = modelDef->ModelHandle();
			}
		}

		if ( !renderEntity->hModel ) {
			renderEntity->hModel = renderModelManager->FindModel( temp );
		}
	}
	if ( renderEntity->hModel ) {
		renderEntity->bounds = renderEntity->hModel->Bounds( renderEntity );
	} else {
		renderEntity->bounds.Zero();
	}

	temp = args->GetString( "skin" );
	if ( temp[0] != '\0' ) {
		renderEntity->customSkin = declManager->FindSkin( temp );
	} else if ( modelDef ) {
		renderEntity->customSkin = modelDef->GetDefaultSkin();
	}

	temp = args->GetString( "shader" );
	if ( temp[0] != '\0' ) {
		renderEntity->customShader = declManager->FindMaterial( temp );
	}

	args->GetVector( "origin", "0 0 0", renderEntity->origin );

	// get the rotation matrix in either full form, or single angle form
	if ( !args->GetMatrix( "rotation", "1 0 0 0 1 0 0 0 1", renderEntity->axis ) ) {
		angle = args->GetFloat( "angle" );
		if ( angle != 0.0f ) {
			renderEntity->axis = idAngles( 0.0f, angle, 0.0f ).ToMat3();
		} else {
			renderEntity->axis.Identity();
		}
	}

	renderEntity->referenceSound = NULL;

	// get shader parms
	args->GetVector( "_color", "1 1 1", color );
	renderEntity->shaderParms[ SHADERPARM_RED ]		= color[0];
	renderEntity->shaderParms[ SHADERPARM_GREEN ]	= color[1];
	renderEntity->shaderParms[ SHADERPARM_BLUE ]	= color[2];
	renderEntity->shaderParms[ 3 ]					= args->GetFloat( "shaderParm3", "1" );
	renderEntity->shaderParms[ 4 ]					= args->GetFloat( "shaderParm4", "0" );
	renderEntity->shaderParms[ 5 ]					= args->GetFloat( "shaderParm5", "0" );
	renderEntity->shaderParms[ 6 ]					= args->GetFloat( "shaderParm6", "0" );
	renderEntity->shaderParms[ 7 ]					= args->GetFloat( "shaderParm7", "0" );
	renderEntity->shaderParms[ 8 ]					= args->GetFloat( "shaderParm8", "0" );
	renderEntity->shaderParms[ 9 ]					= args->GetFloat( "shaderParm9", "0" );
	renderEntity->shaderParms[ 10 ]					= args->GetFloat( "shaderParm10", "0" );
	renderEntity->shaderParms[ 11 ]	= args->GetFloat( "shaderParm11", "0" );

	// check noDynamicInteractions flag
	renderEntity->noDynamicInteractions = args->GetBool( "noDynamicInteractions" );

	// check noshadows flag
	renderEntity->noShadow = args->GetBool( "noshadows" );

	// check noselfshadows flag
	renderEntity->noSelfShadow = args->GetBool( "noselfshadows" );

	// init any guis, including entity-specific states
	for( i = 0; i < MAX_RENDERENTITY_GUI; i++ ) {
		temp = args->GetString( i == 0 ? "gui" : va( "gui%d", i + 1 ) );
		if ( temp[ 0 ] != '\0' ) {
			AddRenderGui( temp, &renderEntity->gui[ i ], args );
		}
	}
}

/*
================
idGameEdit::ParseSpawnArgsToRefSound

parse the sound parameters
this is the canonical refSound parm parsing,
which should be used by dmap and the editor
================
*/
void idGameEdit::ParseSpawnArgsToRefSound( const idDict *args, refSound_t *refSound ) {
	const char	*temp;

	memset( refSound, 0, sizeof( *refSound ) );

	refSound->parms.minDistance = args->GetFloat( "s_mindistance" );
	refSound->parms.maxDistance = args->GetFloat( "s_maxdistance" );
	refSound->parms.volume = args->GetFloat( "s_volume" );
	refSound->parms.shakes = args->GetFloat( "s_shakes" );

	args->GetVector( "origin", "0 0 0", refSound->origin );

	refSound->referenceSound  = NULL;

	// if a diversity is not specified, every sound start will make
	// a random one.  Specifying diversity is usefull to make multiple
	// lights all share the same buzz sound offset, for instance.
	refSound->diversity = args->GetFloat( "s_diversity", "-1" );
	refSound->waitfortrigger = args->GetBool( "s_waitfortrigger" );

	if ( args->GetBool( "s_omni" ) ) {
		refSound->parms.soundShaderFlags |= SSF_OMNIDIRECTIONAL;
	}
	if ( args->GetBool( "s_looping" ) ) {
		refSound->parms.soundShaderFlags |= SSF_LOOPING;
	}
	if ( args->GetBool( "s_occlusion" ) ) {
		refSound->parms.soundShaderFlags |= SSF_NO_OCCLUSION;
	}
	if ( args->GetBool( "s_global" ) ) {
		refSound->parms.soundShaderFlags |= SSF_GLOBAL;
	}
	if ( args->GetBool( "s_unclamped" ) ) {
		refSound->parms.soundShaderFlags |= SSF_UNCLAMPED;
	}
	refSound->parms.soundClass = args->GetInt( "s_soundClass" );

	temp = args->GetString( "s_shader" );
	if ( temp[0] != '\0' ) {
		refSound->shader = declManager->FindSound( temp );
	}
}

/*
===============
idEntity::UpdateChangeableSpawnArgs

Any key val pair that might change during the course of the game ( via a gui or whatever )
should be initialize here so a gui or other trigger can change something and have it updated
properly. An optional source may be provided if the values reside in an outside dictionary and
first need copied over to spawnArgs
===============
*/
void idEntity::UpdateChangeableSpawnArgs( const idDict *source ) {
	int i;
	const char *target;

	if ( !source ) {
		source = &spawnArgs;
	}
	cameraTarget = NULL;
	target = source->GetString( "cameraTarget" );
	if ( target && target[0] ) {
		// update the camera taget
		PostEventMS( &EV_UpdateCameraTarget, 0 );
	}

	for ( i = 0; i < MAX_RENDERENTITY_GUI; i++ ) {
		UpdateGuiParms( renderEntity.gui[ i ], source );
	}
}

/*
================
idEntity::idEntity
================
*/
idEntity::idEntity()
{
	DM_LOG(LC_FUNCTION, LT_DEBUG)LOGSTRING("this: %08lX %s\r", this, __FUNCTION__);

	m_Signal = 0;
	entityNumber	= ENTITYNUM_NONE;
	entityDefNumber = -1;

	spawnNode.SetOwner( this );
	activeNode.SetOwner( this );

	snapshotNode.SetOwner( this );
	snapshotSequence = -1;
	snapshotBits = 0;

	thinkFlags		= 0;
	dormantStart	= 0;
	cinematic		= false;
	renderView		= NULL;
	cameraTarget	= NULL;
	health			= 0;

	physics			= NULL;
	bindMaster		= NULL;
	bindJoint		= INVALID_JOINT;
	bindBody		= -1;
	teamMaster		= NULL;
	teamChain		= NULL;
	signals			= NULL;

	memset( PVSAreas, 0, sizeof( PVSAreas ) );
	numPVSAreas		= -1;

	memset( &fl, 0, sizeof( fl ) );
	fl.neverDormant	= true;			// most entities never go dormant

	memset( &renderEntity, 0, sizeof( renderEntity ) );
	modelDefHandle	= -1;
	memset( &refSound, 0, sizeof( refSound ) );

	memset( &m_renderTrigger, 0, sizeof( m_renderTrigger ) );
	m_renderTrigger.axis.Identity();
	m_renderTriggerHandle = -1;
	m_renderWaitingThread = 0;

	mpGUIState = -1;

	m_bFrobable = false;
	m_FrobDistance = 0;
	m_FrobBias = 1.0f;
	m_FrobActionScript = "";
	m_bFrobbed = false;
	m_bFrobHighlightState = false;
	m_FrobChangeTime = 0;
	m_FrobPeerFloodFrame = 0;
	m_bIsObjective = false;
	m_bIsClimbableRope = false;
	m_bIsMantleable = false;

	// We give all the entities a Stim/Response collection so that we wont have to worry
	// about the pointer being available all the time. The memory footprint of that 
	// object is rather small, so this doesn't really hurt that 
	// much and makes the code easier to control.
	m_StimResponseColl = new CStimResponseCollection;

	// Since we have a function to return inventories/etc, and many entities won't
	// have anything to do with inventories, I figure I'd better wait until
	// absolutely necessary to create these.
	m_Inventory			= NULL;
	m_InventoryItem		= NULL;
	m_InventoryCursor	= NULL;
}

/*
================
idEntity::FixupLocalizedStrings
================
*/
void idEntity::FixupLocalizedStrings()
{
	for ( int i = 0; i < spawnArgs.GetNumKeyVals(); i++ ) {
		const idKeyValue *kv = spawnArgs.GetKeyVal( i );
		if ( idStr::Cmpn( kv->GetValue(), STRTABLE_ID, STRTABLE_ID_LENGTH ) == 0 ){
			spawnArgs.Set( kv->GetKey(), common->GetLanguageDict()->GetString( kv->GetValue() ) );
		}
	}
}

/*
================
idEntity::Spawn
================
*/
void idEntity::Spawn( void )
{
	int					i;
	const char			*temp;
	idVec3				origin;
	idMat3				axis;
	const idKeyValue	*networkSync;
	const char			*classname;
	const char			*scriptObjectName;
	idEntity			*ent;

	gameLocal.RegisterEntity( this );

	spawnArgs.GetString( "classname", NULL, &classname );
	const idDeclEntityDef *def = gameLocal.FindEntityDef( classname, false );
	if ( def ) {
		entityDefNumber = def->Index();
	}

	FixupLocalizedStrings();

	// parse static models the same way the editor display does
	gameEdit->ParseSpawnArgsToRenderEntity( &spawnArgs, &renderEntity );

	renderEntity.entityNum = entityNumber;
	
	// go dormant within 5 frames so that when the map starts most monsters are dormant
	dormantStart = gameLocal.time - DELAY_DORMANT_TIME + gameLocal.msec * 5;

	origin = renderEntity.origin;
	axis = renderEntity.axis;

	// do the audio parsing the same way dmap and the editor do
	gameEdit->ParseSpawnArgsToRefSound( &spawnArgs, &refSound );

	// only play SCHANNEL_PRIVATE when sndworld->PlaceListener() is called with this listenerId
	// don't spatialize sounds from the same entity
	refSound.listenerId = entityNumber + 1;

	cameraTarget = NULL;
	temp = spawnArgs.GetString( "cameraTarget" );
	if ( temp && temp[0] ) {
		// update the camera taget
		PostEventMS( &EV_UpdateCameraTarget, 0 );
	}

	for ( i = 0; i < MAX_RENDERENTITY_GUI; i++ ) {
		UpdateGuiParms( renderEntity.gui[ i ], &spawnArgs );

		// Add the (potentially NULL) GUI as an external GUI to our overlaysys,
		// so that the script functions to interact with GUIs can interact with it.
		if ( m_overlays.createOverlay( 0, OVERLAYS_MIN_HANDLE + i ) >= OVERLAYS_MIN_HANDLE )
			m_overlays.setGui( OVERLAYS_MIN_HANDLE + i, renderEntity.gui[ i ] );
		else
			gameLocal.Warning( "Unable to create overlay for renderentity GUI: %d\n", OVERLAYS_MIN_HANDLE + i );
	}

	fl.solidForTeam = spawnArgs.GetBool( "solidForTeam", "0" );
	fl.neverDormant = spawnArgs.GetBool( "neverDormant", "0" );
	fl.hidden = spawnArgs.GetBool( "hide", "0" );
	if ( fl.hidden ) {
		// make sure we're hidden, since a spawn function might not set it up right
		PostEventMS( &EV_Hide, 0 );
	}
	cinematic = spawnArgs.GetBool( "cinematic", "0" );

	networkSync = spawnArgs.FindKey( "networkSync" );
	if ( networkSync ) {
		fl.networkSync = ( atoi( networkSync->GetValue() ) != 0 );
	}

#if 0
	if ( !gameLocal.isClient ) {
		// common->DPrintf( "NET: DBG %s - %s is synced: %s\n", spawnArgs.GetString( "classname", "" ), GetType()->classname, fl.networkSync ? "true" : "false" );
		if ( spawnArgs.GetString( "classname", "" )[ 0 ] == '\0' && !fl.networkSync ) {
			common->DPrintf( "NET: WRN %s entity, no classname, and no networkSync?\n", GetType()->classname );
		}
	}
#endif

	// every object will have a unique name
	temp = spawnArgs.GetString( "name", va( "%s_%s_%d", GetClassname(), spawnArgs.GetString( "classname" ), entityNumber));
	SetName(temp);
	DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING("this: %08lX   Name: [%s]\r", this, temp);

	// if we have targets, wait until all entities are spawned to get them
	if ( spawnArgs.MatchPrefix( "target" ) || spawnArgs.MatchPrefix( "guiTarget" ) ) {
		if ( gameLocal.GameState() == GAMESTATE_STARTUP ) {
			PostEventMS( &EV_FindTargets, 0 );
		} else {
			// not during spawn, so it's ok to get the targets
			FindTargets();
		}
	}

	health = spawnArgs.GetInt( "health" );

	InitDefaultPhysics( origin, axis );

	SetOrigin( origin );
	SetAxis( axis );

	temp = spawnArgs.GetString( "model" );
	if ( temp && *temp ) 
	{
		SetModel( temp );
	}

	if ( spawnArgs.GetString( "bind", "", &temp ) ) 
	{
		PostEventMS( &EV_SpawnBind, 0 );
	}

	//TDM: Spawn and attach any attachments
	const idKeyValue *kv = spawnArgs.MatchPrefix( "def_attach", NULL );
	while ( kv ) {
		idDict args;

		args.Set( "classname", kv->GetValue().c_str() );

		// make items non-touchable so the player can't take them out of the character's hands
		args.Set( "no_touch", "1" );

		// don't let them drop to the floor
		args.Set( "dropToFloor", "0" );
		
		gameLocal.SpawnEntityDef( args, &ent );
		if ( !ent ) {
			gameLocal.Error( "Couldn't spawn '%s' to attach to entity '%s'", kv->GetValue().c_str(), name.c_str() );
		} else {
			Attach( ent );
		}
		kv = spawnArgs.MatchPrefix( "def_attach", kv );
	}


	// auto-start a sound on the entity
	if ( refSound.shader && !refSound.waitfortrigger ) {
		StartSoundShader( refSound.shader, SND_CHANNEL_ANY, 0, false, NULL );
	}

	// setup script object
	if ( ShouldConstructScriptObjectAtSpawn() && spawnArgs.GetString( "scriptobject", NULL, &scriptObjectName ) ) {
		if ( !scriptObject.SetType( scriptObjectName ) ) {
			gameLocal.Error( "Script object '%s' not found on entity '%s'.", scriptObjectName, name.c_str() );
		}

		ConstructScriptObject();
	}
	
	// TDM: Set contents based on new spawnArg
	if( spawnArgs.FindKey( "clipmodel_contents" ) )
		GetPhysics()->SetContents( spawnArgs.GetInt("clipmodel_contents") );

	m_StimResponseColl->ParseSpawnArgsToStimResponse(&spawnArgs, this);

	// Check if some entities exists that want to get stuffed in our own
	// inventory.
	CheckInventoryInit();

	// Check if we have to put ourselve into someones inventory.
	InitInventory();

	LoadTDMSettings();
}

/*
================
idEntity::~idEntity
================
*/
idEntity::~idEntity( void )
{
	DM_LOG(LC_FUNCTION, LT_DEBUG)LOGSTRING("this: %08lX [%s]\r", this, __FUNCTION__);

	gameLocal.RemoveResponse(this);
	gameLocal.RemoveStim(this);

	if ( gameLocal.GameState() != GAMESTATE_SHUTDOWN && !gameLocal.isClient && fl.networkSync && entityNumber >= MAX_CLIENTS ) {
		idBitMsg	msg;
		byte		msgBuf[ MAX_GAME_MESSAGE_SIZE ];

		msg.Init( msgBuf, sizeof( msgBuf ) );
		msg.WriteByte( GAME_RELIABLE_MESSAGE_DELETE_ENT );
		msg.WriteBits( gameLocal.GetSpawnId( this ), 32 );
		networkSystem->ServerSendReliableMessage( -1, msg );
	}

	DeconstructScriptObject();
	scriptObject.Free();

	if ( thinkFlags ) {
		BecomeInactive( thinkFlags );
	}
	activeNode.Remove();

	Signal( SIG_REMOVED );

	// we have to set back the default physics object before unbinding because the entity
	// specific physics object might be an entity variable and as such could already be destroyed.
	SetPhysics( NULL );

	// remove any entities that are bound to me
	RemoveBinds();

	// unbind from master
	Unbind();
	QuitTeam();

	gameLocal.RemoveEntityFromHash( name.c_str(), this );

	delete renderView;
	renderView = NULL;

	delete signals;
	signals = NULL;

	FreeModelDef();
	FreeSoundEmitter( false );

	if ( m_renderTriggerHandle != -1 ) {
		gameRenderWorld->FreeEntityDef( m_renderTriggerHandle );
	}

	gameLocal.UnregisterEntity( this );
	gameLocal.RemoveStim(this);
	gameLocal.RemoveResponse(this);
	delete m_StimResponseColl;

	// Delete our inventory/item/cursor, if necessary.
	if ( m_Inventory != NULL )
		delete m_Inventory;

	m_FrobPeers.Clear();
}

/*
================
idEntity::Save
================
*/
void idEntity::Save( idSaveGame *savefile ) const
{
	int i, j;

	savefile->WriteInt( entityNumber );
	savefile->WriteInt( entityDefNumber );

	// spawnNode and activeNode are restored by gameLocal

	savefile->WriteInt( snapshotSequence );
	savefile->WriteInt( snapshotBits );

	savefile->WriteDict( &spawnArgs );
	savefile->WriteString( name );
	scriptObject.Save( savefile );

	savefile->WriteInt( thinkFlags );
	savefile->WriteInt( dormantStart );
	savefile->WriteBool( cinematic );

	savefile->WriteObject( cameraTarget );

	savefile->WriteInt( health );

	savefile->WriteInt( targets.Num() );
	for( i = 0; i < targets.Num(); i++ ) {
		targets[ i ].Save( savefile );
	}

	entityFlags_s flags = fl;

	LittleBitField( &flags, sizeof( flags ) );

	savefile->Write( &flags, sizeof( flags ) );


	savefile->WriteRenderEntity( renderEntity );
	savefile->WriteInt( modelDefHandle );
	savefile->WriteRefSound( refSound );

	savefile->WriteRenderEntity( m_renderTrigger );
	savefile->WriteInt( m_renderTriggerHandle );
	savefile->WriteInt( m_renderWaitingThread );

	m_overlays.Save( savefile );

	savefile->WriteObject( bindMaster );
	savefile->WriteJoint( bindJoint );
	savefile->WriteInt( bindBody );
	savefile->WriteObject( teamMaster );
	savefile->WriteObject( teamChain );

	savefile->WriteStaticObject( defaultPhysicsObj );

	savefile->WriteInt( numPVSAreas );
	for( i = 0; i < MAX_PVS_AREAS; i++ ) {
		savefile->WriteInt( PVSAreas[ i ] );
	}

	if ( !signals ) {
		savefile->WriteBool( false );
	} else {
		savefile->WriteBool( true );
		for( i = 0; i < NUM_SIGNALS; i++ ) {
			savefile->WriteInt( signals->signal[ i ].Num() );
			for( j = 0; j < signals->signal[ i ].Num(); j++ ) {
				savefile->WriteInt( signals->signal[ i ][ j ].threadnum );
				savefile->WriteString( signals->signal[ i ][ j ].function->Name() );
			}
		}
	}

	savefile->WriteInt( mpGUIState );
	savefile->WriteObject(m_Inventory);
}

/*
================
idEntity::Restore
================
*/
void idEntity::Restore( idRestoreGame *savefile )
{
	int			i, j;
	int			num;
	idStr		funcname;

	savefile->ReadInt( entityNumber );
	savefile->ReadInt( entityDefNumber );

	// spawnNode and activeNode are restored by gameLocal

	savefile->ReadInt( snapshotSequence );
	savefile->ReadInt( snapshotBits );

	savefile->ReadDict( &spawnArgs );
	savefile->ReadString( name );
	SetName( name );

	scriptObject.Restore( savefile );

	savefile->ReadInt( thinkFlags );
	savefile->ReadInt( dormantStart );
	savefile->ReadBool( cinematic );

	savefile->ReadObject( reinterpret_cast<idClass *&>( cameraTarget ) );

	savefile->ReadInt( health );

	targets.Clear();
	savefile->ReadInt( num );
	targets.SetNum( num );
	for( i = 0; i < num; i++ ) {
		targets[ i ].Restore( savefile );
	}

	savefile->Read( &fl, sizeof( fl ) );
	LittleBitField( &fl, sizeof( fl ) );

	savefile->ReadRenderEntity( renderEntity );
	savefile->ReadInt( modelDefHandle );
	savefile->ReadRefSound( refSound );

	savefile->ReadRenderEntity( m_renderTrigger );
	savefile->ReadInt( m_renderTriggerHandle );
	savefile->ReadInt( m_renderWaitingThread );

	m_overlays.Restore( savefile );
	for ( i = 0; i < MAX_RENDERENTITY_GUI; i++ ) {
		// Add the (potentially NULL) GUI as an external GUI to our overlaysys,
		// so that the script functions to interact with GUIs can interact with it.
		// If the overlay isn't external, then the entity inheriting us must have
		// messed with the overlays, and doesn't want us to set it. If it IS
		// external, then they'll be resetting it anyways, so no harm done.
		if ( m_overlays.isExternal( OVERLAYS_MIN_HANDLE + i ) )
			m_overlays.setGui( OVERLAYS_MIN_HANDLE + i, renderEntity.gui[ i ] );
	}

	savefile->ReadObject( reinterpret_cast<idClass *&>( bindMaster ) );
	savefile->ReadJoint( bindJoint );
	savefile->ReadInt( bindBody );
	savefile->ReadObject( reinterpret_cast<idClass *&>( teamMaster ) );
	savefile->ReadObject( reinterpret_cast<idClass *&>( teamChain ) );

	savefile->ReadStaticObject( defaultPhysicsObj );
	RestorePhysics( &defaultPhysicsObj );

	savefile->ReadInt( numPVSAreas );
	for( i = 0; i < MAX_PVS_AREAS; i++ ) {
		savefile->ReadInt( PVSAreas[ i ] );
	}

	bool readsignals;
	savefile->ReadBool( readsignals );
	if ( readsignals ) {
		signals = new signalList_t;
		for( i = 0; i < NUM_SIGNALS; i++ ) {
			savefile->ReadInt( num );
			signals->signal[ i ].SetNum( num );
			for( j = 0; j < num; j++ ) {
				savefile->ReadInt( signals->signal[ i ][ j ].threadnum );
				savefile->ReadString( funcname );
				signals->signal[ i ][ j ].function = gameLocal.program.FindFunction( funcname );
				if ( !signals->signal[ i ][ j ].function ) {
					savefile->Error( "Function '%s' not found", funcname.c_str() );
				}
			}
		}
	}

	savefile->ReadInt( mpGUIState );

	savefile->ReadObject( reinterpret_cast<idClass *&>( m_Inventory ) );
	savefile->ReadObject( reinterpret_cast<idClass *&>( m_InventoryItem ) );
	savefile->ReadObject( reinterpret_cast<idClass *&>( m_InventoryCursor ) );

	// restore must retrieve modelDefHandle from the renderer
	if ( modelDefHandle != -1 ) {
		modelDefHandle = gameRenderWorld->AddEntityDef( &renderEntity );
	}

	// restore must retrieve m_renderTriggerHandle from the renderer
	if ( m_renderTriggerHandle != -1 ) {
		m_renderTriggerHandle = gameRenderWorld->AddEntityDef( &m_renderTrigger );
	}
}

/*
================
idEntity::GetEntityDefName
================
*/
const char * idEntity::GetEntityDefName( void ) const
{
	if ( entityDefNumber < 0 ) {
		return "*unknown*";
	}
	return declManager->DeclByIndex( DECL_ENTITYDEF, entityDefNumber, false )->GetName();
}

/*
================
idEntity::SetName
================
*/
void idEntity::SetName( const char *newname )
{
	if ( name.Length() ) {
		gameLocal.RemoveEntityFromHash( name.c_str(), this );
		gameLocal.program.SetEntity( name, NULL );
	}

	name = newname;
	if ( name.Length() ) {
		if ( ( name == "NULL" ) || ( name == "null_entity" ) ) {
			gameLocal.Error( "Cannot name entity '%s'.  '%s' is reserved for script.", name.c_str(), name.c_str() );
		}
		gameLocal.AddEntityToHash( name.c_str(), this );
		gameLocal.program.SetEntity( name, this );
	}
}

/*
================
idEntity::GetName
================
*/
const char * idEntity::GetName( void ) const {
	return name.c_str();
}


/***********************************************************************

	Thinking
	
***********************************************************************/

/*
================
idEntity::Think
================
*/
void idEntity::Think( void )
{
	RunPhysics();
	Present();
}

/*
================
idEntity::DoDormantTests

Monsters and other expensive entities that are completely closed
off from the player can skip all of their work
================
*/
bool idEntity::DoDormantTests( void )
{
	if ( fl.neverDormant ) {
		return false;
	}

	// if the monster area is not topologically connected to a player
	if ( !gameLocal.InPlayerConnectedArea( this ) ) {
		if ( dormantStart == 0 ) {
			dormantStart = gameLocal.time;
		}
		if ( gameLocal.time - dormantStart < DELAY_DORMANT_TIME ) {
			// just got closed off, don't go dormant yet
			return false;
		}
		return true;
	} else {
		// the monster area is topologically connected to a player, but if
		// the monster hasn't been woken up before, do the more precise PVS check
		if ( !fl.hasAwakened ) {
			if ( !gameLocal.InPlayerPVS( this ) ) {
				return true;		// stay dormant
			}
		}

		// wake up
		dormantStart = 0;
		fl.hasAwakened = true;		// only go dormant when area closed off now, not just out of PVS
		return false;
	}

//	return false;
}

/*
================
idEntity::CheckDormant

Monsters and other expensive entities that are completely closed
off from the player can skip all of their work
================
*/
bool idEntity::CheckDormant( void )
{
	bool dormant;
	
	dormant = DoDormantTests();
	if ( dormant && !fl.isDormant ) {
		fl.isDormant = true;
		DormantBegin();
	} else if ( !dormant && fl.isDormant ) {
		fl.isDormant = false;
		DormantEnd();
	}

	return dormant;
}

/*
================
idEntity::DormantBegin

called when entity becomes dormant
================
*/
void idEntity::DormantBegin( void ) {
}

/*
================
idEntity::DormantEnd

called when entity wakes from being dormant
================
*/
void idEntity::DormantEnd( void ) {
}

/*
================
idEntity::IsActive
================
*/
bool idEntity::IsActive( void ) const {
	return activeNode.InList();
}

/*
================
idEntity::BecomeActive
================
*/
void idEntity::BecomeActive( int flags )
{
	if ( ( flags & TH_PHYSICS ) ) {
		// enable the team master if this entity is part of a physics team
		if ( teamMaster && teamMaster != this ) {
			teamMaster->BecomeActive( TH_PHYSICS );
		} else if ( !( thinkFlags & TH_PHYSICS ) ) {
			// if this is a pusher
			if ( physics->IsType( idPhysics_Parametric::Type ) || physics->IsType( idPhysics_Actor::Type ) ) {
				gameLocal.sortPushers = true;
			}
		}
	}

	int oldFlags = thinkFlags;
	thinkFlags |= flags;
	if ( thinkFlags ) {
		if ( !IsActive() ) {
			activeNode.AddToEnd( gameLocal.activeEntities );
		} else if ( !oldFlags ) {
			// we became inactive this frame, so we have to decrease the count of entities to deactivate
			gameLocal.numEntitiesToDeactivate--;
		}
	}
}

/*
================
idEntity::BecomeInactive
================
*/
void idEntity::BecomeInactive( int flags )
{
	if ( ( flags & TH_PHYSICS ) ) {
		// may only disable physics on a team master if no team members are running physics or bound to a joints
		if ( teamMaster == this ) {
			for ( idEntity *ent = teamMaster->teamChain; ent; ent = ent->teamChain ) {
				if ( ( ent->thinkFlags & TH_PHYSICS ) || ( ( ent->bindMaster == this ) && ( ent->bindJoint != INVALID_JOINT ) ) ) {
					flags &= ~TH_PHYSICS;
					break;
				}
			}
		}
	}

	if ( thinkFlags ) {
		thinkFlags &= ~flags;
		if ( !thinkFlags && IsActive() ) {
			gameLocal.numEntitiesToDeactivate++;
		}
	}

	if ( ( flags & TH_PHYSICS ) ) {
		// if this entity has a team master
		if ( teamMaster && teamMaster != this ) {
			// if the team master is at rest
			if ( teamMaster->IsAtRest() ) {
				teamMaster->BecomeInactive( TH_PHYSICS );
			}
		}
	}
}

/***********************************************************************

	Visuals
	
***********************************************************************/

/*
================
idEntity::SetShaderParm
================
*/
void idEntity::SetShaderParm( int parmnum, float value )
{
	if ( ( parmnum < 0 ) || ( parmnum >= MAX_ENTITY_SHADER_PARMS ) ) {
		gameLocal.Warning( "shader parm index (%d) out of range", parmnum );
		return;
	}

	renderEntity.shaderParms[ parmnum ] = value;
	UpdateVisuals();
}

/*
================
idEntity::SetColor
================
*/
void idEntity::SetColor( float red, float green, float blue ) {
	renderEntity.shaderParms[ SHADERPARM_RED ]		= red;
	renderEntity.shaderParms[ SHADERPARM_GREEN ]	= green;
	renderEntity.shaderParms[ SHADERPARM_BLUE ]		= blue;
	UpdateVisuals();
}

/*
================
idEntity::SetColor
================
*/
void idEntity::SetColor( const idVec3 &color ) {
	SetColor( color[ 0 ], color[ 1 ], color[ 2 ] );
	UpdateVisuals();
}

/*
================
idEntity::GetColor
================
*/
void idEntity::GetColor( idVec3 &out ) const {
	out[ 0 ] = renderEntity.shaderParms[ SHADERPARM_RED ];
	out[ 1 ] = renderEntity.shaderParms[ SHADERPARM_GREEN ];
	out[ 2 ] = renderEntity.shaderParms[ SHADERPARM_BLUE ];
}

/*
================
idEntity::SetColor
================
*/
void idEntity::SetColor( const idVec4 &color ) {
	renderEntity.shaderParms[ SHADERPARM_RED ]		= color[ 0 ];
	renderEntity.shaderParms[ SHADERPARM_GREEN ]	= color[ 1 ];
	renderEntity.shaderParms[ SHADERPARM_BLUE ]		= color[ 2 ];
	renderEntity.shaderParms[ SHADERPARM_ALPHA ]	= color[ 3 ];
	UpdateVisuals();
}

/*
================
idEntity::GetColor
================
*/
void idEntity::GetColor( idVec4 &out ) const {
	out[ 0 ] = renderEntity.shaderParms[ SHADERPARM_RED ];
	out[ 1 ] = renderEntity.shaderParms[ SHADERPARM_GREEN ];
	out[ 2 ] = renderEntity.shaderParms[ SHADERPARM_BLUE ];
	out[ 3 ] = renderEntity.shaderParms[ SHADERPARM_ALPHA ];
}

/*
================
idEntity::UpdateAnimationControllers
================
*/
bool idEntity::UpdateAnimationControllers( void ) {
	// any ragdoll and IK animation controllers should be updated here
	return false;
}

/*
================
idEntity::SetModel
================
*/
void idEntity::SetModel( const char *modelname ) {
	assert( modelname );

	FreeModelDef();

	// FIXME: temp hack to replace obsolete ips particles with prt particles
	if ( strstr( modelname, ".ips" ) != NULL ) {
		idStr str = modelname;
		str.Replace( ".ips", ".prt" );
		renderEntity.hModel = renderModelManager->FindModel( str );
	} else {
		renderEntity.hModel = renderModelManager->FindModel( modelname );
	}

	if ( renderEntity.hModel ) {
		renderEntity.hModel->Reset();
	}

	renderEntity.callback = NULL;

	renderEntity.numJoints = 0;
	renderEntity.joints = NULL;
	if ( renderEntity.hModel ) {
		renderEntity.bounds = renderEntity.hModel->Bounds( &renderEntity );
	} else {
		renderEntity.bounds.Zero();
	}

	UpdateVisuals();
}

/*
================
idEntity::SetSkin
================
*/
void idEntity::SetSkin( const idDeclSkin *skin ) {
	renderEntity.customSkin = skin;
	UpdateVisuals();
}

/*
================
idEntity::GetSkin
================
*/
const idDeclSkin *idEntity::GetSkin( void ) const {
	return renderEntity.customSkin;
}

/*
================
idEntity::FreeModelDef
================
*/
void idEntity::FreeModelDef( void ) {
	if ( modelDefHandle != -1 ) {
		gameRenderWorld->FreeEntityDef( modelDefHandle );
		modelDefHandle = -1;
	}
}

/*
================
idEntity::FreeLightDef
================
*/
void idEntity::FreeLightDef( void ) {
}

/*
================
idEntity::IsHidden
================
*/
bool idEntity::IsHidden( void ) const {
	return fl.hidden;
}

/*
================
idEntity::Hide
================
*/
void idEntity::Hide( void ) {
	if ( !IsHidden() ) {
		fl.hidden = true;
		FreeModelDef();
		UpdateVisuals();
	}
}

/*
================
idEntity::Show
================
*/
void idEntity::Show( void ) {
	if ( IsHidden() ) {
		fl.hidden = false;
		UpdateVisuals();
	}
}

/*
================
idEntity::UpdateModelTransform
================
*/
void idEntity::UpdateModelTransform( void ) {
	idVec3 origin;
	idMat3 axis;

	if ( GetPhysicsToVisualTransform( origin, axis ) ) {
		renderEntity.axis = axis * GetPhysics()->GetAxis();
		renderEntity.origin = GetPhysics()->GetOrigin() + origin * renderEntity.axis;
	} else {
		renderEntity.axis = GetPhysics()->GetAxis();
		renderEntity.origin = GetPhysics()->GetOrigin();
	}
}

/*
================
idEntity::UpdateModel
================
*/
void idEntity::UpdateModel( void ) {
	UpdateModelTransform();

	// check if the entity has an MD5 model
	idAnimator *animator = GetAnimator();
	if(animator && animator->ModelHandle())
	{
		// set the callback to update the joints
		renderEntity.callback = idEntity::ModelCallback;
	}

	// set to invalid number to force an update the next time the PVS areas are retrieved
	ClearPVSAreas();

	// ensure that we call Present this frame
	BecomeActive( TH_UPDATEVISUALS );
}

/*
================
idEntity::UpdateVisuals
================
*/
void idEntity::UpdateVisuals( void ) {
	UpdateModel();
	UpdateSound();
}

/*
================
idEntity::UpdatePVSAreas
================
*/
void idEntity::UpdatePVSAreas( void ) {
	int localNumPVSAreas, localPVSAreas[32];
	idBounds modelAbsBounds;
	int i;

	modelAbsBounds.FromTransformedBounds( renderEntity.bounds, renderEntity.origin, renderEntity.axis );
	localNumPVSAreas = gameLocal.pvs.GetPVSAreas( modelAbsBounds, localPVSAreas, sizeof( localPVSAreas ) / sizeof( localPVSAreas[0] ) );

	// FIXME: some particle systems may have huge bounds and end up in many PVS areas
	// the first MAX_PVS_AREAS may not be visible to a network client and as a result the particle system may not show up when it should
	if ( localNumPVSAreas > MAX_PVS_AREAS ) {
		localNumPVSAreas = gameLocal.pvs.GetPVSAreas( idBounds( modelAbsBounds.GetCenter() ).Expand( 64.0f ), localPVSAreas, sizeof( localPVSAreas ) / sizeof( localPVSAreas[0] ) );
	}

	for ( numPVSAreas = 0; numPVSAreas < MAX_PVS_AREAS && numPVSAreas < localNumPVSAreas; numPVSAreas++ ) {
		PVSAreas[numPVSAreas] = localPVSAreas[numPVSAreas];
	}

	for( i = numPVSAreas; i < MAX_PVS_AREAS; i++ ) {
		PVSAreas[ i ] = 0;
	}
}

/*
================
idEntity::UpdatePVSAreas
================
*/
void idEntity::UpdatePVSAreas( const idVec3 &pos ) {
	int i;

	numPVSAreas = gameLocal.pvs.GetPVSAreas( idBounds( pos ), PVSAreas, MAX_PVS_AREAS );
	i = numPVSAreas;
	while ( i < MAX_PVS_AREAS ) {
		PVSAreas[ i++ ] = 0;
	}
}

/*
================
idEntity::GetNumPVSAreas
================
*/
int idEntity::GetNumPVSAreas( void ) {
	if ( numPVSAreas < 0 ) {
		UpdatePVSAreas();
	}
	return numPVSAreas;
}

/*
================
idEntity::GetPVSAreas
================
*/
const int *idEntity::GetPVSAreas( void ) {
	if ( numPVSAreas < 0 ) {
		UpdatePVSAreas();
	}
	return PVSAreas;
}

/*
================
idEntity::ClearPVSAreas
================
*/
void idEntity::ClearPVSAreas( void ) {
	numPVSAreas = -1;
}

/*
================
idEntity::PhysicsTeamInPVS

  FIXME: for networking also return true if any of the entity shadows is in the PVS
================
*/
bool idEntity::PhysicsTeamInPVS( pvsHandle_t pvsHandle ) {
	idEntity *part;

	if ( teamMaster ) {
		for ( part = teamMaster; part; part = part->teamChain ) {
			if ( gameLocal.pvs.InCurrentPVS( pvsHandle, part->GetPVSAreas(), part->GetNumPVSAreas() ) ) {
				return true;
			}
		}
	} else {
		return gameLocal.pvs.InCurrentPVS( pvsHandle, GetPVSAreas(), GetNumPVSAreas() );
	}
	return false;
}

/*
==============
idEntity::ProjectOverlay
==============
*/
void idEntity::ProjectOverlay( const idVec3 &origin, const idVec3 &dir, float size, const char *material ) {
	float s, c;
	idMat3 axis, axistemp;
	idVec3 localOrigin, localAxis[2];
	idPlane localPlane[2];

	// make sure the entity has a valid model handle
	if ( modelDefHandle < 0 ) {
		return;
	}

	// only do this on dynamic md5 models
	if ( renderEntity.hModel->IsDynamicModel() != DM_CACHED ) {
		return;
	}

	idMath::SinCos16( gameLocal.random.RandomFloat() * idMath::TWO_PI, s, c );

	axis[2] = -dir;
	axis[2].NormalVectors( axistemp[0], axistemp[1] );
	axis[0] = axistemp[ 0 ] * c + axistemp[ 1 ] * -s;
	axis[1] = axistemp[ 0 ] * -s + axistemp[ 1 ] * -c;

	renderEntity.axis.ProjectVector( origin - renderEntity.origin, localOrigin );
	renderEntity.axis.ProjectVector( axis[0], localAxis[0] );
	renderEntity.axis.ProjectVector( axis[1], localAxis[1] );

	size = 1.0f / size;
	localAxis[0] *= size;
	localAxis[1] *= size;

	localPlane[0] = localAxis[0];
	localPlane[0][3] = -( localOrigin * localAxis[0] ) + 0.5f;

	localPlane[1] = localAxis[1];
	localPlane[1][3] = -( localOrigin * localAxis[1] ) + 0.5f;

	const idMaterial *mtr = declManager->FindMaterial( material );

	// project an overlay onto the model
	gameRenderWorld->ProjectOverlay( modelDefHandle, localPlane, mtr );

	// make sure non-animating models update their overlay
	UpdateVisuals();
}

/*
================
idEntity::Present

Present is called to allow entities to generate refEntities, lights, etc for the renderer.
================
*/
void idEntity::Present(void)
{
/*
	if( m_bFrobable )
	{
*/
/*
		DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("this: %08lX    FrobDistance: %lu\r", this, m_FrobDistance);
		DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("RenderEntity: %08lX\r", renderEntity);
		DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("SurfaceInView: %u\r", renderEntity.allowSurfaceInViewID);
		DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("RenderModel: %08lX\r", renderEntity.hModel);
		DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("CustomShader: %08lX\r", renderEntity.customShader);
		DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("ReferenceShader: %08lX\r", renderEntity.referenceShader);
		DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("ReferenceShader: %08lX\r", renderEntity.referenceShader);

		for(int i = 0; i < MAX_ENTITY_SHADER_PARMS; i++)
			DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Shaderparam[%u]: %f\r", i, renderEntity.shaderParms[i]);

		DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("ForceUpdate: %u\r", renderEntity.forceUpdate);

		renderEntity.customShader = gameLocal.GetGlobalMaterial();
	}
*/

	if(!gameLocal.isNewFrame )
		return;

	if( m_bFrobable )
	{
		UpdateFrob();
		UpdateFrobDisplay();
	}

	// don't present to the renderer if the entity hasn't changed
	if(!(thinkFlags & TH_UPDATEVISUALS))
		return;

	if( !m_bFrobable )
		BecomeInactive( TH_UPDATEVISUALS );

	// camera target for remote render views
	if(cameraTarget && gameLocal.InPlayerPVS(this))
		renderEntity.remoteRenderView = cameraTarget->GetRenderView();

	// if set to invisible, skip
	if(!renderEntity.hModel || IsHidden())
		return;

	// add to refresh list
	if ( modelDefHandle == -1 ) {
		modelDefHandle = gameRenderWorld->AddEntityDef( &renderEntity );
	} else {
		gameRenderWorld->UpdateEntityDef( modelDefHandle, &renderEntity );
	}

	PresentRenderTrigger();
}

/*
================
idEntity::GetRenderEntity
================
*/
renderEntity_t *idEntity::GetRenderEntity( void ) {
	return &renderEntity;
}

/*
================
idEntity::GetModelDefHandle
================
*/
int idEntity::GetModelDefHandle( void ) {
	return modelDefHandle;
}

/*
================
idEntity::UpdateRenderEntity
================
*/
bool idEntity::UpdateRenderEntity( renderEntity_s *renderEntity, const renderView_t *renderView )
{
	if ( gameLocal.inCinematic && gameLocal.skipCinematic ) {
		return false;
	}

	idAnimator *animator = GetAnimator();
	if ( animator ) {
		return animator->CreateFrame( gameLocal.time, false );
	}

	return false;
}

/*
================
idEntity::ModelCallback

	NOTE: may not change the game state whatsoever!
================
*/
bool idEntity::ModelCallback( renderEntity_s *renderEntity, const renderView_t *renderView )
{
	idEntity *ent;

	ent = gameLocal.entities[ renderEntity->entityNum ];

	if ( !ent ) {
		gameLocal.Error( "idEntity::ModelCallback: callback with NULL game entity" );
	}

	return ent->UpdateRenderEntity( renderEntity, renderView );
}

/*
================
idEntity::GetAnimator

Subclasses will be responsible for allocating animator.
================
*/
idAnimator *idEntity::GetAnimator( void ) {
	return NULL;
}

/*
=============
idEntity::GetRenderView

This is used by remote camera views to look from an entity
=============
*/
renderView_t *idEntity::GetRenderView( void ) {
	if ( !renderView ) {
		renderView = new renderView_t;
	}
	memset( renderView, 0, sizeof( *renderView ) );

	renderView->vieworg = GetPhysics()->GetOrigin();
	renderView->fov_x = 120;
	renderView->fov_y = 120;
	renderView->viewaxis = GetPhysics()->GetAxis();

	// copy global shader parms
	for( int i = 0; i < MAX_GLOBAL_SHADER_PARMS; i++ ) {
		renderView->shaderParms[ i ] = gameLocal.globalShaderParms[ i ];
	}

	renderView->globalMaterial = gameLocal.GetGlobalMaterial();

	renderView->time = gameLocal.time;

	return renderView;
}

/***********************************************************************

  Sound
	
***********************************************************************/

/*
================
idEntity::CanPlayChatterSounds

Used for playing chatter sounds on monsters.
================
*/
bool idEntity::CanPlayChatterSounds( void ) const {
	return true;
}

/*
================
idEntity::StartSound
================
*/
bool idEntity::StartSound( const char *soundName, const s_channelType channel, int soundShaderFlags, bool broadcast, int *length ) 
{
	const idSoundShader *shader;
	const char *sound;

	if ( length ) 
		*length = 0;

	// we should ALWAYS be playing sounds from the def.
	// hardcoded sounds MUST be avoided at all times because they won't get precached.
	assert( idStr::Icmpn( soundName, "snd_", 4 ) == 0 );

	if ( !spawnArgs.GetString( soundName, "", &sound ) ) 
		return false;

	if ( sound[0] == '\0' ) 
		return false;

	if ( !gameLocal.isNewFrame ) 
	{
		// don't play the sound, but don't report an error
		return true;
	}
	
	// DarkMod sound propagation:
	PropSoundDirect( soundName, true, false );

	// play the audible sound
	shader = declManager->FindSound( sound );
	return StartSoundShader( shader, channel, soundShaderFlags, broadcast, length );
}

/*
================
idEntity::StartSoundShader
================
*/
bool idEntity::StartSoundShader( const idSoundShader *shader, const s_channelType channel, int soundShaderFlags, bool broadcast, int *length ) {
	float diversity;
	int len;

	if ( length ) {
		*length = 0;
	}

	if ( !shader ) {
		return false;
	}

	if ( !gameLocal.isNewFrame ) {
		return true;
	}

	if ( gameLocal.isServer && broadcast ) {
		idBitMsg	msg;
		byte		msgBuf[MAX_EVENT_PARAM_SIZE];

		msg.Init( msgBuf, sizeof( msgBuf ) );
		msg.BeginWriting();
		msg.WriteLong( gameLocal.ServerRemapDecl( -1, DECL_SOUND, shader->Index() ) );
		msg.WriteByte( channel );
		ServerSendEvent( EVENT_STARTSOUNDSHADER, &msg, false, -1 );
	}

	// set a random value for diversity unless one was parsed from the entity
	if ( refSound.diversity < 0.0f ) {
		diversity = gameLocal.random.RandomFloat();
	} else {
		diversity = refSound.diversity;
	}

	// if we don't have a soundEmitter allocated yet, get one now
	if ( !refSound.referenceSound ) {
		refSound.referenceSound = gameSoundWorld->AllocSoundEmitter();
	}

	UpdateSound();

	len = refSound.referenceSound->StartSound( shader, channel, diversity, soundShaderFlags );
	if ( length ) {
		*length = len;
	}

	// set reference to the sound for shader synced effects
	renderEntity.referenceSound = refSound.referenceSound;

	return true;
}

/*
================
idEntity::StopSound
================
*/
void idEntity::StopSound( const s_channelType channel, bool broadcast ) {
	if ( !gameLocal.isNewFrame ) {
		return;
	}

	if ( gameLocal.isServer && broadcast ) {
		idBitMsg	msg;
		byte		msgBuf[MAX_EVENT_PARAM_SIZE];

		msg.Init( msgBuf, sizeof( msgBuf ) );
		msg.BeginWriting();
		msg.WriteByte( channel );
		ServerSendEvent( EVENT_STOPSOUNDSHADER, &msg, false, -1 );
	}

	if ( refSound.referenceSound ) {
		refSound.referenceSound->StopSound( channel );
	}
}

/*
================
idEntity::SetSoundVolume

  Must be called before starting a new sound.
================
*/
void idEntity::SetSoundVolume( float volume ) {
	refSound.parms.volume = volume;
}

/*
================
idEntity::UpdateSound
================
*/
void idEntity::UpdateSound( void ) {
	if ( refSound.referenceSound ) {
		idVec3 origin;
		idMat3 axis;

		if ( GetPhysicsToSoundTransform( origin, axis ) ) {
			refSound.origin = GetPhysics()->GetOrigin() + origin * axis;
		} else {
			refSound.origin = GetPhysics()->GetOrigin();
		}

		refSound.referenceSound->UpdateEmitter( refSound.origin, refSound.listenerId, &refSound.parms );
	}
}

/*
================
idEntity::GetListenerId
================
*/
int idEntity::GetListenerId( void ) const {
	return refSound.listenerId;
}

/*
================
idEntity::GetSoundEmitter
================
*/
idSoundEmitter *idEntity::GetSoundEmitter( void ) const {
	return refSound.referenceSound;
}

/*
================
idEntity::FreeSoundEmitter
================
*/
void idEntity::FreeSoundEmitter( bool immediate ) {
	if ( refSound.referenceSound ) {
		refSound.referenceSound->Free( immediate );
		refSound.referenceSound = NULL;
	}
}

/*
================
idEntity::PropSoundS
================
*/

// note: the format for kv pair is: <global sound name>:volMod,durMod
// the last two values should be optional, so <global sound name>:volMod,
// and <global sound name>,durMod also work.

void idEntity::PropSoundS( const char *localName, const char *globalName, float VolModIn )
{
	int start, end = -1, len;
	bool bHasComma(false), bHasColon(false), bFoundSnd(false);
	float volMod(0.0), durMod(1.0);
	idStr gName(globalName), locName, tempstr;

	// if there is no local name, skip all the loading of local flags
	// and parms.
	if( localName == NULL )
	{
		bFoundSnd = gameLocal.m_sndProp->CheckSound( globalName, false );
		DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("PropSoundS: Propagating global sound %s without checking local sound\r", globalName);
		// note: if we are doing the global only, gName remains set to globalName
		goto Quit;
	}

	DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("PropSoundS: Found local sound %s, parsing local sound modifiers\r", localName);

	locName = localName;

	// parse the volMod and durMod if they are tacked on to the globalName
	len = gName.Length();

	// parse volMod, when durMod may or may not be present
	if( (start = gName.Find(':')) != -1 )
	{
		bHasColon = true;
		start++;
		end = gName.Find(',');

		if( end == -1 )
			end = gName.Length();

		tempstr = gName.Mid(start, (end - start));
		
		if( !tempstr.IsNumeric() || start >= end )
		{
			gameLocal.Warning( "[Soundprop] Bad volume multiplier for sound %s on entity %s.", localName, name.c_str() );
			DM_LOG(LC_SOUND, LT_WARNING)LOGSTRING("Bad volume multiplier for sound %s on entity %s.\r", localName, name.c_str() );
		}
		else
		{
			volMod = atof( tempstr.c_str() );
			DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Found local volume modifier %f \r", volMod);
		}
	}

	// parse durMod
	if( (start = gName.Find(',')) != -1 )
	{
		bHasComma = true;
		start++;
		end = gName.Length();

		tempstr = gName.Mid(start, (end - start));
		if( !tempstr.IsNumeric() || start >= end )
		{
			gameLocal.Warning( "[Soundprop] Bad duration multiplier for sound %s on entity %s.", localName, name.c_str() );
			DM_LOG(LC_SOUND, LT_WARNING)LOGSTRING("Bad duration multiplier for sound %s on entity %s.\r", localName, name.c_str() );
		}
		else
		{
			durMod = atof( tempstr.c_str() );
			DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Found local duration modifier %f \r", durMod);
		}
	}

	// TODO: Get the extra flags, freqmod and bandwidthmod from another key/value pair
	// we will need locName for this.
	
	// strip the durmod and volmod off of the global name
	if( bHasColon )
		end = gName.Find(':');
	else if( bHasComma && !bHasColon )
		end = gName.Find(',');
	else if( !bHasComma && !bHasColon )
		end = gName.Length();

	gName = gName.Mid(0, end);

	// add the input volume modifier
	volMod += VolModIn;

	bFoundSnd = gameLocal.m_sndProp->CheckSound( gName.c_str(), false );

Quit:
	if( bFoundSnd )
	{
		gameLocal.m_sndProp->Propagate( volMod, durMod, gName, 
										GetPhysics()->GetOrigin(), 
										this ); 
	}

	return;
}

/*
================
idEntity::PropSoundE
================
*/

void idEntity::PropSoundE( const char *localName, const char *globalName, float VolModIn )
{
	bool bFoundSnd(false);
	float volMod(0.0);
	idStr gName(globalName), locName(localName), tempstr;

	if( localName == NULL )
	{
		bFoundSnd = gameLocal.m_sndProp->CheckSound( globalName, true );
		// if we are doing the global only, gName remains set to globalName
		goto Quit;
	}

	// do the parsing of the local vars that modify the Env. sound

	// strip gName of all modifiers at the end

	// add the input volume modifier
	volMod += VolModIn;

Quit:
	
	if( bFoundSnd )
	{
		//TODO : Call soundprop to add env. sound to list
		// have not written this sndprop functionality yet!
	}
	else
	{
		// log error, did not find sound in sound def file
	}
	return;
}

/*
================
idEntity::PropSoundDirect
================
*/

void idEntity::PropSoundDirect( const char *sndName, bool bForceLocal, bool bAssumeEnv, float VolModIn )
{
	idStr sprName, sprNameSG, sprNameEG;
	bool bIsSusp(false), bIsEnv(false);

	// uncomment for debugging (spams the logfile since it plays for every sound that happens)
	// DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("PropSoundDirect: Attempting to propagate sound \"%s\" Forcelocal = %d\r", sndName, (int) bForceLocal );
	
	sprName = sndName;

	sprName.StripLeading("snd_");

	bIsSusp = spawnArgs.GetString( ("sprS_" + sprName) , "", sprNameSG );
	bIsEnv =  spawnArgs.GetString( ("sprE_" + sprName) , "", sprNameEG );

	if ( bForceLocal && ( !(idStr::Icmpn( sndName, "snd_", 4 ) == 0)
		 || ( !bIsSusp && !bIsEnv ) ) )  
	{
		// uncomment for debugging
		// DM_LOG(LC_SOUND, LT_WARNING)LOGSTRING("Attempted to propagate nonexistant local sound \"%s\" (forceLocal = true)\r", sndName );
		// gameLocal.Warning("[PropSound] Attempted to propagate nonexistant local sound \"%s\" (forceLocal = true)", sndName );
		goto Quit;
	}

	if ( !bIsSusp && !bIsEnv ) 
	{
		// play the unmodified, global sound directly
		sprNameSG = sndName;
		sprNameEG = sndName;
	}

	if (bIsSusp)
	{
		PropSoundS( sprName.c_str(), sprNameSG.c_str(), VolModIn );
		DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Found local suspicious sound def for %s on entity, attempting to propagate with global sound %s\r", sprName.c_str(), sprNameSG.c_str() );
		// exit here, because we don't want to allow playing both
		// env. sound AND susp. sound for the same sound and entity
		goto Quit;
	}
	if (bIsEnv)
	{
		DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Found local environmental sound def for %s on entity, attempting to propagating with global sound %s\r", sprName.c_str(), sprNameEG.c_str() );
		PropSoundE( sprName.c_str(), sprNameEG.c_str(), VolModIn );
		goto Quit;
	}

	// play the global sound directly.
	if ( bAssumeEnv )
	{
		DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Did not find local def for sound %s, attempting to propagate it as global environmental\r", sprNameEG.c_str() );
		PropSoundE( NULL, sprNameEG.c_str(), VolModIn );
		goto Quit;
	}
	else
	{
		DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Did not find local def for sound %s, attempting to propagate it as global suspicious\r", sprNameSG.c_str() );
		PropSoundS( NULL, sprNameSG.c_str(), VolModIn );
	}

Quit:
	return;
}



/***********************************************************************

  entity binding
	
***********************************************************************/

/*
================
idEntity::PreBind
================
*/
void idEntity::PreBind( void )
{
}

/*
================
idEntity::PostBind
================
*/
void idEntity::PostBind( void )
{
}

/*
================
idEntity::PreUnbind
================
*/
void idEntity::PreUnbind( void )
{
}

/*
================
idEntity::PostUnbind
================
*/
void idEntity::PostUnbind( void )
{
}

/*
================
idEntity::InitBind
================
*/
bool idEntity::InitBind( idEntity *master )
{
	if ( master == this ) {
		gameLocal.Error( "Tried to bind an object to itself." );
		return false;
	}

	if ( this == gameLocal.world ) {
		gameLocal.Error( "Tried to bind world to another entity" );
		return false;
	}

	// unbind myself from my master
	Unbind();

	// add any bind constraints to an articulated figure
	if ( master && IsType( idAFEntity_Base::Type ) ) {
		static_cast<idAFEntity_Base *>(this)->AddBindConstraints();
	}

// TDM: Removed the inability to bind something to the world
// Might have to put it back in if something unforseen happens later because of this
//	if ( !master || master == gameLocal.world ) 
	if ( !master ) 
	{
		// this can happen in scripts, so safely exit out.
		return false;
	}

	return true;
}

/*
================
idEntity::FinishBind
================
*/
void idEntity::FinishBind( void )
{
	// set the master on the physics object
	physics->SetMaster( bindMaster, fl.bindOrientated );

	// We are now separated from our previous team and are either
	// an individual, or have a team of our own.  Now we can join
	// the new bindMaster's team.  Bindmaster must be set before
	// joining the team, or we will be placed in the wrong position
	// on the team.
	JoinTeam( bindMaster );

	// if our bindMaster is enabled during a cinematic, we must be, too
	cinematic = bindMaster->cinematic;

	// make sure the team master is active so that physics get run
	teamMaster->BecomeActive( TH_PHYSICS );
	
	// Notify bind master of this binding
	bindMaster->BindNotify( this );
}

/*
================
idEntity::Bind

  bind relative to the visual position of the master
================
*/
void idEntity::Bind( idEntity *master, bool orientated )
{
	if ( !InitBind( master ) ) {
		return;
	}

	PreBind();

	bindJoint = INVALID_JOINT;
	bindBody = -1;
	bindMaster = master;
	fl.bindOrientated = orientated;

	FinishBind();

	PostBind( );
}

/*
================
idEntity::BindToJoint

  bind relative to a joint of the md5 model used by the master
================
*/
void idEntity::BindToJoint( idEntity *master, const char *jointname, bool orientated ) {
	jointHandle_t	jointnum;
	idAnimator		*masterAnimator;

	if ( !InitBind( master ) ) {
		return;
	}

	masterAnimator = master->GetAnimator();
	if ( !masterAnimator ) {
		gameLocal.Warning( "idEntity::BindToJoint: entity '%s' cannot support skeletal models.", master->GetName() );
		return;
	}

	jointnum = masterAnimator->GetJointHandle( jointname );
	if ( jointnum == INVALID_JOINT ) {
		gameLocal.Warning( "idEntity::BindToJoint: joint '%s' not found on entity '%s'.", jointname, master->GetName() );
	}

	PreBind();

	bindJoint = jointnum;
	bindBody = -1;
	bindMaster = master;
	fl.bindOrientated = orientated;

	FinishBind();

	PostBind();
}

/*
================
idEntity::BindToJoint

  bind relative to a joint of the md5 model used by the master
================
*/
void idEntity::BindToJoint( idEntity *master, jointHandle_t jointnum, bool orientated ) {

	if ( !InitBind( master ) ) {
		return;
	}

	PreBind();

	bindJoint = jointnum;
	bindBody = -1;
	bindMaster = master;
	fl.bindOrientated = orientated;

	FinishBind();

	PostBind();
}

/*
================
idEntity::BindToBody

  bind relative to a collision model used by the physics of the master
================
*/
void idEntity::BindToBody( idEntity *master, int bodyId, bool orientated ) {

	if ( !InitBind( master ) ) {
		return;
	}

	if ( bodyId < 0 ) {
		gameLocal.Warning( "idEntity::BindToBody: body '%d' not found.", bodyId );
	}

	PreBind();

	bindJoint = INVALID_JOINT;
	bindBody = bodyId;
	bindMaster = master;
	fl.bindOrientated = orientated;

	FinishBind();

	PostBind();
}

/*
================
idEntity::Unbind
================
*/
void idEntity::Unbind( void ) {
	idEntity *	prev;
	idEntity *	next;
	idEntity *	last;
	idEntity *	ent;

	// remove any bind constraints from an articulated figure
	if ( IsType( idAFEntity_Base::Type ) ) {
		static_cast<idAFEntity_Base *>(this)->RemoveBindConstraints();
	}

	if ( !bindMaster ) {
		return;
	}

	// TDM: Notify bind master of unbinding
	bindMaster->UnbindNotify( this );

	if ( !teamMaster ) {
		// Teammaster already has been freed
		bindMaster = NULL;
		return;
	}

	PreUnbind();

	if ( physics ) {
		physics->SetMaster( NULL, fl.bindOrientated );
	}

	// We're still part of a team, so that means I have to extricate myself
	// and any entities that are bound to me from the old team.
	// Find the node previous to me in the team
	prev = teamMaster;
	for( ent = teamMaster->teamChain; ent && ( ent != this ); ent = ent->teamChain ) {
		prev = ent;
	}

	assert( ent == this ); // If ent is not pointing to this, then something is very wrong.

	// Find the last node in my team that is bound to me.
	// Also find the first node not bound to me, if one exists.
	last = this;
	for( next = teamChain; next != NULL; next = next->teamChain ) {
		if ( !next->IsBoundTo( this ) ) {
			break;
		}

		// Tell them I'm now the teamMaster
		next->teamMaster = this;
		last = next;
	}

	// disconnect the last member of our team from the old team
	last->teamChain = NULL;

	// connect up the previous member of the old team to the node that
	// follow the last node bound to me (if one exists).
	if ( teamMaster != this ) {
		prev->teamChain = next;
		if ( !next && ( teamMaster == prev ) ) {
			prev->teamMaster = NULL;
		}
	} else if ( next ) {
		// If we were the teamMaster, then the nodes that were not bound to me are now
		// a disconnected chain.  Make them into their own team.
		for( ent = next; ent->teamChain != NULL; ent = ent->teamChain ) {
			ent->teamMaster = next;
		}
		next->teamMaster = next;
	}

	// If we don't have anyone on our team, then clear the team variables.
	if ( teamChain ) {
		// make myself my own team
		teamMaster = this;
	} else {
		// no longer a team
		teamMaster = NULL;
	}

	bindJoint = INVALID_JOINT;
	bindBody = -1;
	bindMaster = NULL;

	PostUnbind();
}

/*
================
idEntity::RemoveBinds
================
*/
void idEntity::RemoveBinds( void ) {
	idEntity *ent;
	idEntity *next;

	for( ent = teamChain; ent != NULL; ent = next ) {
		next = ent->teamChain;
		if ( ent->bindMaster == this ) {
			ent->Unbind();

			if( ent->spawnArgs.GetBool( "removeWithMaster", "1" ) ) {
				ent->PostEventMS( &EV_Remove, 0 );
			}
			next = teamChain;
		}
	}
}

/*
================
idEntity::IsBound
================
*/
bool idEntity::IsBound( void ) const {
	if ( bindMaster ) {
		return true;
	}
	return false;
}

/*
================
idEntity::IsBoundTo
================
*/
bool idEntity::IsBoundTo( idEntity *master ) const {
	idEntity *ent;

	if ( !bindMaster ) {
		return false;
	}

	for ( ent = bindMaster; ent != NULL; ent = ent->bindMaster ) {
		if ( ent == master ) {
			return true;
		}
	}

	return false;
}

/*
================
idEntity::GetBindMaster
================
*/
idEntity *idEntity::GetBindMaster( void ) const {
	return bindMaster;
}

/*
================
idEntity::GetBindJoint
================
*/
jointHandle_t idEntity::GetBindJoint( void ) const {
	return bindJoint;
}

/*
================
idEntity::GetBindBody
================
*/
int idEntity::GetBindBody( void ) const {
	return bindBody;
}

/*
================
idEntity::GetTeamMaster
================
*/
idEntity *idEntity::GetTeamMaster( void ) const {
	return teamMaster;
}

/*
================
idEntity::GetNextTeamEntity
================
*/
idEntity *idEntity::GetNextTeamEntity( void ) const {
	return teamChain;
}

/*
=====================
idEntity::ConvertLocalToWorldTransform
=====================
*/
void idEntity::ConvertLocalToWorldTransform( idVec3 &offset, idMat3 &axis ) {
	UpdateModelTransform();

	offset = renderEntity.origin + offset * renderEntity.axis;
	axis *= renderEntity.axis;
}

/*
================
idEntity::GetLocalVector

Takes a vector in worldspace and transforms it into the parent
object's localspace.

Note: Does not take origin into acount.  Use getLocalCoordinate to
convert coordinates.
================
*/
idVec3 idEntity::GetLocalVector( const idVec3 &vec ) const {
	idVec3	pos;

	if ( !bindMaster ) {
		return vec;
	}

	idVec3	masterOrigin;
	idMat3	masterAxis;

	GetMasterPosition( masterOrigin, masterAxis );
	masterAxis.ProjectVector( vec, pos );

	return pos;
}

/*
================
idEntity::GetLocalCoordinates

Takes a vector in world coordinates and transforms it into the parent
object's local coordinates.
================
*/
idVec3 idEntity::GetLocalCoordinates( const idVec3 &vec ) const {
	idVec3	pos;

	if ( !bindMaster ) {
		return vec;
	}

	idVec3	masterOrigin;
	idMat3	masterAxis;

	GetMasterPosition( masterOrigin, masterAxis );
	masterAxis.ProjectVector( vec - masterOrigin, pos );

	return pos;
}

/*
================
idEntity::GetWorldVector

Takes a vector in the parent object's local coordinates and transforms
it into world coordinates.

Note: Does not take origin into acount.  Use getWorldCoordinate to
convert coordinates.
================
*/
idVec3 idEntity::GetWorldVector( const idVec3 &vec ) const {
	idVec3	pos;

	if ( !bindMaster ) {
		return vec;
	}

	idVec3	masterOrigin;
	idMat3	masterAxis;

	GetMasterPosition( masterOrigin, masterAxis );
	masterAxis.UnprojectVector( vec, pos );

	return pos;
}

/*
================
idEntity::GetWorldCoordinates

Takes a vector in the parent object's local coordinates and transforms
it into world coordinates.
================
*/
idVec3 idEntity::GetWorldCoordinates( const idVec3 &vec ) const {
	idVec3	pos;

	if ( !bindMaster ) {
		return vec;
	}

	idVec3	masterOrigin;
	idMat3	masterAxis;

	GetMasterPosition( masterOrigin, masterAxis );
	masterAxis.UnprojectVector( vec, pos );
	pos += masterOrigin;

	return pos;
}

/*
================
idEntity::GetMasterPosition
================
*/
bool idEntity::GetMasterPosition( idVec3 &masterOrigin, idMat3 &masterAxis ) const {
	idVec3		localOrigin;
	idMat3		localAxis;
	idAnimator	*masterAnimator;

	if ( bindMaster ) {
		// if bound to a joint of an animated model
		if ( bindJoint != INVALID_JOINT ) {
			masterAnimator = bindMaster->GetAnimator();
			if ( !masterAnimator ) {
				masterOrigin = vec3_origin;
				masterAxis = mat3_identity;
				return false;
			} else {
				masterAnimator->GetJointTransform( bindJoint, gameLocal.time, masterOrigin, masterAxis );
				masterAxis *= bindMaster->renderEntity.axis;
				masterOrigin = bindMaster->renderEntity.origin + masterOrigin * bindMaster->renderEntity.axis;
			}
		} else if ( bindBody >= 0 && bindMaster->GetPhysics() ) {
			masterOrigin = bindMaster->GetPhysics()->GetOrigin( bindBody );
			masterAxis = bindMaster->GetPhysics()->GetAxis( bindBody );
		} else {
			masterOrigin = bindMaster->renderEntity.origin;
			masterAxis = bindMaster->renderEntity.axis;
		}
		return true;
	} else {
		masterOrigin = vec3_origin;
		masterAxis = mat3_identity;
		return false;
	}
}

/*
================
idEntity::GetWorldVelocities
================
*/
void idEntity::GetWorldVelocities( idVec3 &linearVelocity, idVec3 &angularVelocity ) const {

	linearVelocity = physics->GetLinearVelocity();
	angularVelocity = physics->GetAngularVelocity();

	if ( bindMaster ) {
		idVec3 masterOrigin, masterLinearVelocity, masterAngularVelocity;
		idMat3 masterAxis;

		// get position of master
		GetMasterPosition( masterOrigin, masterAxis );

		// get master velocities
		bindMaster->GetWorldVelocities( masterLinearVelocity, masterAngularVelocity );

		// linear velocity relative to master plus master linear and angular velocity
		linearVelocity = linearVelocity * masterAxis + masterLinearVelocity +
								masterAngularVelocity.Cross( GetPhysics()->GetOrigin() - masterOrigin );
	}
}

/*
================
idEntity::JoinTeam
================
*/
void idEntity::JoinTeam( idEntity *teammember ) {
	idEntity *ent;
	idEntity *master;
	idEntity *prev;
	idEntity *next;

	// if we're already on a team, quit it so we can join this one
	if ( teamMaster && ( teamMaster != this ) ) {
		QuitTeam();
	}

	assert( teammember );

	if ( teammember == this ) {
		teamMaster = this;
		return;
	}

	// check if our new team mate is already on a team
	master = teammember->teamMaster;
	if ( !master ) {
		// he's not on a team, so he's the new teamMaster
		master = teammember;
		teammember->teamMaster = teammember;
		teammember->teamChain = this;

		// make anyone who's bound to me part of the new team
		for( ent = teamChain; ent != NULL; ent = ent->teamChain ) {
			ent->teamMaster = master;
		}
	} else {
		// skip past the chain members bound to the entity we're teaming up with
		prev = teammember;
		next = teammember->teamChain;
		if ( bindMaster ) {
			// if we have a bindMaster, join after any entities bound to the entity
			// we're joining
			while( next && next->IsBoundTo( teammember ) ) {
				prev = next;
				next = next->teamChain;
			}
		} else {
			// if we're not bound to someone, then put us at the end of the team
			while( next ) {
				prev = next;
				next = next->teamChain;
			}
		}

		// make anyone who's bound to me part of the new team and
		// also find the last member of my team
		for( ent = this; ent->teamChain != NULL; ent = ent->teamChain ) {
			ent->teamChain->teamMaster = master;
		}

    	prev->teamChain = this;
		ent->teamChain = next;
	}

	teamMaster = master;

	// reorder the active entity list 
	gameLocal.sortTeamMasters = true;
}

/*
================
idEntity::QuitTeam
================
*/
void idEntity::QuitTeam( void ) {
	idEntity *ent;

	if ( !teamMaster ) {
		return;
	}

	// check if I'm the teamMaster
	if ( teamMaster == this ) {
		// do we have more than one teammate?
		if ( !teamChain->teamChain ) {
			// no, break up the team
			teamChain->teamMaster = NULL;
		} else {
			// yes, so make the first teammate the teamMaster
			for( ent = teamChain; ent; ent = ent->teamChain ) {
				ent->teamMaster = teamChain;
			}
		}
	} else {
		assert( teamMaster );
		assert( teamMaster->teamChain );

		// find the previous member of the teamChain
		ent = teamMaster;
		while( ent->teamChain != this ) {
			assert( ent->teamChain ); // this should never happen
			ent = ent->teamChain;
		}

		// remove this from the teamChain
		ent->teamChain = teamChain;

		// if no one is left on the team, break it up
		if ( !teamMaster->teamChain ) {
			teamMaster->teamMaster = NULL;
		}
	}

	teamMaster = NULL;
	teamChain = NULL;
}

/***********************************************************************

  Physics.
	
***********************************************************************/

/*
================
idEntity::InitDefaultPhysics
================
*/
void idEntity::InitDefaultPhysics( const idVec3 &origin, const idMat3 &axis )
{
	const char *temp;
	idClipModel *clipModel = NULL;

	DM_LOG(LC_ENTITY, LT_DEBUG)LOGSTRING("Entity [%s] test for clipmodel\r", name.c_str());

	// check if a clipmodel key/value pair is set
	if ( spawnArgs.GetString( "clipmodel", "", &temp ) ) {
		if ( idClipModel::CheckModel( temp ) ) {
			clipModel = new idClipModel( temp );
		}
	}

	if(!spawnArgs.GetBool( "noclipmodel", "0" ))
	{
		// check if mins/maxs or size key/value pairs are set
		if ( !clipModel )
		{
			idVec3 size;
			idBounds bounds;
			bool setClipModel = false;

			if ( spawnArgs.GetVector( "mins", NULL, bounds[0] ) &&
				spawnArgs.GetVector( "maxs", NULL, bounds[1] ) )
			{
				setClipModel = true;
				if ( bounds[0][0] > bounds[1][0] || bounds[0][1] > bounds[1][1] || bounds[0][2] > bounds[1][2] )
				{
					gameLocal.Error( "Invalid bounds '%s'-'%s' on entity '%s'", bounds[0].ToString(), bounds[1].ToString(), name.c_str() );
				}
			} 
			else
			if ( spawnArgs.GetVector( "size", NULL, size ) )
			{
				if ( ( size.x < 0.0f ) || ( size.y < 0.0f ) || ( size.z < 0.0f ) )
				{
					gameLocal.Error( "Invalid size '%s' on entity '%s'", size.ToString(), name.c_str() );
				}
				bounds[0].Set( size.x * -0.5f, size.y * -0.5f, 0.0f );
				bounds[1].Set( size.x * 0.5f, size.y * 0.5f, size.z );
				setClipModel = true;
			}

			if ( setClipModel ) {
				int numSides;
				idTraceModel trm;

				if ( spawnArgs.GetInt( "cylinder", "0", numSides ) && numSides > 0 ) {
					trm.SetupCylinder( bounds, numSides < 3 ? 3 : numSides );
				} else if ( spawnArgs.GetInt( "cone", "0", numSides ) && numSides > 0 ) {
					trm.SetupCone( bounds, numSides < 3 ? 3 : numSides );
				} else {
					trm.SetupBox( bounds );
				}
				clipModel = new idClipModel( trm );
			}
		}

		// check if the visual model can be used as collision model
		if ( !clipModel ) {
			temp = spawnArgs.GetString( "model" );
			if ( ( temp != NULL ) && ( *temp != 0 ) ) {
				if ( idClipModel::CheckModel( temp ) ) {
					clipModel = new idClipModel( temp );
				}
			}
		}
	}
	else
		DM_LOG(LC_ENTITY, LT_DEBUG)LOGSTRING("Entity [%s] does not contain a clipmodel\r", name.c_str());

	defaultPhysicsObj.SetSelf( this );
	defaultPhysicsObj.SetClipModel( clipModel, 1.0f );
	defaultPhysicsObj.SetOrigin( origin );
	defaultPhysicsObj.SetAxis( axis );

	physics = &defaultPhysicsObj;
}

/*
================
idEntity::SetPhysics
================
*/
void idEntity::SetPhysics( idPhysics *phys ) {
	// clear any contacts the current physics object has
	if ( physics ) {
		physics->ClearContacts();
	}
	// set new physics object or set the default physics if NULL
	if ( phys != NULL ) {
		defaultPhysicsObj.SetClipModel( NULL, 1.0f );
		physics = phys;
		physics->Activate();
	} else {
		physics = &defaultPhysicsObj;
	}
	physics->UpdateTime( gameLocal.time );
	physics->SetMaster( bindMaster, fl.bindOrientated );
}

/*
================
idEntity::RestorePhysics
================
*/
void idEntity::RestorePhysics( idPhysics *phys ) {
	assert( phys != NULL );
	// restore physics pointer
	physics = phys;
}

/*
================
idEntity::GetPhysics
================
*/
idPhysics *idEntity::GetPhysics( void ) const {
	return physics;
}

/*
================
idEntity::RunPhysics
================
*/
bool idEntity::RunPhysics( void ) {
	int			i, reachedTime, startTime, endTime;
	idEntity *	part, *blockedPart, *blockingEntity(NULL);
	trace_t		results;
	bool		moved;

	// don't run physics if not enabled
	if ( !( thinkFlags & TH_PHYSICS ) ) {
		// however do update any animation controllers
		if ( UpdateAnimationControllers() ) {
			BecomeActive( TH_ANIMATE );
		}
		return false;
	}

	// if this entity is a team slave don't do anything because the team master will handle everything
	if ( teamMaster && teamMaster != this ) {
		return false;
	}

	startTime = gameLocal.previousTime;
	endTime = gameLocal.time;

	gameLocal.push.InitSavingPushedEntityPositions();
	blockedPart = NULL;

	// save the physics state of the whole team and disable the team for collision detection
	for ( part = this; part != NULL; part = part->teamChain ) {
		if ( part->physics ) {
			if ( !part->fl.solidForTeam ) {
				part->physics->DisableClip();
			}
			part->physics->SaveState();
		}
	}

	// move the whole team
	for ( part = this; part != NULL; part = part->teamChain ) {

		if ( part->physics ) {

			// run physics
			moved = part->physics->Evaluate( endTime - startTime, endTime );

			// check if the object is blocked
			blockingEntity = part->physics->GetBlockingEntity();
			if ( blockingEntity ) {
				blockedPart = part;
				break;
			}

			// if moved or forced to update the visual position and orientation from the physics
			if ( moved || part->fl.forcePhysicsUpdate ) {
				part->UpdateFromPhysics( false );
			}

			// update any animation controllers here so an entity bound
			// to a joint of this entity gets the correct position
			if ( part->UpdateAnimationControllers() ) {
				part->BecomeActive( TH_ANIMATE );
			}
		}
	}

	// enable the whole team for collision detection
	for ( part = this; part != NULL; part = part->teamChain ) {
		if ( part->physics ) 
		{
// Ish: I think Id screwed up here, but am not positive
			// if ( !part->fl.solidForTeam ) {
				part->physics->EnableClip();
			// }
		}
	}

	// if one of the team entities is a pusher and blocked
	if ( blockedPart ) {
		// move the parts back to the previous position
		for ( part = this; part != blockedPart; part = part->teamChain ) {

			if ( part->physics ) {

				// restore the physics state
				part->physics->RestoreState();

				// move back the visual position and orientation
				part->UpdateFromPhysics( true );
			}
		}
		for ( part = this; part != NULL; part = part->teamChain ) {
			if ( part->physics ) {
				// update the physics time without moving
				part->physics->UpdateTime( endTime );
			}
		}

		// restore the positions of any pushed entities
		gameLocal.push.RestorePushedEntityPositions();

		if ( gameLocal.isClient ) {
			return false;
		}

		// if the master pusher has a "blocked" function, call it
		Signal( SIG_BLOCKED );
		ProcessEvent( &EV_TeamBlocked, blockedPart, blockingEntity );
		// call the blocked function on the blocked part
		blockedPart->ProcessEvent( &EV_PartBlocked, blockingEntity );
		return false;
	}

	// set pushed
	for ( i = 0; i < gameLocal.push.GetNumPushedEntities(); i++ ) {
		idEntity *ent = gameLocal.push.GetPushedEntity( i );
		ent->physics->SetPushed( endTime - startTime );
	}

	if ( gameLocal.isClient ) {
		return true;
	}

	// post reached event if the current time is at or past the end point of the motion
	for ( part = this; part != NULL; part = part->teamChain ) {

		if ( part->physics ) {

			reachedTime = part->physics->GetLinearEndTime();
			if ( startTime < reachedTime && endTime >= reachedTime ) {
				part->ProcessEvent( &EV_ReachedPos );
			}
			reachedTime = part->physics->GetAngularEndTime();
			if ( startTime < reachedTime && endTime >= reachedTime ) {
				part->ProcessEvent( &EV_ReachedAng );
			}
		}
	}

	return true;
}

/*
================
idEntity::UpdateFromPhysics
================
*/
void idEntity::UpdateFromPhysics( bool moveBack ) {

	if ( IsType( idActor::Type ) ) {
		idActor *actor = static_cast<idActor *>( this );

		// set master delta angles for actors
		if ( GetBindMaster() ) {
			idAngles delta = actor->GetDeltaViewAngles();
			if ( moveBack ) {
				delta.yaw -= static_cast<idPhysics_Actor *>(physics)->GetMasterDeltaYaw();
			} else {
				delta.yaw += static_cast<idPhysics_Actor *>(physics)->GetMasterDeltaYaw();
			}
			actor->SetDeltaViewAngles( delta );
		}
	}

	UpdateVisuals();
}

/*
================
idEntity::SetOrigin
================
*/
void idEntity::SetOrigin( const idVec3 &org ) {

	GetPhysics()->SetOrigin( org );

	UpdateVisuals();
}

/*
================
idEntity::SetAxis
================
*/
void idEntity::SetAxis( const idMat3 &axis ) {

	if ( GetPhysics()->IsType( idPhysics_Actor::Type ) ) {
		static_cast<idActor *>(this)->viewAxis = axis;
	} else {
		GetPhysics()->SetAxis( axis );
	}

	UpdateVisuals();
}

/*
================
idEntity::SetAngles
================
*/
void idEntity::SetAngles( const idAngles &ang ) {
	SetAxis( ang.ToMat3() );
}

/*
================
idEntity::GetFloorPos
================
*/
bool idEntity::GetFloorPos( float max_dist, idVec3 &floorpos ) const {
	trace_t result;

	if ( !GetPhysics()->HasGroundContacts() ) {
		GetPhysics()->ClipTranslation( result, GetPhysics()->GetGravityNormal() * max_dist, NULL );
		if ( result.fraction < 1.0f ) {
			floorpos = result.endpos;
			return true;
		} else {
			floorpos = GetPhysics()->GetOrigin();
			return false;
		}
	} else {
		floorpos = GetPhysics()->GetOrigin();
		return true;
	}
}

/*
================
idEntity::GetPhysicsToVisualTransform
================
*/
bool idEntity::GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis ) {
	return false;
}

/*
================
idEntity::GetPhysicsToSoundTransform
================
*/
bool idEntity::GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis ) {
	// by default play the sound at the center of the bounding box of the first clip model
	if ( GetPhysics()->GetNumClipModels() > 0 ) {
		origin = GetPhysics()->GetBounds().GetCenter();
		axis.Identity();
		return true;
	}
	return false;
}

/*
================
idEntity::Collide
================
*/

bool idEntity::Collide( const trace_t &collision, const idVec3 &velocity ) {
	// this entity collides with collision.c.entityNum
	/*
	float v, f;
	idVec3 dir;
	idEntity *ent;
	const idMaterial *material(NULL);
	idStr SndNameLocal;
	const char *SndName(NULL);
	static const float BOUNCE_SOUND_MIN_VELOCITY	= 45.0f;
	static const float BOUNCE_SOUND_MAX_VELOCITY	= 65.0f;

	v = -( velocity * collision.c.normal );
	if ( v > BOUNCE_SOUND_MIN_VELOCITY && gameLocal.time > nextSoundTime ) 
	{
		material = collision.c.material;
		if( material != NULL)
		{
			g_Global.GetSurfName( material, SndNameLocal );
			SndNameLocal = "snd_bounce_" + SndNameLocal;
			SndName = spawnArgs.GetString( SndNameLocal.c_str() );

			if( *SndName == '\0' )
				SndNameLocal = "snd_bounce";
		}

		f = v > BOUNCE_SOUND_MAX_VELOCITY ? 1.0f : idMath::Sqrt( v - BOUNCE_SOUND_MIN_VELOCITY ) * ( 1.0f / idMath::Sqrt( BOUNCE_SOUND_MAX_VELOCITY - BOUNCE_SOUND_MIN_VELOCITY ) );
		if ( StartSound( SndNameLocal.c_str(), SND_CHANNEL_ANY, 0, false, NULL ) ) {
			// don't set the volume unless there is a bounce sound as it overrides the entire channel
			// which causes footsteps on ai's to not honor their shader parms
			SetSoundVolume( f );
		}
		nextSoundTime = gameLocal.time + 500;
	}

	if ( canDamage && damage.Length() && gameLocal.time > nextDamageTime ) {
		ent = gameLocal.entities[ collision.c.entityNum ];
		if ( ent && v > minDamageVelocity ) 
		{
			f = v > maxDamageVelocity ? 1.0f : idMath::Sqrt( v - minDamageVelocity ) * ( 1.0f / idMath::Sqrt( maxDamageVelocity - minDamageVelocity ) );
			dir = velocity;
			dir.NormalizeFast();
			ent->Damage( this, GetPhysics()->GetClipModel()->GetOwner(), dir, damage, f, CLIPMODEL_ID_TO_JOINT_HANDLE(collision.c.id), const_cast<trace_t *>(&collision) );
			nextDamageTime = gameLocal.time + 1000;
		}
	}

	//Darkmod: Cause a tactile alert if it collides with an AI
	
	ent = gameLocal.entities[ collision.c.entityNum ];
	if ( ent )
	{
		if( ent->IsType( idAI::Type ) )
		{
			idAI *alertee = static_cast<idAI *>(ent);
			alertee->TactileAlert( this );
		}
	}

	if ( fxCollide.Length() && gameLocal.time > nextCollideFxTime ) {
		idEntityFx::StartFx( fxCollide, &collision.c.point, NULL, this, false );
		nextCollideFxTime = gameLocal.time + 3500;
	}

	*/
	return false;
}
/*
================
idEntity::GetImpactInfo
================
*/
void idEntity::GetImpactInfo( idEntity *ent, int id, const idVec3 &point, impactInfo_t *info ) {
	GetPhysics()->GetImpactInfo( id, point, info );
}

/*
================
idEntity::ApplyImpulse
================
*/
void idEntity::ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse ) {
	GetPhysics()->ApplyImpulse( id, point, impulse );
}

/*
================
idEntity::AddForce
================
*/
void idEntity::AddForce( idEntity *ent, int id, const idVec3 &point, const idVec3 &force ) {
	GetPhysics()->AddForce( id, point, force );
}

/*
================
idEntity::ActivatePhysics
================
*/
void idEntity::ActivatePhysics( idEntity *ent ) {
	GetPhysics()->Activate();
}

/*
================
idEntity::IsAtRest
================
*/
bool idEntity::IsAtRest( void ) const {
	return GetPhysics()->IsAtRest();
}

/*
================
idEntity::GetRestStartTime
================
*/
int idEntity::GetRestStartTime( void ) const {
	return GetPhysics()->GetRestStartTime();
}

/*
================
idEntity::AddContactEntity
================
*/
void idEntity::AddContactEntity( idEntity *ent ) {
	GetPhysics()->AddContactEntity( ent );
}

/*
================
idEntity::RemoveContactEntity
================
*/
void idEntity::RemoveContactEntity( idEntity *ent ) {
	GetPhysics()->RemoveContactEntity( ent );
}



/***********************************************************************

	Damage
	
***********************************************************************/

/*
============
idEntity::CanDamage

Returns true if the inflictor can directly damage the target.  Used for
explosions and melee attacks.
============
*/
bool idEntity::CanDamage( const idVec3 &origin, idVec3 &damagePoint ) const {
	idVec3 	dest;
	trace_t	tr;
	idVec3 	midpoint;

	// use the midpoint of the bounds instead of the origin, because
	// bmodels may have their origin at 0,0,0
	midpoint = ( GetPhysics()->GetAbsBounds()[0] + GetPhysics()->GetAbsBounds()[1] ) * 0.5;

	dest = midpoint;
	gameLocal.clip.TracePoint( tr, origin, dest, MASK_SOLID, NULL );
	if ( tr.fraction == 1.0 || ( gameLocal.GetTraceEntity( tr ) == this ) ) {
		damagePoint = tr.endpos;
		return true;
	}

	// this should probably check in the plane of projection, rather than in world coordinate
	dest = midpoint;
	dest[0] += 15.0;
	dest[1] += 15.0;
	gameLocal.clip.TracePoint( tr, origin, dest, MASK_SOLID, NULL );
	if ( tr.fraction == 1.0 || ( gameLocal.GetTraceEntity( tr ) == this ) ) {
		damagePoint = tr.endpos;
		return true;
	}

	dest = midpoint;
	dest[0] += 15.0;
	dest[1] -= 15.0;
	gameLocal.clip.TracePoint( tr, origin, dest, MASK_SOLID, NULL );
	if ( tr.fraction == 1.0 || ( gameLocal.GetTraceEntity( tr ) == this ) ) {
		damagePoint = tr.endpos;
		return true;
	}

	dest = midpoint;
	dest[0] -= 15.0;
	dest[1] += 15.0;
	gameLocal.clip.TracePoint( tr, origin, dest, MASK_SOLID, NULL );
	if ( tr.fraction == 1.0 || ( gameLocal.GetTraceEntity( tr ) == this ) ) {
		damagePoint = tr.endpos;
		return true;
	}

	dest = midpoint;
	dest[0] -= 15.0;
	dest[1] -= 15.0;
	gameLocal.clip.TracePoint( tr, origin, dest, MASK_SOLID, NULL );
	if ( tr.fraction == 1.0 || ( gameLocal.GetTraceEntity( tr ) == this ) ) {
		damagePoint = tr.endpos;
		return true;
	}

	dest = midpoint;
	dest[2] += 15.0;
	gameLocal.clip.TracePoint( tr, origin, dest, MASK_SOLID, NULL );
	if ( tr.fraction == 1.0 || ( gameLocal.GetTraceEntity( tr ) == this ) ) {
		damagePoint = tr.endpos;
		return true;
	}

	dest = midpoint;
	dest[2] -= 15.0;
	gameLocal.clip.TracePoint( tr, origin, dest, MASK_SOLID, NULL );
	if ( tr.fraction == 1.0 || ( gameLocal.GetTraceEntity( tr ) == this ) ) {
		damagePoint = tr.endpos;
		return true;
	}

	return false;
}

/*
================
idEntity::DamageFeedback

callback function for when another entity recieved damage from this entity.  damage can be adjusted and returned to the caller.
================
*/
void idEntity::DamageFeedback( idEntity *victim, idEntity *inflictor, int &damage ) {
	// implemented in subclasses
}

/*
============
Damage

this		entity that is being damaged
inflictor	entity that is causing the damage
attacker	entity that caused the inflictor to damage targ
	example: this=monster, inflictor=rocket, attacker=player

dir			direction of the attack for knockback in global space
point		point at which the damage is being inflicted, used for headshots
damage		amount of damage being inflicted

inflictor, attacker, dir, and point can be NULL for environmental effects

============
*/
void idEntity::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, 
					  const char *damageDefName, const float damageScale, 
					  const int location, trace_t *tr ) 
{
	if ( !fl.takedamage ) {
		return;
	}

	if ( !inflictor ) {
		inflictor = gameLocal.world;
	}

	if ( !attacker ) {
		attacker = gameLocal.world;
	}

	const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName );
	if ( !damageDef ) {
		gameLocal.Error( "Unknown damageDef '%s'\n", damageDefName );
	}

	int	damage = damageDef->GetInt( "damage" );

	// inform the attacker that they hit someone
	attacker->DamageFeedback( this, inflictor, damage );
	if ( damage ) {
		// do the damage
		health -= damage;
		if ( health <= 0 ) {
			if ( health < -999 ) {
				health = -999;
			}

			Killed( inflictor, attacker, damage, dir, location );
		} else {
			Pain( inflictor, attacker, damage, dir, location );
		}
	}
}

/*
================
idEntity::AddDamageEffect
================
*/
void idEntity::AddDamageEffect( const trace_t &collision, const idVec3 &velocity, const char *damageDefName ) {
	const char *sound, *decal, *key;
	idStr surfName;

	const idDeclEntityDef *def = gameLocal.FindEntityDef( damageDefName, false );
	if ( def == NULL ) {
		return;
	}

	g_Global.GetSurfName( collision.c.material, surfName );

	// start impact sound based on material type
	key = va( "snd_%s", surfName.c_str() );
	sound = spawnArgs.GetString( key );
	if ( *sound == '\0' ) {
		sound = def->dict.GetString( key );
	}
	if ( *sound != '\0' ) {
		StartSoundShader( declManager->FindSound( sound ), SND_CHANNEL_BODY, 0, false, NULL );
	}

	if ( g_decals.GetBool() ) {
		// place a wound overlay on the model
		key = va( "mtr_wound_%s", surfName.c_str() );
		decal = spawnArgs.RandomPrefix( key, gameLocal.random );
		if ( *decal == '\0' ) {
			decal = def->dict.RandomPrefix( key, gameLocal.random );
		}
		if ( *decal != '\0' ) {
			idVec3 dir = velocity;
			dir.Normalize();
			ProjectOverlay( collision.c.point, dir, 20.0f, decal );
		}
	}
}

/*
============
idEntity::Pain

Called whenever an entity recieves damage.  Returns whether the entity responds to the pain.
This is a virtual function that subclasses are expected to implement.
============
*/
bool idEntity::Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	return false;
}

/*
============
idEntity::Killed

Called whenever an entity's health is reduced to 0 or less.
This is a virtual function that subclasses are expected to implement.
============
*/
void idEntity::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
}


/***********************************************************************

  Script functions
	
***********************************************************************/

/*
================
idEntity::ShouldConstructScriptObjectAtSpawn

Called during idEntity::Spawn to see if it should construct the script object or not.
Overridden by subclasses that need to spawn the script object themselves.
================
*/
bool idEntity::ShouldConstructScriptObjectAtSpawn( void ) const {
	return true;
}

/*
================
idEntity::ConstructScriptObject

Called during idEntity::Spawn.  Calls the constructor on the script object.
Can be overridden by subclasses when a thread doesn't need to be allocated.
================
*/
idThread *idEntity::ConstructScriptObject( void ) {
	idThread		*thread;
	const function_t *constructor;

	// init the script object's data
	scriptObject.ClearObject();

	// call script object's constructor
	constructor = scriptObject.GetConstructor();
	if ( constructor ) {
		// start a thread that will initialize after Spawn is done being called
		thread = new idThread();
		thread->SetThreadName( name.c_str() );
		thread->CallFunction( this, constructor, true );
		thread->DelayedStart( 0 );
	} else {
		thread = NULL;
	}

	// clear out the object's memory
	scriptObject.ClearObject();

	return thread;
}

/*
================
idEntity::DeconstructScriptObject

Called during idEntity::~idEntity.  Calls the destructor on the script object.
Can be overridden by subclasses when a thread doesn't need to be allocated.
Not called during idGameLocal::MapShutdown.
================
*/
void idEntity::DeconstructScriptObject( void ) {
	idThread		*thread;
	const function_t *destructor;

	// don't bother calling the script object's destructor on map shutdown
	if ( gameLocal.GameState() == GAMESTATE_SHUTDOWN ) {
		return;
	}

	// call script object's destructor
	destructor = scriptObject.GetDestructor();
	if ( destructor ) {
		// start a thread that will run immediately and be destroyed
		thread = new idThread();
		thread->SetThreadName( name.c_str() );
		thread->CallFunction( this, destructor, true );
		thread->Execute();
		delete thread;
	}
}

/*
================
idEntity::HasSignal
================
*/
bool idEntity::HasSignal( signalNum_t signalnum ) const {
	if ( !signals ) {
		return false;
	}
	assert( ( signalnum >= 0 ) && ( signalnum < NUM_SIGNALS ) );
	return ( signals->signal[ signalnum ].Num() > 0 );
}

/*
================
idEntity::SetSignal
================
*/
void idEntity::SetSignal( signalNum_t signalnum, idThread *thread, const function_t *function ) {
	int			i;
	int			num;
	signal_t	sig;
	int			threadnum;

	assert( ( signalnum >= 0 ) && ( signalnum < NUM_SIGNALS ) );

	if ( !signals ) {
		signals = new signalList_t;
	}

	assert( thread );
	threadnum = thread->GetThreadNum();

	num = signals->signal[ signalnum ].Num();
	for( i = 0; i < num; i++ ) {
		if ( signals->signal[ signalnum ][ i ].threadnum == threadnum ) {
			signals->signal[ signalnum ][ i ].function = function;
			return;
		}
	}

	if ( num >= MAX_SIGNAL_THREADS ) {
		thread->Error( "Exceeded maximum number of signals per object" );
	}

	sig.threadnum = threadnum;
	sig.function = function;
	signals->signal[ signalnum ].Append( sig );
}

/*
================
idEntity::ClearSignal
================
*/
void idEntity::ClearSignal( idThread *thread, signalNum_t signalnum ) {
	assert( thread );
	if ( ( signalnum < 0 ) || ( signalnum >= NUM_SIGNALS ) ) {
		gameLocal.Error( "Signal out of range" );
	}

	if ( !signals ) {
		return;
	}

	signals->signal[ signalnum ].Clear();
}

/*
================
idEntity::ClearSignalThread
================
*/
void idEntity::ClearSignalThread( signalNum_t signalnum, idThread *thread ) {
	int	i;
	int	num;
	int	threadnum;

	assert( thread );

	if ( ( signalnum < 0 ) || ( signalnum >= NUM_SIGNALS ) ) {
		gameLocal.Error( "Signal out of range" );
	}

	if ( !signals ) {
		return;
	}

	threadnum = thread->GetThreadNum();

	num = signals->signal[ signalnum ].Num();
	for( i = 0; i < num; i++ ) {
		if ( signals->signal[ signalnum ][ i ].threadnum == threadnum ) {
			signals->signal[ signalnum ].RemoveIndex( i );
			return;
		}
	}
}

/*
================
idEntity::Signal
================
*/
void idEntity::Signal( signalNum_t signalnum ) {
	int			i;
	int			num;
	signal_t	sigs[ MAX_SIGNAL_THREADS ];
	idThread	*thread;

	assert( ( signalnum >= 0 ) && ( signalnum < NUM_SIGNALS ) );

	if ( !signals ) {
		return;
	}

	// we copy the signal list since each thread has the potential
	// to end any of the threads in the list.  By copying the list
	// we don't have to worry about the list changing as we're
	// processing it.
	num = signals->signal[ signalnum ].Num();
	for( i = 0; i < num; i++ ) {
		sigs[ i ] = signals->signal[ signalnum ][ i ];
	}

	// clear out the signal list so that we don't get into an infinite loop
	signals->signal[ signalnum ].Clear();

	for( i = 0; i < num; i++ ) {
		thread = idThread::GetThread( sigs[ i ].threadnum );
		if ( thread ) {
			thread->CallFunction( this, sigs[ i ].function, true );
			thread->Execute();
		}
	}
}

/*
================
idEntity::SignalEvent
================
*/
void idEntity::SignalEvent( idThread *thread, signalNum_t signalnum ) {
	if ( ( signalnum < 0 ) || ( signalnum >= NUM_SIGNALS ) ) {
		gameLocal.Error( "Signal out of range" );
	}

	if ( !signals ) {
		return;
	}

	Signal( signalnum );
}

/***********************************************************************

  Guis.
	
***********************************************************************/


/*
================
idEntity::TriggerGuis
================
*/
void idEntity::TriggerGuis( void ) {
	int i;
	for ( i = 0; i < MAX_RENDERENTITY_GUI; i++ ) {
		if ( renderEntity.gui[ i ] ) {
			renderEntity.gui[ i ]->Trigger( gameLocal.time );
		}
	}
}

/*
================
idEntity::HandleGuiCommands
================
*/
bool idEntity::HandleGuiCommands( idEntity *entityGui, const char *cmds ) {
	idEntity *targetEnt;
	bool ret = false;
	if ( entityGui && cmds && *cmds ) {
		idLexer src;
		idToken token, token2, token3, token4;
		src.LoadMemory( cmds, strlen( cmds ), "guiCommands" );
		while( 1 ) {

			if ( !src.ReadToken( &token ) ) {
				return ret;
			}

			if ( token == ";" ) {
				continue;
			}

			if ( token.Icmp( "activate" ) == 0 ) {
				bool targets = true;
				if ( src.ReadToken( &token2 ) ) {
					if ( token2 == ";" ) {
						src.UnreadToken( &token2 );
					} else {
						targets = false;
					}
				}

				if ( targets ) {
					entityGui->ActivateTargets( this );
				} else {
					idEntity *ent = gameLocal.FindEntity( token2 );
					if ( ent ) {
						ent->Signal( SIG_TRIGGER );
						ent->PostEventMS( &EV_Activate, 0, this );
					}
				}

				entityGui->renderEntity.shaderParms[ SHADERPARM_MODE ] = 1.0f;
				continue;
			}


			if ( token.Icmp( "runScript" ) == 0 ) {
				if ( src.ReadToken( &token2 ) ) {
					while( src.CheckTokenString( "::" ) ) {
						idToken token3;
						if ( !src.ReadToken( &token3 ) ) {
							gameLocal.Error( "Expecting function name following '::' in gui for entity '%s'", entityGui->name.c_str() );
						}
						token2 += "::" + token3;
					}
					const function_t *func = gameLocal.program.FindFunction( token2 );
					if ( !func ) {
						gameLocal.Error( "Can't find function '%s' for gui in entity '%s'", token2.c_str(), entityGui->name.c_str() );
					} else {
						idThread *thread = new idThread( func );
						thread->DelayedStart( 0 );
					}
				}
				continue;
			}

			if ( token.Icmp("play") == 0 ) {
				if ( src.ReadToken( &token2 ) ) {
					const idSoundShader *shader = declManager->FindSound(token2);
					entityGui->StartSoundShader( shader, SND_CHANNEL_ANY, 0, false, NULL );
				}
				continue;
			}

			if ( token.Icmp( "setkeyval" ) == 0 ) {
				if ( src.ReadToken( &token2 ) && src.ReadToken(&token3) && src.ReadToken( &token4 ) ) {
					idEntity *ent = gameLocal.FindEntity( token2 );
					if ( ent ) {
						ent->spawnArgs.Set( token3, token4 );
						ent->UpdateChangeableSpawnArgs( NULL );
						ent->UpdateVisuals();
					}
				}
				continue;
			}

			if ( token.Icmp( "setshaderparm" ) == 0 ) {
				if ( src.ReadToken( &token2 ) && src.ReadToken(&token3) ) {
					entityGui->SetShaderParm( atoi( token2 ), atof( token3 ) );
					entityGui->UpdateVisuals();
				}
				continue;
			}

			if ( token.Icmp("close") == 0 ) {
				ret = true;
				continue;
			}

			if ( !token.Icmp( "turkeyscore" ) ) {
				if ( src.ReadToken( &token2 ) && entityGui->renderEntity.gui[0] ) {
					int score = entityGui->renderEntity.gui[0]->State().GetInt( "score" );
					score += atoi( token2 );
					entityGui->renderEntity.gui[0]->SetStateInt( "score", score );
					if ( gameLocal.GetLocalPlayer() && score >= 25000 && !gameLocal.GetLocalPlayer()->inventory.turkeyScore ) {
						gameLocal.GetLocalPlayer()->GiveEmail( "highScore" );
						gameLocal.GetLocalPlayer()->inventory.turkeyScore = true;
					}
				}
				continue;
			}


			// handy for debugging GUI stuff
			if ( !token.Icmp( "print" ) ) {
				idStr msg;
				while ( src.ReadToken( &token2 ) ) {
					if ( token2 == ";" ) {
						src.UnreadToken( &token2 );
						break;
					}
					msg += token2.c_str();
				}
				common->Printf( "ent gui 0x%x '%s': %s\n", entityNumber, name.c_str(), msg.c_str() );
				continue;
			}

			// if we get to this point we don't know how to handle it
			src.UnreadToken(&token);
			if ( !HandleSingleGuiCommand( entityGui, &src ) ) {
				// not handled there see if entity or any of its targets can handle it
				// this will only work for one target atm
				if ( entityGui->HandleSingleGuiCommand( entityGui, &src ) ) {
					continue;
				}

				int c = entityGui->targets.Num();
				int i;
				for ( i = 0; i < c; i++) {
					targetEnt = entityGui->targets[ i ].GetEntity();
					if ( targetEnt && targetEnt->HandleSingleGuiCommand( entityGui, &src ) ) {
						break;
					}
				}

				if ( i == c ) {
					// not handled
					common->DPrintf( "idEntity::HandleGuiCommands: '%s' not handled\n", token.c_str() );
					src.ReadToken( &token );
				}
			}

		}
	}
	return ret;
}

/*
================
idEntity::HandleSingleGuiCommand
================
*/
bool idEntity::HandleSingleGuiCommand( idEntity *entityGui, idLexer *src ) {
	return false;
}

/***********************************************************************

  Targets
	
***********************************************************************/

/*
===============
idEntity::FindTargets

We have to wait until all entities are spawned
Used to build lists of targets after the entity is spawned.  Since not all entities
have been spawned when the entity is created at map load time, we have to wait
===============
*/
void idEntity::FindTargets( void ) {
	int			i;

	// targets can be a list of multiple names
	gameLocal.GetTargets( spawnArgs, targets, "target" );

	// ensure that we don't target ourselves since that could cause an infinite loop when activating entities
	for( i = 0; i < targets.Num(); i++ ) {
		if ( targets[ i ].GetEntity() == this ) {
			gameLocal.Error( "Entity '%s' is targeting itself", name.c_str() );
		}
	}
}

/*
================
idEntity::RemoveNullTargets
================
*/
void idEntity::RemoveNullTargets( void ) {
	int i;

	for( i = targets.Num() - 1; i >= 0; i-- ) {
		if ( !targets[ i ].GetEntity() ) {
			targets.RemoveIndex( i );
		}
	}
}

/*
==============================
idEntity::ActivateTargets

"activator" should be set to the entity that initiated the firing.
==============================
*/
void idEntity::ActivateTargets( idEntity *activator ) const {
	idEntity	*ent;
	int			i, j;
	
	for( i = 0; i < targets.Num(); i++ ) {
		ent = targets[ i ].GetEntity();
		if ( !ent ) {
			continue;
		}
		if ( ent->RespondsTo( EV_Activate ) || ent->HasSignal( SIG_TRIGGER ) ) {
			ent->Signal( SIG_TRIGGER );
			ent->ProcessEvent( &EV_Activate, activator );
		} 		
		for ( j = 0; j < MAX_RENDERENTITY_GUI; j++ ) {
			if ( ent->renderEntity.gui[ j ] ) {
				ent->renderEntity.gui[ j ]->Trigger( gameLocal.time );
			}
		}
	}
}

/***********************************************************************

  Misc.
	
***********************************************************************/

/*
================
idEntity::Teleport
================
*/
void idEntity::Teleport( const idVec3 &origin, const idAngles &angles, idEntity *destination ) {
	GetPhysics()->SetOrigin( origin );
	GetPhysics()->SetAxis( angles.ToMat3() );

	UpdateVisuals();
}

/*
============
idEntity::TouchTriggers

  Activate all trigger entities touched at the current position.
============
*/
bool idEntity::TouchTriggers( void ) const {
	int				i, numClipModels, numEntities;
	idClipModel *	cm;
	idClipModel *	clipModels[ MAX_GENTITIES ];
	idEntity *		ent;
	trace_t			trace;

	memset( &trace, 0, sizeof( trace ) );
	trace.endpos = GetPhysics()->GetOrigin();
	trace.endAxis = GetPhysics()->GetAxis();

	numClipModels = gameLocal.clip.ClipModelsTouchingBounds( GetPhysics()->GetAbsBounds(), CONTENTS_TRIGGER, clipModels, MAX_GENTITIES );
	numEntities = 0;

	for ( i = 0; i < numClipModels; i++ ) {
		cm = clipModels[ i ];

		// don't touch it if we're the owner
		if ( cm->GetOwner() == this ) {
			continue;
		}

		ent = cm->GetEntity();

		if ( !ent->RespondsTo( EV_Touch ) && !ent->HasSignal( SIG_TOUCH ) ) {
			continue;
		}

		if ( !GetPhysics()->ClipContents( cm ) ) {
			continue;
		}

		numEntities++;

		trace.c.contents = cm->GetContents();
		trace.c.entityNum = cm->GetEntity()->entityNumber;
		trace.c.id = cm->GetId();

		ent->Signal( SIG_TOUCH );
		ent->ProcessEvent( &EV_Touch, this, &trace );

		if ( !gameLocal.entities[ entityNumber ] ) {
			gameLocal.Printf( "entity was removed while touching triggers\n" );
			return true;
		}
	}

	return ( numEntities != 0 );
}

/*
================
idEntity::GetSpline
================
*/
idCurve_Spline<idVec3> *idEntity::GetSpline( void ) const {
	int i, numPoints, t;
	const idKeyValue *kv;
	idLexer lex;
	idVec3 v;
	idCurve_Spline<idVec3> *spline;
	const char *curveTag = "curve_";

	kv = spawnArgs.MatchPrefix( curveTag );
	if ( !kv ) {
		return NULL;
	}

	idStr str = kv->GetKey().Right( kv->GetKey().Length() - strlen( curveTag ) );
	if ( str.Icmp( "CatmullRomSpline" ) == 0 ) {
		spline = new idCurve_CatmullRomSpline<idVec3>();
	} else if ( str.Icmp( "nubs" ) == 0 ) {
		spline = new idCurve_NonUniformBSpline<idVec3>();
	} else if ( str.Icmp( "nurbs" ) == 0 ) {
		spline = new idCurve_NURBS<idVec3>();
	} else {
		spline = new idCurve_BSpline<idVec3>();
	}

	spline->SetBoundaryType( idCurve_Spline<idVec3>::BT_CLAMPED );

	lex.LoadMemory( kv->GetValue(), kv->GetValue().Length(), curveTag );
	numPoints = lex.ParseInt();
	lex.ExpectTokenString( "(" );
	for ( t = i = 0; i < numPoints; i++, t += 100 ) {
		v.x = lex.ParseFloat();
		v.y = lex.ParseFloat();
		v.z = lex.ParseFloat();
		spline->AddValue( t, v );
	}
	lex.ExpectTokenString( ")" );

	return spline;
}

/*
===============
idEntity::ShowEditingDialog
===============
*/
void idEntity::ShowEditingDialog( void ) {
}

/***********************************************************************

   Events
	
***********************************************************************/

/*
================
idEntity::Event_GetName
================
*/
void idEntity::Event_GetName( void ) {
	idThread::ReturnString( name.c_str() );
}

/*
================
idEntity::Event_SetName
================
*/
void idEntity::Event_SetName( const char *newname ) {
	SetName( newname );
}

/*
================
idEntity::Event_GetTypeName
================
*/
void idEntity::Event_IsType( const char *pstr_typeName ) 
{
	idTypeInfo* p_namedType = GetClass (pstr_typeName);
	if (p_namedType == NULL)
	{
		idThread::ReturnInt (0);
	}
	else
	{
		if (IsType (*p_namedType))
		{
			idThread::ReturnInt (1);
		}
		else
		{
			idThread::ReturnInt (0);
		}
	}
}


/*
===============
idEntity::Event_FindTargets
===============
*/
void idEntity::Event_FindTargets( void ) {
	FindTargets();
}

/*
============
idEntity::Event_ActivateTargets

Activates any entities targeted by this entity.  Mainly used as an
event to delay activating targets.
============
*/
void idEntity::Event_ActivateTargets( idEntity *activator ) {
	ActivateTargets( activator );
}

/*
================
idEntity::Event_NumTargets
================
*/
void idEntity::Event_NumTargets( void ) {
	idThread::ReturnFloat( targets.Num() );
}

/*
================
idEntity::Event_GetTarget
================
*/
void idEntity::Event_GetTarget( float index ) {
	int i;

	i = ( int )index;
	if ( ( i < 0 ) || i >= targets.Num() ) {
		idThread::ReturnEntity( NULL );
	} else {
		idThread::ReturnEntity( targets[ i ].GetEntity() );
	}
}

/*
================
idEntity::Event_RandomTarget
================
*/
void idEntity::Event_RandomTarget( const char *ignore ) {
	int			num;
	idEntity	*ent;
	int			i;
	int			ignoreNum;

	RemoveNullTargets();
	if ( !targets.Num() ) {
		idThread::ReturnEntity( NULL );
		return;
	}

	ignoreNum = -1;
	if ( ignore && ( ignore[ 0 ] != 0 ) && ( targets.Num() > 1 ) ) {
		for( i = 0; i < targets.Num(); i++ ) {
			ent = targets[ i ].GetEntity();
			if ( ent && ( ent->name == ignore ) ) {
				ignoreNum = i;
				break;
			}
		}
	}

	if ( ignoreNum >= 0 ) {
		num = gameLocal.random.RandomInt( targets.Num() - 1 );
		if ( num >= ignoreNum ) {
			num++;
		}
	} else {
		num = gameLocal.random.RandomInt( targets.Num() );
	}

	ent = targets[ num ].GetEntity();
	idThread::ReturnEntity( ent );
}

/*
================
idEntity::Event_BindToJoint
================
*/
void idEntity::Event_BindToJoint( idEntity *master, const char *jointname, float orientated ) {
	BindToJoint( master, jointname, ( orientated != 0.0f ) );
}

/*
================
idEntity::Event_RemoveBinds
================
*/
void idEntity::Event_RemoveBinds( void ) {
	RemoveBinds();
}

/*
================
idEntity::Event_Bind
================
*/
void idEntity::Event_Bind( idEntity *master ) {
	Bind( master, true );
}

/*
================
idEntity::Event_BindPosition
================
*/
void idEntity::Event_BindPosition( idEntity *master ) {
	Bind( master, false );
}

/*
================
idEntity::Event_Unbind
================
*/
void idEntity::Event_Unbind( void ) {
	Unbind();
}

/*
================
idEntity::Event_SpawnBind
================
*/
void idEntity::Event_SpawnBind( void ) {
	idEntity		*parent;
	const char		*bind, *joint, *bindanim;
	jointHandle_t	bindJoint;
	bool			bindOrientated;
	int				id;
	const idAnim	*anim;
	int				animNum;
	idAnimator		*parentAnimator;
	
	if ( spawnArgs.GetString( "bind", "", &bind ) ) {
		if ( idStr::Icmp( bind, "worldspawn" ) == 0 ) {
			//FIXME: Completely unneccessary since the worldspawn is called "world"
			parent = gameLocal.world;
		} else {
			parent = gameLocal.FindEntity( bind );
		}
		bindOrientated = spawnArgs.GetBool( "bindOrientated", "1" );
		if ( parent ) {
			// bind to a joint of the skeletal model of the parent
			if ( spawnArgs.GetString( "bindToJoint", "", &joint ) && *joint ) {
				parentAnimator = parent->GetAnimator();
				if ( !parentAnimator ) {
					gameLocal.Error( "Cannot bind to joint '%s' on '%s'.  Entity does not support skeletal models.", joint, name.c_str() );
				}
				bindJoint = parentAnimator->GetJointHandle( joint );
				if ( bindJoint == INVALID_JOINT ) {
					gameLocal.Error( "Joint '%s' not found for bind on '%s'", joint, name.c_str() );
				}

				// bind it relative to a specific anim
				if ( ( parent->spawnArgs.GetString( "bindanim", "", &bindanim ) || parent->spawnArgs.GetString( "anim", "", &bindanim ) ) && *bindanim ) {
					animNum = parentAnimator->GetAnim( bindanim );
					if ( !animNum ) {
						gameLocal.Error( "Anim '%s' not found for bind on '%s'", bindanim, name.c_str() );
					}
					anim = parentAnimator->GetAnim( animNum );
					if ( !anim ) {
						gameLocal.Error( "Anim '%s' not found for bind on '%s'", bindanim, name.c_str() );
					}

					// make sure parent's render origin has been set
					parent->UpdateModelTransform();

					//FIXME: need a BindToJoint that accepts a joint position
					parentAnimator->CreateFrame( gameLocal.time, true );
					idJointMat *frame = parent->renderEntity.joints;
					gameEdit->ANIM_CreateAnimFrame( parentAnimator->ModelHandle(), anim->MD5Anim( 0 ), parent->renderEntity.numJoints, frame, 0, parentAnimator->ModelDef()->GetVisualOffset(), parentAnimator->RemoveOrigin() );
					BindToJoint( parent, joint, bindOrientated );
					parentAnimator->ForceUpdate();
				} else {
					BindToJoint( parent, joint, bindOrientated );
				}
			}
			// bind to a body of the physics object of the parent
			else if ( spawnArgs.GetInt( "bindToBody", "0", id ) ) {
				BindToBody( parent, id, bindOrientated );
			}
			// bind to the parent
			else {
				Bind( parent, bindOrientated );
			}
		}
	}
}

/*
================
idEntity::Event_SetOwner
================
*/
void idEntity::Event_SetOwner( idEntity *owner ) {
	int i;

	for ( i = 0; i < GetPhysics()->GetNumClipModels(); i++ ) {
		GetPhysics()->GetClipModel( i )->SetOwner( owner );
	}
}

/*
================
idEntity::Event_SetModel
================
*/
void idEntity::Event_SetModel( const char *modelname ) {
	SetModel( modelname );
}

/*
================
idEntity::Event_SetSkin
================
*/
void idEntity::Event_SetSkin( const char *skinname ) {
	renderEntity.customSkin = declManager->FindSkin( skinname );
	UpdateVisuals();
}

/*
================
idEntity::Event_GetShaderParm
================
*/
void idEntity::Event_GetShaderParm( int parmnum ) {
	if ( ( parmnum < 0 ) || ( parmnum >= MAX_ENTITY_SHADER_PARMS ) ) {
		gameLocal.Error( "shader parm index (%d) out of range", parmnum );
	}

	idThread::ReturnFloat( renderEntity.shaderParms[ parmnum ] );
}

/*
================
idEntity::Event_SetShaderParm
================
*/
void idEntity::Event_SetShaderParm( int parmnum, float value ) {
	SetShaderParm( parmnum, value );
}

/*
================
idEntity::Event_SetShaderParms
================
*/
void idEntity::Event_SetShaderParms( float parm0, float parm1, float parm2, float parm3 ) {
	renderEntity.shaderParms[ SHADERPARM_RED ]		= parm0;
	renderEntity.shaderParms[ SHADERPARM_GREEN ]	= parm1;
	renderEntity.shaderParms[ SHADERPARM_BLUE ]		= parm2;
	renderEntity.shaderParms[ SHADERPARM_ALPHA ]	= parm3;
	UpdateVisuals();
}


/*
================
idEntity::Event_SetColor
================
*/
void idEntity::Event_SetColor( float red, float green, float blue ) {
	SetColor( red, green, blue );
}

/*
================
idEntity::Event_GetColor
================
*/
void idEntity::Event_GetColor( void ) {
	idVec3 out;

	GetColor( out );
	idThread::ReturnVector( out );
}

/*
================
idEntity::Event_IsHidden
================
*/
void idEntity::Event_IsHidden( void ) {
	idThread::ReturnInt( fl.hidden );
}

/*
================
idEntity::Event_Hide
================
*/
void idEntity::Event_Hide( void ) {
	Hide();
}

/*
================
idEntity::Event_Show
================
*/
void idEntity::Event_Show( void ) {
	Show();
}

/*
================
idEntity::Event_CacheSoundShader
================
*/
void idEntity::Event_CacheSoundShader( const char *soundName ) {
	declManager->FindSound( soundName );
}

/*
================
idEntity::Event_StartSoundShader
================
*/
void idEntity::Event_StartSoundShader( const char *soundName, int channel ) {
	int length;

	StartSoundShader( declManager->FindSound( soundName ), (s_channelType)channel, 0, false, &length );
	idThread::ReturnFloat( MS2SEC( length ) );
}

/*
================
idEntity::Event_StopSound
================
*/
void idEntity::Event_StopSound( int channel, int netSync ) {
	StopSound( channel, ( netSync != 0 ) );
}

/*
================
idEntity::Event_StartSound 
================
*/
void idEntity::Event_StartSound( const char *soundName, int channel, int netSync ) {
	int time;
	
	StartSound( soundName, ( s_channelType )channel, 0, ( netSync != 0 ), &time );
	idThread::ReturnFloat( MS2SEC( time ) );
}

/*
================
idEntity::Event_FadeSound
================
*/
void idEntity::Event_FadeSound( int channel, float to, float over ) {
	if ( refSound.referenceSound ) {
		refSound.referenceSound->FadeSound( channel, to, over );
	}
}

/*
================
idEntity::Event_GetWorldOrigin
================
*/
void idEntity::Event_GetWorldOrigin( void ) {
	idThread::ReturnVector( GetPhysics()->GetOrigin() );
}

/*
================
idEntity::Event_SetWorldOrigin
================
*/
void idEntity::Event_SetWorldOrigin( idVec3 const &org ) {
	idVec3 neworg = GetLocalCoordinates( org );
	SetOrigin( neworg );
}

/*
================
idEntity::Event_SetOrigin
================
*/
void idEntity::Event_SetOrigin( idVec3 const &org ) {
	SetOrigin( org );
}

/*
================
idEntity::Event_GetOrigin
================
*/
void idEntity::Event_GetOrigin( void ) {
	idThread::ReturnVector( GetLocalCoordinates( GetPhysics()->GetOrigin() ) );
}

/*
================
idEntity::Event_SetAngles
================
*/
void idEntity::Event_SetAngles( idAngles const &ang ) {
	SetAngles( ang );
}

/*
================
idEntity::Event_GetAngles
================
*/
void idEntity::Event_GetAngles( void ) {
	idAngles ang = GetPhysics()->GetAxis().ToAngles();
	idThread::ReturnVector( idVec3( ang[0], ang[1], ang[2] ) );
}

/*
================
idEntity::Event_SetLinearVelocity
================
*/
void idEntity::Event_SetLinearVelocity( const idVec3 &velocity ) {
	GetPhysics()->SetLinearVelocity( velocity );
}

/*
================
idEntity::Event_GetLinearVelocity
================
*/
void idEntity::Event_GetLinearVelocity( void ) {
	idThread::ReturnVector( GetPhysics()->GetLinearVelocity() );
}

/*
================
idEntity::Event_SetAngularVelocity
================
*/
void idEntity::Event_SetAngularVelocity( const idVec3 &velocity ) {
	GetPhysics()->SetAngularVelocity( velocity );
}

/*
================
idEntity::Event_GetAngularVelocity
================
*/
void idEntity::Event_GetAngularVelocity( void ) {
	idThread::ReturnVector( GetPhysics()->GetAngularVelocity() );
}

/*
================
idEntity::Event_SetSize
================
*/
void idEntity::Event_SetSize( idVec3 const &mins, idVec3 const &maxs ) {
	GetPhysics()->SetClipBox( idBounds( mins, maxs ), 1.0f );
}

/*
================
idEntity::Event_GetSize
================
*/
void idEntity::Event_GetSize( void ) {
	idBounds bounds;

	bounds = GetPhysics()->GetBounds();
	idThread::ReturnVector( bounds[1] - bounds[0] );
}

/*
================
idEntity::Event_GetMins
================
*/
void idEntity::Event_GetMins( void ) {
	idThread::ReturnVector( GetPhysics()->GetBounds()[0] );
}

/*
================
idEntity::Event_GetMaxs
================
*/
void idEntity::Event_GetMaxs( void ) {
	idThread::ReturnVector( GetPhysics()->GetBounds()[1] );
}

/*
================
idEntity::Event_Touches
================
*/
void idEntity::Event_Touches( idEntity *ent ) {
	if ( !ent ) {
		idThread::ReturnInt( false );
		return;
	}

	const idBounds &myBounds = GetPhysics()->GetAbsBounds();
	const idBounds &entBounds = ent->GetPhysics()->GetAbsBounds();

	idThread::ReturnInt( myBounds.IntersectsBounds( entBounds ) );
}

/*
================
idEntity::Event_GetNextKey
================
*/
void idEntity::Event_GetNextKey( const char *prefix, const char *lastMatch ) {
	const idKeyValue *kv;
	const idKeyValue *previous;

	if ( *lastMatch ) {
		previous = spawnArgs.FindKey( lastMatch );
	} else {
		previous = NULL;
	}

	kv = spawnArgs.MatchPrefix( prefix, previous );
	if ( !kv ) {
		idThread::ReturnString( "" );
	} else {
		idThread::ReturnString( kv->GetKey() );
	}
}

/*
================
idEntity::Event_SetKey
================
*/
void idEntity::Event_SetKey( const char *key, const char *value ) {
	spawnArgs.Set( key, value );
}

/*
================
idEntity::Event_GetKey
================
*/
void idEntity::Event_GetKey( const char *key ) {
	const char *value;

	spawnArgs.GetString( key, "", &value );
	idThread::ReturnString( value );
}

/*
================
idEntity::Event_GetIntKey
================
*/
void idEntity::Event_GetIntKey( const char *key ) {
	int value;

	spawnArgs.GetInt( key, "0", value );

	// scripts only support floats
	idThread::ReturnFloat( value );
}

/*
================
idEntity::Event_GetFloatKey
================
*/
void idEntity::Event_GetFloatKey( const char *key ) {
	float value;

	spawnArgs.GetFloat( key, "0", value );
	idThread::ReturnFloat( value );
}

/*
================
idEntity::Event_GetVectorKey
================
*/
void idEntity::Event_GetVectorKey( const char *key ) {
	idVec3 value;

	spawnArgs.GetVector( key, "0 0 0", value );
	idThread::ReturnVector( value );
}

/*
================
idEntity::Event_GetEntityKey
================
*/
void idEntity::Event_GetEntityKey( const char *key ) {
	idEntity *ent;
	const char *entname;

	if ( !spawnArgs.GetString( key, NULL, &entname ) ) {
		idThread::ReturnEntity( NULL );
		return;
	}

	ent = gameLocal.FindEntity( entname );
	if ( !ent ) {
		gameLocal.Warning( "Couldn't find entity '%s' specified in '%s' key in entity '%s'", entname, key, name.c_str() );
	}

	idThread::ReturnEntity( ent );
}

/*
================
idEntity::Event_RemoveKey
================
*/
void idEntity::Event_RemoveKey( const char *key ) {
	spawnArgs.Delete( key );
}

/*
================
idEntity::Event_RestorePosition
================
*/
void idEntity::Event_RestorePosition( void ) {
	idVec3		org;
	idAngles	angles;
	idMat3		axis;
	idEntity *	part;

	spawnArgs.GetVector( "origin", "0 0 0", org );

	// get the rotation matrix in either full form, or single angle form
	if ( spawnArgs.GetMatrix( "rotation", "1 0 0 0 1 0 0 0 1", axis ) ) {
		angles = axis.ToAngles();
	} else {
   		angles[ 0 ] = 0;
   		angles[ 1 ] = spawnArgs.GetFloat( "angle" );
   		angles[ 2 ] = 0;
	}

	Teleport( org, angles, NULL );

	for ( part = teamChain; part != NULL; part = part->teamChain ) {
		if ( part->bindMaster != this ) {
			continue;
		}
		if ( part->GetPhysics()->IsType( idPhysics_Parametric::Type ) ) {
			if ( static_cast<idPhysics_Parametric *>(part->GetPhysics())->IsPusher() ) {
				gameLocal.Warning( "teleported '%s' which has the pushing mover '%s' bound to it\n", GetName(), part->GetName() );
			}
		} else if ( part->GetPhysics()->IsType( idPhysics_AF::Type ) ) {
			gameLocal.Warning( "teleported '%s' which has the articulated figure '%s' bound to it\n", GetName(), part->GetName() );
		}
	}
}

/*
================
idEntity::Event_UpdateCameraTarget
================
*/
void idEntity::Event_UpdateCameraTarget( void ) {
	const char *target;
	const idKeyValue *kv;
	idVec3 dir;

	target = spawnArgs.GetString( "cameraTarget" );

	cameraTarget = gameLocal.FindEntity( target );

	if ( cameraTarget ) {
		kv = cameraTarget->spawnArgs.MatchPrefix( "target", NULL );
		while( kv ) {
			idEntity *ent = gameLocal.FindEntity( kv->GetValue() );
			if ( ent && idStr::Icmp( ent->GetEntityDefName(), "target_null" ) == 0) {
				dir = ent->GetPhysics()->GetOrigin() - cameraTarget->GetPhysics()->GetOrigin();
				dir.Normalize();
				cameraTarget->SetAxis( dir.ToMat3() );
				SetAxis(dir.ToMat3());
				break;						
			}
			kv = cameraTarget->spawnArgs.MatchPrefix( "target", kv );
		}
	}
	UpdateVisuals();
}

/*
================
idEntity::Event_DistanceTo
================
*/
void idEntity::Event_DistanceTo( idEntity *ent ) {
	if ( !ent ) {
		// just say it's really far away
		idThread::ReturnFloat( MAX_WORLD_SIZE );
	} else {
		float dist = ( GetPhysics()->GetOrigin() - ent->GetPhysics()->GetOrigin() ).LengthFast();
		idThread::ReturnFloat( dist );
	}
}

/*
================
idEntity::Event_DistanceToPoint
================
*/
void idEntity::Event_DistanceToPoint( const idVec3 &point ) {
	float dist = ( GetPhysics()->GetOrigin() - point ).LengthFast();
	idThread::ReturnFloat( dist );
}

/*
================
idEntity::Event_StartFx
================
*/
void idEntity::Event_StartFx( const char *fx ) {
	idEntityFx::StartFx( fx, NULL, NULL, this, true );
}

/*
================
idEntity::Event_WaitFrame
================
*/
void idEntity::Event_WaitFrame( void ) {
	idThread *thread;
	
	thread = idThread::CurrentThread();
	if ( thread ) {
		thread->WaitFrame();
	}
}

/*
=====================
idEntity::Event_Wait
=====================
*/
void idEntity::Event_Wait( float time ) {
	idThread *thread = idThread::CurrentThread();

	if ( !thread ) {
		gameLocal.Error( "Event 'wait' called from outside thread" );
	}

	thread->WaitSec( time );
}

/*
=====================
idEntity::Event_HasFunction
=====================
*/
void idEntity::Event_HasFunction( const char *name ) {
	const function_t *func;

	func = scriptObject.GetFunction( name );
	if ( func ) {
		idThread::ReturnInt( true );
	} else {
		idThread::ReturnInt( false );
	}
}

/*
=====================
idEntity::Event_CallFunction
=====================
*/
void idEntity::Event_CallFunction( const char *funcname ) {
	const function_t *func;
	idThread *thread;

	thread = idThread::CurrentThread();
	if ( !thread ) {
		gameLocal.Error( "Event 'callFunction' called from outside thread" );
	}

	func = scriptObject.GetFunction( funcname );
	if ( !func ) {
		gameLocal.Error( "Unknown function '%s' in '%s'", funcname, scriptObject.GetTypeName() );
	}

	if ( func->type->NumParameters() != 1 ) {
		gameLocal.Error( "Function '%s' has the wrong number of parameters for 'callFunction'", funcname );
	}
	if ( !scriptObject.GetTypeDef()->Inherits( func->type->GetParmType( 0 ) ) ) {
		gameLocal.Error( "Function '%s' is the wrong type for 'callFunction'", funcname );
	}

	// function args will be invalid after this call
	thread->CallFunction( this, func, false );
}

/*
================
idEntity::Event_SetNeverDormant
================
*/
void idEntity::Event_SetNeverDormant( int enable ) {
	fl.neverDormant	= ( enable != 0 );
	dormantStart = 0;
}

#ifdef MOD_WATERPHYSICS

/*

================

idEntity::Event_GetMass		MOD_WATERPHYSICS

================

*/

void idEntity::Event_GetMass( int id ) {

	idThread::ReturnFloat(physics->GetMass(id));

}



/*

================

idEntity::Event_IsInLiquid	MOD_WATERPHYSICS

================

*/

void idEntity::Event_IsInLiquid( void ) {

	idThread::ReturnInt(physics->GetWater() != NULL);

}

#endif		// MOD_WATERPHYSICS


/*

================

idEntity::Event_CopyBind

================

*/

void idEntity::Event_CopyBind( idEntity *other ) {

	idEntity *master = other->GetBindMaster();

	jointHandle_t joint = other->GetBindJoint();

	int body = other->GetBindBody();



	if( joint != INVALID_JOINT ) {

		// joint is specified so bind to that joint

		this->BindToJoint( master, joint, true );

	}

	else if( body >= 0 ) { 

		// body is specified so bind to it

		this->BindToBody( master, body, true );

	}

	else {

		// no joint and no body specified to bind to master

		this->Bind( master, true );

	}

}


/***********************************************************************

   Network
	
***********************************************************************/

/*
================
idEntity::ClientPredictionThink
================
*/
void idEntity::ClientPredictionThink( void ) {
	RunPhysics();
	Present();
}

/*
================
idEntity::WriteBindToSnapshot
================
*/
void idEntity::WriteBindToSnapshot( idBitMsgDelta &msg ) const {
	int bindInfo;

	if ( bindMaster ) {
		bindInfo = bindMaster->entityNumber;
		bindInfo |= ( fl.bindOrientated & 1 ) << GENTITYNUM_BITS;
		if ( bindJoint != INVALID_JOINT ) {
			bindInfo |= 1 << ( GENTITYNUM_BITS + 1 );
			bindInfo |= bindJoint << ( 3 + GENTITYNUM_BITS );
		} else if ( bindBody != -1 ) {
			bindInfo |= 2 << ( GENTITYNUM_BITS + 1 );
			bindInfo |= bindBody << ( 3 + GENTITYNUM_BITS );
		}
	} else {
		bindInfo = ENTITYNUM_NONE;
	}
	msg.WriteBits( bindInfo, GENTITYNUM_BITS + 3 + 9 );
}

/*
================
idEntity::ReadBindFromSnapshot
================
*/
void idEntity::ReadBindFromSnapshot( const idBitMsgDelta &msg ) {
	int bindInfo, bindEntityNum, bindPos;
	bool bindOrientated;
	idEntity *master;

	bindInfo = msg.ReadBits( GENTITYNUM_BITS + 3 + 9 );
	bindEntityNum = bindInfo & ( ( 1 << GENTITYNUM_BITS ) - 1 );

	if ( bindEntityNum != ENTITYNUM_NONE ) {
		master = gameLocal.entities[ bindEntityNum ];

		bindOrientated = ( bindInfo >> GENTITYNUM_BITS ) & 1;
		bindPos = ( bindInfo >> ( GENTITYNUM_BITS + 3 ) );
		switch( ( bindInfo >> ( GENTITYNUM_BITS + 1 ) ) & 3 ) {
			case 1: {
				BindToJoint( master, (jointHandle_t) bindPos, bindOrientated );
				break;
			}
			case 2: {
				BindToBody( master, bindPos, bindOrientated );
				break;
			}
			default: {
				Bind( master, bindOrientated );
				break;
			}
		}
	} else if ( bindMaster ) {
		Unbind();
	}
}

/*
================
idEntity::WriteColorToSnapshot
================
*/
void idEntity::WriteColorToSnapshot( idBitMsgDelta &msg ) const {
	idVec4 color;

	color[0] = renderEntity.shaderParms[ SHADERPARM_RED ];
	color[1] = renderEntity.shaderParms[ SHADERPARM_GREEN ];
	color[2] = renderEntity.shaderParms[ SHADERPARM_BLUE ];
	color[3] = renderEntity.shaderParms[ SHADERPARM_ALPHA ];
	msg.WriteLong( PackColor( color ) );
}

/*
================
idEntity::ReadColorFromSnapshot
================
*/
void idEntity::ReadColorFromSnapshot( const idBitMsgDelta &msg ) {
	idVec4 color;

	UnpackColor( msg.ReadLong(), color );
	renderEntity.shaderParms[ SHADERPARM_RED ] = color[0];
	renderEntity.shaderParms[ SHADERPARM_GREEN ] = color[1];
	renderEntity.shaderParms[ SHADERPARM_BLUE ] = color[2];
	renderEntity.shaderParms[ SHADERPARM_ALPHA ] = color[3];
}

/*
================
idEntity::WriteGUIToSnapshot
================
*/
void idEntity::WriteGUIToSnapshot( idBitMsgDelta &msg ) const {
	// no need to loop over MAX_RENDERENTITY_GUI at this time
	if ( renderEntity.gui[ 0 ] ) {
		msg.WriteByte( renderEntity.gui[ 0 ]->State().GetInt( "networkState" ) );
	} else {
		msg.WriteByte( 0 );
	}
}

/*
================
idEntity::ReadGUIFromSnapshot
================
*/
void idEntity::ReadGUIFromSnapshot( const idBitMsgDelta &msg ) {
	int state;
	idUserInterface *gui;
	state = msg.ReadByte( );
	gui = renderEntity.gui[ 0 ];
	if ( gui && state != mpGUIState ) {
		mpGUIState = state;
		gui->SetStateInt( "networkState", state );
		gui->HandleNamedEvent( "networkState" );
	}
}

/*
================
idEntity::WriteToSnapshot
================
*/
void idEntity::WriteToSnapshot( idBitMsgDelta &msg ) const {
}

/*
================
idEntity::ReadFromSnapshot
================
*/
void idEntity::ReadFromSnapshot( const idBitMsgDelta &msg ) {
}

/*
================
idEntity::ServerSendEvent

   Saved events are also sent to any client that connects late so all clients
   always receive the events nomatter what time they join the game.
================
*/
void idEntity::ServerSendEvent( int eventId, const idBitMsg *msg, bool saveEvent, int excludeClient ) const {
	idBitMsg	outMsg;
	byte		msgBuf[MAX_GAME_MESSAGE_SIZE];

	if ( !gameLocal.isServer ) {
		return;
	}

	// prevent dupe events caused by frame re-runs
	if ( !gameLocal.isNewFrame ) {
		return;
	}

	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.BeginWriting();
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_EVENT );	
	outMsg.WriteBits( gameLocal.GetSpawnId( this ), 32 );
	outMsg.WriteByte( eventId );
	outMsg.WriteLong( gameLocal.time );
	if ( msg ) {
		outMsg.WriteBits( msg->GetSize(), idMath::BitsForInteger( MAX_EVENT_PARAM_SIZE ) );
		outMsg.WriteData( msg->GetData(), msg->GetSize() );
	} else {
		outMsg.WriteBits( 0, idMath::BitsForInteger( MAX_EVENT_PARAM_SIZE ) );
	}

		if ( excludeClient != -1 ) {
			networkSystem->ServerSendReliableMessageExcluding( excludeClient, outMsg );
		} else {
			networkSystem->ServerSendReliableMessage( -1, outMsg );
		}

		if ( saveEvent ) {
			gameLocal.SaveEntityNetworkEvent( this, eventId, msg );
		}
	}

/*
================
idEntity::ClientSendEvent
================
*/
void idEntity::ClientSendEvent( int eventId, const idBitMsg *msg ) const {
	idBitMsg	outMsg;
	byte		msgBuf[MAX_GAME_MESSAGE_SIZE];

	if ( !gameLocal.isClient ) {
		return;
	}

	// prevent dupe events caused by frame re-runs
	if ( !gameLocal.isNewFrame ) {
		return;
	}

	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.BeginWriting();
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_EVENT );
	outMsg.WriteBits( gameLocal.GetSpawnId( this ), 32 );
	outMsg.WriteByte( eventId );
	outMsg.WriteLong( gameLocal.time );
	if ( msg ) {
		outMsg.WriteBits( msg->GetSize(), idMath::BitsForInteger( MAX_EVENT_PARAM_SIZE ) );
		outMsg.WriteData( msg->GetData(), msg->GetSize() );
	} else {
		outMsg.WriteBits( 0, idMath::BitsForInteger( MAX_EVENT_PARAM_SIZE ) );
	}

	networkSystem->ClientSendReliableMessage( outMsg );
}

/*
================
idEntity::ServerReceiveEvent
================
*/
bool idEntity::ServerReceiveEvent( int event, int time, const idBitMsg &msg ) {
	switch( event ) {
		case 0: {
		}
		default: {
			return false;
		}
	}
}

/*
================
idEntity::ClientReceiveEvent
================
*/
bool idEntity::ClientReceiveEvent( int event, int time, const idBitMsg &msg ) {
	int					index;
	const idSoundShader	*shader;
	s_channelType		channel;

	switch( event ) {
		case EVENT_STARTSOUNDSHADER: {
			// the sound stuff would early out
			assert( gameLocal.isNewFrame );
			if ( time < gameLocal.realClientTime - 1000 ) {
				// too old, skip it ( reliable messages don't need to be parsed in full )
				common->DPrintf( "ent 0x%x: start sound shader too old (%d ms)\n", entityNumber, gameLocal.realClientTime - time );
				return true;
			}
			index = gameLocal.ClientRemapDecl( DECL_SOUND, msg.ReadLong() );
			if ( index >= 0 && index < declManager->GetNumDecls( DECL_SOUND ) ) {
				shader = declManager->SoundByIndex( index, false );
				channel = (s_channelType)msg.ReadByte();
				StartSoundShader( shader, channel, 0, false, NULL );
			}
			return true;
		}
		case EVENT_STOPSOUNDSHADER: {
			// the sound stuff would early out
			assert( gameLocal.isNewFrame );
			channel = (s_channelType)msg.ReadByte();
			StopSound( channel, false );
			return true;
		}
		default: {
			return false;
		}
	}
//	return false;
}

/*
===============================================================================

  idAnimatedEntity
	
===============================================================================
*/

const idEventDef EV_GetJointHandle( "getJointHandle", "s", 'd' );
const idEventDef EV_ClearAllJoints( "clearAllJoints" );
const idEventDef EV_ClearJoint( "clearJoint", "d" );
const idEventDef EV_SetJointPos( "setJointPos", "ddv" );
const idEventDef EV_SetJointAngle( "setJointAngle", "ddv" );
const idEventDef EV_GetJointPos( "getJointPos", "d", 'v' );
const idEventDef EV_GetJointAngle( "getJointAngle", "d", 'v' );

CLASS_DECLARATION( idEntity, idAnimatedEntity )
	EVENT( EV_GetJointHandle,		idAnimatedEntity::Event_GetJointHandle )
	EVENT( EV_ClearAllJoints,		idAnimatedEntity::Event_ClearAllJoints )
	EVENT( EV_ClearJoint,			idAnimatedEntity::Event_ClearJoint )
	EVENT( EV_SetJointPos,			idAnimatedEntity::Event_SetJointPos )
	EVENT( EV_SetJointAngle,		idAnimatedEntity::Event_SetJointAngle )
	EVENT( EV_GetJointPos,			idAnimatedEntity::Event_GetJointPos )
	EVENT( EV_GetJointAngle,		idAnimatedEntity::Event_GetJointAngle )
END_CLASS

/*
================
idAnimatedEntity::idAnimatedEntity
================
*/
idAnimatedEntity::idAnimatedEntity() {
	animator.SetEntity( this );
	damageEffects = NULL;
}

/*
================
idAnimatedEntity::~idAnimatedEntity
================
*/
idAnimatedEntity::~idAnimatedEntity() {
	damageEffect_t	*de;

	for ( de = damageEffects; de; de = damageEffects ) {
		damageEffects = de->next;
		delete de;
	}
}

/*
================
idAnimatedEntity::Save

archives object for save game file
================
*/
void idAnimatedEntity::Save( idSaveGame *savefile ) const {
	animator.Save( savefile );

	// Wounds are very temporary, ignored at this time
	//damageEffect_t			*damageEffects;
}

/*
================
idAnimatedEntity::Restore

unarchives object from save game file
================
*/
void idAnimatedEntity::Restore( idRestoreGame *savefile ) {
	animator.Restore( savefile );

	// check if the entity has an MD5 model
	if ( animator.ModelHandle() )
	{
		// set the callback to update the joints
		renderEntity.callback = idEntity::ModelCallback;

		animator.GetJoints( &renderEntity.numJoints, &renderEntity.joints );
		animator.GetBounds( gameLocal.time, renderEntity.bounds );
		if ( modelDefHandle != -1 ) {
			gameRenderWorld->UpdateEntityDef( modelDefHandle, &renderEntity );
		}
	}
}

/*
================
idAnimatedEntity::ClientPredictionThink
================
*/
void idAnimatedEntity::ClientPredictionThink( void ) {
	RunPhysics();
	UpdateAnimation();
	Present();
}

/*
================
idAnimatedEntity::Think
================
*/
void idAnimatedEntity::Think( void ) {
	RunPhysics();
	UpdateAnimation();
	Present();
	UpdateDamageEffects();
}

/*
================
idAnimatedEntity::UpdateAnimation
================
*/
void idAnimatedEntity::UpdateAnimation( void ) {
	// don't do animations if they're not enabled
	if ( !( thinkFlags & TH_ANIMATE ) ) {
		return;
	}

	// is the model an MD5?
	if ( !animator.ModelHandle() ) {
		// no, so nothing to do
		return;
	}

	// call any frame commands that have happened in the past frame
	if ( !fl.hidden ) {
		animator.ServiceAnims( gameLocal.previousTime, gameLocal.time );
	}

	// if the model is animating then we have to update it
	if ( !animator.FrameHasChanged( gameLocal.time ) ) {
		// still fine the way it was
		return;
	}

	// get the latest frame bounds
	animator.GetBounds( gameLocal.time, renderEntity.bounds );
	if ( renderEntity.bounds.IsCleared() && !fl.hidden ) {
		gameLocal.DPrintf( "%d: inside out bounds\n", gameLocal.time );
	}

	// update the renderEntity
	UpdateVisuals();

	// the animation is updated
	animator.ClearForceUpdate();
}

/*
================
idAnimatedEntity::GetAnimator
================
*/
idAnimator *idAnimatedEntity::GetAnimator( void ) {
	return &animator;
}

/*
================
idAnimatedEntity::SetModel
================
*/
void idAnimatedEntity::SetModel( const char *modelname ) {
	FreeModelDef();

	renderEntity.hModel = animator.SetModel( modelname );
	if ( !renderEntity.hModel ) {
		idEntity::SetModel( modelname );
		return;
	}

	if ( !renderEntity.customSkin ) {
		renderEntity.customSkin = animator.ModelDef()->GetDefaultSkin();
	}

	// set the callback to update the joints
	renderEntity.callback = idEntity::ModelCallback;

	animator.GetJoints( &renderEntity.numJoints, &renderEntity.joints );
	animator.GetBounds( gameLocal.time, renderEntity.bounds );

	UpdateVisuals();
}

/*
=====================
idAnimatedEntity::GetJointWorldTransform
=====================
*/
bool idAnimatedEntity::GetJointWorldTransform( jointHandle_t jointHandle, int currentTime, idVec3 &offset, idMat3 &axis ) {
	if ( !animator.GetJointTransform( jointHandle, currentTime, offset, axis ) ) {
		return false;
	}

	ConvertLocalToWorldTransform( offset, axis );
	return true;
}

/*
==============
idAnimatedEntity::GetJointTransformForAnim
==============
*/
bool idAnimatedEntity::GetJointTransformForAnim( jointHandle_t jointHandle, int animNum, int frameTime, idVec3 &offset, idMat3 &axis ) const {
	const idAnim	*anim;
	int				numJoints;
	idJointMat		*frame;

	anim = animator.GetAnim( animNum );
	if ( !anim ) {
		assert( 0 );
		return false;
	}

	numJoints = animator.NumJoints();
	if ( ( jointHandle < 0 ) || ( jointHandle >= numJoints ) ) {
		assert( 0 );
		return false;
	}

	frame = ( idJointMat * )_alloca16( numJoints * sizeof( idJointMat ) );
	gameEdit->ANIM_CreateAnimFrame( animator.ModelHandle(), anim->MD5Anim( 0 ), renderEntity.numJoints, frame, frameTime, animator.ModelDef()->GetVisualOffset(), animator.RemoveOrigin() );

	offset = frame[ jointHandle ].ToVec3();
	axis = frame[ jointHandle ].ToMat3();
	
	return true;
}

/*
==============
idAnimatedEntity::AddDamageEffect

  Dammage effects track the animating impact position, spitting out particles.
==============
*/
void idAnimatedEntity::AddDamageEffect( const trace_t &collision, const idVec3 &velocity, const char *damageDefName ) {
	jointHandle_t jointNum;
	idVec3 origin, dir, localDir, localOrigin, localNormal;
	idMat3 axis;

	if ( !g_bloodEffects.GetBool() || renderEntity.joints == NULL ) {
		return;
	}

	const idDeclEntityDef *def = gameLocal.FindEntityDef( damageDefName, false );
	if ( def == NULL ) {
		return;
	}

	jointNum = CLIPMODEL_ID_TO_JOINT_HANDLE( collision.c.id );
	if ( jointNum == INVALID_JOINT ) {
		return;
	}

	dir = velocity;
	dir.Normalize();

	axis = renderEntity.joints[jointNum].ToMat3() * renderEntity.axis;
	origin = renderEntity.origin + renderEntity.joints[jointNum].ToVec3() * renderEntity.axis;

	localOrigin = ( collision.c.point - origin ) * axis.Transpose();
	localNormal = collision.c.normal * axis.Transpose();
	localDir = dir * axis.Transpose();

	AddLocalDamageEffect( jointNum, localOrigin, localNormal, localDir, def, collision.c.material );

	if ( gameLocal.isServer ) {
		idBitMsg	msg;
		byte		msgBuf[MAX_EVENT_PARAM_SIZE];

		msg.Init( msgBuf, sizeof( msgBuf ) );
		msg.BeginWriting();
		msg.WriteShort( (int)jointNum );
		msg.WriteFloat( localOrigin[0] );
		msg.WriteFloat( localOrigin[1] );
		msg.WriteFloat( localOrigin[2] );
		msg.WriteDir( localNormal, 24 );
		msg.WriteDir( localDir, 24 );
		msg.WriteLong( gameLocal.ServerRemapDecl( -1, DECL_ENTITYDEF, def->Index() ) );
		msg.WriteLong( gameLocal.ServerRemapDecl( -1, DECL_MATERIAL, collision.c.material->Index() ) );
		ServerSendEvent( EVENT_ADD_DAMAGE_EFFECT, &msg, false, -1 );
	}
}

/*
==============
idAnimatedEntity::GetDefaultSurfaceType
==============
*/
int	idAnimatedEntity::GetDefaultSurfaceType( void ) const {
	return SURFTYPE_METAL;
}

/*
==============
idAnimatedEntity::AddLocalDamageEffect
==============
*/
void idAnimatedEntity::AddLocalDamageEffect( jointHandle_t jointNum, const idVec3 &localOrigin, const idVec3 &localNormal, const idVec3 &localDir, const idDeclEntityDef *def, const idMaterial *collisionMaterial ) {
	const char *sound, *splat, *decal, *bleed, *key;
	damageEffect_t	*de;
	idVec3 origin, dir;
	idMat3 axis;
	idStr surfName;

	axis = renderEntity.joints[jointNum].ToMat3() * renderEntity.axis;
	origin = renderEntity.origin + renderEntity.joints[jointNum].ToVec3() * renderEntity.axis;

	origin = origin + localOrigin * axis;
	dir = localDir * axis;

	int type = collisionMaterial->GetSurfaceType();

	if ( type == SURFTYPE_NONE ) {
		surfName = gameLocal.sufaceTypeNames[ GetDefaultSurfaceType() ];
	}
	else
	{
		g_Global.GetSurfName( collisionMaterial, surfName );
	}

	// start impact sound based on material type
	key = va( "snd_%s", surfName.c_str() );
	sound = spawnArgs.GetString( key );
	if ( *sound == '\0' ) {
		sound = def->dict.GetString( key );
	}
	if ( *sound != '\0' ) {
		StartSoundShader( declManager->FindSound( sound ), SND_CHANNEL_BODY, 0, false, NULL );
	}

	// blood splats are thrown onto nearby surfaces
	key = va( "mtr_splat_%s", surfName.c_str() );
	splat = spawnArgs.RandomPrefix( key, gameLocal.random );
	if ( *splat == '\0' ) {
		splat = def->dict.RandomPrefix( key, gameLocal.random );
	}
	if ( *splat != '\0' ) {
		gameLocal.BloodSplat( origin, dir, 64.0f, splat );
	}

	// can't see wounds on the player model in single player mode
	if ( !( IsType( idPlayer::Type ) && !gameLocal.isMultiplayer ) ) {
		// place a wound overlay on the model
		key = va( "mtr_wound_%s", surfName.c_str() );
		decal = spawnArgs.RandomPrefix( key, gameLocal.random );
		if ( *decal == '\0' ) {
			decal = def->dict.RandomPrefix( key, gameLocal.random );
		}
		if ( *decal != '\0' ) {
			ProjectOverlay( origin, dir, 20.0f, decal );
		}
	}

	// a blood spurting wound is added
	key = va( "smoke_wound_%s", surfName.c_str() );
	bleed = spawnArgs.GetString( key );
	if ( *bleed == '\0' ) {
		bleed = def->dict.GetString( key );
	}
	if ( *bleed != '\0' ) {
		de = new damageEffect_t;
		de->next = this->damageEffects;
		this->damageEffects = de;

		de->jointNum = jointNum;
		de->localOrigin = localOrigin;
		de->localNormal = localNormal;
		de->type = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, bleed ) );
		de->time = gameLocal.time;
	}
}

/*
==============
idAnimatedEntity::UpdateDamageEffects
==============
*/
void idAnimatedEntity::UpdateDamageEffects( void ) {
	damageEffect_t	*de, **prev;

	// free any that have timed out
	prev = &this->damageEffects;
	while ( *prev ) {
		de = *prev;
		if ( de->time == 0 ) {	// FIXME:SMOKE
			*prev = de->next;
			delete de;
		} else {
			prev = &de->next;
		}
	}

	if ( !g_bloodEffects.GetBool() ) {
		return;
	}

	// emit a particle for each bleeding wound
	for ( de = this->damageEffects; de; de = de->next ) {
		idVec3 origin, start;
		idMat3 axis;

		animator.GetJointTransform( de->jointNum, gameLocal.time, origin, axis );
		axis *= renderEntity.axis;
		origin = renderEntity.origin + origin * renderEntity.axis;
		start = origin + de->localOrigin * axis;
		if ( !gameLocal.smokeParticles->EmitSmoke( de->type, de->time, gameLocal.random.CRandomFloat(), start, axis ) ) {
			de->time = 0;
		}
	}
}

/*
================
idAnimatedEntity::ClientReceiveEvent
================
*/
bool idAnimatedEntity::ClientReceiveEvent( int event, int time, const idBitMsg &msg ) {
	int damageDefIndex;
	int materialIndex;
	jointHandle_t jointNum;
	idVec3 localOrigin, localNormal, localDir;

	switch( event ) {
		case EVENT_ADD_DAMAGE_EFFECT: {
			jointNum = (jointHandle_t) msg.ReadShort();
			localOrigin[0] = msg.ReadFloat();
			localOrigin[1] = msg.ReadFloat();
			localOrigin[2] = msg.ReadFloat();
			localNormal = msg.ReadDir( 24 );
			localDir = msg.ReadDir( 24 );
			damageDefIndex = gameLocal.ClientRemapDecl( DECL_ENTITYDEF, msg.ReadLong() );
			materialIndex = gameLocal.ClientRemapDecl( DECL_MATERIAL, msg.ReadLong() );
			const idDeclEntityDef *damageDef = static_cast<const idDeclEntityDef *>( declManager->DeclByIndex( DECL_ENTITYDEF, damageDefIndex ) );
			const idMaterial *collisionMaterial = static_cast<const idMaterial *>( declManager->DeclByIndex( DECL_MATERIAL, materialIndex ) );
			AddLocalDamageEffect( jointNum, localOrigin, localNormal, localDir, damageDef, collisionMaterial );
			return true;
		}
		default: {
			return idEntity::ClientReceiveEvent( event, time, msg );
		}
	}
//	return false;
}

/*
================
idAnimatedEntity::Attach( idEntity *ent )
================
*/
void idAnimatedEntity::Attach( idEntity *ent ) 
{
	idVec3			origin;
	idMat3			axis;
	jointHandle_t	joint;
	idStr			jointName;
	idAngles		angleOffset;
	idVec3			originOffset;

	jointName = ent->spawnArgs.GetString( "joint" );
	joint = animator.GetJointHandle( jointName );
	if ( joint == INVALID_JOINT ) 
	{
		gameLocal.Error( "Joint '%s' not found for attaching '%s' on '%s'", jointName.c_str(), ent->GetClassname(), name.c_str() );
	}

	angleOffset = ent->spawnArgs.GetAngles( "angles" );
	originOffset = ent->spawnArgs.GetVector( "origin" );

	GetJointWorldTransform( joint, gameLocal.time, origin, axis );

	ent->SetOrigin( origin + originOffset * renderEntity.axis );
	idMat3 rotate = angleOffset.ToMat3();
	idMat3 newAxis = rotate * axis;
	ent->SetAxis( newAxis );
	ent->BindToJoint( this, joint, true );
	ent->cinematic = cinematic;
}

/*
================
idAnimatedEntity::Event_GetJointHandle

looks up the number of the specified joint.  returns INVALID_JOINT if the joint is not found.
================
*/
void idAnimatedEntity::Event_GetJointHandle( const char *jointname ) {
	jointHandle_t joint;

	joint = animator.GetJointHandle( jointname );
	idThread::ReturnInt( joint );
}

/*
================
idAnimatedEntity::Event_ClearAllJoints

removes any custom transforms on all joints
================
*/
void idAnimatedEntity::Event_ClearAllJoints( void ) {
	animator.ClearAllJoints();
}

/*
================
idAnimatedEntity::Event_ClearJoint

removes any custom transforms on the specified joint
================
*/
void idAnimatedEntity::Event_ClearJoint( jointHandle_t jointnum ) {
	animator.ClearJoint( jointnum );
}

/*
================
idAnimatedEntity::Event_SetJointPos

modifies the position of the joint based on the transform type
================
*/
void idAnimatedEntity::Event_SetJointPos( jointHandle_t jointnum, jointModTransform_t transform_type, const idVec3 &pos ) {
	animator.SetJointPos( jointnum, transform_type, pos );
}

/*
================
idAnimatedEntity::Event_SetJointAngle

modifies the orientation of the joint based on the transform type
================
*/
void idAnimatedEntity::Event_SetJointAngle( jointHandle_t jointnum, jointModTransform_t transform_type, const idAngles &angles ) {
	idMat3 mat;

	mat = angles.ToMat3();
	animator.SetJointAxis( jointnum, transform_type, mat );
}

/*
================
idAnimatedEntity::Event_GetJointPos

returns the position of the joint in worldspace
================
*/
void idAnimatedEntity::Event_GetJointPos( jointHandle_t jointnum ) {
	idVec3 offset;
	idMat3 axis;

	if ( !GetJointWorldTransform( jointnum, gameLocal.time, offset, axis ) ) {
		gameLocal.Warning( "Joint # %d out of range on entity '%s'",  jointnum, name.c_str() );
	}

	idThread::ReturnVector( offset );
}

/*
================
idAnimatedEntity::Event_GetJointAngle

returns the orientation of the joint in worldspace
================
*/
void idAnimatedEntity::Event_GetJointAngle( jointHandle_t jointnum ) {
	idVec3 offset;
	idMat3 axis;

	if ( !GetJointWorldTransform( jointnum, gameLocal.time, offset, axis ) ) {
		gameLocal.Warning( "Joint # %d out of range on entity '%s'",  jointnum, name.c_str() );
	}

	idAngles ang = axis.ToAngles();
	idVec3 vec( ang[ 0 ], ang[ 1 ], ang[ 2 ] );
	idThread::ReturnVector( vec );
}

bool idEntity::AddToMasterList(idList<idStr> &MasterList, idStr &str)
{
	bool bRc = false;
	idEntity *ent;
	int i, n;
	bool bFound;

	// The master may not be the same as this entity.
	if(str == name)
		goto Quit;

	DM_LOG(LC_FROBBING, LT_INFO)LOGSTRING("[%s] Master set to [%s]\r", name.c_str(), str.c_str());
	if((ent = gameLocal.FindEntity(str.c_str())) != NULL)
	{
		DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Master entity %08lX [%s] is updated.\r", ent, ent->name.c_str());
		i = 0;
		n = MasterList.Num();
		bFound = false;
		for(i = 0; i < n; i++)
		{
			if(MasterList[i] == name)
			{
				bFound = true;
				break;
			}
		}
		if(bFound == false)
			MasterList.Append(name);
	}
	else
	{
		DM_LOG(LC_FROBBING, LT_ERROR)LOGSTRING("Master entity [%s] not found.\r", str.c_str());
		goto Quit;
	}

	bRc = true;

Quit:
	return bRc;
}

void idEntity::LoadTDMSettings(void)
{
	idStr str;

	// If an item has the frobable flag set to true we will use the 
	// the default value. If the frobdistance is set in the item
	// it will override the defaultsetting. If none of that is set
	// the frobdistance will be set to 0 meaning no frobbing on that item.
	// If the frobsetting is alread initialized we can skip this.
	if(m_FrobDistance == 0)
	{
		spawnArgs.GetBool("frobable", "0", m_bFrobable);
		spawnArgs.GetInt("frob_distance", "0", m_FrobDistance);
		spawnArgs.GetFloat("frob_bias", "1.0", m_FrobBias);

		if( m_FrobDistance <= 0  )
			m_FrobDistance = g_Global.m_DefaultFrobDistance;
	}

	// update the max frobdistance if necessary
	if( m_FrobDistance > g_Global.m_MaxFrobDistance )
		g_Global.m_MaxFrobDistance = m_FrobDistance;

	// Override the frob action script to apply custom events to 
	// specific entities.
	if(spawnArgs.GetString("frob_action_script", "", str))
		m_FrobActionScript = str;

	// Check if this entity is associated to a master frob entity.
	if(spawnArgs.GetString("frob_master", "", str))
	{
		if(AddToMasterList(m_FrobList, str) == true)
			m_MasterFrob = str;
	}

	const idKeyValue *kv = spawnArgs.MatchPrefix( "frob_peer", NULL );
	// Fill the list of frob peers
	while( kv )
	{
		idStr temp = kv->GetValue();
		if( !temp.IsEmpty() )
			m_FrobPeers.Append( temp );
		kv = spawnArgs.MatchPrefix( "frob_peer", kv );
	}

	// Check if this entity can be used by others.
	if(spawnArgs.GetString("used_by", "", str))
		ParseUsedByList(m_UsedBy, str);

	m_bIsObjective = spawnArgs.GetBool( "objective_ent", "0" );

	m_bIsClimbableRope = spawnArgs.GetBool( "is_climbable_rope", "0" );

	m_bIsMantleable = spawnArgs.GetBool( "is_mantleable", "1" );

	DM_LOG(LC_FROBBING, LT_INFO)LOGSTRING("[%s] this: %08lX FrobDistance: %u\r", name.c_str(), this, m_FrobDistance);
}

void idEntity::UpdateFrob(void)
{
	idPlayer *player;
	CDarkModPlayer *pDM;

	player = gameLocal.GetLocalPlayer();
	pDM = g_Global.m_DarkModPlayer;

	// If we have no player there is no point in doing this. :)
	if(player == NULL || pDM == NULL || IsHidden() )
		goto Quit;

	if( !m_bFrobbed )	
	{
		// Check if we just stopped being highlighted
		// Some tricks here due to the fact that we don't know which will
		// update first: The entity that used to be frobbed and now is not, or 
		// a new entity that is now frobbed.

		// If this is true, this entity is updating before the newly frobbed entity
		if(pDM->m_FrobEntity == this)
		{
			pDM->m_FrobEntity = NULL;
			pDM->m_FrobEntityPrevious = NULL;

			// stop highlight, tell peers
			FrobHighlight( false );
		}
		// Otherwise, we are updating AFTER the newly frobbed entity
		// and should not set m_FrobEntity to NULL.
		else if( pDM->m_FrobEntityPrevious == this )
		{
			// if this one updates last, set the previous frob entity to 
			// the newly frobbed ent for the next frame.
			pDM->m_FrobEntityPrevious = pDM->m_FrobEntity;

			// stop highlight, tell peers
			FrobHighlight(false );
		}

		goto Quit;
	}

	// We are frobbed this frame
	// set m_bFrobbed back to false for next frame
	m_bFrobbed = false;

	// Check if we are newly frobbed this frame
	if( pDM->m_FrobEntity != this )
	{
		pDM->m_FrobEntity = this;
		FrobHighlight( true );

		// again there's a trick here for syncronicity
		// we don't want to overwrite it if the old frob entity has not updated yet,
		// so if it's NULL, we know that old frob ent already updated.
		if( pDM->m_FrobEntityPrevious == NULL )
			pDM->m_FrobEntityPrevious = this;
	}

Quit:
	return;
}

void idEntity::FrobHighlight( bool bVal )
{
	idEntity *ent = NULL;

	// Stop flooding the frob peers if we've been updated this frame
	// NOTE: Also, a bVal of true overrides a bVal of false in the case where
	// focus shifts to one frob peer to another (one will flood a false and one will flood a true)
	if(m_FrobPeerFloodFrame == gameLocal.framenum || bVal == m_bFrobHighlightState)
		goto Quit;

	m_bFrobHighlightState = bVal;

	// update our timestamp
	m_FrobChangeTime = gameLocal.time;
	m_FrobPeerFloodFrame = gameLocal.framenum;

	// resolve the peer names into entities
	// TODO: Bad for performance?  Only happens on the frame it's hilighted/unhilighted though
	for( int i=0; i < m_FrobPeers.Num(); i++ )
	{
		if( m_FrobPeers[i].IsEmpty() )
			continue;

		ent = gameLocal.FindEntity( m_FrobPeers[i].c_str() );
		// don't call it on self, would get stuck in a loop
		if(ent != NULL && ent != this)
			ent->FrobHighlight( bVal );
	}

	DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Entity [%s] is highlighted\r", name.c_str());

Quit:
	return;
}

void idEntity::UpdateFrobDisplay( void )
{
	float param = renderEntity.shaderParms[ 11 ];
	int TimePassed = 0;
	
	if( (!param && !m_bFrobHighlightState) || ((param == 1.0) && m_bFrobHighlightState)  )
		goto Quit;

	TimePassed = ( gameLocal.time - m_FrobChangeTime );
	
	if( m_bFrobHighlightState )
		param += (float) TimePassed / (float) cv_frob_fadetime.GetInteger();
	else
		param -= (float) TimePassed / (float) cv_frob_fadetime.GetInteger();

	m_FrobChangeTime = gameLocal.time;

	// clamp between 0 and 1
	param = idMath::ClampFloat(0.0, 1.0, param);
	DM_LOG(LC_FROBBING,LT_DEBUG)LOGSTRING("Frob fader: Entity %s, param = %f\r", name.c_str(), param );
	SetShaderParm(11, param);

Quit:
	return;
}

// Default for entities that are frobable and movable is to pick it up and
// carry it around (probably throwing or dropping it).
void idEntity::FrobAction(bool bMaster)
{
	idEntity *ent;

	if( IsHidden() )
		goto Quit;

	if(m_FrobActionScript.Length() == 0)
	{
		DM_LOG(LC_FROBBING, LT_ERROR)LOGSTRING("(%08lX->[%s]) FrobAction has been triggered with empty FrobActionScript!\r", this, name.c_str());

		// Default: grab it if it is one of the grabbable classes
		if( IsType( idMoveable::Type ) || IsType( idAFEntity_Base::Type )
			|| IsType( idMoveableItem::Type ) )
		{
			// Fix : Do not pick up live, conscious AI
			if( IsType( idAI::Type ) )
			{
				idAI *AISelf = static_cast<idAI *>(this);
				if( AISelf->health > 0 && !AISelf->IsKnockedOut() )
					goto Quit;
			}

			g_Global.m_DarkModPlayer->grabber->Update( gameLocal.GetLocalPlayer() );
		}

		goto Quit;
	}

	DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("This: [%s]   Master: %08lX (%u)\r", name.c_str(), m_MasterFrob.c_str(), bMaster);

	// The player can frob any entity in a chain so we must make sure that all 
	// others, in the chain, are also called. The master has no special meaning
	// except that it has to be the first in a chain. The master also doesn't have
	// to be the same entity type.
	if(bMaster == true && m_MasterFrob.Length() != 0)
	{
		DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Master entity [%s] is called.\r", m_MasterFrob.c_str());
		if((ent = gameLocal.FindEntity(m_MasterFrob.c_str())) != NULL)
			ent->FrobAction(false);
		else
			DM_LOG(LC_FROBBING, LT_ERROR)LOGSTRING("Master entity [%s] not found.\r", m_MasterFrob.c_str());
	}
	else
	{
		int i, n;

		n = m_FrobList.Num();
		for(i = 0; i < n; i++)
		{
			DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Trying linked entity [%s]\r", m_FrobList[i].c_str());
			if((ent = gameLocal.FindEntity(m_FrobList[i].c_str())) != NULL)
			{
				DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Calling linked entity [%s]\r", m_FrobList[i].c_str());
				ent->FrobAction(false);
			}
			else
				DM_LOG(LC_FROBBING, LT_ERROR)LOGSTRING("Linked entity [%s] not found\r", m_FrobList[i].c_str());
		}

		if(m_FrobActionScript.Length() > 0)
            CallScriptFunctionArgs(m_FrobActionScript.c_str(), true, 0, "e", this);

		StartSound( "snd_acquire", SND_CHANNEL_ANY, 0, false, NULL );
	}

Quit:
	return;
}

void idEntity::ParseUsedByList(idList<idStr> &list, idStr &s)
{
	idStr str;
	int i, n;
	int x, y;
	bool bFound;

	DM_LOG(LC_MISC, LT_INFO)LOGSTRING("Parsing [%s]\r", s.c_str());
	i = 0;
	while(1)
	{
//		DM_LOG(LC_MISC, LT_DEBUG)LOGSTRING("Offset - 1: %u:%u [%s]\r", i, n, &s[i]);
		// If no seperator is found, the remainder of the string
		// is copied.
		if((n = s.Find(';', i)) == -1)
			n = s.Length();

//		DM_LOG(LC_MISC, LT_DEBUG)LOGSTRING("Offset - 2: %u:%u [%s]\r", i, n, &s[i]);
		n -= i;
		s.Mid(i, n, str);
		i += n;
		if(s[i] == ';')
			i++;
//		DM_LOG(LC_MISC, LT_DEBUG)LOGSTRING("Offset - 3: %u:%u [%s]\r", i, n, &s[i]);
		if(str.Length() == 0)
			break;

		y = list.Num();
		bFound = false;
		for(x = 0; x < y; x++)
		{
			if(list[x] == str)
			{
				bFound = true;
				break;
			}
		}

		if(bFound == false)
		{
			DM_LOG(LC_MISC, LT_DEBUG)LOGSTRING("Append [%s] to uselist\r", str.c_str());
			list.Append(str);
		}
	}
}

bool idEntity::UsedBy(idEntity *ent)
{
	return false;
}

void idEntity::SetFrobbed( bool val )
{
	m_bFrobbed = val;
}

bool idEntity::IsFrobbed( void )
{
	return m_bFrobbed;
}

void idEntity::StimAdd(int Type, float Radius)
{
	AddStim(Type, Radius);
}

void idEntity::StimRemove(int Type)
{
	RemoveStim(Type);
}

void idEntity::StimEnable(int Type, int State)
{
	CStim *stim;

	if((stim = m_StimResponseColl->GetStim(Type)) != NULL)
		stim->EnableSR(State);
}

void idEntity::StimClearIgnoreList (int Type)
{
	CStim *stim = m_StimResponseColl->GetStim(Type);

	if(stim)
	{
		stim->m_ResponseIgnore.Clear();
	}

}

void idEntity::ResponseEnable(int Type, int State)
{
	CResponse *resp;

	if((resp = m_StimResponseColl->GetResponse(Type)) != NULL)
		resp->EnableSR(State);
}

void idEntity::ResponseAdd(int Type)
{
	AddResponse(Type);
}

void idEntity::ResponseRemove(int Type)
{
	RemoveResponse(Type);
}

void idEntity::RemoveStim(int Type)
{
	if(m_StimResponseColl->RemoveStim(Type) == 0)
		gameLocal.RemoveStim(this);
}

void idEntity::RemoveResponse(int Type)
{
	if(m_StimResponseColl->RemoveResponse(Type) == 0)
		gameLocal.RemoveResponse(this);
}


CStim *idEntity::AddStim(int Type, float Radius, bool Removable, bool Default)
{
	CStim *pStim;

	pStim = m_StimResponseColl->AddStim(this, Type, Radius, Removable, Default);
	gameLocal.AddStim(this);

	return pStim;

}

CResponse *idEntity::AddResponse(int Type, bool Removable, bool Default)
{
	CResponse *pResp;

	pResp = m_StimResponseColl->AddResponse(this, Type, Removable, Default);
	gameLocal.AddResponse(this);

	return pResp;

}

void idEntity::ResponseIgnore(int StimType, idEntity *e)
{
	CStim *stim = m_StimResponseColl->GetStim(StimType);

	if(stim)
		stim->AddResponseIgnore(e);
}

void idEntity::ResponseAllow(int StimType, idEntity *e)
{
	CStim *stim = m_StimResponseColl->GetStim(StimType);

	if(stim)
		stim->RemoveResponseIgnore(e);
}

void idEntity::ResponseSetAction(int StimType, const char *s)
{
	CResponse *resp = m_StimResponseColl->GetResponse(StimType);

	if(resp)
		resp->SetResponseAction(s);
}

/**	Called when m_renderTrigger is rendered.
 *	It will resume the m_renderWaitingThread.
 */
bool idEntity::WaitForRenderTriggered( renderEntity_s* renderEntity, const renderView_s* view )
{
	// NOTE: We must avoid changing the game state from within this function.
	// However, we'll add an event to resume the suspended thread.

	idEntity* ent = gameLocal.entities[ renderEntity->entityNum ];
	if ( !ent )
		gameLocal.Error( "idEntity::WaitForRenderTriggered: callback with NULL game entity" );

	renderEntity->callback = NULL;

	if ( ent->m_renderWaitingThread )
	{
		// Fortunately, this doesn't execute the script immediately, so
		// I think it's safe to call from here.
		idThread::ObjectMoveDone( ent->m_renderWaitingThread, ent );
		ent->m_renderWaitingThread = 0;
	}

	return false;
}

/**	Called to update m_renderTrigger after the render entity is modified.
 *	Only updates the render trigger if a thread is waiting for it.
 */
void idEntity::PresentRenderTrigger()
{
	if ( !m_renderWaitingThread )
	{
		goto Quit;
	}

	// Update the renderTrigger to match renderEntity's bounding box.
	// In order to ensure that other code can easily add to the bounding
	// box, we're going to pre-rotate the bounding box, and keep the axis
	// unrotated.
	m_renderTrigger.origin = renderEntity.origin;
	m_renderTrigger.bounds = renderEntity.bounds * renderEntity.axis;
	// I haven't yet figured out where renderEntity.entityNum is set...
	// but it looks like we have to manually set it for m_renderTrigger.
	m_renderTrigger.entityNum = entityNumber;

	// Update the renderTrigger in the render world.
	if ( m_renderTriggerHandle == -1 )
		m_renderTriggerHandle = gameRenderWorld->AddEntityDef( &m_renderTrigger );
	else
		gameRenderWorld->UpdateEntityDef( m_renderTriggerHandle, &m_renderTrigger );

	Quit:
	return;
}

/*
================
idEntity::Inventory

This returns the inventory object of this entity. If this entity doesn't
have one, it creates the inventory.
================
*/
CInventory* idEntity::Inventory()
{
	if(m_Inventory == NULL )
	{
		m_Inventory = new CInventory();
		m_Inventory->SetOwner(this);
	}

	return m_Inventory;
}

/*
================
idEntity::InventoryCursor

This returns the inventory cursor object of this entity. If this entity
doesn't have one, it creates the inventory cursor.

The cursor is intended for arbitrary use, and need not point to this
entity's inventory.
================
*/
CInventoryCursor* idEntity::InventoryCursor()
{
	if(m_InventoryCursor == NULL)
		m_InventoryCursor = Inventory()->CreateCursor();

	return m_InventoryCursor;
}

void idEntity::Event_PropSound( const char *sndName )
{
	PropSoundDirect( sndName, false, false, 0.0f );
}

void idEntity::Event_PropSoundMod( const char *sndName, float VolModIn )
{
	PropSoundDirect( sndName, false, false, VolModIn );
}

/*
================
idEntity::Event_InPVS

Returns non-zero if this entity is in the player's PVS, zero otherwise.
================
*/
void idEntity::Event_InPVS()
{
	idThread::ReturnFloat( gameLocal.InPlayerPVS( this ) );
}

/*
================
idEntity::Event_WaitForRender

Called by sys.waitForRender(ent) to find out if it can wait for this entity to render.
================
*/
void idEntity::Event_WaitForRender()
{
	if ( !m_renderWaitingThread )
	{
		// Give the renderTrigger an invisible model to prevent a black cube from showing up.
		if ( !m_renderTrigger.hModel )
			m_renderTrigger.hModel = renderModelManager->FindModel( EMPTY_MODEL );
		if ( !m_renderTrigger.hModel )
			gameLocal.Error( "Unable to load model: %s\n", EMPTY_MODEL );

		m_renderTrigger.callback = idEntity::WaitForRenderTriggered;
		m_renderWaitingThread = idThread::CurrentThreadNum();

		// Make sure Present() gets called.
		BecomeActive( TH_UPDATEVISUALS );

		idThread::ReturnInt( true );
	} else {
		idThread::ReturnInt( false );
	}
}

/*
================
idEntity::Event_SetGui

Changes the GUI file that's loaded by an overlay. Overlays 1, 2 and 3 are
reserved for the entity GUIs, and special code is implemented to change them.
A changed GUI does not see gui_parm spawn args. (for one thing, if there's a
script changing GUIs, then the script could easily send spawnargs to the GUI
if it wanted to.)
================
*/
void idEntity::Event_SetGui( int handle, const char *guiFile ) {
	if ( !uiManager->CheckGui(guiFile) ) {
		gameLocal.Warning( "Unable to load GUI file: %s\n", guiFile );
		goto Quit;
	}

	if ( !m_overlays.exists( handle ) ) {
		gameLocal.Warning( "Non-existant GUI handle: %d\n", handle );
		goto Quit;
	}

	// Entity GUIs are handled differently from regular ones.
	if ( handle >= 1 && handle <= MAX_RENDERENTITY_GUI ) {

		assert( m_overlays.isExternal( handle ) );

		if ( renderEntity.gui[ handle-1 ] && renderEntity.gui[ handle-1 ]->IsUniqued() ) {

			// We're dealing with an existing unique GUI.
			// We need to read a new GUI into it.

			// Clear the state.
			const idDict &state = renderEntity.gui[ handle-1 ]->State();
			const idKeyValue *kv;
			while ( ( kv = state.MatchPrefix( "" ) ) != NULL )
				renderEntity.gui[ handle-1 ]->DeleteStateVar( kv->GetKey() );

			renderEntity.gui[ handle-1 ]->InitFromFile( guiFile );

		} else {

			// We're either dealing with a non-existant GUI, or a non-unique one.
			// It's safe to just set the render entity to point to a new GUI without
			// bothering to deallocate the previous GUI.
			renderEntity.gui[ handle-1 ] = uiManager->FindGui( guiFile, true, true );
			m_overlays.setGui( handle, renderEntity.gui[ handle - 1 ] );
			assert( renderEntity.gui[ handle-1 ] );

		}

	} else if ( !m_overlays.isExternal( handle ) ) {

		bool result = m_overlays.setGui( handle, guiFile );
		assert( result );

	} else {
		gameLocal.Warning( "Cannot call setGui() on external handle: %d\n", handle );
	}

	Quit:
	return;
}

/*
================
idEntity::Event_GetGui

Returns the file loaded by a specific GUI.
================
*/
void idEntity::Event_GetGui( int handle ) {
	idUserInterface *gui = m_overlays.getGui( handle );
	if ( gui )
		idThread::ReturnString( gui->Name() );
	else
		idThread::ReturnString( "" );
}

void idEntity::SetGuiString(int handle, const char *key, const char *val)
{
	if(m_overlays.exists(handle))
	{
		idUserInterface *gui = m_overlays.getGui(handle);
		if(gui == NULL)
		{
			DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Handle points to NULL GUI: %d [%s]\r", handle, key);
			goto Quit;
		}

		if(!gui->IsUniqued())
		{
			DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("GUI is not unique. Handle: %d [%s]\r", handle, key);
			goto Quit;
		}

		gui->SetStateString(key, val);
		gui->StateChanged(gameLocal.time);
	}
	else
	{
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Non-existant GUI handle: %d\r", handle);
	}

Quit:
	return;
}

/*
================
idEntity::Event_SetGuiString

Sets a parameter for a GUI. (note that the GUI needs to be unique)
================
*/
void idEntity::Event_SetGuiString(int handle, const char *key, const char *val)
{
	SetGuiString(handle, key, val);
}

const char *idEntity::GetGuiString(int handle, const char *key)
{
	const char *retStr = NULL;

	if(m_overlays.exists(handle))
	{
		idUserInterface *gui = m_overlays.getGui(handle);
		if(gui)
			retStr = gui->GetStateString(key);
		else
			DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Handle points to NULL GUI: %d [%s]\r", handle, key);
	}
	else
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Handle points to NULL GUI: %d [%s]\r", handle, key);

	return retStr;
}

void idEntity::Event_GetGuiString(int handle, const char *key)
{
	const char *retStr = GetGuiString(handle, key);

	if(retStr == NULL)
		retStr = "";

	idThread::ReturnString(retStr);
}

void idEntity::SetGuiFloat( int handle, const char *key, float f)
{
	if(m_overlays.exists(handle))
	{
		idUserInterface *gui = m_overlays.getGui(handle);
		if(gui == NULL)
		{
			DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Handle points to NULL GUI: %d [%s]\r", handle, key);
			goto Quit;
		}

		if(!gui->IsUniqued())
		{
			DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("GUI is not unique. Handle: %d [%s]\r", handle, key);
			goto Quit;
		}

		gui->SetStateFloat(key, f);
		gui->StateChanged(gameLocal.time);
	}
	else
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("setGui: Non-existant GUI handle: %d\r", handle);

Quit:
	return;
}

void idEntity::Event_SetGuiFloat(int handle, const char *key, float f)
{
	SetGuiFloat(handle, key, f);
}

float idEntity::GetGuiFloat(int handle, const char *key)
{
	float retVal = 0;
	if(m_overlays.exists(handle))
	{
		idUserInterface *gui = m_overlays.getGui( handle );
		if (gui)
			retVal = gui->GetStateFloat(key);
		else
			DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Handle points to NULL GUI: %d [%s]\r", handle, key);
	}
	else
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("setGui: Non-existant GUI handle: %d\r", handle);

	return retVal;
}

void idEntity::Event_GetGuiFloat(int handle, const char *key)
{
	idThread::ReturnFloat(GetGuiFloat(handle, key));
}

void idEntity::SetGuiInt( int handle, const char *key, int n)
{
	if(m_overlays.exists(handle))
	{
		idUserInterface *gui = m_overlays.getGui(handle);
		if(gui == NULL)
		{
			DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Handle points to NULL GUI: %d [%s]\r", handle, key);
			goto Quit;
		}

		if(!gui->IsUniqued())
		{
			DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("GUI is not unique. Handle: %d [%s]\r", handle, key);
			goto Quit;
		}

		gui->SetStateInt(key, n);
		gui->StateChanged(gameLocal.time);
	}
	else
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("setGui: Non-existant GUI handle: %d\r", handle);

Quit:
	return;
}

void idEntity::Event_SetGuiInt(int handle, const char *key, int n)
{
	SetGuiInt(handle, key, n);
}

int idEntity::GetGuiInt(int handle, const char *key)
{
	int retVal = 0;
	if(m_overlays.exists(handle))
	{
		idUserInterface *gui = m_overlays.getGui(handle);
		if (gui)
			retVal = gui->GetStateInt(key);
		else
			DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Handle points to NULL GUI: %d [%s]\r", handle, key);
	}
	else
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("setGui: Non-existant GUI handle: %d\r", handle);

	return retVal;
}

void idEntity::Event_GetGuiInt(int handle, const char *key)
{
	idThread::ReturnInt(GetGuiInt(handle, key));
}

/*
================
idEntity::Event_SetGuiStringFromKey

This is a kludge. It's hopefully temporary, but probably not. Anyway, it's
used by readables to bypass the 127 char limit on string variables in scripts.
================
*/
void idEntity::Event_SetGuiStringFromKey( int handle, const char *key, idEntity *src, const char *spawnArg )
{
	if ( !src )
	{
		gameLocal.Warning( "Unable to get key, since the source entity was NULL.\n" );
		goto Quit;
	}

	if(!m_overlays.exists(handle))
	{
		gameLocal.Warning( "Non-existant GUI handle: %d\n", handle );
		goto Quit;
	}

	idUserInterface *gui = m_overlays.getGui( handle );
	if(!gui)
	{
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Handle points to NULL GUI: %d [%s]\r", handle, key);
		goto Quit;
	}

	if(!gui->IsUniqued())
	{
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Non-existant GUI handle: %d\r", handle);
		goto Quit;
	}

	gui->SetStateString( key, src->spawnArgs.GetString( spawnArg, "" ) );
	gui->StateChanged( gameLocal.time );

	Quit:
	return;
}

void idEntity::CallGui(int handle, const char *namedEvent)
{
	if(m_overlays.exists(handle))
	{
		idUserInterface *gui = m_overlays.getGui( handle );
		if(gui == NULL)
		{
			DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Handle points to NULL GUI: %d [%s]\r", handle, namedEvent);
			goto Quit;
		}

		if(!gui->IsUniqued())
		{
			DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("GUI is not unique. Handle: %d [%s]\r", handle, namedEvent);
			goto Quit;
		}

		gui->HandleNamedEvent( namedEvent );
	}
	else
	{
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("setGui: Non-existant GUI handle: %d [%s]\r", handle, namedEvent);
	}

Quit:
	return;
}

/*
================
idEntity::Event_CallGui

Calls a named event in a GUI. (note that the GUI needs to be unique)
================
*/
void idEntity::Event_CallGui(int handle, const char *namedEvent)
{
	CallGui(handle, namedEvent);
}

/*
================
idEntity::Event_LoadExternalData

Used to load an external xdata declaration into this
object's spawn args. The prefix will be prepended to
the names of all keys in the declaration.
================
*/
void idEntity::Event_LoadExternalData( const char *xdFile, const char* prefix )
{
	const tdmDeclXData *xd = static_cast< const tdmDeclXData* >( declManager->FindType( DECL_XDATA, xdFile, false ) );
	if ( xd != NULL ) {
		const idDict *data = &(xd->m_data);
		const idKeyValue *kv;

		int i = data->GetNumKeyVals();
		while ( i-- ) {
			kv = data->GetKeyVal(i);
			spawnArgs.Set( prefix + kv->GetKey(), kv->GetValue() );
		}

		idThread::ReturnInt( 1 );
	} 
	else
	{
		gameLocal.Warning("Non-existant xdata declaration: %s\n", xdFile);
		idThread::ReturnInt( 0 );
	}
}

/*
================
idEntity::Event_GetInventory

Returns the entity containing us.
================
*/
void idEntity::Event_GetInventory()
{
/*	CInventoryItem* item = InventoryItem();
	if ( item != NULL ) {
		CInventory* inv = item->Inventory();
		if ( inv != NULL ) {
			idThread::ReturnEntity( inv->m_owner.GetEntity() );
		} else {
			idThread::ReturnEntity( NULL );
		}
	}
*/
}

idThread *idEntity::CallScriptFunctionArgs(const char *fkt, bool ClearStack, int delay, const char *fmt, ...)
{
	idThread *pThread = NULL;
	va_list argptr;

	const function_t *pScriptFkt = scriptObject.GetFunction(fkt);
	if(pScriptFkt == NULL)
	{
		DM_LOG(LC_MISC, LT_DEBUG)LOGSTRING("Action: %s not found in local space, checking for global namespace.\r", fkt);
		pScriptFkt = gameLocal.program.FindFunction(fkt);
	}

	if(pScriptFkt)
	{
		DM_LOG(LC_MISC, LT_DEBUG)LOGSTRING("Running scriptfunction\r");
		pThread = new idThread(pScriptFkt);
		va_start(argptr, fmt);
		pThread->CallFunctionArgsVN(pScriptFkt, ClearStack, fmt, argptr);
		va_end(argptr);
		pThread->DelayedStart(delay);
	}
	else
		DM_LOG(LC_MISC, LT_ERROR)LOGSTRING("Scriptfunction not found! [%s]\r", fkt);

	return pThread;
}

void idEntity::Attach( idEntity *ent ) 
{
	idVec3			origin;
	idMat3			axis;
	idAngles		angleOffset;
	idVec3			originOffset;

	angleOffset = ent->spawnArgs.GetAngles( "angles" );
	originOffset = ent->spawnArgs.GetVector( "origin" );

	origin = GetPhysics()->GetOrigin();
	axis = GetPhysics()->GetAxis();

	ent->SetOrigin( origin + originOffset * renderEntity.axis );
	idMat3 rotate = angleOffset.ToMat3();
	idMat3 newAxis = rotate * axis;
	ent->SetAxis( newAxis );
	ent->Bind( this, true );
	ent->cinematic = cinematic;
}

void idEntity::BindNotify( idEntity *ent )
{
}

void idEntity::UnbindNotify( idEntity *ent )
{
}

void idEntity::Event_TimerCreate(int StimType, int Hour, int Minute, int Seconds, int Millisecond)
{
	CStim *stim = m_StimResponseColl->GetStim(StimType);
	CStimResponseTimer *timer;

	DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Create Timer: Stimtype-%d Hour: %d  Minute: %d   Seconds: %d   Milliseconds: %d\r",
		StimType, Hour, Minute, Seconds, Millisecond);
	timer = stim->CreateTimer();
	timer->SetTimer(Hour, Minute, Seconds, Millisecond);
}

void idEntity::Event_TimerStop(int StimType)
{
	CStim *stim = m_StimResponseColl->GetStim(StimType);
	CStimResponseTimer *timer = (stim != NULL) ? stim->GetTimer() : NULL;

	DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("StopTimer: Stimtype-%d \r", StimType);
	if(timer == NULL)
		goto Quit;

	timer->Stop();

Quit:
	return;
}

void idEntity::Event_TimerStart(int StimType)
{
	CStim *stim = m_StimResponseColl->GetStim(StimType);
	CStimResponseTimer *timer = (stim != NULL) ? stim->GetTimer() : NULL;

	DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("StartTimer: Stimtype-%d \r", StimType);
	if(timer == NULL)
		goto Quit;

	timer->Start(sys->GetClockTicks());

Quit:
	return;
}

void idEntity::Event_TimerRestart(int StimType)
{
	CStim *stim = m_StimResponseColl->GetStim(StimType);
	CStimResponseTimer *timer = (stim != NULL) ? stim->GetTimer() : NULL;

	DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("RestartTimer: Stimtype-%d \r", StimType);
	if(timer == NULL)
		goto Quit;

	timer->Restart(sys->GetClockTicks());

Quit:
	return;
}

void idEntity::Event_TimerReset(int StimType)
{
	CStim *stim = m_StimResponseColl->GetStim(StimType);
	CStimResponseTimer *timer = (stim != NULL) ? stim->GetTimer() : NULL;

	DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("ResetTimer: Stimtype-%d \r", StimType);
	if(timer == NULL)
		goto Quit;

	timer->Reset();

Quit:
	return;
}

void idEntity::Event_TimerSetState(int StimType, int State)
{
	CStim *stim = m_StimResponseColl->GetStim(StimType);
	CStimResponseTimer *timer = (stim != NULL) ? stim->GetTimer() : NULL;

	DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("SetTimerState: Stimtype-%d State: %d\r", StimType, State);
	if(timer == NULL)
		goto Quit;

	timer->SetState((CStimResponseTimer::TimerState)State);

Quit:
	return;
}

void idEntity::Event_SetFrobable( bool bVal )
{
	m_bFrobable = bVal;

	if( m_bFrobable )
		BecomeActive( TH_UPDATEVISUALS );
}

void idEntity::Event_IsFrobable( void )
{
	idThread::ReturnInt( (int) m_bFrobable );
}

void idEntity::Event_IsHilighted( void )
{
	int retVal = 0;
	if ( m_bFrobHighlightState )
		retVal++;
	if ( m_bFrobbed )
		retVal++;
	idThread::ReturnInt( retVal );
}

/**
* This is separate from m_bFrobbed due to peer frob highlighting,
* to let an entity display the highlight when not frobbed.
**/
bool                    m_bFrobHighlightState;



SDK_SIGNAL idEntity::GetSDKSignalId(void)
{
	SDK_SIGNAL rc = ++m_Signal;

	if(rc == 0)
		rc = ++m_Signal;

	return rc;
}

SDK_SIGNAL idEntity::AddSDKSignal(E_SDK_SIGNAL_STATE (*oFkt)(idEntity *oObject, void *pData), void *pData)
{
	SDK_SIGNAL rc  = 0;
	SDKSignalInfo *s;

	if(oFkt == NULL)
		goto Quit;

	s = new SDKSignalInfo;
	s->m_Object = this;
	s->m_Data = pData;
	s->m_Signaled = false;
	s->m_Fkt = oFkt;
	s->m_Id = GetSDKSignalId();
	m_SignalList.Append(s);
	gameLocal.AddSDKSignal(this);

	rc = s->m_Id;

Quit:
	return rc;
}

void idEntity::CheckSDKSignal(void)
{
	int i, n;
	SDKSignalInfo *s;

	// Since we are modifying the loopvariable, we use a 'while' her instead of 'for'.
	n = m_SignalList.Num();
	i = 0;
	while(i < n)
	{
		s = m_SignalList[i];
		if(s->m_Signaled == true)
		{
			if(s->m_Fkt(s->m_Object, s->m_Data) == SIG_CONTINUE)
				s->m_Signaled = false;
			else
			{
				m_SignalList.Remove(s);
				delete s;
				i--;
				n--;
			}
		}

		i++;
	}
}

void idEntity::SDKSignal(SDK_SIGNAL Id, int bState)
{
	int i, n;
	SDKSignalInfo *s;

	n = m_SignalList.Num();
	for(i = 0; i < n; i++)
	{
		s = m_SignalList[i];
		if(s->m_Id == Id)
		{
			s->m_Signaled = bState;
			break;
		}
	}
}

/*
================
idEntity::Event_RangedThreatTo
================
*/
void idEntity::Event_RangedThreatTo(idEntity* target)
{
	// This needs to be virtual, but I don't think events themselves
	// can be virtual; so this just wraps a virtual method.
	float result = this->RangedThreatTo(target);
	idThread::ReturnFloat(result);
}

/*
================
idEntity::RangedThreatTo

Return nonzero if this entity could potentially
attack the given (target) entity at range, or
entities in general if target is NULL.
For example, for the player this should return 1
if the player has a bow equipped and 0 otherwise.
================
*/
float idEntity::RangedThreatTo(idEntity* target)
{
	// Most entities are not capable of attacking at range
	return 0;
}

void idEntity::GetTeamChildren( idList<idEntity *> *list )
{
	idEntity *NextEnt = NULL;
	
	list->Clear();
	for( NextEnt = GetNextTeamEntity(); NextEnt != NULL; NextEnt = NextEnt->GetNextTeamEntity() )
	{
		list->Append( NextEnt );
	}
}

void idEntity::Event_GetBindMaster( void )
{
	idThread::ReturnEntity( GetBindMaster() );
}

void idEntity::Event_NumBindChildren( void )
{
	idList<idEntity *> ChildList;
	GetTeamChildren( &ChildList );

	idThread::ReturnInt( ChildList.Num() );
}

void idEntity::Event_GetBindChild( int ind )
{
	idEntity *pReturnVal( NULL );

	idList<idEntity *> ChildList;
	GetTeamChildren( &ChildList );

	if( (ChildList.Num() - 1) >= ind )
		pReturnVal = ChildList[ ind ];

	idThread::ReturnEntity( pReturnVal );
}

/*
================
idEntity::Event_ReplaceItem

Replaces another entity's item with our own, keeping the same
spot in the inventory and moving its cursors to point to us.
================
*/
void idEntity::Event_ReplaceItem(idEntity *old_item, idEntity *new_item)
{
}

void idEntity::Event_GetNextItem(int wrap)
{
}

void idEntity::Event_GetPrevItem(int wrap)
{
}

void idEntity::Event_SetCursorGroup(const char *groupname)
{
}

void idEntity::Event_SetCursorGroupItem(const char *itemname, const char *groupname)
{
}

void idEntity::Event_SetCursorItem(const char *itemname)
{
}

void idEntity::Event_GetCursorGroup(void)
{
}

void idEntity::Event_GetCursorItem(void)
{
}

void idEntity::Event_AddItem(idEntity *item)
{
	if(item == NULL || name == NULL)
		goto Quit;

	m_Inventory->PutItem(item, this);

Quit:
	return;
}

void idEntity::Event_GetGroupItem(const char *name, const char *group)
{
}

void idEntity::Event_GetItem(const char *name)
{
}

void idEntity::Event_GetLoot(int LootType)
{
	int Gold, Jewelry, Goods, Total;
	int rc = 0;

	Total = Inventory()->GetLoot(Gold, Jewelry, Goods);

	switch(LootType)
	{
		case CInventoryItem::LT_GOLD:
			rc = Gold;
		break;

		case CInventoryItem::LT_GOODS:
			rc = Goods;
		break;

		case CInventoryItem::LT_JEWELS:
			rc = Jewelry;
		break;

		default:
			rc = Total;
		break;
	}

	idThread::ReturnInt(rc);
}

void idEntity::InitInventory(void)
{
	// Check if this object should be put into the inventory of some entity
	// when the object spawns. Default is no.
	idStr inv;
	idStr target;

	spawnArgs.GetString("inv_map_start", "0", inv);
	spawnArgs.GetString("inv_target", "", target);
	if(inv[0] == '1')
	{
		idStr grp;
		idPlayer *p = gameLocal.GetLocalPlayer();
		if(p)
		{
			CInventoryItem *item;
			item = p->Inventory()->PutItem(this, p);
		}
		else
			gameLocal.AddInventoryEntity(target, name);		// Schedule us for later addition to the inventory
	}
}

void idEntity::CheckInventoryInit(void)
{
	bool cont;
	idEntity *e;

	// Check if we have some items that we need to add to our inventory.
	while((cont = gameLocal.GetInventoryEntity(name, &e)) == true)
	{
		if(e)
			e->InitInventory();
	}
}

CInventoryItem *idEntity::AddToInventory(idEntity *ent, idUserInterface *_hud)
{
	CInventoryCursor *crsr = InventoryCursor();
	CInventoryItem *rc = NULL;
	CInventoryItem *prev = NULL;
	idStr s;
	int v = 0;

	if(ent == NULL)
		goto Quit;

	// Check if we have an inventory item.
	if(ent->spawnArgs.GetString("inv_name", "", s) == false)
		goto Quit;

	prev = crsr->GetCurrentItem();
	rc = crsr->Inventory()->PutItem(ent, this);

	ent->spawnArgs.GetString("snd_acquire", "", s);
	if(s.Length() == 0)
		s = cv_tdm_inv_loot_sound.GetString();

	StartSoundShader(declManager->FindSound(s), SCHANNEL_ANY, 0, false, &v);

	if(rc)
	{
		if(ent->spawnArgs.GetString("inv_hud", "", s) != false)
		{
			ent->spawnArgs.GetInt("inv_hud_layer", "0", v);
			rc->SetHUD(s, v);
		}
	}

	crsr->SetCurrentItem(rc);
	if(_hud != NULL)
		inventoryChangeSelection(_hud, true, prev);

Quit:
	return rc;
}

void idEntity::Event_DestroyOverlay(int layer)
{
	DestroyOverlay(layer);
}

void idEntity::DestroyOverlay(int layer)
{
}

void idEntity::Event_CreateOverlay( const char *guiFile, int layer )
{
	idThread::ReturnInt(CreateOverlay(guiFile, layer));
}

int idEntity::CreateOverlay(const char *guiFile, int layer)
{
	int rc = OVERLAYS_INVALID_HANDLE;
	int handle = OVERLAYS_INVALID_HANDLE;

	if(guiFile == NULL || guiFile[0] == 0)
	{
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Invalid GUI file name\r");
		goto Quit;
	}

	if(!uiManager->CheckGui(guiFile))
	{
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Unable to load GUI file: [%s]\r", guiFile);
		goto Quit;
	}
	handle = m_overlays.createOverlay( layer );
	if(handle == OVERLAYS_INVALID_HANDLE)
	{
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Unable to create overlay for GUI [%s]\r", guiFile);
		goto Quit;
	}

	m_overlays.setGui(handle, guiFile);
	idUserInterface *gui = m_overlays.getGui(handle);
	if(gui == NULL)
	{
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Unable to load GUI [%s] into overlay.\r", guiFile);
		goto Quit;
	}

	gui->SetStateInt("handle", handle);
	gui->Activate(true, gameLocal.time);
	// Let's set a good default value for whether or not the overlay is interactive.
	m_overlays.setInteractive(handle, gui->IsInteractive());

	rc = handle;

Quit:
	if(rc == OVERLAYS_INVALID_HANDLE)
	{
		if(handle != OVERLAYS_INVALID_HANDLE)
			m_overlays.destroyOverlay(handle);
	}

	return rc;
}

void idEntity::inventoryChangeSelection(idUserInterface *_hud, bool bUpdate, CInventoryItem *Prev)
{
}

/**
* If this entity (or any entity that it is attached to) has mantling disabled,
* then this returns false. Otherwise, returns true.
**/
bool idEntity::IsMantleable()
{
	bool returnVal = true;
	idEntity* ent=this;
	while (ent!=NULL)
	{
		if (!ent->m_bIsMantleable)
		{
			returnVal = false;
			break;
		}
		ent = ent->GetBindMaster();
	}
	return returnVal;
}

int idEntity::heal(const char* healDefName, float healScale) {
	const idDict* healDef = gameLocal.FindEntityDefDict( healDefName );
	if ( !healDef ) {
		gameLocal.Error( "Unknown healDef '%s'\n", healDefName );
	}

	int	healAmount = static_cast<int>(healDef->GetInt( "heal_amount" ) * healScale);

	// Check if the entity can be healed in the first place
	if ( healAmount != 0 && health < spawnArgs.GetInt("health")) {
		// Yes, heal the entity
		health += healAmount;

		// It may be possible that negative healAmounts are applied and the entity dies
		if ( health <= 0 ) {
			if ( health < -999 ) {
				health = -999;
			}

			Killed( gameLocal.world, gameLocal.world, abs(healAmount), GetLocalCoordinates(GetPhysics()->GetOrigin()), 0);
		} else {
			// greebo: Do nothing, if health > 0, a "Healed" function could be invoked here
			//Pain( inflictor, attacker, damage, dir, location );
		}

		return 1;
	}
	else {
		return 0;
	}
}

// Deals damage to this entity, documentation: see header
void idEntity::Event_Damage( idEntity *inflictor, idEntity *attacker, 
								 const idVec3 &dir, const char *damageDefName, 
								 const float damageScale )
{
	// Pass the call to the regular member method of this class
	Damage(inflictor, attacker, dir, damageDefName, damageScale, 0, NULL);
}

void idEntity::Event_Heal( const char *healDefName, const float healScale )
{
	// Pass the call to the idEntity::heal method
	idThread::ReturnInt(heal(healDefName, healScale));
}
