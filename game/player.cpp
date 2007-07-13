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

#pragma warning(disable : 4355) // greebo: Disable warning "'this' used in constructor"

static bool init_version = FileVersionList("$Id$", init_version);

#include "game_local.h"
#include "../DarkMod/DarkModGlobals.h"
#include "../DarkMod/PlayerData.h"
#include "../DarkMod/Intersection.h"
#include "../DarkMod/Relations.h"
#include "../DarkMod/darkModAASFindHidingSpots.h"
#include "../DarkMod/StimResponse/StimResponseCollection.h"
#include "../DarkMod/MissionData.h"
#include "../DarkMod/Inventory/Inventory.h"
#include "../DarkMod/Inventory/WeaponItem.h"
#include "../DarkMod/KeyboardHook.h"
/*
===============================================================================

	Player control of the Doom Marine.
	This object handles all player movement and world interaction.

===============================================================================
*/

bool NextFrame = true;

// amount of health per dose from the health station
const int HEALTH_PER_DOSE = 10;

// time before a weapon dropped to the floor disappears
const int WEAPON_DROP_TIME = 20 * 1000;

// time before a next or prev weapon switch happens
const int WEAPON_SWITCH_DELAY = 150;

// how many units to raise spectator above default view height so it's in the head of someone
const int SPECTATE_RAISE = 25;

const int HEALTHPULSE_TIME = 333;

// minimum speed to bob and play run/walk animations at
const float MIN_BOB_SPEED = 5.0f;

const idEventDef EV_Player_GetButtons( "getButtons", NULL, 'd' );
const idEventDef EV_Player_GetMove( "getMove", NULL, 'v' );
const idEventDef EV_Player_GetViewAngles( "getViewAngles", NULL, 'v' );
const idEventDef EV_Player_StopFxFov( "stopFxFov" );
const idEventDef EV_Player_EnableWeapon( "enableWeapon" );
const idEventDef EV_Player_DisableWeapon( "disableWeapon" );
const idEventDef EV_Player_GetCurrentWeapon( "getCurrentWeapon", NULL, 's' );
const idEventDef EV_Player_GetPreviousWeapon( "getPreviousWeapon", NULL, 's' );
const idEventDef EV_Player_SelectWeapon( "selectWeapon", "s" );
const idEventDef EV_Player_GetWeaponEntity( "getWeaponEntity", NULL, 'e' );
const idEventDef EV_Player_OpenPDA( "openPDA" );
const idEventDef EV_Player_InPDA( "inPDA", NULL, 'd' );
const idEventDef EV_Player_ExitTeleporter( "exitTeleporter" );
const idEventDef EV_Player_StopAudioLog( "stopAudioLog" );
const idEventDef EV_Player_HideTip( "hideTip" );
const idEventDef EV_Player_LevelTrigger( "levelTrigger" );
const idEventDef EV_SpectatorTouch( "spectatorTouch", "et" );
const idEventDef EV_Player_GetIdealWeapon( "getIdealWeapon", NULL, 's' );

const idEventDef EV_Player_GetEyePos( "getEyePos", NULL, 'v' );
const idEventDef EV_Player_SetImmobilization( "setImmobilization", "sd" );
const idEventDef EV_Player_GetImmobilization( "getImmobilization", "s", 'd' );
const idEventDef EV_Player_GetNextImmobilization( "getNextImmobilization", "ss", 's' );
const idEventDef EV_Player_SetHinderance( "setHinderance", "sff" );
const idEventDef EV_Player_GetHinderance( "getHinderance", "s", 'v' );
const idEventDef EV_Player_GetNextHinderance( "getNextHinderance", "ss", 's' );

const idEventDef EV_Player_SetGui( "setGui", "ds" );
const idEventDef EV_Player_GetInventoryOverlay( "getInventoryOverlay", NULL, 'd' );

const idEventDef EV_Player_PlayStartSound( "playStartSound", NULL );
const idEventDef EV_Player_MissionFailed("missionFailed", NULL );
const idEventDef EV_Player_DeathMenu("deathMenu", NULL );
const idEventDef EV_Player_HoldEntity( "holdEntity", "E", 'f' );
const idEventDef EV_Player_HeldEntity( "heldEntity", NULL, 'E' );

const idEventDef EV_Player_RopeRemovalCleanup( "ropeRemovalCleanup", "e" );
// NOTE: The following all take the "user" objective indices, starting at 1 instead of 0
const idEventDef EV_Player_SetObjectiveState( "setObjectiveState", "dd" );
const idEventDef EV_Player_GetObjectiveState( "getObjectiveState", "d", 'd');
const idEventDef EV_Player_SetObjectiveComp( "setObjectiveComp", "ddd" );
const idEventDef EV_Player_GetObjectiveComp( "getObjectiveComp", "dd", 'd' );
const idEventDef EV_Player_ObjectiveUnlatch( "objectiveUnlatch", "d" );
const idEventDef EV_Player_ObjectiveCompUnlatch( "objectiveCompUnlatch", "dd" );
const idEventDef EV_Player_SetObjectiveVisible( "setObjectiveVisible", "dd" );
const idEventDef EV_Player_SetObjectiveOptional( "setObjectiveOptional", "dd" );
const idEventDef EV_Player_SetObjectiveOngoing( "setObjectiveOngoing", "dd" );
const idEventDef EV_Player_SetObjectiveEnabling( "setObjectiveEnabling", "ds" );
// greebo: This allows scripts to set the "healthpool" for gradual healing
const idEventDef EV_Player_GiveHealthPool("giveHealthPool", "f");

CLASS_DECLARATION( idActor, idPlayer )
	EVENT( EV_Player_GetButtons,			idPlayer::Event_GetButtons )
	EVENT( EV_Player_GetMove,				idPlayer::Event_GetMove )
	EVENT( EV_Player_GetViewAngles,			idPlayer::Event_GetViewAngles )
	EVENT( EV_Player_StopFxFov,				idPlayer::Event_StopFxFov )
	EVENT( EV_Player_EnableWeapon,			idPlayer::Event_EnableWeapon )
	EVENT( EV_Player_DisableWeapon,			idPlayer::Event_DisableWeapon )
	EVENT( EV_Player_GetCurrentWeapon,		idPlayer::Event_GetCurrentWeapon )
	EVENT( EV_Player_GetPreviousWeapon,		idPlayer::Event_GetPreviousWeapon )
	EVENT( EV_Player_SelectWeapon,			idPlayer::Event_SelectWeapon )
	EVENT( EV_Player_GetWeaponEntity,		idPlayer::Event_GetWeaponEntity )
	EVENT( EV_Player_OpenPDA,				idPlayer::Event_OpenPDA )
	EVENT( EV_Player_InPDA,					idPlayer::Event_InPDA )
	EVENT( EV_Player_ExitTeleporter,		idPlayer::Event_ExitTeleporter )
	EVENT( EV_Player_StopAudioLog,			idPlayer::Event_StopAudioLog )
	EVENT( EV_Player_HideTip,				idPlayer::Event_HideTip )
	EVENT( EV_Player_LevelTrigger,			idPlayer::Event_LevelTrigger )
	EVENT( EV_Gibbed,						idPlayer::Event_Gibbed )

	EVENT( EV_Player_GetEyePos,				idPlayer::Event_GetEyePos )
	EVENT( EV_Player_SetImmobilization,		idPlayer::Event_SetImmobilization )
	EVENT( EV_Player_GetImmobilization,		idPlayer::Event_GetImmobilization )
	EVENT( EV_Player_GetNextImmobilization,	idPlayer::Event_GetNextImmobilization )
	EVENT( EV_Player_SetHinderance,			idPlayer::Event_SetHinderance )
	EVENT( EV_Player_GetHinderance,			idPlayer::Event_GetHinderance )
	EVENT( EV_Player_GetNextHinderance,		idPlayer::Event_GetNextHinderance )

	EVENT( EV_Player_SetGui,				idPlayer::Event_SetGui )
	EVENT( EV_Player_GetInventoryOverlay,	idPlayer::Event_GetInventoryOverlay )

	EVENT( EV_Player_PlayStartSound,		idPlayer::Event_PlayStartSound )
	EVENT( EV_Player_MissionFailed,			idPlayer::Event_MissionFailed )
	EVENT( EV_Player_DeathMenu,				idPlayer::Event_LoadDeathMenu )
	EVENT( EV_Player_HoldEntity,			idPlayer::Event_HoldEntity )
	EVENT( EV_Player_HeldEntity,			idPlayer::Event_HeldEntity )
	EVENT( EV_Player_RopeRemovalCleanup,	idPlayer::Event_RopeRemovalCleanup )
	EVENT( EV_Player_SetObjectiveState,		idPlayer::Event_SetObjectiveState )
	EVENT( EV_Player_GetObjectiveState,		idPlayer::Event_GetObjectiveState )
	EVENT( EV_Player_SetObjectiveComp,		idPlayer::Event_SetObjectiveComp )
	EVENT( EV_Player_GetObjectiveComp,		idPlayer::Event_GetObjectiveComp )
	EVENT( EV_Player_ObjectiveUnlatch,		idPlayer::Event_ObjectiveUnlatch )
	EVENT( EV_Player_ObjectiveCompUnlatch,	idPlayer::Event_ObjectiveComponentUnlatch )
	EVENT( EV_Player_SetObjectiveVisible,	idPlayer::Event_SetObjectiveVisible )
	EVENT( EV_Player_SetObjectiveOptional,	idPlayer::Event_SetObjectiveOptional )
	EVENT( EV_Player_SetObjectiveOngoing,	idPlayer::Event_SetObjectiveOngoing )
	EVENT( EV_Player_SetObjectiveEnabling,	idPlayer::Event_SetObjectiveEnabling )

	EVENT( EV_Player_GiveHealthPool,		idPlayer::Event_GiveHealthPool)

END_CLASS

const int MAX_RESPAWN_TIME = 10000;
const int RAGDOLL_DEATH_TIME = 3000;
const int MAX_PDAS = 64;
const int MAX_PDA_ITEMS = 128;
const int STEPUP_TIME = 200;
const int MAX_INVENTORY_ITEMS = 20;

idVec3 idPlayer::colorBarTable[ 5 ] = {
	idVec3( 0.25f, 0.25f, 0.25f ),
	idVec3( 1.00f, 0.00f, 0.00f ),
	idVec3( 0.00f, 0.80f, 0.10f ),
	idVec3( 0.20f, 0.50f, 0.80f ),
	idVec3( 1.00f, 0.80f, 0.10f )
};

/*
==============
idInventory::Clear
==============
*/
void idInventory::Clear( void ) {
	maxHealth		= 0;

	items.DeleteContents( true );
	memset(pdasViewed, 0, 4 * sizeof( pdasViewed[0] ) );
	pdas.Clear();
	videos.Clear();
	emails.Clear();
	selVideo = 0;
	selEMail = 0;
	selPDA = 0;
	selAudio = 0;
	pdaOpened = false;
	turkeyScore = false;

	lastGiveTime = 0;
}

/*
==============
idInventory::GetPersistantData
==============
*/
void idInventory::GetPersistantData( idDict &dict )
{
	int		i;
	idStr	key;

	// pdas viewed
	for ( i = 0; i < 4; i++ ) {
		dict.SetInt( va("pdasViewed_%i", i), pdasViewed[i] );
	}

	dict.SetInt( "selPDA", selPDA );
	dict.SetInt( "selVideo", selVideo );
	dict.SetInt( "selEmail", selEMail );
	dict.SetInt( "selAudio", selAudio );
	dict.SetInt( "pdaOpened", pdaOpened );
	dict.SetInt( "turkeyScore", turkeyScore );

	// pdas
	for ( i = 0; i < pdas.Num(); i++ ) {
		sprintf( key, "pda_%i", i );
		dict.Set( key, pdas[ i ] );
	}
	dict.SetInt( "pdas", pdas.Num() );

	// video cds
	for ( i = 0; i < videos.Num(); i++ ) {
		sprintf( key, "video_%i", i );
		dict.Set( key, videos[ i ].c_str() );
	}
	dict.SetInt( "videos", videos.Num() );

	// emails
	for ( i = 0; i < emails.Num(); i++ ) {
		sprintf( key, "email_%i", i );
		dict.Set( key, emails[ i ].c_str() );
	}
	dict.SetInt( "emails", emails.Num() );
}

/*
==============
idInventory::RestoreInventory
==============
*/
void idInventory::RestoreInventory( idPlayer *owner, const idDict &dict ) {
	int			i;
	int			num;
	idDict		*item;
	idStr		key;
	idStr		itemname;
	const idKeyValue *kv;

	Clear();

	// health/armor
	maxHealth		= dict.GetInt( "maxhealth", "100" );

	// items
	num = dict.GetInt( "items" );
	items.SetNum( num );
	for( i = 0; i < num; i++ ) {
		item = new idDict();
		items[ i ] = item;
		sprintf( itemname, "item_%i ", i );
		kv = dict.MatchPrefix( itemname );
		while( kv ) {
			key = kv->GetKey();
			key.Strip( itemname );
			item->Set( key, kv->GetValue() );
			kv = dict.MatchPrefix( itemname, kv );
		}
	}

	// pdas viewed
	for ( i = 0; i < 4; i++ ) {
		pdasViewed[i] = dict.GetInt(va("pdasViewed_%i", i));
	}

	selPDA = dict.GetInt( "selPDA" );
	selEMail = dict.GetInt( "selEmail" );
	selVideo = dict.GetInt( "selVideo" );
	selAudio = dict.GetInt( "selAudio" );
	pdaOpened = dict.GetBool( "pdaOpened" );
	turkeyScore = dict.GetBool( "turkeyScore" );

	// pdas
	num = dict.GetInt( "pdas" );
	pdas.SetNum( num );
	for ( i = 0; i < num; i++ ) {
		sprintf( itemname, "pda_%i", i );
		pdas[i] = dict.GetString( itemname, "default" );
	}

	// videos
	num = dict.GetInt( "videos" );
	videos.SetNum( num );
	for ( i = 0; i < num; i++ ) {
		sprintf( itemname, "video_%i", i );
		videos[i] = dict.GetString( itemname, "default" );
	}

	// emails
	num = dict.GetInt( "emails" );
	emails.SetNum( num );
	for ( i = 0; i < num; i++ ) {
		sprintf( itemname, "email_%i", i );
		emails[i] = dict.GetString( itemname, "default" );
	}

#ifdef ID_DEMO_BUILD
		Give( owner, dict, "weapon", dict.GetString( "weapon" ), NULL, false );
#else
	if ( g_skill.GetInteger() >= 3 ) {
		Give( owner, dict, "weapon", dict.GetString( "weapon_nightmare" ), NULL, false );
	} else {
		Give( owner, dict, "weapon", dict.GetString( "weapon" ), NULL, false );
	}
#endif
}

/*
==============
idInventory::Save
==============
*/
void idInventory::Save( idSaveGame *savefile ) const {
	int i;

	savefile->WriteInt( maxHealth );

	savefile->WriteInt( items.Num() );
	for( i = 0; i < items.Num(); i++ ) {
		savefile->WriteDict( items[ i ] );
	}

	savefile->WriteInt( pdasViewed[0] );

	savefile->WriteInt( pdasViewed[1] );

	savefile->WriteInt( pdasViewed[2] );

	savefile->WriteInt( pdasViewed[3] );


	savefile->WriteInt( selPDA );
	savefile->WriteInt( selVideo );
	savefile->WriteInt( selEMail );
	savefile->WriteInt( selAudio );
	savefile->WriteBool( pdaOpened );
	savefile->WriteBool( turkeyScore );

	savefile->WriteInt( pdas.Num() );
	for( i = 0; i < pdas.Num(); i++ ) {
		savefile->WriteString( pdas[ i ] );
	}

	savefile->WriteInt( pdaSecurity.Num() );
	for( i=0; i < pdaSecurity.Num(); i++ ) {
		savefile->WriteString( pdaSecurity[ i ] );
	}

	savefile->WriteInt( videos.Num() );
	for( i = 0; i < videos.Num(); i++ ) {
		savefile->WriteString( videos[ i ] );
	}

	savefile->WriteInt( emails.Num() );
	for ( i = 0; i < emails.Num(); i++ ) {
		savefile->WriteString( emails[ i ] );
	}

	savefile->WriteInt( lastGiveTime );
}

/*
==============
idInventory::Restore
==============
*/
void idInventory::Restore( idRestoreGame *savefile ) {
	int i, num;

	savefile->ReadInt( maxHealth );

	savefile->ReadInt( num );
	for( i = 0; i < num; i++ ) {
		idDict *itemdict = new idDict;

		savefile->ReadDict( itemdict );
		items.Append( itemdict );
	}

	// pdas
	savefile->ReadInt( pdasViewed[0] );

	savefile->ReadInt( pdasViewed[1] );

	savefile->ReadInt( pdasViewed[2] );

	savefile->ReadInt( pdasViewed[3] );


	savefile->ReadInt( selPDA );
	savefile->ReadInt( selVideo );
	savefile->ReadInt( selEMail );
	savefile->ReadInt( selAudio );
	savefile->ReadBool( pdaOpened );
	savefile->ReadBool( turkeyScore );

	savefile->ReadInt( num );
	for( i = 0; i < num; i++ ) {
		idStr strPda;
		savefile->ReadString( strPda );
		pdas.Append( strPda );
	}

	// pda security clearances
	savefile->ReadInt( num );
	for ( i = 0; i < num; i++ ) {
		idStr invName;
		savefile->ReadString( invName );
		pdaSecurity.Append( invName );
	}

	// videos
	savefile->ReadInt( num );
	for( i = 0; i < num; i++ ) {
		idStr strVideo;
		savefile->ReadString( strVideo );
		videos.Append( strVideo );
	}

	// email
	savefile->ReadInt( num );
	for( i = 0; i < num; i++ ) {
		idStr strEmail;
		savefile->ReadString( strEmail );
		emails.Append( strEmail );
	}

	savefile->ReadInt( lastGiveTime );
}

/*
==============
idInventory::Give
==============
*/
bool idInventory::Give( idPlayer *owner, const idDict &spawnArgs, const char *statname, const char *value, int *idealWeapon, bool updateHud ) {
	int						i;
	const char				*pos;
	const char				*end;
	int						len;
	idStr					weaponString;
//	int						max;
	const idDeclEntityDef	*weaponDecl;
	bool					tookWeapon;
//	int						amount;
//	idItemInfo				info;
//	const char				*name;

	/*if ( !idStr::Icmpn( statname, "ammo_", 5 ) ) {
		i = AmmoIndexForAmmoClass( statname );
		max = MaxAmmoForAmmoClass( owner, statname );
		if ( ammo[ i ] >= max ) {
			return false;
		}
		amount = atoi( value );
		if ( amount ) {			
			ammo[ i ] += amount;
			if ( ( max > 0 ) && ( ammo[ i ] > max ) ) {
				ammo[ i ] = max;
			}
			ammoPulse = true;

			name = AmmoPickupNameForIndex( i );
			if ( idStr::Length( name ) ) {
				AddPickupName( name, "" );
			}
		}
	} else 
	if ( !idStr::Icmp( statname, "armor" ) ) {
		if ( armor >= maxarmor ) {
			return false;	// can't hold any more, so leave the item
		}
		amount = atoi( value );
		if ( amount ) {
			armor += amount;
			if ( armor > maxarmor ) {
				armor = maxarmor;
			}
			nextArmorDepleteTime = 0;
			armorPulse = true;
		}
	} else if ( idStr::FindText( statname, "inclip_" ) == 0 ) {
		i = WeaponIndexForAmmoClass( spawnArgs, statname + 7 );
		if ( i != -1 ) {
			// set, don't add. not going over the clip size limit.
			clip[ i ] = atoi( value );
		}
	} else if ( !idStr::Icmp( statname, "berserk" ) ) {
		GivePowerUp( owner, BERSERK, SEC2MS( atof( value ) ) );
	} else if ( !idStr::Icmp( statname, "mega" ) ) {
		GivePowerUp( owner, MEGAHEALTH, SEC2MS( atof( value ) ) );
	} else*/ if ( !idStr::Icmp( statname, "weapon" ) ) {
		tookWeapon = false;
		for( pos = value; pos != NULL; pos = end ) {
			end = strchr( pos, ',' );
			if ( end ) {
				len = end - pos;
				end++;
			} else {
				len = strlen( pos );
			}

			idStr weaponName( pos, 0, len );

			// find the number of the matching weapon name
			for( i = 0; i < MAX_WEAPONS; i++ ) {
				if ( weaponName == spawnArgs.GetString( va( "def_weapon%d", i ) ) ) {
					break;
				}
			}

			if ( i >= MAX_WEAPONS ) {
				gameLocal.Error( "Unknown weapon '%s'", weaponName.c_str() );
			}

			// cache the media for this weapon
			weaponDecl = gameLocal.FindEntityDef( weaponName, false );

			/*if ( !gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) || ( weaponName == "weapon_fists" ) || ( weaponName == "weapon_soulcube" ) ) {
				if ( ( weapons & ( 1 << i ) ) == 0 || gameLocal.isMultiplayer ) {
					if ( owner->GetUserInfo()->GetBool( "ui_autoSwitch" ) && idealWeapon ) {
						assert( !gameLocal.isClient );
						*idealWeapon = i;
					} 
					if ( owner->hud && updateHud && lastGiveTime + 1000 < gameLocal.time ) {
						owner->hud->SetStateInt( "newWeapon", i );
						owner->hud->HandleNamedEvent( "newWeapon" );
						lastGiveTime = gameLocal.time;
					}
					weaponPulse = true;
					weapons |= ( 1 << i );
					tookWeapon = true;
				}
			}*/
		}
		return tookWeapon;
	} else if ( !idStr::Icmp( statname, "item" ) || !idStr::Icmp( statname, "icon" ) || !idStr::Icmp( statname, "name" ) ) {
		// ignore these as they're handled elsewhere
		return false;
	} else {
		// unknown item
		gameLocal.Warning( "Unknown stat '%s' added to player's inventory", statname );
		return false;
	}

	return true;
}

/*
===============
idInventoy::Drop
===============
*/
void idInventory::Drop( const idDict &spawnArgs, const char *weapon_classname, int weapon_index ) {
	// remove the weapon bit
	// also remove the ammo associated with the weapon as we pushed it in the item
	assert( weapon_index != -1 || weapon_classname );
	if ( weapon_index == -1 ) {
		for( weapon_index = 0; weapon_index < MAX_WEAPONS; weapon_index++ ) {
			if ( !idStr::Icmp( weapon_classname, spawnArgs.GetString( va( "def_weapon%d", weapon_index ) ) ) ) {
				break;
			}
		}
		if ( weapon_index >= MAX_WEAPONS ) {
			gameLocal.Error( "Unknown weapon '%s'", weapon_classname );
		}
	} else if ( !weapon_classname ) {
		weapon_classname = spawnArgs.GetString( va( "def_weapon%d", weapon_index ) );
	}
	//weapons &= ( 0xffffffff ^ ( 1 << weapon_index ) );
}

/*
==============
idPlayer::idPlayer
==============
*/
idPlayer::idPlayer() :
	m_ButtonStateTracker(this)
{
	memset( &usercmd, 0, sizeof( usercmd ) );

	noclip					= false;
	godmode					= false;

	spawnAnglesSet			= false;
	spawnAngles				= ang_zero;
	viewAngles				= ang_zero;
	cmdAngles				= ang_zero;

	oldButtons				= 0;
	buttonMask				= 0;
	oldFlags				= 0;

	lastHitTime				= 0;
	lastSndHitTime			= 0;
	lastSavingThrowTime		= 0;

	weapon					= NULL;

	hud						= NULL;
	objectiveSystem			= NULL;
	objectiveSystemOpen		= false;

	heartRate				= BASE_HEARTRATE;
	heartInfo.Init( 0, 0, 0, 0 );
	lastHeartAdjust			= 0;
	lastHeartBeat			= 0;
	lastDmgTime				= 0;
	deathClearContentsTime	= 0;
	lastArmorPulse			= -10000;
	stamina					= 0.0f;
	healthPool				= 0.0f;
	nextHealthPulse			= 0;
	healthPulse				= false;
	nextHealthTake			= 0;
	healthTake				= false;
	healthPoolTimeInterval	= HEALTHPULSE_TIME;
	healthPoolTimeIntervalFactor = 1.0f;
	healthPoolStepAmount	= 5;

	scoreBoardOpen			= false;
	forceScoreBoard			= false;
	forceRespawn			= false;
	spectating				= false;
	spectator				= 0;
	colorBar				= vec3_zero;
	colorBarIndex			= 0;
	forcedReady				= false;
	wantSpectate			= false;

	lastHitToggle			= false;

	minRespawnTime			= 0;
	maxRespawnTime			= 0;

	firstPersonViewOrigin	= vec3_zero;
	firstPersonViewAxis		= mat3_identity;

	hipJoint				= INVALID_JOINT;
	chestJoint				= INVALID_JOINT;
	headJoint				= INVALID_JOINT;

	bobFoot					= 0;
	bobFrac					= 0.0f;
	bobfracsin				= 0.0f;
	bobCycle				= 0;
	xyspeed					= 0.0f;
	stepUpTime				= 0;
	stepUpDelta				= 0.0f;
	idealLegsYaw			= 0.0f;
	legsYaw					= 0.0f;
	legsForward				= true;
	oldViewYaw				= 0.0f;
	viewBobAngles			= ang_zero;
	viewBob					= vec3_zero;
	landChange				= 0;
	landTime				= 0;

	currentWeapon			= -1;
	idealWeapon				= -1;
	previousWeapon			= -1;
	weaponSwitchTime		=  0;
	weaponEnabled			= true;
	weapon_soulcube			= -1;
	weapon_pda				= -1;
	weapon_fists			= -1;
	showWeaponViewModel		= true;

	skin					= NULL;
	powerUpSkin				= NULL;
	baseSkinName			= "";

	numProjectilesFired		= 0;
	numProjectileHits		= 0;

	airless					= false;
	airTics					= 0;
	lastAirDamage			= 0;
	underWaterEffectsActive	= false;
	underWaterGUIHandle		= -1;

	gibDeath				= false;
	gibsLaunched			= false;
	gibsDir					= vec3_zero;

	zoomFov.Init( 0, 0, 0, 0 );
	centerView.Init( 0, 0, 0, 0 );
	fxFov					= false;

	influenceFov			= 0;
	influenceActive			= 0;
	influenceRadius			= 0.0f;
	influenceEntity			= NULL;
	influenceMaterial		= NULL;
	influenceSkin			= NULL;

	privateCameraView		= NULL;

	m_ListenerLoc			= vec3_zero;
	m_DoorListenLoc			= vec3_zero;

	// m_immobilization.Clear();
	m_immobilizationCache	= 0;

	// m_hinderance.Clear();
	m_hinderanceCache	= 1.0f;

	memset( loggedViewAngles, 0, sizeof( loggedViewAngles ) );
	memset( loggedAccel, 0, sizeof( loggedAccel ) );
	currentLoggedAccel	= 0;

	focusTime				= 0;
	focusGUIent				= NULL;
	focusUI					= NULL;
	focusCharacter			= NULL;
	talkCursor				= 0;
	focusVehicle			= NULL;
	cursor					= NULL;
	
	oldMouseX				= 0;
	oldMouseY				= 0;

	pdaAudio				= "";
	pdaVideo				= "";
	pdaVideoWave			= "";

	lastDamageDef			= 0;
	lastDamageDir			= vec3_zero;
	lastDamageLocation		= 0;
	smoothedFrame			= 0;
	smoothedOriginUpdated	= false;
	smoothedOrigin			= vec3_zero;
	smoothedAngles			= ang_zero;

	fl.networkSync			= true;

	latchedTeam				= -1;
	doingDeathSkin			= false;
	weaponGone				= false;
	useInitialSpawns		= false;
	tourneyRank				= 0;
	lastSpectateTeleport	= 0;
	tourneyLine				= 0;
	hiddenWeapon			= false;
	tipUp					= false;
	objectiveUp				= false;
	teleportEntity			= NULL;
	teleportKiller			= -1;
	respawning				= false;
	ready					= false;
	leader					= false;
	lastSpectateChange		= 0;
	lastTeleFX				= -9999;
	weaponCatchup			= false;
	lastSnapshotSequence	= 0;

	MPAim					= -1;
	lastMPAim				= -1;
	lastMPAimTime			= 0;
	MPAimFadeTime			= 0;
	MPAimHighlight			= false;

	spawnedTime				= 0;
	lastManOver				= false;
	lastManPlayAgain		= false;
	lastManPresent			= false;

	isTelefragged			= false;

	isLagged				= false;
	isChatting				= false;
	selfSmooth				= false;

	m_bGrabberActive		= false;
	m_bDraggingBody			= false;
	m_bShoulderingBody		= false;

	m_LeanButtonTimeStamp	= 0;
	mInventoryOverlay		= -1;
	m_WeaponCursor			= NULL;
	m_ContinuousUse			= false;
}

/*
==============
idPlayer::LinkScriptVariables

set up conditions for animation
==============
*/
void idPlayer::LinkScriptVariables( void ) {
	AI_FORWARD.LinkTo(			scriptObject, "AI_FORWARD" );
	AI_BACKWARD.LinkTo(			scriptObject, "AI_BACKWARD" );
	AI_STRAFE_LEFT.LinkTo(		scriptObject, "AI_STRAFE_LEFT" );
	AI_STRAFE_RIGHT.LinkTo(		scriptObject, "AI_STRAFE_RIGHT" );
	AI_ATTACK_HELD.LinkTo(		scriptObject, "AI_ATTACK_HELD" );
	AI_WEAPON_FIRED.LinkTo(		scriptObject, "AI_WEAPON_FIRED" );
	AI_JUMP.LinkTo(				scriptObject, "AI_JUMP" );
	AI_DEAD.LinkTo(				scriptObject, "AI_DEAD" );
	AI_CROUCH.LinkTo(			scriptObject, "AI_CROUCH" );
	AI_ONGROUND.LinkTo(			scriptObject, "AI_ONGROUND" );
	AI_ONLADDER.LinkTo(			scriptObject, "AI_ONLADDER" );
	AI_HARDLANDING.LinkTo(		scriptObject, "AI_HARDLANDING" );
	AI_SOFTLANDING.LinkTo(		scriptObject, "AI_SOFTLANDING" );
	AI_RUN.LinkTo(				scriptObject, "AI_RUN" );
	AI_PAIN.LinkTo(				scriptObject, "AI_PAIN" );
	AI_RELOAD.LinkTo(			scriptObject, "AI_RELOAD" );
	AI_TELEPORT.LinkTo(			scriptObject, "AI_TELEPORT" );
	AI_TURN_LEFT.LinkTo(		scriptObject, "AI_TURN_LEFT" );
	AI_TURN_RIGHT.LinkTo(		scriptObject, "AI_TURN_RIGHT" );
	AI_LEAN_LEFT.LinkTo(		scriptObject, "AI_LEAN_LEFT" );
	AI_LEAN_RIGHT.LinkTo(		scriptObject, "AI_LEAN_RIGHT" );
	AI_LEAN_FORWARD.LinkTo(		scriptObject, "AI_LEAN_FORWARD" );
	AI_CREEP.LinkTo(			scriptObject, "AI_CREEP" );
}

/*
==============
idPlayer::SetupWeaponEntity
==============
*/
void idPlayer::SetupWeaponEntity( void ) {
	int w;
	const char *weap;

	if ( weapon.GetEntity() ) {
		// get rid of old weapon
		weapon.GetEntity()->Clear();
		currentWeapon = -1;
	} else if ( !gameLocal.isClient ) {
		weapon = static_cast<idWeapon *>( gameLocal.SpawnEntityType( idWeapon::Type, NULL ) );
		weapon.GetEntity()->SetOwner( this );
		currentWeapon = -1;
	}

	for( w = 0; w < MAX_WEAPONS; w++ ) {
		weap = spawnArgs.GetString( va( "def_weapon%d", w ) );
		if ( weap && *weap ) {
			idWeapon::CacheWeapon( weap );
		}
	}
}

/*
==============
idPlayer::Init
==============
*/
void idPlayer::Init( void ) {
	const char			*value;
	const idKeyValue	*kv;

	noclip					= false;
	godmode					= false;

	oldButtons				= 0;
	oldFlags				= 0;

	currentWeapon			= -1;
	idealWeapon				= -1;
	previousWeapon			= -1;
	weaponSwitchTime		= 0;
	weaponEnabled			= true;
	weapon_soulcube			= SlotForWeapon( "weapon_soulcube" );
	weapon_pda				= SlotForWeapon( "weapon_pda" );
	weapon_fists			= SlotForWeapon( "weapon_fists" );
	showWeaponViewModel		= GetUserInfo()->GetBool( "ui_showGun" );

	lastDmgTime				= 0;
	lastArmorPulse			= -10000;
	lastHeartAdjust			= 0;
	lastHeartBeat			= 0;
	heartInfo.Init( 0, 0, 0, 0 );

	bobCycle				= 0;
	bobFrac					= 0.0f;
	landChange				= 0;
	landTime				= 0;
	zoomFov.Init( 0, 0, 0, 0 );

	centerView.Init( 0, 0, 0, 0 );
	fxFov					= false;

	influenceFov			= 0;
	influenceActive			= 0;
	influenceRadius			= 0.0f;
	influenceEntity			= NULL;
	influenceMaterial		= NULL;
	influenceSkin			= NULL;

	currentLoggedAccel		= 0;

	focusTime				= 0;
	focusGUIent				= NULL;
	focusUI					= NULL;
	focusCharacter			= NULL;
	talkCursor				= 0;
	focusVehicle			= NULL;

	// remove any damage effects
	playerView.ClearEffects();

	// damage values
	fl.takedamage			= true;
	ClearPain();

	// restore persistent data
	RestorePersistantInfo();

	bobCycle		= 0;
	stamina			= 0.0f;
	healthPool		= 0.0f;
	nextHealthPulse = 0;
	healthPulse		= false;
	nextHealthTake	= 0;
	healthTake		= false;

	SetupWeaponEntity();
	currentWeapon = -1;
	previousWeapon = -1;

	heartRate = BASE_HEARTRATE;
	AdjustHeartRate( BASE_HEARTRATE, 0.0f, 0.0f, true );

	idealLegsYaw = 0.0f;
	legsYaw = 0.0f;
	legsForward	= true;
	oldViewYaw = 0.0f;

	// set the pm_ cvars
	if ( !gameLocal.isMultiplayer || gameLocal.isServer ) {
		kv = spawnArgs.MatchPrefix( "pm_", NULL );
		while( kv ) {
			cvarSystem->SetCVarString( kv->GetKey(), kv->GetValue() );
			kv = spawnArgs.MatchPrefix( "pm_", kv );
		}
	}

	// disable stamina on hell levels
	if ( gameLocal.world && gameLocal.world->spawnArgs.GetBool( "no_stamina" ) ) {
		pm_stamina.SetFloat( 0.0f );
	}

	// stamina always initialized to maximum
	stamina = pm_stamina.GetFloat();

	// air always initialized to maximum too
	airTics = pm_airTics.GetFloat();
	airless = false;

	gibDeath = false;
	gibsLaunched = false;
	gibsDir.Zero();

	// set the gravity
	physicsObj.SetGravity( gameLocal.GetGravity() );

	// start out standing
	SetEyeHeight( pm_normalviewheight.GetFloat() );

	stepUpTime = 0;
	stepUpDelta = 0.0f;
	viewBobAngles.Zero();
	viewBob.Zero();

	value = spawnArgs.GetString( "model" );
	if ( value && ( *value != 0 ) ) {
		SetModel( value );
	}

	if ( cursor ) {
		cursor->SetStateInt( "talkcursor", 0 );
		cursor->SetStateString( "combatcursor", "1" );
		cursor->SetStateString( "itemcursor", "0" );
		cursor->SetStateString( "guicursor", "0" );
	}

	if ( ( gameLocal.isMultiplayer || g_testDeath.GetBool() ) && skin ) {
		SetSkin( skin );
		renderEntity.shaderParms[6] = 0.0f;
	} else if ( spawnArgs.GetString( "spawn_skin", NULL, &value ) ) {
		skin = declManager->FindSkin( value );
		SetSkin( skin );
		renderEntity.shaderParms[6] = 0.0f;
	}

	value = spawnArgs.GetString( "bone_hips", "" );
	hipJoint = animator.GetJointHandle( value );
	if ( hipJoint == INVALID_JOINT ) {
		gameLocal.Error( "Joint '%s' not found for 'bone_hips' on '%s'", value, name.c_str() );
	}

	value = spawnArgs.GetString( "bone_chest", "" );
	chestJoint = animator.GetJointHandle( value );
	if ( chestJoint == INVALID_JOINT ) {
		gameLocal.Error( "Joint '%s' not found for 'bone_chest' on '%s'", value, name.c_str() );
	}

	value = spawnArgs.GetString( "bone_head", "" );
	headJoint = animator.GetJointHandle( value );
	if ( headJoint == INVALID_JOINT ) {
		gameLocal.Error( "Joint '%s' not found for 'bone_head' on '%s'", value, name.c_str() );
	}

	// initialize the script variables
	AI_FORWARD		= false;
	AI_BACKWARD		= false;
	AI_STRAFE_LEFT	= false;
	AI_STRAFE_RIGHT	= false;
	AI_ATTACK_HELD	= false;
	AI_WEAPON_FIRED	= false;
	AI_JUMP			= false;
	AI_DEAD			= false;
	AI_CROUCH		= false;
	AI_ONGROUND		= true;
	AI_ONLADDER		= false;
	AI_HARDLANDING	= false;
	AI_SOFTLANDING	= false;
	AI_RUN			= false;
	AI_PAIN			= false;
	AI_RELOAD		= false;
	AI_TELEPORT		= false;
	AI_TURN_LEFT	= false;
	AI_TURN_RIGHT	= false;

	AI_LEAN_LEFT	= false;
	AI_LEAN_RIGHT	= false;
	AI_LEAN_FORWARD	= false;

	AI_CREEP		= false;



	// reset the script object
	ConstructScriptObject();

	// execute the script so the script object's constructor takes effect immediately
	scriptThread->Execute();
	
	forceScoreBoard		= false;
	forcedReady			= false;

	privateCameraView	= NULL;

	lastSpectateChange	= 0;
	lastTeleFX			= -9999;

	hiddenWeapon		= false;
	tipUp				= false;
	objectiveUp			= false;
	teleportEntity		= NULL;
	teleportKiller		= -1;
	leader				= false;

	SetPrivateCameraView( NULL );

	lastSnapshotSequence	= 0;

	MPAim				= -1;
	lastMPAim			= -1;
	lastMPAimTime		= 0;
	MPAimFadeTime		= 0;
	MPAimHighlight		= false;

	if ( hud ) {
		hud->HandleNamedEvent( "aim_clear" );
	}

	cvarSystem->SetCVarBool( "ui_chat", false );
}

/*
==============
idPlayer::Spawn

Prepare any resources used by the player.
==============
*/
void idPlayer::Spawn( void ) 
{
	int			i;
	idStr		temp;
	idBounds	bounds;

	if ( entityNumber >= MAX_CLIENTS ) {
		gameLocal.Error( "entityNum > MAX_CLIENTS for player.  Player may only be spawned with a client." );
	}

	// allow thinking during cinematics
	cinematic = true;

	if ( gameLocal.isMultiplayer ) {
		// always start in spectating state waiting to be spawned in
		// do this before SetClipModel to get the right bounding box
		spectating = true;
	}

	m_immobilization.Clear();
	m_immobilizationCache = 0;

	m_hinderance.Clear();
	m_hinderanceCache = 1.0f;

	// set our collision model
	physicsObj.SetSelf( this );
	SetClipModel();
	physicsObj.SetMass( spawnArgs.GetFloat( "mass", "100" ) );
	physicsObj.SetContents( CONTENTS_BODY );
	// SR CONTENTS_RESPONSE FIX
	if( m_StimResponseColl->HasResponse() )
		physicsObj.SetContents( physicsObj.GetContents() | CONTENTS_RESPONSE );
	
	physicsObj.SetClipMask( MASK_PLAYERSOLID );
	SetPhysics( &physicsObj );
	InitAASLocation();

	skin = renderEntity.customSkin;

	// only the local player needs guis
	if ( !gameLocal.isMultiplayer || entityNumber == gameLocal.localClientNum ) {

		// load HUD
		if ( gameLocal.isMultiplayer ) {
			// I need the HUD uniqued for the inventory code to interact with it.
			//hud = uiManager->FindGui( "guis/mphud.gui", true, false, true );
			hud = uiManager->FindGui( "guis/mphud.gui", true, true );
		} else if ( spawnArgs.GetString( "hud", "", temp ) ) {
			// I need the HUD uniqued for the inventory code to interact with it.
			//hud = uiManager->FindGui( temp, true, false, true );
			hud = uiManager->FindGui( temp, true, true );
		}
		if ( hud ) {
			hud->Activate( true, gameLocal.time );
		}

		// load cursor
		if ( spawnArgs.GetString( "cursor", "", temp ) ) {
			cursor = uiManager->FindGui( temp, true, gameLocal.isMultiplayer, gameLocal.isMultiplayer );
		}
		if ( cursor ) {
			cursor->Activate( true, gameLocal.time );
		}

		objectiveSystem = uiManager->FindGui( "guis/pda.gui", true, false, true );
		objectiveSystemOpen = false;
	}

	// Remove entity GUIs from our overlay system.
	for ( i = 0; i < MAX_RENDERENTITY_GUI; i++ )
		m_overlays.destroyOverlay( OVERLAYS_MIN_HANDLE + i );
	// Add the HUD.
	if ( m_overlays.createOverlay( 0, LAYER_MAIN_HUD ) >= OVERLAYS_MIN_HANDLE )
		m_overlays.setGui( OVERLAYS_MIN_HANDLE , hud );
	else
		gameLocal.Warning( "Unable to create overlay for HUD.\n" );


	SetLastHitTime( 0 );

	// load the armor sound feedback
	declManager->FindSound( "player_sounds_hitArmor" );

	// set up conditions for animation
	LinkScriptVariables();

	animator.RemoveOriginOffset( true );

	// initialize user info related settings
	// on server, we wait for the userinfo broadcast, as this controls when the player is initially spawned in game
	if ( gameLocal.isClient || entityNumber == gameLocal.localClientNum ) {
		UserInfoChanged(false);
	}

	// create combat collision hull for exact collision detection
	SetCombatModel();

	// init the damage effects
	playerView.SetPlayerEntity( this );

	// supress model in non-player views, but allow it in mirrors and remote views
	renderEntity.suppressSurfaceInViewID = entityNumber+1;

	// don't project shadow on self or weapon
	renderEntity.noSelfShadow = true;

	idAFAttachment *headEnt = head.GetEntity();
	if ( headEnt ) {
		headEnt->GetRenderEntity()->suppressSurfaceInViewID = entityNumber+1;
		headEnt->GetRenderEntity()->noSelfShadow = true;
	}

	if ( gameLocal.isMultiplayer )
	{
		Init();
		Hide();	// properly hidden if starting as a spectator
		if ( !gameLocal.isClient )
		{
			// set yourself ready to spawn. idMultiplayerGame will decide when/if appropriate and call SpawnFromSpawnSpot
			SetupWeaponEntity();
			SpawnFromSpawnSpot();
			forceRespawn = true;
			assert( spectating );
		}
	}
	else
	{
		SetupWeaponEntity();
		SpawnFromSpawnSpot();
	}

	// trigger playtesting item gives, if we didn't get here from a previous level
	// the devmap key will be set on the first devmap, but cleared on any level
	// transitions
	if ( !gameLocal.isMultiplayer && gameLocal.serverInfo.FindKey( "devmap" ) ) {
		// fire a trigger with the name "devmap"
		idEntity *ent = gameLocal.FindEntity( "devmap" );
		if ( ent ) {
			ent->ActivateTargets( this );
		}
	}

	if ( gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) )
	{
		hiddenWeapon = true;
		if ( weapon.GetEntity() )
			weapon.GetEntity()->LowerWeapon();
		idealWeapon = 0;
	}
	else
		hiddenWeapon = false;
	
	if ( hud )
	{
		UpdateHudWeapon();
		hud->StateChanged( gameLocal.time );
	}

	tipUp = false;
	objectiveUp = false;

	// See if any levelTriggers are pending. Trigger them now, if this is the case
	if ( levelTriggers.Num() )
	{
		PostEventMS( &EV_Player_LevelTrigger, 0 );
	}

	inventory.pdaOpened = false;
	inventory.selPDA = 0;

	// copy step volumes over from cvars
	UpdateMoveVolumes();

	if ( !gameLocal.isMultiplayer )
	{
		if ( g_skill.GetInteger() < 2 )
		{
			if ( health < 25 )
			{
				health = 25;
			}
			if ( g_useDynamicProtection.GetBool() )
			{
				g_damageScale.SetFloat( 1.0f );
			}
		}
		else
		{
			g_damageScale.SetFloat( 1.0f );
			g_armorProtection.SetFloat( ( g_skill.GetInteger() < 2 ) ? 0.4f : 0.2f );
#ifndef ID_DEMO_BUILD
			if ( g_skill.GetInteger() == 3 )
			{
				healthTake = true;
				nextHealthTake = gameLocal.time + g_healthTakeTime.GetInteger() * 1000;
			}
#endif
		}
	}

	//FIX: Set the walkspeed back to the stored value.
	pm_walkspeed.SetFloat( gameLocal.m_walkSpeed );
	SetupInventory();
}

CInventoryWeaponItem* idPlayer::getCurrentWeaponItem() {
	if (m_WeaponCursor == NULL) {
		return NULL;
	}

	return dynamic_cast<CInventoryWeaponItem*>(m_WeaponCursor->GetCurrentItem());
}

void idPlayer::addWeaponsToInventory() {

	for (unsigned int i = 0; i < MAX_WEAPONS; i++) {
		// Construct the spawnarg name
		idStr key(va("def_weapon%d", i));

		idStr weaponDef = spawnArgs.GetString(key.c_str());
		if (!weaponDef.IsEmpty()) {
			const idDict* entityDef = gameLocal.FindEntityDefDict(weaponDef.c_str());

			if (entityDef != NULL) {
				DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Adding weapon to inventory: %s\r", weaponDef.c_str());
				// Allocate a new weapon item using the found entityDef
				CInventoryWeaponItem* item = new CInventoryWeaponItem(*entityDef, weaponDef, this);
				
				item->setWeaponIndex(i);

				// Add it to the weapon category
				m_WeaponCursor->GetCurrentCategory()->PutItem(item);
			}
			else {
				DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Weapon entityDef not found: %s\r", weaponDef.c_str());
			}
		}
	}

	DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Total number of weapons found: %d\r", m_WeaponCursor->GetCurrentCategory()->size());
}

void idPlayer::SetupInventory()
{
	mInventoryOverlay = CreateOverlay(cv_tdm_inv_hud_file.GetString(), LAYER_INVENTORY);
	CInventory *inv = Inventory();
	int idx = 0;

	// We create a cursor and a category for the weapons, which is then locked
	// to this category, so we can only cycle within that one group.
	m_WeaponCursor = inv->CreateCursor();
	inv->CreateCategory(TDM_PLAYER_WEAPON_CATEGORY, &idx);
	m_WeaponCursor->SetCurrentCategory(idx);
	m_WeaponCursor->SetCategoryLock(true);

	// greebo: Parse the spawnargs and add the weapon items to the inventory.
	addWeaponsToInventory();

	// Now create the standard cursor for all the other inventory items (excl. weapons)
	CInventoryCursor *crsr = InventoryCursor();
	CInventoryItem *it;

	// We set the filter to ignore the weapon category, since this will be
	// handled by the weapon cursor. We don't want the weapons to show up
	// in the weapon slot AND in the inventory at the same time.
	crsr->SetCategoryIgnored(TDM_PLAYER_WEAPON_CATEGORY);

	// The player always gets a dumyyentry (so the player can have an empty space if he 
	// chooses to not see the inventory all the time.
	it = new CInventoryItem(this);
	it->SetName(TDM_DUMMY_ITEM);
	it->SetType(CInventoryItem::IT_DUMMY);
	it->SetCount(0);
	it->SetStackable(false);
	crsr->Inventory()->PutItem(it, TDM_INVENTORY_DEFAULT_GROUP);

	// And the player also always gets a loot entry, as he is supposed to find loot in
	// 99.99% of the maps. That's the point of the game, remember? :)
	idTypeInfo *cls = idClass::GetClass("idItem");
	idEntity *ent = static_cast<idEntity *>(cls->CreateInstance());
	ent->CallSpawn();
	ent->SetName(TDM_LOOT_INFO_ITEM);
	ent->spawnArgs.Set("scriptobject", TDM_LOOT_SCRIPTOBJECT);
	ent->scriptObject.SetType(TDM_LOOT_SCRIPTOBJECT);
	ent->ConstructScriptObject();

	it = new CInventoryItem(this);
	it->SetName(TDM_LOOT_INFO_ITEM);
	it->SetItemEntity(ent);
	it->SetType(CInventoryItem::IT_ITEM);
	it->SetOverlay(cv_tdm_inv_loot_hud.GetString(), CreateOverlay(cv_tdm_inv_loot_hud.GetString(), LAYER_INVENTORY));
	it->SetCount(0);
	it->SetStackable(false);
	crsr->Inventory()->PutItem(it, cv_tdm_inv_loot_group.GetString());

	// Focus on the empty dummy inventory item
	crsr->SetCurrentItem(TDM_DUMMY_ITEM);
}


/*
==============
idPlayer::~idPlayer()

Release any resources used by the player.
==============
*/
idPlayer::~idPlayer()
{
	delete weapon.GetEntity();
	weapon = NULL;
}

/*
===========
idPlayer::Save
===========
*/
void idPlayer::Save( idSaveGame *savefile ) const {
	int i;

	savefile->WriteUsercmd( usercmd );
	playerView.Save( savefile );

	savefile->WriteBool( noclip );
	savefile->WriteBool( godmode );

	// don't save spawnAnglesSet, since we'll have to reset them after loading the savegame
	savefile->WriteAngles( spawnAngles );
	savefile->WriteAngles( viewAngles );
	savefile->WriteAngles( cmdAngles );

	savefile->WriteInt( buttonMask );
	savefile->WriteInt( oldButtons );
	savefile->WriteInt( oldFlags );

	savefile->WriteInt( lastHitTime );
	savefile->WriteInt( lastSndHitTime );
	savefile->WriteInt( lastSavingThrowTime );

	// idBoolFields don't need to be saved, just re-linked in Restore

	inventory.Save( savefile );

	savefile->WriteInt( levelTriggers.Num() );
	for ( i = 0; i < levelTriggers.Num(); i++ ) {
		savefile->WriteString( levelTriggers[i].levelName );
		savefile->WriteString( levelTriggers[i].triggerName );
	}

	weapon.Save( savefile );

	savefile->WriteUserInterface( hud, false );
	savefile->WriteUserInterface( objectiveSystem, false );
	savefile->WriteBool( objectiveSystemOpen );

	savefile->WriteInt( weapon_soulcube );
	savefile->WriteInt( weapon_pda );
	savefile->WriteInt( weapon_fists );

	savefile->WriteInt( heartRate );

	savefile->WriteFloat( heartInfo.GetStartTime() );
	savefile->WriteFloat( heartInfo.GetDuration() );
	savefile->WriteFloat( heartInfo.GetStartValue() );
	savefile->WriteFloat( heartInfo.GetEndValue() );

	savefile->WriteInt( lastHeartAdjust );
	savefile->WriteInt( lastHeartBeat );
	savefile->WriteInt( lastDmgTime );
	savefile->WriteInt( deathClearContentsTime );
	savefile->WriteBool( doingDeathSkin );
	savefile->WriteInt( lastArmorPulse );
	savefile->WriteFloat( stamina );
	savefile->WriteFloat( healthPool );
	savefile->WriteInt( nextHealthPulse );
	savefile->WriteBool( healthPulse );
	savefile->WriteInt( nextHealthTake );
	savefile->WriteBool( healthTake );

	savefile->WriteBool( hiddenWeapon );
	soulCubeProjectile.Save( savefile );

	savefile->WriteInt( spectator );
	savefile->WriteVec3( colorBar );
	savefile->WriteInt( colorBarIndex );
	savefile->WriteBool( scoreBoardOpen );
	savefile->WriteBool( forceScoreBoard );
	savefile->WriteBool( forceRespawn );
	savefile->WriteBool( spectating );
	savefile->WriteInt( lastSpectateTeleport );
	savefile->WriteBool( lastHitToggle );
	savefile->WriteBool( forcedReady );
	savefile->WriteBool( wantSpectate );
	savefile->WriteBool( weaponGone );
	savefile->WriteBool( useInitialSpawns );
	savefile->WriteInt( latchedTeam );
	savefile->WriteInt( tourneyRank );
	savefile->WriteInt( tourneyLine );

	teleportEntity.Save( savefile );
	savefile->WriteInt( teleportKiller );

	savefile->WriteInt( minRespawnTime );
	savefile->WriteInt( maxRespawnTime );

	savefile->WriteVec3( firstPersonViewOrigin );
	savefile->WriteMat3( firstPersonViewAxis );

	// don't bother saving dragEntity since it's a dev tool

	savefile->WriteJoint( hipJoint );
	savefile->WriteJoint( chestJoint );
	savefile->WriteJoint( headJoint );

	savefile->WriteStaticObject( physicsObj );

	savefile->WriteInt( aasLocation.Num() );
	for( i = 0; i < aasLocation.Num(); i++ ) {
		savefile->WriteInt( aasLocation[ i ].areaNum );
		savefile->WriteVec3( aasLocation[ i ].pos );
	}

	savefile->WriteInt( bobFoot );
	savefile->WriteFloat( bobFrac );
	savefile->WriteFloat( bobfracsin );
	savefile->WriteInt( bobCycle );
	savefile->WriteFloat( xyspeed );
	savefile->WriteInt( stepUpTime );
	savefile->WriteFloat( stepUpDelta );
	savefile->WriteFloat( idealLegsYaw );
	savefile->WriteFloat( legsYaw );
	savefile->WriteBool( legsForward );
	savefile->WriteFloat( oldViewYaw );
	savefile->WriteAngles( viewBobAngles );
	savefile->WriteVec3( viewBob );
	savefile->WriteInt( landChange );
	savefile->WriteInt( landTime );

	savefile->WriteInt( currentWeapon );
	savefile->WriteInt( idealWeapon );
	savefile->WriteInt( previousWeapon );
	savefile->WriteInt( weaponSwitchTime );
	savefile->WriteBool( weaponEnabled );
	savefile->WriteBool( showWeaponViewModel );

	savefile->WriteSkin( skin );
	savefile->WriteSkin( powerUpSkin );
	savefile->WriteString( baseSkinName );

	savefile->WriteInt( numProjectilesFired );
	savefile->WriteInt( numProjectileHits );

	savefile->WriteBool( airless );
	savefile->WriteInt( airTics );
	savefile->WriteInt( lastAirDamage );

	savefile->WriteBool( gibDeath );
	savefile->WriteBool( gibsLaunched );
	savefile->WriteVec3( gibsDir );

	savefile->WriteFloat( zoomFov.GetStartTime() );
	savefile->WriteFloat( zoomFov.GetDuration() );
	savefile->WriteFloat( zoomFov.GetStartValue() );
	savefile->WriteFloat( zoomFov.GetEndValue() );

	savefile->WriteFloat( centerView.GetStartTime() );
	savefile->WriteFloat( centerView.GetDuration() );
	savefile->WriteFloat( centerView.GetStartValue() );
	savefile->WriteFloat( centerView.GetEndValue() );

	savefile->WriteBool( fxFov );

	savefile->WriteFloat( influenceFov );
	savefile->WriteInt( influenceActive );
	savefile->WriteFloat( influenceRadius );
	savefile->WriteObject( influenceEntity );
	savefile->WriteMaterial( influenceMaterial );
	savefile->WriteSkin( influenceSkin );

	savefile->WriteObject( privateCameraView );
	savefile->WriteVec3( m_ListenerLoc );
	savefile->WriteVec3( m_DoorListenLoc );

	savefile->WriteDict( &m_immobilization );
	savefile->WriteInt( m_immobilizationCache );

	savefile->WriteDict( &m_hinderance );
	savefile->WriteFloat( m_hinderanceCache );

	for( i = 0; i < NUM_LOGGED_VIEW_ANGLES; i++ ) {
		savefile->WriteAngles( loggedViewAngles[ i ] );
	}
	for( i = 0; i < NUM_LOGGED_ACCELS; i++ ) {
		savefile->WriteInt( loggedAccel[ i ].time );
		savefile->WriteVec3( loggedAccel[ i ].dir );
	}
	savefile->WriteInt( currentLoggedAccel );

	savefile->WriteObject( focusGUIent );
	// can't save focusUI
	savefile->WriteObject( focusCharacter );
	savefile->WriteInt( talkCursor );
	savefile->WriteInt( focusTime );
	savefile->WriteObject( focusVehicle );
	savefile->WriteUserInterface( cursor, false );

	savefile->WriteInt( oldMouseX );
	savefile->WriteInt( oldMouseY );

	savefile->WriteString( pdaAudio );
	savefile->WriteString( pdaVideo );
	savefile->WriteString( pdaVideoWave );

	savefile->WriteBool( tipUp );
	savefile->WriteBool( objectiveUp );

	savefile->WriteInt( lastDamageDef );
	savefile->WriteVec3( lastDamageDir );
	savefile->WriteInt( lastDamageLocation );
	savefile->WriteInt( smoothedFrame );
	savefile->WriteBool( smoothedOriginUpdated );
	savefile->WriteVec3( smoothedOrigin );
	savefile->WriteAngles( smoothedAngles );

	savefile->WriteBool( ready );
	savefile->WriteBool( respawning );
	savefile->WriteBool( leader );
	savefile->WriteInt( lastSpectateChange );
	savefile->WriteInt( lastTeleFX );

	savefile->WriteFloat( pm_stamina.GetFloat() );

	savefile->WriteBool( m_bGrabberActive );
	savefile->WriteBool( m_bDraggingBody );
	savefile->WriteBool( m_bShoulderingBody );

	if(hud)
	{
		hud->SetStateString( "message", common->GetLanguageDict()->GetString( "#str_02916" ) );
		hud->HandleNamedEvent( "Message" );
	}
}

/*
===========
idPlayer::Restore
===========
*/
void idPlayer::Restore( idRestoreGame *savefile ) {
	int	  i;
	int	  num;
	float set;

	savefile->ReadUsercmd( usercmd );
	playerView.Restore( savefile );

	savefile->ReadBool( noclip );
	savefile->ReadBool( godmode );

	savefile->ReadAngles( spawnAngles );
	savefile->ReadAngles( viewAngles );
	savefile->ReadAngles( cmdAngles );

	memset( usercmd.angles, 0, sizeof( usercmd.angles ) );
	SetViewAngles( viewAngles );
	spawnAnglesSet = true;

	savefile->ReadInt( buttonMask );
	savefile->ReadInt( oldButtons );
	savefile->ReadInt( oldFlags );

	usercmd.flags = 0;
	oldFlags = 0;

	savefile->ReadInt( lastHitTime );
	savefile->ReadInt( lastSndHitTime );
	savefile->ReadInt( lastSavingThrowTime );

	// Re-link idBoolFields to the scriptObject, values will be restored in scriptObject's restore
	LinkScriptVariables();

	inventory.Restore( savefile );

	savefile->ReadInt( num );
	for ( i = 0; i < num; i++ ) {
		idLevelTriggerInfo lti;
		savefile->ReadString( lti.levelName );
		savefile->ReadString( lti.triggerName );
		levelTriggers.Append( lti );
	}

	weapon.Restore( savefile );

	for ( i = 0; i < inventory.emails.Num(); i++ ) {
		GetPDA()->AddEmail( inventory.emails[i] );
	}

	savefile->ReadUserInterface( hud );
	savefile->ReadUserInterface( objectiveSystem );
	savefile->ReadBool( objectiveSystemOpen );

	savefile->ReadInt( weapon_soulcube );
	savefile->ReadInt( weapon_pda );
	savefile->ReadInt( weapon_fists );

	savefile->ReadInt( heartRate );

	savefile->ReadFloat( set );
	heartInfo.SetStartTime( set );
	savefile->ReadFloat( set );
	heartInfo.SetDuration( set );
	savefile->ReadFloat( set );
	heartInfo.SetStartValue( set );
	savefile->ReadFloat( set );
	heartInfo.SetEndValue( set );

	savefile->ReadInt( lastHeartAdjust );
	savefile->ReadInt( lastHeartBeat );
	savefile->ReadInt( lastDmgTime );
	savefile->ReadInt( deathClearContentsTime );
	savefile->ReadBool( doingDeathSkin );
	savefile->ReadInt( lastArmorPulse );
	savefile->ReadFloat( stamina );
	savefile->ReadFloat( healthPool );
	savefile->ReadInt( nextHealthPulse );
	savefile->ReadBool( healthPulse );
	savefile->ReadInt( nextHealthTake );
	savefile->ReadBool( healthTake );

	savefile->ReadBool( hiddenWeapon );
	soulCubeProjectile.Restore( savefile );

	savefile->ReadInt( spectator );
	savefile->ReadVec3( colorBar );
	savefile->ReadInt( colorBarIndex );
	savefile->ReadBool( scoreBoardOpen );
	savefile->ReadBool( forceScoreBoard );
	savefile->ReadBool( forceRespawn );
	savefile->ReadBool( spectating );
	savefile->ReadInt( lastSpectateTeleport );
	savefile->ReadBool( lastHitToggle );
	savefile->ReadBool( forcedReady );
	savefile->ReadBool( wantSpectate );
	savefile->ReadBool( weaponGone );
	savefile->ReadBool( useInitialSpawns );
	savefile->ReadInt( latchedTeam );
	savefile->ReadInt( tourneyRank );
	savefile->ReadInt( tourneyLine );

	teleportEntity.Restore( savefile );
	savefile->ReadInt( teleportKiller );

	savefile->ReadInt( minRespawnTime );
	savefile->ReadInt( maxRespawnTime );

	savefile->ReadVec3( firstPersonViewOrigin );
	savefile->ReadMat3( firstPersonViewAxis );

	// don't bother saving dragEntity since it's a dev tool
	dragEntity.Clear();

	savefile->ReadJoint( hipJoint );
	savefile->ReadJoint( chestJoint );
	savefile->ReadJoint( headJoint );

	savefile->ReadStaticObject( physicsObj );
	RestorePhysics( &physicsObj );

	savefile->ReadInt( num );
	aasLocation.SetGranularity( 1 );
	aasLocation.SetNum( num );
	for( i = 0; i < num; i++ ) {
		savefile->ReadInt( aasLocation[ i ].areaNum );
		savefile->ReadVec3( aasLocation[ i ].pos );
	}

	savefile->ReadInt( bobFoot );
	savefile->ReadFloat( bobFrac );
	savefile->ReadFloat( bobfracsin );
	savefile->ReadInt( bobCycle );
	savefile->ReadFloat( xyspeed );
	savefile->ReadInt( stepUpTime );
	savefile->ReadFloat( stepUpDelta );
	savefile->ReadFloat( idealLegsYaw );
	savefile->ReadFloat( legsYaw );
	savefile->ReadBool( legsForward );
	savefile->ReadFloat( oldViewYaw );
	savefile->ReadAngles( viewBobAngles );
	savefile->ReadVec3( viewBob );
	savefile->ReadInt( landChange );
	savefile->ReadInt( landTime );

	savefile->ReadInt( currentWeapon );
	savefile->ReadInt( idealWeapon );
	savefile->ReadInt( previousWeapon );
	savefile->ReadInt( weaponSwitchTime );
	savefile->ReadBool( weaponEnabled );
	savefile->ReadBool( showWeaponViewModel );

	savefile->ReadSkin( skin );
	savefile->ReadSkin( powerUpSkin );
	savefile->ReadString( baseSkinName );

	savefile->ReadInt( numProjectilesFired );
	savefile->ReadInt( numProjectileHits );

	savefile->ReadBool( airless );
	savefile->ReadInt( airTics );
	savefile->ReadInt( lastAirDamage );

	savefile->ReadBool( gibDeath );
	savefile->ReadBool( gibsLaunched );
	savefile->ReadVec3( gibsDir );

	savefile->ReadFloat( set );
	zoomFov.SetStartTime( set );
	savefile->ReadFloat( set );
	zoomFov.SetDuration( set );
	savefile->ReadFloat( set );
	zoomFov.SetStartValue( set );
	savefile->ReadFloat( set );
	zoomFov.SetEndValue( set );

	savefile->ReadFloat( set );
	centerView.SetStartTime( set );
	savefile->ReadFloat( set );
	centerView.SetDuration( set );
	savefile->ReadFloat( set );
	centerView.SetStartValue( set );
	savefile->ReadFloat( set );
	centerView.SetEndValue( set );

	savefile->ReadBool( fxFov );

	savefile->ReadFloat( influenceFov );
	savefile->ReadInt( influenceActive );
	savefile->ReadFloat( influenceRadius );
	savefile->ReadObject( reinterpret_cast<idClass *&>( influenceEntity ) );
	savefile->ReadMaterial( influenceMaterial );
	savefile->ReadSkin( influenceSkin );

	savefile->ReadObject( reinterpret_cast<idClass *&>( privateCameraView ) );
	savefile->ReadVec3( m_ListenerLoc );
	savefile->ReadVec3( m_DoorListenLoc );

	savefile->ReadDict( &m_immobilization );
	savefile->ReadInt( m_immobilizationCache );

	savefile->ReadDict( &m_hinderance );
	savefile->ReadFloat( m_hinderanceCache );

	for( i = 0; i < NUM_LOGGED_VIEW_ANGLES; i++ ) {
		savefile->ReadAngles( loggedViewAngles[ i ] );
	}
	for( i = 0; i < NUM_LOGGED_ACCELS; i++ ) {
		savefile->ReadInt( loggedAccel[ i ].time );
		savefile->ReadVec3( loggedAccel[ i ].dir );
	}
	savefile->ReadInt( currentLoggedAccel );

	savefile->ReadObject( reinterpret_cast<idClass *&>( focusGUIent ) );
	// can't save focusUI
	focusUI = NULL;
	savefile->ReadObject( reinterpret_cast<idClass *&>( focusCharacter ) );
	savefile->ReadInt( talkCursor );
	savefile->ReadInt( focusTime );
	savefile->ReadObject( reinterpret_cast<idClass *&>( focusVehicle ) );
	savefile->ReadUserInterface( cursor );

	savefile->ReadInt( oldMouseX );
	savefile->ReadInt( oldMouseY );

	savefile->ReadString( pdaAudio );
	savefile->ReadString( pdaVideo );
	savefile->ReadString( pdaVideoWave );

	savefile->ReadBool( tipUp );
	savefile->ReadBool( objectiveUp );

	savefile->ReadInt( lastDamageDef );
	savefile->ReadVec3( lastDamageDir );
	savefile->ReadInt( lastDamageLocation );
	savefile->ReadInt( smoothedFrame );
	savefile->ReadBool( smoothedOriginUpdated );
	savefile->ReadVec3( smoothedOrigin );
	savefile->ReadAngles( smoothedAngles );

	savefile->ReadBool( ready );
	savefile->ReadBool( respawning );
	savefile->ReadBool( leader );
	savefile->ReadInt( lastSpectateChange );
	savefile->ReadInt( lastTeleFX );

	// set the pm_ cvars
	const idKeyValue	*kv;
	kv = spawnArgs.MatchPrefix( "pm_", NULL );
	while( kv ) {
		cvarSystem->SetCVarString( kv->GetKey(), kv->GetValue() );
		kv = spawnArgs.MatchPrefix( "pm_", kv );
	}

	savefile->ReadFloat( set );
	pm_stamina.SetFloat( set );

	savefile->ReadBool( m_bGrabberActive );
	savefile->ReadBool( m_bDraggingBody );
	savefile->ReadBool( m_bShoulderingBody );

	// create combat collision hull for exact collision detection
	SetCombatModel();

	// Necessary, since the overlay system can't save external GUI pointers.
	if ( m_overlays.isExternal( OVERLAYS_MIN_HANDLE  ) )
		m_overlays.setGui( OVERLAYS_MIN_HANDLE, hud );
	else
		gameLocal.Warning( "Unable to relink HUD to overlay system.\n" );
}

/*
===============
idPlayer::PrepareForRestart
================
*/
void idPlayer::PrepareForRestart( void ) {
	Spectate( true );
	forceRespawn = true;
	
	// we will be restarting program, clear the client entities from program-related things first
	ShutdownThreads();

	// the sound world is going to be cleared, don't keep references to emitters
	FreeSoundEmitter( false );
}

/*
===============
idPlayer::Restart
================
*/
void idPlayer::Restart( void ) {
	idActor::Restart();
	
	// client needs to setup the animation script object again
	if ( gameLocal.isClient ) {
		Init();
	} else {
		// choose a random spot and prepare the point of view in case player is left spectating
		assert( spectating );
		SpawnFromSpawnSpot();
	}

	useInitialSpawns = true;
	UpdateSkinSetup( true );
}

/*
===============
idPlayer::ServerSpectate
================
*/
void idPlayer::ServerSpectate( bool spectate ) {
	assert( !gameLocal.isClient );

	if ( spectating != spectate ) {
		Spectate( spectate );
		if ( spectate ) {
			SetSpectateOrigin();
		} else {
			if ( gameLocal.gameType == GAME_DM ) {
				// make sure the scores are reset so you can't exploit by spectating and entering the game back
				// other game types don't matter, as you either can't join back, or it's team scores
				gameLocal.mpGame.ClearFrags( entityNumber );
			}
		}
	}
	if ( !spectate ) {
		SpawnFromSpawnSpot();
	}
}

/*
===========
idPlayer::SelectInitialSpawnPoint

Try to find a spawn point marked 'initial', otherwise
use normal spawn selection.
============
*/
void idPlayer::SelectInitialSpawnPoint( idVec3 &origin, idAngles &angles ) {
	idEntity *spot;
	idStr skin;

	spot = gameLocal.SelectInitialSpawnPoint( this );

	// set the player skin from the spawn location
	if ( spot->spawnArgs.GetString( "skin", NULL, skin ) ) {
		spawnArgs.Set( "spawn_skin", skin );
	}

	// activate the spawn locations targets
	spot->PostEventMS( &EV_ActivateTargets, 0, this );

	origin = spot->GetPhysics()->GetOrigin();
	origin[2] += 4.0f + CM_BOX_EPSILON;		// move up to make sure the player is at least an epsilon above the floor
	angles = spot->GetPhysics()->GetAxis().ToAngles();
}

/*
===========
idPlayer::SpawnFromSpawnSpot

Chooses a spawn location and spawns the player
============
*/
void idPlayer::SpawnFromSpawnSpot( void ) {
	idVec3		spawn_origin;
	idAngles	spawn_angles;
	
	SelectInitialSpawnPoint( spawn_origin, spawn_angles );
	SpawnToPoint( spawn_origin, spawn_angles );
}

/*
===========
idPlayer::SpawnToPoint

Called every time a client is placed fresh in the world:
after the first ClientBegin, and after each respawn
Initializes all non-persistant parts of playerState

when called here with spectating set to true, just place yourself and init
============
*/
void idPlayer::SpawnToPoint( const idVec3 &spawn_origin, const idAngles &spawn_angles ) {
	idVec3 spec_origin;

	assert( !gameLocal.isClient );

	respawning = true;

	Init();

	fl.noknockback = false;

	// stop any ragdolls being used
	StopRagdoll();

	// set back the player physics
	SetPhysics( &physicsObj );

	physicsObj.SetClipModelAxis();
	physicsObj.EnableClip();

	if ( !spectating ) {
		SetCombatContents( true );
	}

	physicsObj.SetLinearVelocity( vec3_origin );

	// setup our initial view
	if ( !spectating ) {
		SetOrigin( spawn_origin );
	} else {
		spec_origin = spawn_origin;
		spec_origin[ 2 ] += pm_normalheight.GetFloat();
		spec_origin[ 2 ] += SPECTATE_RAISE;
		SetOrigin( spec_origin );
	}

	// if this is the first spawn of the map, we don't have a usercmd yet,
	// so the delta angles won't be correct.  This will be fixed on the first think.
	viewAngles = ang_zero;
	SetDeltaViewAngles( ang_zero );
	SetViewAngles( spawn_angles );
	spawnAngles = spawn_angles;
	spawnAnglesSet = false;

	legsForward = true;
	legsYaw = 0.0f;
	idealLegsYaw = 0.0f;
	oldViewYaw = viewAngles.yaw;

	if ( spectating ) {
		Hide();
	} else {
		Show();
	}

	if ( gameLocal.isMultiplayer ) {
		if ( !spectating ) {
			// we may be called twice in a row in some situations. avoid a double fx and 'fly to the roof'
			if ( lastTeleFX < gameLocal.time - 1000 ) {
				idEntityFx::StartFx( spawnArgs.GetString( "fx_spawn" ), &spawn_origin, NULL, this, true );
				lastTeleFX = gameLocal.time;
			}
		}
		AI_TELEPORT = true;
	} else {
		AI_TELEPORT = false;
	}

	// kill anything at the new position
	if ( !spectating ) {
		physicsObj.SetClipMask( MASK_PLAYERSOLID ); // the clip mask is usually maintained in Move(), but KillBox requires it
		gameLocal.KillBox( this );
	}

	// don't allow full run speed for a bit
	physicsObj.SetKnockBack( 100 );

	// set our respawn time and buttons so that if we're killed we don't respawn immediately
	minRespawnTime = gameLocal.time;
	maxRespawnTime = gameLocal.time;
	if ( !spectating ) {
		forceRespawn = false;
	}

	privateCameraView = NULL;

	BecomeActive( TH_THINK );

	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	Think();

	respawning			= false;
	lastManOver			= false;
	lastManPlayAgain	= false;
	isTelefragged		= false;
}

/*
===============
idPlayer::SavePersistantInfo

Saves any inventory and player stats when changing levels.
===============
*/
void idPlayer::SavePersistantInfo( void ) {
	idDict &playerInfo = gameLocal.persistentPlayerInfo[entityNumber];

	playerInfo.Clear();
	inventory.GetPersistantData( playerInfo );
	playerInfo.SetInt( "health", health );
	playerInfo.SetInt( "current_weapon", currentWeapon );
}

/*
===============
idPlayer::RestorePersistantInfo

Restores any inventory and player stats when changing levels.
===============
*/
void idPlayer::RestorePersistantInfo( void ) {
	if ( gameLocal.isMultiplayer ) {
		gameLocal.persistentPlayerInfo[entityNumber].Clear();
	}

	spawnArgs.Copy( gameLocal.persistentPlayerInfo[entityNumber] );

	inventory.RestoreInventory( this, spawnArgs );
	health = spawnArgs.GetInt( "health", "100" );
	if ( !gameLocal.isClient ) {
		idealWeapon = spawnArgs.GetInt( "current_weapon", "1" );
	}
}

/*
================
idPlayer::GetUserInfo
================
*/
idDict *idPlayer::GetUserInfo( void ) {
	return &gameLocal.userInfo[ entityNumber ];
}

/*
==============
idPlayer::UpdateSkinSetup
==============
*/
void idPlayer::UpdateSkinSetup( bool restart ) {
	if ( restart ) {
		team = ( idStr::Icmp( GetUserInfo()->GetString( "ui_team" ), "Blue" ) == 0 );
	}
	if ( gameLocal.gameType == GAME_TDM ) {
		if ( team ) {
			baseSkinName = "skins/characters/player/marine_mp_blue";
		} else {
			baseSkinName = "skins/characters/player/marine_mp_red";
		}
		if ( !gameLocal.isClient && team != latchedTeam ) {
			gameLocal.mpGame.SwitchToTeam( entityNumber, latchedTeam, team );
		}
		latchedTeam = team;
	} else {
		baseSkinName = GetUserInfo()->GetString( "ui_skin" );
	}
	if ( !baseSkinName.Length() ) {
		baseSkinName = "skins/characters/player/marine_mp";
	}
	skin = declManager->FindSkin( baseSkinName, false );
	assert( skin );
	// match the skin to a color band for scoreboard
	if ( baseSkinName.Find( "red" ) != -1 ) {
		colorBarIndex = 1;
	} else if ( baseSkinName.Find( "green" ) != -1 ) {
		colorBarIndex = 2;
	} else if ( baseSkinName.Find( "blue" ) != -1 ) {
		colorBarIndex = 3;
	} else if ( baseSkinName.Find( "yellow" ) != -1 ) {
		colorBarIndex = 4;
	} else {
		colorBarIndex = 0;
	}
	colorBar = colorBarTable[ colorBarIndex ];
}

/*
==============
idPlayer::BalanceTDM
==============
*/
bool idPlayer::BalanceTDM( void ) {
	int			i, balanceTeam, teamCount[2];
	idEntity	*ent;

	teamCount[ 0 ] = teamCount[ 1 ] = 0;
	for( i = 0; i < gameLocal.numClients; i++ ) {
		ent = gameLocal.entities[ i ];
		if ( ent && ent->IsType( idPlayer::Type ) ) {
			teamCount[ static_cast< idPlayer * >( ent )->team ]++;
		}
	}
	balanceTeam = -1;
	if ( teamCount[ 0 ] < teamCount[ 1 ] ) {
		balanceTeam = 0;
	} else if ( teamCount[ 0 ] > teamCount[ 1 ] ) {
		balanceTeam = 1;
	}
	if ( balanceTeam != -1 && team != balanceTeam ) {
		common->DPrintf( "team balance: forcing player %d to %s team\n", entityNumber, balanceTeam ? "blue" : "red" );
		team = balanceTeam;
		GetUserInfo()->Set( "ui_team", team ? "Blue" : "Red" );
		return true;
	}
	return false;
}

/*
==============
idPlayer::UserInfoChanged
==============
*/
bool idPlayer::UserInfoChanged( bool canModify ) {
	idDict	*userInfo;
	bool	modifiedInfo;
	bool	spec;
	bool	newready;

	userInfo = GetUserInfo();
	showWeaponViewModel = userInfo->GetBool( "ui_showGun" );

	if ( !gameLocal.isMultiplayer ) {
		return false;
	}

	modifiedInfo = false;

	spec = ( idStr::Icmp( userInfo->GetString( "ui_spectate" ), "Spectate" ) == 0 );
	if ( gameLocal.serverInfo.GetBool( "si_spectators" ) ) {
		// never let spectators go back to game while sudden death is on
		if ( canModify && gameLocal.mpGame.GetGameState() == idMultiplayerGame::SUDDENDEATH && !spec && wantSpectate == true ) {

			userInfo->Set( "ui_spectate", "Spectate" );
			modifiedInfo |= true;
		} else {
			if ( spec != wantSpectate && !spec ) {
				// returning from spectate, set forceRespawn so we don't get stuck in spectate forever
				forceRespawn = true;
			}
			wantSpectate = spec;
		}
	} else {
		if ( canModify && spec ) {

			userInfo->Set( "ui_spectate", "Play" );
			modifiedInfo |= true;
		} else if ( spectating ) {  
			// allow player to leaving spectator mode if they were in it when si_spectators got turned off

			forceRespawn = true;
		}
		wantSpectate = false;
	}
	newready = ( idStr::Icmp( userInfo->GetString( "ui_ready" ), "Ready" ) == 0 );
	if ( ready != newready && gameLocal.mpGame.GetGameState() == idMultiplayerGame::WARMUP && !wantSpectate ) {
		gameLocal.mpGame.AddChatLine( common->GetLanguageDict()->GetString( "#str_07180" ), userInfo->GetString( "ui_name" ), newready ? common->GetLanguageDict()->GetString( "#str_04300" ) : common->GetLanguageDict()->GetString( "#str_04301" ) );
	}
	ready = newready;
	team = ( idStr::Icmp( userInfo->GetString( "ui_team" ), "Blue" ) == 0 );
	// server maintains TDM balance
	if ( canModify && gameLocal.gameType == GAME_TDM && !gameLocal.mpGame.IsInGame( entityNumber ) && g_balanceTDM.GetBool() ) {

		modifiedInfo |= BalanceTDM( );
	}
	UpdateSkinSetup( false );
	
	isChatting = userInfo->GetBool( "ui_chat", "0" );
	if ( canModify && isChatting && AI_DEAD ) {

		// if dead, always force chat icon off.
		isChatting = false;
		userInfo->SetBool( "ui_chat", false );
		modifiedInfo |= true;
	}

	return modifiedInfo;
}

/*
===============
idPlayer::UpdateHudAmmo
===============
*/
void idPlayer::UpdateHudAmmo( idUserInterface *_hud ) {
	CInventoryWeaponItem* item = getCurrentWeaponItem();

	if (_hud != NULL && item != NULL) {
		int ammoAmount = item->getAmmo();

		// Hide ammo display for ammo == -1 and weapons without ammo
		_hud->SetStateBool("ammo_visible", !(item->allowedEmpty() || ammoAmount < 0));
		_hud->SetStateString("ammo_amount", va("%d", ammoAmount));
	}
}

/*
===============
idPlayer::UpdateHudStats
===============
*/
void idPlayer::UpdateHudStats( idUserInterface *_hud )
{
	int staminapercentage;
	float max_stamina;

	assert( _hud );

	max_stamina = pm_stamina.GetFloat();
	if ( !max_stamina ) {
		// stamina disabled, so show full stamina bar
		staminapercentage = 100.0f;
	} else {
		staminapercentage = idMath::FtoiFast( 100.0f * stamina / max_stamina );
	}

	_hud->SetStateInt( "player_health", health );
	_hud->SetStateInt( "player_stamina", staminapercentage );
	_hud->SetStateInt( "player_shadow", 1 );

	_hud->SetStateInt( "player_hr", heartRate );
	_hud->SetStateInt( "player_nostamina", ( max_stamina == 0 ) ? 1 : 0 );

	_hud->HandleNamedEvent( "updateArmorHealthAir" );

	if ( healthPulse ) {
		_hud->HandleNamedEvent( "healthPulse" );
		StartSound( "snd_healthpulse", SND_CHANNEL_ITEM, 0, false, NULL );
		healthPulse = false;
	}

	if ( healthTake ) {
		_hud->HandleNamedEvent( "healthPulse" );
		StartSound( "snd_healthtake", SND_CHANNEL_ITEM, 0, false, NULL );
		healthTake = false;
	}

	UpdateHudAmmo( _hud );

	// TODO: This is only needed in case fade in/out is still running.
	//inventoryChangeSelection( _hud );
}

/*
===============
idPlayer::UpdateHudWeapon
===============
*/
void idPlayer::UpdateHudWeapon( bool flashWeapon ) {
	idUserInterface *hud = idPlayer::hud;

	// if updating the hud of a followed client
	if ( gameLocal.localClientNum >= 0 && gameLocal.entities[ gameLocal.localClientNum ] && gameLocal.entities[ gameLocal.localClientNum ]->IsType( idPlayer::Type ) ) {
		idPlayer *p = static_cast< idPlayer * >( gameLocal.entities[ gameLocal.localClientNum ] );
		if ( p->spectating && p->spectator == entityNumber ) {
			assert( p->hud );
			hud = p->hud;
		}
	}

	if (hud == NULL || m_WeaponCursor == NULL) {
		return;
	}

	int curWeaponIndex = m_WeaponCursor->GetCurrentItemIndex();

	for (int i = 0; i < MAX_WEAPONS; i++) {
		// Determine the weapon state (2 for selected, 0 for not selected)
		int weapstate = (i == curWeaponIndex) ? 2 : 0;
		
		// Construct the HUD weapon string
		hud->SetStateInt(va("weapon%d", i), weapstate);
	}

	if ( flashWeapon ) {
		hud->HandleNamedEvent( "weaponChange" );
	}
}

void idPlayer::PrintDebugHUD(void)
{
	idStr strText;
	idVec3 a, b, c;
	int y;

	// TODO: Remove this when no longer needed.
	y = 100;
	sprintf(strText, "ViewOrg:    x: %f   y: %f   z: %f", renderView->vieworg.x, renderView->vieworg.y, renderView->vieworg.z);
	renderSystem->DrawSmallStringExt(1, y, strText.c_str( ), idVec4( 1, 1, 1, 1 ), false, declManager->FindMaterial( "textures/bigchars" ));
	y += 12;
	renderView->viewaxis.GetMat3Params(a, b, c);
	sprintf(strText, "ViewMatrix:", renderView->vieworg.x, renderView->vieworg.y, renderView->vieworg.z);
	renderSystem->DrawSmallStringExt(1, y, strText.c_str( ), idVec4( 1, 1, 1, 1 ), false, declManager->FindMaterial( "textures/bigchars" ));
	y += 12;
	sprintf(strText, "x: %f   y: %f   z: %f", a.x, a.y, a.z);
	renderSystem->DrawSmallStringExt(1, y, strText.c_str( ), idVec4( 1, 1, 1, 1 ), false, declManager->FindMaterial( "textures/bigchars" ));
	y += 12;
	sprintf(strText, "x: %f   y: %f   z: %f", b.x, b.y, b.z);
	renderSystem->DrawSmallStringExt(1, y, strText.c_str( ), idVec4( 1, 1, 1, 1 ), false, declManager->FindMaterial( "textures/bigchars" ));
	y += 12;
	sprintf(strText, "x: %f   y: %f   z: %f", c.x, c.y, c.z);
	renderSystem->DrawSmallStringExt(1, y, strText.c_str( ), idVec4( 1, 1, 1, 1 ), false, declManager->FindMaterial( "textures/bigchars" ));
	y += 12;
	sprintf(strText, "FOV x: %f   y: %f", renderView->fov_x, renderView->fov_y);
	renderSystem->DrawSmallStringExt(1, y, strText.c_str( ), idVec4( 1, 1, 1, 1 ), false, declManager->FindMaterial( "textures/bigchars" ));
	y += 12;
	sprintf(strText, "ViewAngles Pitch: %f   Yaw: %f   Roll: %f", viewAngles.pitch, viewAngles.yaw, viewAngles.roll);
	renderSystem->DrawSmallStringExt(1, y, strText.c_str( ), idVec4( 1, 1, 1, 1 ), false, declManager->FindMaterial( "textures/bigchars" ));
	y += 12;
	sprintf(strText, "ViewBobAngles Pitch: %f   Yaw: %f   Roll: %f", viewBobAngles.pitch, viewBobAngles.yaw, viewBobAngles.roll);
	renderSystem->DrawSmallStringExt(1, y, strText.c_str( ), idVec4( 1, 1, 1, 1 ), false, declManager->FindMaterial( "textures/bigchars" ));
	y += 12;
	a = GetEyePosition();
	sprintf(strText, "EyePosition x: %f   y: %f   z: %f", a.x, a.y, a.z);
	renderSystem->DrawSmallStringExt(1, y, strText.c_str( ), idVec4( 1, 1, 1, 1 ), false, declManager->FindMaterial( "textures/bigchars" ));
}

/*
===============
idPlayer::DrawHUD
===============
*/
void idPlayer::DrawHUD(idUserInterface *_hud)
{
//	if(cv_lg_debug.GetInteger() != 0)
//		PrintDebugHUD();

	const char *name;
	if((name = cv_dm_distance.GetString()) != NULL)
	{
		idEntity *e;

		if((e = gameLocal.FindEntity(name)) != NULL)
		{
			idStr strText;
			float d;
			int y;

			d = (GetPhysics()->GetOrigin() - e->GetPhysics()->GetOrigin()).Length();

			y = 100;
			sprintf(strText, "Entity [%s]   distance: %f", name, d);
			renderSystem->DrawSmallStringExt(1, y, strText.c_str( ), idVec4( 1, 1, 1, 1 ), false, declManager->FindMaterial( "textures/bigchars" ));
		}
	}

	if(_hud)
		DM_LOG(LC_SYSTEM, LT_INFO)LOGSTRING("PlayerHUD: [%s]\r", (_hud->Name() == NULL)?"null":_hud->Name());
	else
		DM_LOG(LC_SYSTEM, LT_INFO)LOGSTRING("PlayerHUD: NULL\r");

	if ( !weapon.GetEntity() || influenceActive != INFLUENCE_NONE || privateCameraView || gameLocal.GetCamera() || !_hud || !g_showHud.GetBool() ) {
		return;
	}

	UpdateHudStats( _hud );

	_hud->SetStateString( "weapicon", weapon.GetEntity()->Icon() );

	// FIXME: this is temp to allow the sound meter to show up in the hud
	// it should be commented out before shipping but the code can remain
	// for mod developers to enable for the same functionality
	_hud->SetStateInt( "s_debug", cvarSystem->GetCVarInteger( "s_showLevelMeter" ) );

	weapon.GetEntity()->UpdateGUI();

	//_hud->Redraw( gameLocal.realClientTime );
	m_overlays.drawOverlays();

	// weapon targeting crosshair

	if ( !GuiActive() ) {

		if ( cursor && weapon.GetEntity()->ShowCrosshair() ) {

			cursor->Redraw( gameLocal.realClientTime );

		}

	}


	// Only use this if the old lightgem is selected. This may be usefull for
	// slower machines.
	if(cv_lg_weak.GetBool() == true)
		AdjustLightgem();

	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Setting Lightgemvalue: %u on hud: %08lX\r\r", g_Global.m_DarkModPlayer->m_LightgemValue, hud);
	hud->SetStateInt("lightgem_val", g_Global.m_DarkModPlayer->m_LightgemValue);
}

/*
===============
idPlayer::EnterCinematic
===============
*/
void idPlayer::EnterCinematic( void ) {
	Hide();
	StopAudioLog();
	StopSound( SND_CHANNEL_PDA, false );
	if ( hud ) {
		hud->HandleNamedEvent( "radioChatterDown" );
	}
	
	physicsObj.SetLinearVelocity( vec3_origin );
	
	SetState( "EnterCinematic" );
	UpdateScript();

	if ( weaponEnabled && weapon.GetEntity() ) {
		weapon.GetEntity()->EnterCinematic();
	}

	AI_FORWARD		= false;
	AI_BACKWARD		= false;
	AI_STRAFE_LEFT	= false;
	AI_STRAFE_RIGHT	= false;
	AI_RUN			= false;
	AI_ATTACK_HELD	= false;
	AI_WEAPON_FIRED	= false;
	AI_JUMP			= false;
	AI_CROUCH		= false;
	AI_ONGROUND		= true;
	AI_ONLADDER		= false;
	AI_DEAD			= ( health <= 0 );
	AI_RUN			= false;
	AI_PAIN			= false;
	AI_HARDLANDING	= false;
	AI_SOFTLANDING	= false;
	AI_RELOAD		= false;
	AI_TELEPORT		= false;
	AI_TURN_LEFT	= false;
	AI_TURN_RIGHT	= false;

	AI_LEAN_LEFT	= false;
	AI_LEAN_RIGHT	= false;
	AI_LEAN_FORWARD	= false;

	AI_CREEP		= false;
}

/*
===============
idPlayer::ExitCinematic
===============
*/
void idPlayer::ExitCinematic( void ) {
	Show();

	if ( weaponEnabled && weapon.GetEntity() ) {
		weapon.GetEntity()->ExitCinematic();
	}

	SetState( "ExitCinematic" );
	UpdateScript();
}

/*
=====================
idPlayer::UpdateConditions
=====================
*/
void idPlayer::UpdateConditions( void )
{
	idVec3	velocity;
	float	fallspeed;
	float	forwardspeed;
	float	sidespeed;

	// minus the push velocity to avoid playing the walking animation and sounds when riding a mover
	velocity = physicsObj.GetLinearVelocity() - physicsObj.GetPushedLinearVelocity();
	fallspeed = velocity * physicsObj.GetGravityNormal();

	if ( influenceActive ) {
		AI_FORWARD		= false;
		AI_BACKWARD		= false;
		AI_STRAFE_LEFT	= false;
		AI_STRAFE_RIGHT	= false;
	} else if ( gameLocal.time - lastDmgTime < 500 ) {
		forwardspeed = velocity * viewAxis[ 0 ];
		sidespeed = velocity * viewAxis[ 1 ];
		AI_FORWARD		= AI_ONGROUND && ( forwardspeed > 20.01f );
		AI_BACKWARD		= AI_ONGROUND && ( forwardspeed < -20.01f );
		AI_STRAFE_LEFT	= AI_ONGROUND && ( sidespeed > 20.01f );
		AI_STRAFE_RIGHT	= AI_ONGROUND && ( sidespeed < -20.01f );
	} else if ( xyspeed > MIN_BOB_SPEED ) {
		AI_FORWARD		= AI_ONGROUND && ( usercmd.forwardmove > 0 );
		AI_BACKWARD		= AI_ONGROUND && ( usercmd.forwardmove < 0 );
		AI_STRAFE_LEFT	= AI_ONGROUND && ( usercmd.rightmove < 0 );
		AI_STRAFE_RIGHT	= AI_ONGROUND && ( usercmd.rightmove > 0 );
	} else {
		AI_FORWARD		= false;
		AI_BACKWARD		= false;
		AI_STRAFE_LEFT	= false;
		AI_STRAFE_RIGHT	= false;
	}

	// stamina disabled, always run regardless of stamina
	//AI_RUN			= ( usercmd.buttons & BUTTON_RUN ) && ( ( !pm_stamina.GetFloat() ) || ( stamina > pm_staminathreshold.GetFloat() ) );
	AI_RUN = ( usercmd.buttons & BUTTON_RUN ) && true ;
	AI_DEAD			= ( health <= 0 );
	
	// DarkMod: Catch the creep modifier
	AI_CREEP		=( usercmd.buttons & BUTTON_5 ) && true;

	// Check if the frob is to be a continous action.
	if(m_ContinuousUse == true)
	{
		if (common->ButtonState(KEY_FROM_IMPULSE(IMPULSE_51))) {
			inventoryUseItem(false);
		}
		else {
			m_ContinuousUse = false;
		}
	}
}

/*
==================
WeaponFireFeedback

Called when a weapon fires, generates head twitches, etc
==================
*/
void idPlayer::WeaponFireFeedback( const idDict *weaponDef ) {
	// force a blink
	blink_time = 0;

	// play the fire animation
	AI_WEAPON_FIRED = true;

	// update view feedback
	playerView.WeaponFireFeedback( weaponDef );
}

/*
===============
idPlayer::StopFiring
===============
*/
void idPlayer::StopFiring( void ) {
	AI_ATTACK_HELD	= false;
	AI_WEAPON_FIRED = false;
	AI_RELOAD		= false;
	if ( weapon.GetEntity() ) {
		weapon.GetEntity()->EndAttack();
	}
}

/*
===============
idPlayer::FireWeapon
===============
*/
void idPlayer::FireWeapon( void ) {
	idMat3 axis;
	idVec3 muzzle;

	if ( privateCameraView ) {
		return;
	}

	if ( g_editEntityMode.GetInteger() ) {
		GetViewPos( muzzle, axis );
		if ( gameLocal.editEntities->SelectEntity( muzzle, axis[0], this ) ) {
			return;
		}
	}

	if ( !hiddenWeapon && weapon.GetEntity()->IsReady() ) {
		if ( weapon.GetEntity()->AmmoInClip() || weapon.GetEntity()->AmmoAvailable() ) {
			AI_ATTACK_HELD = true;
			weapon.GetEntity()->BeginAttack();
			if ( ( weapon_soulcube >= 0 ) && ( currentWeapon == weapon_soulcube ) ) {
				if ( hud ) {
					hud->HandleNamedEvent( "soulCubeNotReady" );
				}
				SelectWeapon( previousWeapon, false );
			}
		} else {
			NextBestWeapon();
		}
	}

	if ( hud ) {
		if ( tipUp ) {
			HideTip();
		}
		// may want to track with with a bool as well
		// keep from looking up named events so often
		if ( objectiveUp ) {
			HideObjective();
		}
	}
}

/*
===============
idPlayer::CacheWeapons
===============
*/
void idPlayer::CacheWeapons( void ) {
	// greebo: Cache all weapons, regardless if we have them or not
	for( int w = 0; w < MAX_WEAPONS; w++ ) {
		idStr weap = spawnArgs.GetString( va( "def_weapon%d", w ) );
		if ( weap != "" ) {
			idWeapon::CacheWeapon( weap );
		}
	}
}

/*
===============
idPlayer::Give
===============
*/
bool idPlayer::Give( const char *statname, const char *value ) {
	int amount;

	if ( AI_DEAD ) {
		return false;
	}

	if ( !idStr::Icmp( statname, "health" ) ) {
		if ( health >= inventory.maxHealth ) {
			return false;
		}
		amount = atoi( value );
		if ( amount ) {
			health += amount;
			if ( health > inventory.maxHealth ) {
				health = inventory.maxHealth;
			}
			if ( hud ) {
				hud->HandleNamedEvent( "healthPulse" );
			}
		}

	} else if ( !idStr::Icmp( statname, "stamina" ) ) {
		if ( stamina >= 100 ) {
			return false;
		}
		stamina += atof( value );
		if ( stamina > 100 ) {
			stamina = 100;
		}

	} else if ( !idStr::Icmp( statname, "heartRate" ) ) {
		heartRate += atoi( value );
		if ( heartRate > MAX_HEARTRATE ) {
			heartRate = MAX_HEARTRATE;
		}

	} else if ( !idStr::Icmp( statname, "air" ) ) {
		if ( airTics >= pm_airTics.GetInteger() ) {
			return false;
		}
		airTics += atoi( value ) / 100.0 * pm_airTics.GetInteger();
		if ( airTics > pm_airTics.GetInteger() ) {
			airTics = pm_airTics.GetInteger();
		}
	} else {
		return inventory.Give( this, spawnArgs, statname, value, &idealWeapon, true );
	}
	return true;
}


/*
===============
idPlayer::GiveHealthPool

adds health to the player health pool
===============
*/
void idPlayer::GiveHealthPool( float amt ) {
	
	if ( AI_DEAD ) {
		return;
	}

	if ( health > 0 ) {
		healthPool += amt;
		if ( healthPool > inventory.maxHealth - health ) {
			healthPool = inventory.maxHealth - health;
		}
		nextHealthPulse = gameLocal.time;

		// Reset the values to default
		healthPoolTimeInterval = HEALTHPULSE_TIME;
		healthPoolTimeIntervalFactor = 1.0f;
		healthPoolStepAmount = 5;
	}
}

/*
===============
idPlayer::GiveItem

Returns false if the item shouldn't be picked up

greebo: This routine can probably be removed completely
===============
*/
bool idPlayer::GiveItem( idItem *item ) {
	const idKeyValue	*arg;
	idDict				attr;
	bool				gave;

	if ( gameLocal.isMultiplayer && spectating ) {
		return false;
	}

	item->GetAttributes( attr );
	
	gave = false;

	arg = item->spawnArgs.MatchPrefix( "inv_weapon", NULL );
	if ( arg && hud ) {
		// We need to update the weapon hud manually, but not
		// the armor/ammo/health because they are updated every
		// frame no matter what
		UpdateHudWeapon( false );
		hud->HandleNamedEvent( "weaponPulse" );
	}

	return gave;
}

/*
===============
idPlayer::PowerUpModifier
===============
*/
float idPlayer::PowerUpModifier( int type ) {
	// greebo: Unused at the moment, maybe in the future
	return 1.0f;
}

/*
===============
idPlayer::GivePowerUp
===============
*/
bool idPlayer::GivePowerUp( int powerup, int time ) {
	const char *sound;
	const char *skin;

	if ( powerup >= 0 && powerup < MAX_POWERUPS ) {

		if ( gameLocal.isServer ) {
			idBitMsg	msg;
			byte		msgBuf[MAX_EVENT_PARAM_SIZE];

			msg.Init( msgBuf, sizeof( msgBuf ) );
			msg.WriteShort( powerup );
			msg.WriteBits( 1, 1 );
			ServerSendEvent( EVENT_POWERUP, &msg, false, -1 );
		}

		/*if ( powerup != MEGAHEALTH ) {
			inventory.GivePowerUp( this, powerup, time );
		}*/

		const idDeclEntityDef *def = NULL;

		switch( powerup ) {
			case BERSERK: {
				if ( spawnArgs.GetString( "snd_berserk_third", "", &sound ) ) {
					StartSoundShader( declManager->FindSound( sound ), SND_CHANNEL_DEMONIC, 0, false, NULL );
				}
				if ( baseSkinName.Length() ) {
					powerUpSkin = declManager->FindSkin( baseSkinName + "_berserk" );
				}
				if ( !gameLocal.isClient ) {
					idealWeapon = 0;
				}
				break;
			}
			case INVISIBILITY: {
				spawnArgs.GetString( "skin_invisibility", "", &skin );
				powerUpSkin = declManager->FindSkin( skin );
				// remove any decals from the model
				if ( modelDefHandle != -1 ) {
					gameRenderWorld->RemoveDecals( modelDefHandle );
				}
				if ( weapon.GetEntity() ) {
					weapon.GetEntity()->UpdateSkin();
				}
				if ( spawnArgs.GetString( "snd_invisibility", "", &sound ) ) {
					StartSoundShader( declManager->FindSound( sound ), SND_CHANNEL_ANY, 0, false, NULL );
				}
				break;
			}
			case ADRENALINE: {
				stamina = 100.0f;
				break;
			 }
			case MEGAHEALTH: {
				if ( spawnArgs.GetString( "snd_megahealth", "", &sound ) ) {
					StartSoundShader( declManager->FindSound( sound ), SND_CHANNEL_ANY, 0, false, NULL );
				}
				def = gameLocal.FindEntityDef( "powerup_megahealth", false );
				if ( def ) {
					health = def->dict.GetInt( "inv_health" );
				}
				break;
			 }
		}

		if ( hud ) {
			hud->HandleNamedEvent( "itemPickup" );
		}

		return true;
	} else {
		gameLocal.Warning( "Player given power up %i\n which is out of range", powerup );
	}
	return false;
}

/*
==============
idPlayer::UpdatePowerUps
==============
*/
void idPlayer::UpdatePowerUps( void ) {
	if ( health > 0 ) {
		if ( powerUpSkin ) {
			renderEntity.customSkin = powerUpSkin;
		} else {
			renderEntity.customSkin = skin;
		}
	}

	if ( healthPool && gameLocal.time > nextHealthPulse && !AI_DEAD && health > 0 ) {
		assert( !gameLocal.isClient );	// healthPool never be set on client

		//int amt = ( healthPool > 5 ) ? 5 : healthPool; // old code
		// greebo: Changed step amount to be a variable that can be set from the "outside"
		int amt = ( healthPool > healthPoolStepAmount ) ? healthPoolStepAmount : healthPool;

		health += amt;
		if ( health > inventory.maxHealth ) {
			health = inventory.maxHealth;
			healthPool = 0;
		} else {
			healthPool -= amt;
		}
		nextHealthPulse = gameLocal.time + healthPoolTimeInterval;

		// Check whether we have a valid interval factor and if yes: apply it
		if (healthPoolTimeIntervalFactor > 0) {
			healthPoolTimeInterval *= healthPoolTimeIntervalFactor;
		}

		healthPulse = true;
	}
#ifndef ID_DEMO_BUILD
	if ( !gameLocal.inCinematic && influenceActive == 0 && g_skill.GetInteger() == 3 && gameLocal.time > nextHealthTake && !AI_DEAD && health > g_healthTakeLimit.GetInteger() ) {
		assert( !gameLocal.isClient );	// healthPool never be set on client
		health -= g_healthTakeAmt.GetInteger();
		if ( health < g_healthTakeLimit.GetInteger() ) {
			health = g_healthTakeLimit.GetInteger();
		}
		nextHealthTake = gameLocal.time + g_healthTakeTime.GetInteger() * 1000;
		healthTake = true;
	}
#endif
}

/*
===============
idPlayer::GiveVideo
===============
*/
void idPlayer::GiveVideo( const char *videoName, idDict *item ) {

	if ( videoName == NULL || *videoName == NULL ) {
		return;
	}

	inventory.videos.AddUnique( videoName );

	if ( hud ) {
		hud->HandleNamedEvent( "videoPickup" );
	}
}

/*
===============
idPlayer::GiveSecurity
===============
*/
void idPlayer::GiveSecurity( const char *security ) {
	GetPDA()->SetSecurity( security );
	if ( hud ) {
		hud->SetStateString( "pda_security", "1" );
		hud->HandleNamedEvent( "securityPickup" );
	}
}

/*
===============
idPlayer::GiveEmail
===============
*/
void idPlayer::GiveEmail( const char *emailName ) {

	if ( emailName == NULL || *emailName == NULL ) {
		return;
	}

	inventory.emails.AddUnique( emailName );
	GetPDA()->AddEmail( emailName );

	if ( hud ) {
		hud->HandleNamedEvent( "emailPickup" );
	}
}

/*
===============
idPlayer::GivePDA
===============
*/
void idPlayer::GivePDA( const char *pdaName, idDict *item )
{
	if ( gameLocal.isMultiplayer && spectating ) {
		return;
	}

	if ( item ) {
		inventory.pdaSecurity.AddUnique( item->GetString( "inv_name" ) );
	}

	if ( pdaName == NULL || *pdaName == NULL ) {
		pdaName = "personal";
	}

	const idDeclPDA *pda = static_cast< const idDeclPDA* >( declManager->FindType( DECL_PDA, pdaName ) );

	inventory.pdas.AddUnique( pdaName );

	// Copy any videos over
	for ( int i = 0; i < pda->GetNumVideos(); i++ ) {
		const idDeclVideo *video = pda->GetVideoByIndex( i );
		if ( video ) {
			inventory.videos.AddUnique( video->GetName() );
		}
	}

	// This is kind of a hack, but it works nicely
	// We don't want to display the 'you got a new pda' message during a map load
	if ( gameLocal.GetFrameNum() > 10 ) {
		if ( pda && hud ) {
			idStr pdaName = pda->GetPdaName();
			pdaName.RemoveColors();
			hud->SetStateString( "pda", "1" );
			hud->SetStateString( "pda_text", pdaName );
			const char *sec = pda->GetSecurity();
			hud->SetStateString( "pda_security", ( sec && *sec ) ? "1" : "0" );
			hud->HandleNamedEvent( "pdaPickup" );
		}

		if ( inventory.pdas.Num() == 1 ) {
			GetPDA()->RemoveAddedEmailsAndVideos();
			if ( !objectiveSystemOpen ) {
				TogglePDA();
			}
			objectiveSystem->HandleNamedEvent( "showPDATip" );
			//ShowTip( spawnArgs.GetString( "text_infoTitle" ), spawnArgs.GetString( "text_firstPDA" ), true );
		}

		if ( inventory.pdas.Num() > 1 && pda->GetNumVideos() > 0 && hud ) {
			hud->HandleNamedEvent( "videoPickup" );
		}
	}
}

/*
===============
idPlayer::GiveItem
===============
*/
void idPlayer::GiveItem( const char *itemname ) {
	idDict args;

	args.Set( "classname", itemname );
	args.Set( "owner", name.c_str() );
	gameLocal.SpawnEntityDef( args );
	if ( hud ) {
		hud->HandleNamedEvent( "itemPickup" );
	}
}

/*
==================
idPlayer::SlotForWeapon
==================
*/
int idPlayer::SlotForWeapon( const char *weaponName ) {
	int i;

	for( i = 0; i < MAX_WEAPONS; i++ ) {
		const char *weap = spawnArgs.GetString( va( "def_weapon%d", i ) );
		if ( !idStr::Cmp( weap, weaponName ) ) {
			return i;
		}
	}

	// not found
	return -1;
}

/*
===============
idPlayer::Reload
===============
*/
void idPlayer::Reload( void ) {
	if ( gameLocal.isClient ) {
		return;
	}

	if ( spectating || gameLocal.inCinematic || influenceActive ) {
		return;
	}

	if ( weapon.GetEntity() && weapon.GetEntity()->IsLinked() ) {
		weapon.GetEntity()->Reload();
	}
}

/*
===============
idPlayer::NextBestWeapon
===============
*/
void idPlayer::NextBestWeapon( void ) {
	// greebo: No "best" weapons in TDM, route the call to NextWeapon()
	NextWeapon();
}

/*
===============
idPlayer::NextWeapon
===============
*/
void idPlayer::NextWeapon( void ) {
	if ( !weaponEnabled || spectating || hiddenWeapon || gameLocal.inCinematic || gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) || health < 0 ) {
		return;
	}

	if ( gameLocal.isClient ) {
		return;
	}

	if (m_WeaponCursor == NULL || m_WeaponCursor->GetCurrentCategory() == NULL) {
		return;
	}

	if (m_WeaponCursor->GetCurrentCategory()->size() == 0) {
		return; // no weapons...
	}
	
	// Get the current weaponItem
	CInventoryWeaponItem* curItem = getCurrentWeaponItem();

	if (curItem == NULL) {
		return;
	}

	int curWeaponIndex = curItem->getWeaponIndex();
	int nextWeaponIndex = curWeaponIndex;

	do {
		// Try to select the next weapon item
		nextWeaponIndex++;

		if (nextWeaponIndex >= MAX_WEAPONS) {
			nextWeaponIndex = 0;
		}
	} while (!SelectWeapon(nextWeaponIndex, false) && nextWeaponIndex != curWeaponIndex);
}

/*
===============
idPlayer::PrevWeapon
===============
*/
void idPlayer::PrevWeapon( void ) {
	if ( !weaponEnabled || spectating || hiddenWeapon || gameLocal.inCinematic || gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) || health < 0 ) {
		return;
	}

	if ( gameLocal.isClient ) {
		return;
	}

	if (m_WeaponCursor == NULL || m_WeaponCursor->GetCurrentCategory() == NULL) {
		return;
	}

	if (m_WeaponCursor->GetCurrentCategory()->size() == 0) {
		return; // no weapons...
	}

	// Get the current weaponItem
	CInventoryWeaponItem* curItem = getCurrentWeaponItem();

	if (curItem == NULL) {
		return;
	}

	int curWeaponIndex = curItem->getWeaponIndex();
	int prevWeaponIndex = curWeaponIndex;

	do {
		// Try to select the previous weapon item
		prevWeaponIndex--;

		if (prevWeaponIndex < 0) {
			prevWeaponIndex = MAX_WEAPONS;
		}
	} while (!SelectWeapon(prevWeaponIndex, false) && prevWeaponIndex != curWeaponIndex);
}

/*
===============
idPlayer::SelectWeapon
===============
*/
bool idPlayer::SelectWeapon( int num, bool force ) {
	//const char *weap;

	if ( !weaponEnabled || spectating || gameLocal.inCinematic || health < 0 ) {
		return false;
	}

	if ( ( num < 0 ) || ( num >= MAX_WEAPONS ) ) {
		return false;
	}

	if ( gameLocal.isClient ) {
		return false;
	}

	if ( ( num != weapon_pda ) && gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) ) {
		num = weapon_fists;
		hiddenWeapon ^= 1;
		if ( hiddenWeapon && weapon.GetEntity() ) {
			weapon.GetEntity()->LowerWeapon();
		} else {
			weapon.GetEntity()->RaiseWeapon();
		}
	}	

	if (m_WeaponCursor == NULL) {
		return false;
	}

	// Check if we want to toggle the current weapon item (requested index == current index)
	CInventoryWeaponItem* item = getCurrentWeaponItem();
	if (item != NULL && item->getWeaponIndex() == num && item->isToggleable()) {
		// Requested toggleable weapon is already active, hide it (switch to unarmed)
		num = 0;
	}

	CInventoryCategory* category = m_WeaponCursor->GetCurrentCategory();
	if (category == NULL) {
		return false;
	}

	// Cycle through the weapons and find the one with the given weaponIndex
	for (int i = 0; i < category->size(); i++) {
		// Try to retrieve a weapon item from the given category
		CInventoryWeaponItem* item = dynamic_cast<CInventoryWeaponItem*>(category->GetItem(i));
		
		if (item != NULL) {
			if (item->getWeaponIndex() == num) {
				if (item->getAmmo() <= 0 && !item->allowedEmpty()) {
					DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Weapon requires ammo. Cannot select: %d\r", num);
					break;
				}

				DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Selecting weapon #%d\r", num);
				// Set the cursor onto this item
				m_WeaponCursor->SetCurrentItem(item);

				weaponSwitchTime = gameLocal.time + WEAPON_SWITCH_DELAY;
				idealWeapon = num;
				UpdateHudWeapon();
				return true;
			}
		}
	}

	return false;
}

/*
=================
idPlayer::DropWeapon
=================
*/
void idPlayer::DropWeapon( bool died ) {
	idVec3 forward, up;
	int inclip, ammoavailable;

	assert( !gameLocal.isClient );
	
	if ( spectating || weaponGone || weapon.GetEntity() == NULL ) {
		return;
	}
	
	if ( ( !died && !weapon.GetEntity()->IsReady() ) || weapon.GetEntity()->IsReloading() ) {
		return;
	}
	// ammoavailable is how many shots we can fire
	// inclip is which amount is in clip right now
	ammoavailable = weapon.GetEntity()->AmmoAvailable();
	inclip = weapon.GetEntity()->AmmoInClip();
	
	// don't drop a grenade if we have none left
	if ( !idStr::Icmp( idWeapon::GetAmmoNameForNum( weapon.GetEntity()->GetAmmoType() ), "ammo_grenades" ) && ( ammoavailable - inclip <= 0 ) ) {
		return;
	}

	// expect an ammo setup that makes sense before doing any dropping
	// ammoavailable is -1 for infinite ammo, and weapons like chainsaw
	// a bad ammo config usually indicates a bad weapon state, so we should not drop
	// used to be an assertion check, but it still happens in edge cases
	if ( ( ammoavailable != -1 ) && ( ammoavailable - inclip < 0 ) ) {
		common->DPrintf( "idPlayer::DropWeapon: bad ammo setup\n" );
		return;
	}
	idEntity *item = NULL;
	if ( died ) {
		// ain't gonna throw you no weapon if I'm dead
		item = weapon.GetEntity()->DropItem( vec3_origin, 0, WEAPON_DROP_TIME, died );
	} else {
		viewAngles.ToVectors( &forward, NULL, &up );
		item = weapon.GetEntity()->DropItem( 250.0f * forward + 150.0f * up, 500, WEAPON_DROP_TIME, died );
	}
	if ( !item ) {
		return;
	}
	// set the appropriate ammo in the dropped object
	const idKeyValue * keyval = item->spawnArgs.MatchPrefix( "inv_ammo_" );
	if ( keyval ) {
		item->spawnArgs.SetInt( keyval->GetKey(), ammoavailable );
		idStr inclipKey = keyval->GetKey();
		inclipKey.Insert( "inclip_", 4 );
		item->spawnArgs.SetInt( inclipKey, inclip );
	}
	if ( !died ) {
		// remove from our local inventory completely
		inventory.Drop( spawnArgs, item->spawnArgs.GetString( "inv_weapon" ), -1 );
		weapon.GetEntity()->ResetAmmoClip();
		NextWeapon();
		weapon.GetEntity()->WeaponStolen();
		weaponGone = true;
	}
}

/*
=================
idPlayer::StealWeapon
steal the target player's current weapon
=================
*/
void idPlayer::StealWeapon( idPlayer *player )
{
	assert( !gameLocal.isClient );

	// make sure there's something to steal
	idWeapon *player_weapon = static_cast< idWeapon * >( player->weapon.GetEntity() );
	if ( !player_weapon || !player_weapon->CanDrop() || weaponGone ) {
		return;
	}
	// steal - we need to effectively force the other player to abandon his weapon
	int newweap = player->currentWeapon;
	if ( newweap == -1 ) {
		return;
	}

	const char *weapon_classname = spawnArgs.GetString( va( "def_weapon%d", newweap ) );
	assert( weapon_classname );
	int ammoavailable = player->weapon.GetEntity()->AmmoAvailable();
	int inclip = player->weapon.GetEntity()->AmmoInClip();
	if ( ( ammoavailable != -1 ) && ( ammoavailable - inclip < 0 ) ) {
		// see DropWeapon
		common->DPrintf( "idPlayer::StealWeapon: bad ammo setup\n" );
		// we still steal the weapon, so let's use the default ammo levels
		inclip = -1;
		const idDeclEntityDef *decl = gameLocal.FindEntityDef( weapon_classname );
		assert( decl );
		const idKeyValue *keypair = decl->dict.MatchPrefix( "inv_ammo_" );
		assert( keypair );
		ammoavailable = atoi( keypair->GetValue() );
	}

	player->weapon.GetEntity()->WeaponStolen();
	player->inventory.Drop( player->spawnArgs, NULL, newweap );
	player->SelectWeapon( weapon_fists, false );
	// in case the robbed player is firing rounds with a continuous fire weapon like the chaingun/plasma etc.
	// this will ensure the firing actually stops
	player->weaponGone = true;

	// give weapon, setup the ammo count
	Give( "weapon", weapon_classname );
	idealWeapon = newweap;
}

/*
===============
idPlayer::ActiveGui
===============
*/
idUserInterface *idPlayer::ActiveGui( void ) {
	if ( objectiveSystemOpen )
		return objectiveSystem;

	if ( m_overlays.findInteractive() )
		return m_overlays.findInteractive();

	return focusUI;
}

/*
===============
idPlayer::Weapon_Combat
===============
*/
void idPlayer::Weapon_Combat( void ) {
	if ( influenceActive || !weaponEnabled || gameLocal.inCinematic || privateCameraView ) {
		return;
	}

	weapon.GetEntity()->RaiseWeapon();
	if ( weapon.GetEntity()->IsReloading() ) {
		if ( !AI_RELOAD ) {
			AI_RELOAD = true;
			SetState( "ReloadWeapon" );
			UpdateScript();
		}
	} else {
		AI_RELOAD = false;
	}

	if ( idealWeapon == weapon_soulcube && soulCubeProjectile.GetEntity() != NULL ) {
		idealWeapon = currentWeapon;
	}

	if ( idealWeapon != currentWeapon ) {
		if ( weaponCatchup ) {
			assert( gameLocal.isClient );

			currentWeapon = idealWeapon;
			weaponGone = false;
			animPrefix = spawnArgs.GetString( va( "def_weapon%d", currentWeapon ) );
			weapon.GetEntity()->GetWeaponDef( animPrefix, 0 /*inventory.clip[ currentWeapon ]*/ );
			animPrefix.Strip( "weapon_" );

			weapon.GetEntity()->NetCatchup();
			const function_t *newstate = GetScriptFunction( "NetCatchup" );
			if ( newstate ) {
				SetState( newstate );
				UpdateScript();
			}
			weaponCatchup = false;			
		} else {
			if ( weapon.GetEntity()->IsReady() ) {
				weapon.GetEntity()->PutAway();
			}

			if ( weapon.GetEntity()->IsHolstered() ) {
				assert( idealWeapon >= 0 );
				assert( idealWeapon < MAX_WEAPONS );

				if ( currentWeapon != weapon_pda && !spawnArgs.GetBool( va( "weapon%d_toggle", currentWeapon ) ) ) {
					previousWeapon = currentWeapon;
				}
				currentWeapon = idealWeapon;
				weaponGone = false;
				animPrefix = spawnArgs.GetString( va( "def_weapon%d", currentWeapon ) );
				weapon.GetEntity()->GetWeaponDef( animPrefix, 0/*inventory.clip[ currentWeapon ]*/ );
				animPrefix.Strip( "weapon_" );

				weapon.GetEntity()->Raise();
			}
		}
	} else {
		weaponGone = false;	// if you drop and re-get weap, you may miss the = false above 
		if ( weapon.GetEntity()->IsHolstered() ) {
			if ( !weapon.GetEntity()->AmmoAvailable() ) {
				// weapons can switch automatically if they have no more ammo
				NextBestWeapon();
			} else {
				weapon.GetEntity()->Raise();
				state = GetScriptFunction( "RaiseWeapon" );
				if ( state ) {
					SetState( state );
				}
			}
		}
	}

	// check for attack
	AI_WEAPON_FIRED = false;
	if ( !influenceActive ) {
		if ( ( usercmd.buttons & BUTTON_ATTACK ) && !weaponGone ) {
			FireWeapon();
		} else if ( oldButtons & BUTTON_ATTACK ) {
			AI_ATTACK_HELD = false;
			weapon.GetEntity()->EndAttack();
		}
	}

	// update our ammo clip in our inventory
	if ( ( currentWeapon >= 0 ) && ( currentWeapon < MAX_WEAPONS ) ) {
		//inventory.clip[ currentWeapon ] = weapon.GetEntity()->AmmoInClip();
		if ( hud && ( currentWeapon == idealWeapon ) ) {
			UpdateHudAmmo( hud );
		}
	}
}

/*
===============
idPlayer::Weapon_NPC
===============
*/
void idPlayer::Weapon_NPC( void ) {
	if ( idealWeapon != currentWeapon ) {
		Weapon_Combat();
	}
	StopFiring();
	weapon.GetEntity()->LowerWeapon();

	if ( ( usercmd.buttons & BUTTON_ATTACK ) && !( oldButtons & BUTTON_ATTACK ) ) {
		buttonMask |= BUTTON_ATTACK;
		focusCharacter->TalkTo( this );
	}
}

/*
===============
idPlayer::LowerWeapon
===============
*/
void idPlayer::LowerWeapon( void ) {
	if ( weapon.GetEntity() && !weapon.GetEntity()->IsHidden() ) {
		weapon.GetEntity()->LowerWeapon();
	}
}

/*
===============
idPlayer::RaiseWeapon
===============
*/
void idPlayer::RaiseWeapon( void ) {
	if ( weapon.GetEntity() && weapon.GetEntity()->IsHidden() ) {
		weapon.GetEntity()->RaiseWeapon();
	}
}

/*
===============
idPlayer::WeaponLoweringCallback
===============
*/
void idPlayer::WeaponLoweringCallback( void ) {
	SetState( "LowerWeapon" );
	UpdateScript();
}

/*
===============
idPlayer::WeaponRisingCallback
===============
*/
void idPlayer::WeaponRisingCallback( void ) {
	SetState( "RaiseWeapon" );
	UpdateScript();
}

/*
===============
idPlayer::Weapon_GUI
===============
*/
void idPlayer::Weapon_GUI( void ) {

	if ( !objectiveSystemOpen ) {
		if ( idealWeapon != currentWeapon ) {
			Weapon_Combat();
		}
		StopFiring();
		weapon.GetEntity()->LowerWeapon();
	}

	// disable click prediction for the GUIs. handy to check the state sync does the right thing
	if ( gameLocal.isClient && !net_clientPredictGUI.GetBool() ) {
		return;
	}

	if ( ( oldButtons ^ usercmd.buttons ) & BUTTON_ATTACK ) {
		sysEvent_t ev;
		const char *command = NULL;
		bool updateVisuals = false;

		idUserInterface *ui = ActiveGui();
		if ( ui ) {
			ev = sys->GenerateMouseButtonEvent( 1, ( usercmd.buttons & BUTTON_ATTACK ) != 0 );
			command = ui->HandleEvent( &ev, gameLocal.time, &updateVisuals );
			if ( updateVisuals && focusGUIent && ui == focusUI ) {
				focusGUIent->UpdateVisuals();
			}
		}
		if ( gameLocal.isClient ) {
			// we predict enough, but don't want to execute commands
			return;
		}
		if ( focusGUIent ) {
			HandleGuiCommands( focusGUIent, command );
		} else {
			HandleGuiCommands( this, command );
		}
	}
}

/*
===============
idPlayer::UpdateWeapon
===============
*/
void idPlayer::UpdateWeapon( void ) {
	if ( health <= 0 ) {
		return;
	}

	assert( !spectating );

	if ( gameLocal.isClient ) {
		// clients need to wait till the weapon and it's world model entity
		// are present and synchronized ( weapon.worldModel idEntityPtr to idAnimatedEntity )
		if ( !weapon.GetEntity()->IsWorldModelReady() ) {
			return;
		}
	}

	// always make sure the weapon is correctly setup before accessing it
	if ( !weapon.GetEntity()->IsLinked() ) {
		if ( idealWeapon != -1 ) {
			animPrefix = spawnArgs.GetString( va( "def_weapon%d", idealWeapon ) );
			weapon.GetEntity()->GetWeaponDef( animPrefix, 0/*inventory.clip[ idealWeapon ]*/ );
			assert( weapon.GetEntity()->IsLinked() );
		} else {
			return;
		}
	}

	if ( hiddenWeapon && tipUp && usercmd.buttons & BUTTON_ATTACK ) {
		HideTip();
	}

	if ( g_dragEntity.GetBool() ) {
		StopFiring();
		weapon.GetEntity()->LowerWeapon();
		dragEntity.Update( this );
	} else if ( ActiveGui() ) {
		// gui handling overrides weapon use
		Weapon_GUI();
	} else 	if ( focusCharacter && ( focusCharacter->health > 0 ) ) {
		Weapon_NPC();
	} else if( g_Global.m_DarkModPlayer->grabber->GetSelected() ) {
		StopFiring();
		weapon.GetEntity()->LowerWeapon();
		g_Global.m_DarkModPlayer->grabber->Update( this, true );
	} else {
		Weapon_Combat();
	}
	
	if ( hiddenWeapon ) {
		weapon.GetEntity()->LowerWeapon();
	}

	// update weapon state, particles, dlights, etc
	weapon.GetEntity()->PresentWeapon( showWeaponViewModel );
}

/*
===============
idPlayer::SpectateFreeFly
===============
*/
void idPlayer::SpectateFreeFly( bool force ) {
	idPlayer	*player;
	idVec3		newOrig;
	idVec3		spawn_origin;
	idAngles	spawn_angles;

	player = gameLocal.GetClientByNum( spectator );
	if ( force || gameLocal.time > lastSpectateChange ) {
		spectator = entityNumber;
		if ( player && player != this && !player->spectating && !player->IsInTeleport() ) {
			newOrig = player->GetPhysics()->GetOrigin();
			if ( player->physicsObj.IsCrouching() ) {
				newOrig[ 2 ] += pm_crouchviewheight.GetFloat();
			} else {
				newOrig[ 2 ] += pm_normalviewheight.GetFloat();
			}
			newOrig[ 2 ] += SPECTATE_RAISE;
			idBounds b = idBounds( vec3_origin ).Expand( pm_spectatebbox.GetFloat() * 0.5f );
			idVec3 start = player->GetPhysics()->GetOrigin();
			start[2] += pm_spectatebbox.GetFloat() * 0.5f;
			trace_t t;
			// assuming spectate bbox is inside stand or crouch box
			gameLocal.clip.TraceBounds( t, start, newOrig, b, MASK_PLAYERSOLID, player );
			newOrig.Lerp( start, newOrig, t.fraction );
			SetOrigin( newOrig );
			idAngles angle = player->viewAngles;
			angle[ 2 ] = 0;
			SetViewAngles( angle );
		} else {	
			SelectInitialSpawnPoint( spawn_origin, spawn_angles );
			spawn_origin[ 2 ] += pm_normalviewheight.GetFloat();
			spawn_origin[ 2 ] += SPECTATE_RAISE;
			SetOrigin( spawn_origin );
			SetViewAngles( spawn_angles );
		}
		lastSpectateChange = gameLocal.time + 500;
	}
}

/*
===============
idPlayer::SpectateCycle
===============
*/
void idPlayer::SpectateCycle( void ) {
	idPlayer *player;

	if ( gameLocal.time > lastSpectateChange ) {
		int latchedSpectator = spectator;
		spectator = gameLocal.GetNextClientNum( spectator );
		player = gameLocal.GetClientByNum( spectator );
		assert( player ); // never call here when the current spectator is wrong
		// ignore other spectators
		while ( latchedSpectator != spectator && player->spectating ) {
			spectator = gameLocal.GetNextClientNum( spectator );
			player = gameLocal.GetClientByNum( spectator );
		}
		lastSpectateChange = gameLocal.time + 500;
	}
}

/*
===============
idPlayer::UpdateSpectating
===============
*/
void idPlayer::UpdateSpectating( void ) {
	assert( spectating );
	assert( !gameLocal.isClient );
	assert( IsHidden() );
	idPlayer *player;
	if ( !gameLocal.isMultiplayer ) {
		return;
	}
	player = gameLocal.GetClientByNum( spectator );
	if ( !player || ( player->spectating && player != this ) ) {
		SpectateFreeFly( true );
	} else if ( usercmd.upmove > 0 ) {
		SpectateFreeFly( false );
	} else if ( usercmd.buttons & BUTTON_ATTACK ) {
		SpectateCycle();
	}
}

/*
===============
idPlayer::HandleSingleGuiCommand
===============
*/
bool idPlayer::HandleSingleGuiCommand( idEntity *entityGui, idLexer *src ) {
	idToken token;

	if ( !src->ReadToken( &token ) ) {
		return false;
	}

	if ( token == ";" ) {
		return false;
	}

	if ( token.Icmp( "addhealth" ) == 0 ) {
		if ( entityGui && health < 100 ) {
			int _health = entityGui->spawnArgs.GetInt( "gui_parm1" );
			int amt = ( _health >= HEALTH_PER_DOSE ) ? HEALTH_PER_DOSE : _health;
			_health -= amt;
			entityGui->spawnArgs.SetInt( "gui_parm1", _health );
			if ( entityGui->GetRenderEntity() && entityGui->GetRenderEntity()->gui[ 0 ] ) {
				entityGui->GetRenderEntity()->gui[ 0 ]->SetStateInt( "gui_parm1", _health );
			}
			health += amt;
			if ( health > 100 ) {
				health = 100;
			}
		}
		return true;
	}

	if ( token.Icmp( "ready" ) == 0 ) {
		PerformImpulse( IMPULSE_17 );
		return true;
	}

	if ( token.Icmp( "updatepda" ) == 0 ) {
		UpdatePDAInfo( true );
		return true;
	}

	if ( token.Icmp( "updatepda2" ) == 0 ) {
		UpdatePDAInfo( false );
		return true;
	}

	if ( token.Icmp( "stoppdavideo" ) == 0 ) {
		if ( objectiveSystem && objectiveSystemOpen && pdaVideoWave.Length() > 0 ) {
			StopSound( SND_CHANNEL_PDA, false );
		}
		return true;
	}

	if ( token.Icmp( "close" ) == 0 ) {
		if ( objectiveSystem && objectiveSystemOpen ) {
			TogglePDA();
		}
	}

	if ( token.Icmp( "playpdavideo" ) == 0 ) {
		if ( objectiveSystem && objectiveSystemOpen && pdaVideo.Length() > 0 ) {
			const idMaterial *mat = declManager->FindMaterial( pdaVideo );
			if ( mat ) {
				int c = mat->GetNumStages();
				for ( int i = 0; i < c; i++ ) {
					const shaderStage_t *stage = mat->GetStage(i);
					if ( stage && stage->texture.cinematic ) {
						stage->texture.cinematic->ResetTime( gameLocal.time );
					}
				}
				if ( pdaVideoWave.Length() ) {
					const idSoundShader *shader = declManager->FindSound( pdaVideoWave );
					StartSoundShader( shader, SND_CHANNEL_PDA, 0, false, NULL );
				}
			}
		}
	}

	if ( token.Icmp( "playpdaaudio" ) == 0 ) {
		if ( objectiveSystem && objectiveSystemOpen && pdaAudio.Length() > 0 ) {
			const idSoundShader *shader = declManager->FindSound( pdaAudio );
			int ms;
			StartSoundShader( shader, SND_CHANNEL_PDA, 0, false, &ms );
			StartAudioLog();
			CancelEvents( &EV_Player_StopAudioLog );
			PostEventMS( &EV_Player_StopAudioLog, ms + 150 );
		}
		return true;
	}

	if ( token.Icmp( "stoppdaaudio" ) == 0 ) {
		if ( objectiveSystem && objectiveSystemOpen && pdaAudio.Length() > 0 ) {
			// idSoundShader *shader = declManager->FindSound( pdaAudio );
			StopAudioLog();
			StopSound( SND_CHANNEL_PDA, false );
		}
		return true;
	}

	src->UnreadToken( &token );
	return false;
}

/*
==============
idPlayer::Collide
==============
*/

bool idPlayer::Collide( const trace_t &collision, const idVec3 &velocity ) {
	if( collision.fraction == 1.0f || collision.c.type == CONTACT_NONE // didnt hit anything
		|| gameLocal.isClient // server computes
		)
	{
		return false;
	}
	idEntity *other = gameLocal.entities[ collision.c.entityNum ];
	// don't let player collide with grabber entity
	if ( other && other != g_Global.m_DarkModPlayer->grabber->GetSelected() ) 
	{
		ProcCollisionStims( other );

		other->Signal( SIG_TOUCH );
		if ( !spectating ) {
			if ( other->RespondsTo( EV_Touch ) ) {
				other->ProcessEvent( &EV_Touch, this, &collision );
			}
		} else {
			if ( other->RespondsTo( EV_SpectatorTouch ) ) {
				other->ProcessEvent( &EV_SpectatorTouch, this, &collision );
			}
		}
	}
	return false;
}

/*
================
idPlayer::UpdateLocation

Searches nearby locations 
================
*/
void idPlayer::UpdateLocation( void ) {
	if ( hud ) {
		idLocationEntity *locationEntity = gameLocal.LocationForPoint( GetEyePosition() );
		if ( locationEntity ) {
			hud->SetStateString( "location", locationEntity->GetLocation() );
		} else {
			hud->SetStateString( "location", common->GetLanguageDict()->GetString( "#str_02911" ) );
		}
	}
}

/*
================
idPlayer::ClearFocus

Clears the focus cursor
================
*/
void idPlayer::ClearFocus( void ) {
	focusCharacter	= NULL;
	focusGUIent		= NULL;
	focusUI			= NULL;
	focusVehicle	= NULL;
	talkCursor		= 0;
}

/*
================
idPlayer::UpdateFocus

Searches nearby entities for interactive guis, possibly making one of them
the focus and sending it a mouse move event
================
*/
void idPlayer::UpdateFocus( void ) {
	idClipModel *clipModelList[ MAX_GENTITIES ];
	idClipModel *clip;
	int			listedClipModels;
	idEntity	*oldFocus;
	idEntity	*ent;
	idUserInterface *oldUI;
	idAI		*oldChar;
	int			oldTalkCursor;
	idAFEntity_Vehicle *oldVehicle;
	int			i;
	idVec3		start, end;
	bool		allowFocus;
	const char *command;
	trace_t		trace;
	guiPoint_t	pt;
	sysEvent_t	ev;
	idUserInterface *ui;

	if ( gameLocal.inCinematic ) {
		return;
	}

	// only update the focus character when attack button isn't pressed so players
	// can still chainsaw NPC's
	if ( gameLocal.isMultiplayer || ( !focusCharacter && ( usercmd.buttons & BUTTON_ATTACK ) ) ) {
		allowFocus = false;
	} else {
		allowFocus = true;
	}

	oldFocus		= focusGUIent;
	oldUI			= focusUI;
	oldChar			= focusCharacter;
	oldTalkCursor	= talkCursor;
	oldVehicle		= focusVehicle;

	if ( focusTime <= gameLocal.time ) {
		ClearFocus();
	}

	// don't let spectators interact with GUIs
	if ( spectating ) {
		return;
	}

	start = GetEyePosition();
	end = start + viewAngles.ToForward() * 80.0f;

	// player identification -> names to the hud
	if ( gameLocal.isMultiplayer && entityNumber == gameLocal.localClientNum ) {
		idVec3 end = start + viewAngles.ToForward() * 768.0f;
		gameLocal.clip.TracePoint( trace, start, end, MASK_SHOT_BOUNDINGBOX, this );
		int iclient = -1;
		if ( ( trace.fraction < 1.0f ) && ( trace.c.entityNum < MAX_CLIENTS ) ) {
			iclient = trace.c.entityNum;
		}
		if ( MPAim != iclient ) {
			lastMPAim = MPAim;
			MPAim = iclient;
			lastMPAimTime = gameLocal.realClientTime;
		}
	}

	idBounds bounds( start );
	bounds.AddPoint( end );

	listedClipModels = gameLocal.clip.ClipModelsTouchingBounds( bounds, -1, clipModelList, MAX_GENTITIES );

	// no pretense at sorting here, just assume that there will only be one active
	// gui within range along the trace
	for ( i = 0; i < listedClipModels; i++ ) {
		clip = clipModelList[ i ];
		ent = clip->GetEntity();

		if ( ent->IsHidden() ) {
			continue;
		}

		if ( allowFocus ) {
			if ( ent->IsType( idAFAttachment::Type ) ) {
				idEntity *body = static_cast<idAFAttachment *>( ent )->GetBody();
				if ( body && body->IsType( idAI::Type ) && ( static_cast<idAI *>( body )->GetTalkState() >= TALK_OK ) ) {
					gameLocal.clip.TracePoint( trace, start, end, MASK_SHOT_RENDERMODEL, this );
					if ( ( trace.fraction < 1.0f ) && ( trace.c.entityNum == ent->entityNumber ) ) {
						ClearFocus();
						focusCharacter = static_cast<idAI *>( body );
						talkCursor = 1;
						focusTime = gameLocal.time + FOCUS_TIME;
						break;
					}
				}
				continue;
			}

			if ( ent->IsType( idAI::Type ) ) {
				if ( static_cast<idAI *>( ent )->GetTalkState() >= TALK_OK ) {
					gameLocal.clip.TracePoint( trace, start, end, MASK_SHOT_RENDERMODEL, this );
					if ( ( trace.fraction < 1.0f ) && ( trace.c.entityNum == ent->entityNumber ) ) {
						ClearFocus();
						focusCharacter = static_cast<idAI *>( ent );
						talkCursor = 1;
						focusTime = gameLocal.time + FOCUS_TIME;
						break;
					}
				}
				continue;
			}

			if ( ent->IsType( idAFEntity_Vehicle::Type ) ) {
				gameLocal.clip.TracePoint( trace, start, end, MASK_SHOT_RENDERMODEL, this );
				if ( ( trace.fraction < 1.0f ) && ( trace.c.entityNum == ent->entityNumber ) ) {
					ClearFocus();
					focusVehicle = static_cast<idAFEntity_Vehicle *>( ent );
					focusTime = gameLocal.time + FOCUS_TIME;
					break;
				}
				continue;
			}
		}

		if ( !ent->GetRenderEntity() || !ent->GetRenderEntity()->gui[ 0 ] || !ent->GetRenderEntity()->gui[ 0 ]->IsInteractive() ) {
			continue;
		}

		if ( ent->spawnArgs.GetBool( "inv_item" ) ) {
			// don't allow guis on pickup items focus
			continue;
		}

		pt = gameRenderWorld->GuiTrace( ent->GetModelDefHandle(), start, end );
		if ( pt.x != -1 ) {
			// we have a hit
			renderEntity_t *focusGUIrenderEntity = ent->GetRenderEntity();
			if ( !focusGUIrenderEntity ) {
				continue;
			}

			if ( pt.guiId == 1 ) {
				ui = focusGUIrenderEntity->gui[ 0 ];
			} else if ( pt.guiId == 2 ) {
				ui = focusGUIrenderEntity->gui[ 1 ];
			} else {
				ui = focusGUIrenderEntity->gui[ 2 ];
			}
			
			if ( ui == NULL ) {
				continue;
			}

			ClearFocus();
			focusGUIent = ent;
			focusUI = ui;

			// clamp the mouse to the corner
			ev = sys->GenerateMouseMoveEvent( -2000, -2000 );
			command = focusUI->HandleEvent( &ev, gameLocal.time );
 			HandleGuiCommands( focusGUIent, command );

			// move to an absolute position
			ev = sys->GenerateMouseMoveEvent( pt.x * SCREEN_WIDTH, pt.y * SCREEN_HEIGHT );
			command = focusUI->HandleEvent( &ev, gameLocal.time );
			HandleGuiCommands( focusGUIent, command );
			focusTime = gameLocal.time + FOCUS_GUI_TIME;
			break;
		}
	}

	if ( focusGUIent && focusUI ) {
		if ( !oldFocus || oldFocus != focusGUIent ) {
			command = focusUI->Activate( true, gameLocal.time );
			HandleGuiCommands( focusGUIent, command );
			StartSound( "snd_guienter", SND_CHANNEL_ANY, 0, false, NULL );
			// HideTip();
			// HideObjective();
		}
	} else if ( oldFocus && oldUI ) {
		command = oldUI->Activate( false, gameLocal.time );
		HandleGuiCommands( oldFocus, command );
		StartSound( "snd_guiexit", SND_CHANNEL_ANY, 0, false, NULL );
	}

	if ( cursor && ( oldTalkCursor != talkCursor ) ) {
		cursor->SetStateInt( "talkcursor", talkCursor );
	}

	if ( oldChar != focusCharacter && hud ) {
		if ( focusCharacter ) {
			hud->SetStateString( "npc", focusCharacter->spawnArgs.GetString( "npc_name", "Joe" ) );
			hud->HandleNamedEvent( "showNPC" );
			// HideTip();
			// HideObjective();
		} else {
			hud->SetStateString( "npc", "" );
			hud->HandleNamedEvent( "hideNPC" );
		}
	}
}

/*
=================
idPlayer::CrashLand

Check for hard landings that generate sound events
=================
*/

void idPlayer::CrashLand( const idVec3 &savedOrigin, const idVec3 &savedVelocity ) {
	
	AI_SOFTLANDING = false;
	AI_HARDLANDING = false;
	float delta = idActor::CrashLand( physicsObj, savedOrigin, savedVelocity );
	

	if ( delta > m_delta_fatal )
	{
		AI_HARDLANDING = true;
		landChange = -32;
		landTime = gameLocal.time;
	}
	else if ( delta > m_delta_min )
	{
		AI_HARDLANDING = true;
		landChange = -24;
		landTime = gameLocal.time;
	}
	else if ( delta > (m_delta_min / 4.0f) )
	{
		AI_SOFTLANDING = true;
		landChange	= -8;
		landTime = gameLocal.time;
	}
	// otherwise, just walk on
	
}

/*
===============
idPlayer::BobCycle
===============
*/
void idPlayer::BobCycle( const idVec3 &pushVelocity ) {
	float		bobmove;
	int			old, deltaTime;
	idVec3		vel, gravityDir, velocity;
	idMat3		viewaxis;
	float		bob;
	float		delta;
	float		speed;
	float		f;

	//
	// calculate speed and cycle to be used for
	// all cyclic walking effects
	//
	velocity = physicsObj.GetLinearVelocity() - pushVelocity;

	gravityDir = physicsObj.GetGravityNormal();
	vel = velocity - ( velocity * gravityDir ) * gravityDir;
	xyspeed = vel.LengthFast();

	// do not evaluate the bob for other clients
	// when doing a spectate follow, don't do any weapon bobbing
	if ( gameLocal.isClient && entityNumber != gameLocal.localClientNum ) {
		viewBobAngles.Zero();
		viewBob.Zero();
		return;
	}

	if ( !physicsObj.HasGroundContacts() || influenceActive == INFLUENCE_LEVEL2 || ( gameLocal.isMultiplayer && spectating ) ) {
		// airborne
		bobCycle = 0;
		bobFoot = 0;
		bobfracsin = 0;
	} else if ( ( !usercmd.forwardmove && !usercmd.rightmove ) || ( xyspeed <= MIN_BOB_SPEED ) ) {
		// start at beginning of cycle again
		bobCycle = 0;
		bobFoot = 0;
		bobfracsin = 0;
	} else {
		if ( physicsObj.IsCrouching() ) {
			bobmove = pm_crouchbob.GetFloat();
			// ducked characters never play footsteps
		} else {
			// vary the bobbing based on the speed of the player
			bobmove = pm_walkbob.GetFloat() * ( 1.0f - bobFrac ) + pm_runbob.GetFloat() * bobFrac;
		}

		// check for footstep / splash sounds
		old = bobCycle;
		bobCycle = (int)( old + bobmove * gameLocal.msec ) & 255;
		bobFoot = ( bobCycle & 128 ) >> 7;
		bobfracsin = idMath::Fabs( sin( ( bobCycle & 127 ) / 127.0 * idMath::PI ) );
	}

	// calculate angles for view bobbing
	viewBobAngles.Zero();

	viewaxis = viewAngles.ToMat3() * physicsObj.GetGravityAxis();

	// add angles based on velocity
	delta = velocity * viewaxis[0];
	viewBobAngles.pitch += delta * pm_runpitch.GetFloat();
	
	delta = velocity * viewaxis[1];
	viewBobAngles.roll -= delta * pm_runroll.GetFloat();

	// add angles based on bob
	// make sure the bob is visible even at low speeds
	speed = xyspeed > 200 ? xyspeed : 200;

	delta = bobfracsin * pm_bobpitch.GetFloat() * speed;
	if ( physicsObj.IsCrouching() ) {
		delta *= 3;		// crouching
	}
	viewBobAngles.pitch += delta;
	delta = bobfracsin * pm_bobroll.GetFloat() * speed;
	if ( physicsObj.IsCrouching() ) {
		delta *= 3;		// crouching accentuates roll
	}
	if ( bobFoot & 1 ) {
		delta = -delta;
	}
	viewBobAngles.roll += delta;

	// calculate position for view bobbing
	viewBob.Zero();

	if ( physicsObj.HasSteppedUp() ) {

		// check for stepping up before a previous step is completed
		deltaTime = gameLocal.time - stepUpTime;
		if ( deltaTime < STEPUP_TIME ) {
			stepUpDelta = stepUpDelta * ( STEPUP_TIME - deltaTime ) / STEPUP_TIME + physicsObj.GetStepUp();
		} else {
			stepUpDelta = physicsObj.GetStepUp();
		}
		if ( stepUpDelta > 2.0f * pm_stepsize.GetFloat() ) {
			stepUpDelta = 2.0f * pm_stepsize.GetFloat();
		}
		stepUpTime = gameLocal.time;
	}

	idVec3 gravity = physicsObj.GetGravityNormal();

	// if the player stepped up recently
	deltaTime = gameLocal.time - stepUpTime;
	if ( deltaTime < STEPUP_TIME ) {
		viewBob += gravity * ( stepUpDelta * ( STEPUP_TIME - deltaTime ) / STEPUP_TIME );
	}

	// add bob height after any movement smoothing
	bob = bobfracsin * xyspeed * pm_bobup.GetFloat();
	if ( bob > 6 ) {
		bob = 6;
	}
	viewBob[2] += bob;

	// add fall height
	delta = gameLocal.time - landTime;
	if ( delta < LAND_DEFLECT_TIME ) {
		f = delta / LAND_DEFLECT_TIME;
		viewBob -= gravity * ( landChange * f );
	} else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME ) {
		delta -= LAND_DEFLECT_TIME;
		f = 1.0 - ( delta / LAND_RETURN_TIME );
		viewBob -= gravity * ( landChange * f );
	}
}

/*
================
idPlayer::UpdateDeltaViewAngles
================
*/
void idPlayer::UpdateDeltaViewAngles( const idAngles &angles ) {
	// set the delta angle
	idAngles delta;
	for( int i = 0; i < 3; i++ ) {
		delta[ i ] = angles[ i ] - SHORT2ANGLE( usercmd.angles[ i ] );
	}

	SetDeltaViewAngles( delta );
}

/*
================
idPlayer::SetViewAngles
================
*/
void idPlayer::SetViewAngles( const idAngles &angles ) {
	UpdateDeltaViewAngles( angles );
	viewAngles = angles;
}

/*
================
idPlayer::UpdateViewAngles
================
*/
void idPlayer::UpdateViewAngles( void ) 
{
	int i;
	idAngles delta;
	idAngles TestAngles = viewAngles;

	if ( !noclip && ( gameLocal.inCinematic || privateCameraView || gameLocal.GetCamera() || influenceActive == INFLUENCE_LEVEL2 || objectiveSystemOpen || GetImmobilization() & EIM_VIEW_ANGLE ) ) 
	{
		// no view changes at all, but we still want to update the deltas or else when
		// we get out of this mode, our view will snap to a kind of random angle
		UpdateDeltaViewAngles( viewAngles );
		goto Quit;
	}

	// if dead
	if ( health <= 0 ) {
		if ( pm_thirdPersonDeath.GetBool() ) {
			viewAngles.roll = 0.0f;
			viewAngles.pitch = 30.0f;
		} else {
			viewAngles.roll = 40.0f;
			viewAngles.pitch = -15.0f;
		}
		goto Quit;
	}

	// circularly clamp the angles with deltas
	for ( i = 0; i < 3; i++ ) 
	{
		cmdAngles[i] = SHORT2ANGLE( usercmd.angles[i] );
		if ( influenceActive == INFLUENCE_LEVEL3 ) 
		{
			TestAngles[i] += idMath::ClampFloat( -1.0f, 1.0f, idMath::AngleDelta( idMath::AngleNormalize180( SHORT2ANGLE( usercmd.angles[i]) + deltaViewAngles[i] ) , viewAngles[i] ) );
		} else 
		{
			TestAngles[i] = idMath::AngleNormalize180( SHORT2ANGLE( usercmd.angles[i]) + deltaViewAngles[i] );
		}
	}
	if ( !centerView.IsDone( gameLocal.time ) ) {
		TestAngles.pitch = centerView.GetCurrentValue(gameLocal.time);
	}

	// clamp the pitch
	if ( noclip ) {
		if ( TestAngles.pitch > 89.0f ) {
			// don't let the player look down more than 89 degrees while noclipping
			TestAngles.pitch = 89.0f;
		} else if ( TestAngles.pitch < -89.0f ) {
			// don't let the player look up more than 89 degrees while noclipping
			TestAngles.pitch = -89.0f;
		}
	} else {
		if ( TestAngles.pitch > pm_maxviewpitch.GetFloat() ) {
			// don't let the player look down enough to see the shadow of his (non-existant) feet
			TestAngles.pitch = pm_maxviewpitch.GetFloat();
		} else if ( TestAngles.pitch < pm_minviewpitch.GetFloat() ) {
			// don't let the player look up more than 89 degrees
			TestAngles.pitch = pm_minviewpitch.GetFloat();
		}
	}

	// TDM: Check for collisions due to delta yaw when leaning, overwrite test angles to avoid
	physicsObj.UpdateLeanedInputYaw( TestAngles );
	viewAngles = TestAngles;

	UpdateDeltaViewAngles( viewAngles );

	// orient the model towards the direction we're looking
	// LeanMod: SophisticatedZombie: Added roll to this
	SetAngles( idAngles( 0.0f, viewAngles.yaw, viewAngles.roll ) );

	// save in the log for analyzing weapon angle offsets
	loggedViewAngles[ gameLocal.framenum & (NUM_LOGGED_VIEW_ANGLES-1) ] = viewAngles;

Quit:
	return;
}

/*
==============
idPlayer::AdjustHeartRate

Player heartrate works as follows

DEF_HEARTRATE is resting heartrate

Taking damage when health is above 75 adjusts heart rate by 1 beat per second
Taking damage when health is below 75 adjusts heart rate by 5 beats per second
Maximum heartrate from damage is MAX_HEARTRATE

Firing a weapon adds 1 beat per second up to a maximum of COMBAT_HEARTRATE

Being at less than 25% stamina adds 5 beats per second up to ZEROSTAMINA_HEARTRATE

All heartrates are target rates.. the heart rate will start falling as soon as there have been no adjustments for 5 seconds
Once it starts falling it always tries to get to DEF_HEARTRATE

The exception to the above rule is upon death at which point the rate is set to DYING_HEARTRATE and starts falling 
immediately to zero

Heart rate volumes go from zero ( -40 db for DEF_HEARTRATE to 5 db for MAX_HEARTRATE ) the volume is 
scaled linearly based on the actual rate

Exception to the above rule is once the player is dead, the dying heart rate starts at either the current volume if
it is audible or -10db and scales to 8db on the last few beats
==============
*/
void idPlayer::AdjustHeartRate( int target, float timeInSecs, float delay, bool force ) {

	if ( heartInfo.GetEndValue() == target ) {
		return;
	}

	if ( AI_DEAD && !force ) {
		return;
	}

    lastHeartAdjust = gameLocal.time;

	heartInfo.Init( gameLocal.time + delay * 1000, timeInSecs * 1000, heartRate, target );
}

/*
==============
idPlayer::GetBaseHeartRate
==============
*/
int idPlayer::GetBaseHeartRate( void ) {
	/*
	int base = idMath::FtoiFast( ( BASE_HEARTRATE + LOWHEALTH_HEARTRATE_ADJ ) - ( (float)health / 100.0f ) * LOWHEALTH_HEARTRATE_ADJ );
	int rate = idMath::FtoiFast( base + ( ZEROSTAMINA_HEARTRATE - base ) * ( 1.0f - stamina / pm_stamina.GetFloat() ) );
	int diff = ( lastDmgTime ) ? gameLocal.time - lastDmgTime : 99999;
	rate += ( diff < 5000 ) ? ( diff < 2500 ) ? ( diff < 1000 ) ? 15 : 10 : 5 : 0;
	return rate;
	*/
	return BASE_HEARTRATE;
}

/*
==============
idPlayer::SetCurrentHeartRate
==============
*/
void idPlayer::SetCurrentHeartRate( void ) {
	/// reasons why we should exit
	if( false == airless && health > 0 )
    {
		if( true == m_HeartBeatAllow )
		{
			AdjustHeartRate( BASE_HEARTRATE, 5.5f, 0.0f, false );/// We were allowing so fade it
		}
		else /// We were NOT allowing it so set to default
		{
			heartRate = BASE_HEARTRATE;
		}
		m_HeartBeatAllow = false;
        return;
    }
	/*
	if( false == m_HeartBeatAllow )/// we did not want heartbeat heard so make sure
    {
		heartInfo.Init( gameLocal.time, 0, BASE_HEARTRATE, BASE_HEARTRATE );
		heartRate = BASE_HEARTRATE;
		StopSound( SND_CHANNEL_HEART, false );
    }
	*/
	m_HeartBeatAllow = true;


	int base = idMath::FtoiFast( ( BASE_HEARTRATE + LOWHEALTH_HEARTRATE_ADJ ) - ( (float) health / 100.0f ) * LOWHEALTH_HEARTRATE_ADJ );

	/// removed adrenaline affect - Rich
	heartRate = idMath::FtoiFast( heartInfo.GetCurrentValue( gameLocal.time ) );
	int currentRate = GetBaseHeartRate();
	if ( health >= 0 && gameLocal.time > lastHeartAdjust + 2500 ) {
		AdjustHeartRate( currentRate, 2.5f, 0.0f, false );
	}
	int bps = idMath::FtoiFast( 60.0f / heartRate * 1000.0f );
	if ( gameLocal.time - lastHeartBeat > bps ) {
		int dmgVol = DMG_VOLUME;
		int deathVol = DEATH_VOLUME;
		int zeroVol = ZERO_VOLUME;
		float pct = 0.0;
		if ( heartRate > BASE_HEARTRATE && health > 0 ) {
			pct = (float)(heartRate - base) / (MAX_HEARTRATE - base);
			pct *= ((float)dmgVol - (float)zeroVol);
		} else if ( health <= 0 ) {
			pct = (float)(heartRate - DYING_HEARTRATE) / (BASE_HEARTRATE - DYING_HEARTRATE);
			if ( pct > 1.0f ) {
				pct = 1.0f;
			} else if (pct < 0.0f) {
				pct = 0.0f;
			}
			pct *= ((float)deathVol - (float)zeroVol);
		} 

		pct += (float)zeroVol;

		if ( pct != zeroVol ) {
			StartSound( "snd_heartbeat", SND_CHANNEL_HEART, SSF_PRIVATE_SOUND, false, NULL );
			// modify just this channel to a custom volume
			soundShaderParms_t	parms;
			memset( &parms, 0, sizeof( parms ) );
			parms.volume = pct;
			refSound.referenceSound->ModifySound( SND_CHANNEL_HEART, &parms );
		}

		lastHeartBeat = gameLocal.time;
	}
}

/*
==============
idPlayer::UpdateAir
==============
*/
void idPlayer::UpdateAir( void ) {	
	if ( health <= 0 ) {
		return;
	}

#ifdef MOD_WATERPHYSICS

	idPhysics_Player *phys = dynamic_cast<idPhysics_Player *>(this->GetPhysics());   // MOD_WATERPHYSICS

#endif		// MOD_WATERPHYSICS


	// see if the player is connected to the info_vacuum
	bool	newAirless = false;

	if ( gameLocal.vacuumAreaNum != -1 ) {
		int	num = GetNumPVSAreas();
		if ( num > 0 ) {
			int		areaNum;

			// if the player box spans multiple areas, get the area from the origin point instead,
			// otherwise a rotating player box may poke into an outside area
			if ( num == 1 ) {
				const int	*pvsAreas = GetPVSAreas();
				areaNum = pvsAreas[0];
			} else {
				areaNum = gameRenderWorld->PointInArea( this->GetPhysics()->GetOrigin() );
			}
			newAirless = gameRenderWorld->AreasAreConnected( gameLocal.vacuumAreaNum, areaNum, PS_BLOCK_AIR );
		}
	}

#ifdef MOD_WATERPHYSICS // check if the player is in water

	if( phys != NULL && phys->GetWaterLevel() >= WATERLEVEL_HEAD )      // MOD_WATERPHYSICS

		newAirless = true;	// MOD_WATERPHYSICS

#endif		// MOD_WATERPHYSICS


	if ( newAirless ) {
		if ( !airless ) {
			StartSound( "snd_decompress", SND_CHANNEL_ANY, SSF_GLOBAL, false, NULL );
			StartSound( "snd_noAir", SND_CHANNEL_BODY2, 0, false, NULL );
			if ( hud ) {
				hud->HandleNamedEvent( "noAir" );
			}
		}
		airTics--;
		if ( airTics < 0 ) {
			airTics = 0;
			// check for damage
			const idDict *damageDef = gameLocal.FindEntityDefDict( "damage_noair", false );
			int dmgTiming = 1000 * ((damageDef) ? damageDef->GetFloat( "delay", "3.0" ) : 3.0f );
			if ( gameLocal.time > lastAirDamage + dmgTiming ) {
				Damage( NULL, NULL, vec3_origin, "damage_noair", 1.0f, 0 );
				lastAirDamage = gameLocal.time;
			}
		}
		
	} else {
		if ( airless ) {
			StartSound( "snd_recompress", SND_CHANNEL_ANY, SSF_GLOBAL, false, NULL );
			StopSound( SND_CHANNEL_BODY2, false );
			if ( hud ) {
				hud->HandleNamedEvent( "Air" );
			}
		}
		airTics+=2;	// regain twice as fast as lose
		if ( airTics > pm_airTics.GetInteger() ) {
			airTics = pm_airTics.GetInteger();
		}
	}

	airless = newAirless;

	if ( hud ) {
		hud->SetStateInt( "player_air", 100 * airTics / pm_airTics.GetInteger() );
	}
}

int	idPlayer::getAirTicks() const {
	return airTics;
}

void idPlayer::setAirTicks(int airTicks) {
	airTics = airTicks;
	// Clamp to maximum value
	if( airTics > pm_airTics.GetInteger() ) {
		airTics = pm_airTics.GetInteger();
	}
}

/*
==============
idPlayer::AddGuiPDAData
==============
 */
int idPlayer::AddGuiPDAData( const declType_t dataType, const char *listName, const idDeclPDA *src, idUserInterface *gui ) {
	int c, i;
	idStr work;
	if ( dataType == DECL_EMAIL ) {
		c = src->GetNumEmails();
		for ( i = 0; i < c; i++ ) {
			const idDeclEmail *email = src->GetEmailByIndex( i );
			if ( email == NULL ) {
				work = va( "-\tEmail %d not found\t-", i );
			} else {
				work = email->GetFrom();
				work += "\t";
				work += email->GetSubject();
				work += "\t";
				work += email->GetDate();
			}
			gui->SetStateString( va( "%s_item_%i", listName, i ), work );
		}
		return c;
	} else if ( dataType == DECL_AUDIO ) {
		c = src->GetNumAudios();
		for ( i = 0; i < c; i++ ) {
			const idDeclAudio *audio = src->GetAudioByIndex( i );
			if ( audio == NULL ) {
				work = va( "Audio Log %d not found", i );
			} else {
				work = audio->GetAudioName();
			}
			gui->SetStateString( va( "%s_item_%i", listName, i ), work );
		}
		return c;
	} else if ( dataType == DECL_VIDEO ) {
		c = inventory.videos.Num();
		for ( i = 0; i < c; i++ ) {
			const idDeclVideo *video = GetVideo( i );
			if ( video == NULL ) {
				work = va( "Video CD %s not found", inventory.videos[i].c_str() );
			} else {
				work = video->GetVideoName();
			}
			gui->SetStateString( va( "%s_item_%i", listName, i ), work );
		}
		return c;
	}
	return 0;
}

/*
==============
idPlayer::GetPDA
==============
 */
const idDeclPDA *idPlayer::GetPDA( void ) const {
	if ( inventory.pdas.Num() ) {
		return static_cast< const idDeclPDA* >( declManager->FindType( DECL_PDA, inventory.pdas[ 0 ] ) );
	} else {
		return NULL;
	}
}


/*
==============
idPlayer::GetVideo
==============
*/
const idDeclVideo *idPlayer::GetVideo( int index ) {
	if ( index >= 0 && index < inventory.videos.Num() ) {
		return static_cast< const idDeclVideo* >( declManager->FindType( DECL_VIDEO, inventory.videos[index], false ) );
	}
	return NULL;
}


/*
==============
idPlayer::UpdatePDAInfo
==============
*/
void idPlayer::UpdatePDAInfo( bool updatePDASel ) {
	int j, sel;

	if ( objectiveSystem == NULL ) {
		return;
	}

	assert( hud );

	int currentPDA = objectiveSystem->State().GetInt( "listPDA_sel_0", "0" );
	if ( currentPDA == -1 ) {
		currentPDA = 0;
	}

	if ( updatePDASel ) {
		objectiveSystem->SetStateInt( "listPDAVideo_sel_0", 0 );
		objectiveSystem->SetStateInt( "listPDAEmail_sel_0", 0 );
		objectiveSystem->SetStateInt( "listPDAAudio_sel_0", 0 );
	}

	if ( currentPDA > 0 ) {
		currentPDA = inventory.pdas.Num() - currentPDA;
	}

	// Mark in the bit array that this pda has been read
	if ( currentPDA < 128 ) {
		inventory.pdasViewed[currentPDA >> 5] |= 1 << (currentPDA & 31);
	}

	pdaAudio = "";
	pdaVideo = "";
	pdaVideoWave = "";
	idStr name, data, preview, info, wave;
	for ( j = 0; j < MAX_PDAS; j++ ) {
		objectiveSystem->SetStateString( va( "listPDA_item_%i", j ), "" );
	}
	for ( j = 0; j < MAX_PDA_ITEMS; j++ ) {
		objectiveSystem->SetStateString( va( "listPDAVideo_item_%i", j ), "" );
		objectiveSystem->SetStateString( va( "listPDAAudio_item_%i", j ), "" );
		objectiveSystem->SetStateString( va( "listPDAEmail_item_%i", j ), "" );
		objectiveSystem->SetStateString( va( "listPDASecurity_item_%i", j ), "" );
	}
	for ( j = 0; j < inventory.pdas.Num(); j++ ) {

		const idDeclPDA *pda = static_cast< const idDeclPDA* >( declManager->FindType( DECL_PDA, inventory.pdas[j], false ) );

		if ( pda == NULL ) {
			continue;
		}

		int index = inventory.pdas.Num() - j;
		if ( j == 0 ) {
			// Special case for the first PDA
			index = 0;
		}

		if ( j != currentPDA && j < 128 && inventory.pdasViewed[j >> 5] & (1 << (j & 31)) ) {
			// This pda has been read already, mark in gray
			objectiveSystem->SetStateString( va( "listPDA_item_%i", index), va(S_COLOR_GRAY "%s", pda->GetPdaName()) );
		} else {
			// This pda has not been read yet
		objectiveSystem->SetStateString( va( "listPDA_item_%i", index), pda->GetPdaName() );
		}

		const char *security = pda->GetSecurity();
		if ( j == currentPDA || (currentPDA == 0 && security && *security ) ) {
			if ( *security == NULL ) {
				security = common->GetLanguageDict()->GetString( "#str_00066" );
			}
			objectiveSystem->SetStateString( "PDASecurityClearance", security );
		}

		if ( j == currentPDA ) {

			objectiveSystem->SetStateString( "pda_icon", pda->GetIcon() );
			objectiveSystem->SetStateString( "pda_id", pda->GetID() );
			objectiveSystem->SetStateString( "pda_title", pda->GetTitle() );

			if ( j == 0 ) {
				// Selected, personal pda
				// Add videos
				if ( updatePDASel || !inventory.pdaOpened ) {
				objectiveSystem->HandleNamedEvent( "playerPDAActive" );
				objectiveSystem->SetStateString( "pda_personal", "1" );
					inventory.pdaOpened = true;
				}
				objectiveSystem->SetStateString( "pda_location", hud->State().GetString("location") );
				objectiveSystem->SetStateString( "pda_name", cvarSystem->GetCVarString( "ui_name") );
				AddGuiPDAData( DECL_VIDEO, "listPDAVideo", pda, objectiveSystem );
				sel = objectiveSystem->State().GetInt( "listPDAVideo_sel_0", "0" );
				const idDeclVideo *vid = NULL;
				if ( sel >= 0 && sel < inventory.videos.Num() ) {
					vid = static_cast< const idDeclVideo * >( declManager->FindType( DECL_VIDEO, inventory.videos[ sel ], false ) );
				}
				if ( vid ) {
					pdaVideo = vid->GetRoq();
					pdaVideoWave = vid->GetWave();
					objectiveSystem->SetStateString( "PDAVideoTitle", vid->GetVideoName() );
					objectiveSystem->SetStateString( "PDAVideoVid", vid->GetRoq() );
					objectiveSystem->SetStateString( "PDAVideoIcon", vid->GetPreview() );
					objectiveSystem->SetStateString( "PDAVideoInfo", vid->GetInfo() );
				} else {
					//FIXME: need to precache these in the player def
					objectiveSystem->SetStateString( "PDAVideoVid", "sound/vo/video/welcome.tga" );
					objectiveSystem->SetStateString( "PDAVideoIcon", "sound/vo/video/welcome.tga" );
					objectiveSystem->SetStateString( "PDAVideoTitle", "" );
					objectiveSystem->SetStateString( "PDAVideoInfo", "" );
				}
			} else {
				// Selected, non-personal pda
				// Add audio logs
				if ( updatePDASel ) {
				objectiveSystem->HandleNamedEvent( "playerPDANotActive" );
				objectiveSystem->SetStateString( "pda_personal", "0" );
					inventory.pdaOpened = true;
				}
				objectiveSystem->SetStateString( "pda_location", pda->GetPost() );
				objectiveSystem->SetStateString( "pda_name", pda->GetFullName() );
				int audioCount = AddGuiPDAData( DECL_AUDIO, "listPDAAudio", pda, objectiveSystem );
				objectiveSystem->SetStateInt( "audioLogCount", audioCount );
				sel = objectiveSystem->State().GetInt( "listPDAAudio_sel_0", "0" );
				const idDeclAudio *aud = NULL;
				if ( sel >= 0 ) {
					aud = pda->GetAudioByIndex( sel );
				}
				if ( aud ) {
					pdaAudio = aud->GetWave();
					objectiveSystem->SetStateString( "PDAAudioTitle", aud->GetAudioName() );
					objectiveSystem->SetStateString( "PDAAudioIcon", aud->GetPreview() );
					objectiveSystem->SetStateString( "PDAAudioInfo", aud->GetInfo() );
				} else {
					objectiveSystem->SetStateString( "PDAAudioIcon", "sound/vo/video/welcome.tga" );
					objectiveSystem->SetStateString( "PDAAutioTitle", "" );
					objectiveSystem->SetStateString( "PDAAudioInfo", "" );
				}
			}
			// add emails
			name = "";
			data = "";
			int numEmails = pda->GetNumEmails();
			if ( numEmails > 0 ) {
				AddGuiPDAData( DECL_EMAIL, "listPDAEmail", pda, objectiveSystem );
				sel = objectiveSystem->State().GetInt( "listPDAEmail_sel_0", "-1" );
				if ( sel >= 0 && sel < numEmails ) {
					const idDeclEmail *email = pda->GetEmailByIndex( sel );
					name = email->GetSubject();
					data = email->GetBody();
				}
			}
			objectiveSystem->SetStateString( "PDAEmailTitle", name );
			objectiveSystem->SetStateString( "PDAEmailText", data );
		}
	}
	if ( objectiveSystem->State().GetInt( "listPDA_sel_0", "-1" ) == -1 ) {
		objectiveSystem->SetStateInt( "listPDA_sel_0", 0 );
	}
	objectiveSystem->StateChanged( gameLocal.time );
}

/*
==============
idPlayer::TogglePDA
==============
*/
void idPlayer::TogglePDA( void ) {
	if ( objectiveSystem == NULL ) {
		return;
	}

	if ( inventory.pdas.Num() == 0 ) {
		ShowTip( spawnArgs.GetString( "text_infoTitle" ), spawnArgs.GetString( "text_noPDA" ), true );
		return;
	}

	assert( hud );

	if ( !objectiveSystemOpen ) {
		int j, c = inventory.items.Num();
		objectiveSystem->SetStateInt( "inv_count", c );
		for ( j = 0; j < MAX_INVENTORY_ITEMS; j++ ) {
			objectiveSystem->SetStateString( va( "inv_name_%i", j ), "" );
			objectiveSystem->SetStateString( va( "inv_icon_%i", j ), "" );
			objectiveSystem->SetStateString( va( "inv_text_%i", j ), "" );
		}
		for ( j = 0; j < c; j++ ) {
			idDict *item = inventory.items[j];
			if ( !item->GetBool( "inv_pda" ) ) {
				const char *iname = item->GetString( "inv_name" );
				const char *iicon = item->GetString( "inv_icon" );
				const char *itext = item->GetString( "inv_text" );
				objectiveSystem->SetStateString( va( "inv_name_%i", j ), iname );
				objectiveSystem->SetStateString( va( "inv_icon_%i", j ), iicon );
				objectiveSystem->SetStateString( va( "inv_text_%i", j ), itext );
				const idKeyValue *kv = item->MatchPrefix( "inv_id", NULL );
				if ( kv ) {
					objectiveSystem->SetStateString( va( "inv_id_%i", j ), kv->GetValue() );
				}
			}
		}

		for ( j = 0; j < MAX_WEAPONS; j++ ) {
			const char *weapnum = va( "def_weapon%d", j );
			const char *hudWeap = va( "weapon%d", j );
			int weapstate = 0;
			/*if ( inventory.weapons & ( 1 << j ) ) {
				const char *weap = spawnArgs.GetString( weapnum );
				if ( weap && *weap ) {
					weapstate++;
				}
			}*/
			objectiveSystem->SetStateInt( hudWeap, weapstate );
		}

		objectiveSystem->SetStateInt( "listPDA_sel_0", inventory.selPDA );
		objectiveSystem->SetStateInt( "listPDAVideo_sel_0", inventory.selVideo );
		objectiveSystem->SetStateInt( "listPDAAudio_sel_0", inventory.selAudio );
		objectiveSystem->SetStateInt( "listPDAEmail_sel_0", inventory.selEMail );
		UpdatePDAInfo( false );
		objectiveSystem->Activate( true, gameLocal.time );
		hud->HandleNamedEvent( "pdaPickupHide" );
		hud->HandleNamedEvent( "videoPickupHide" );
	} else {
		inventory.selPDA = objectiveSystem->State().GetInt( "listPDA_sel_0" );
		inventory.selVideo = objectiveSystem->State().GetInt( "listPDAVideo_sel_0" );
		inventory.selAudio = objectiveSystem->State().GetInt( "listPDAAudio_sel_0" );
		inventory.selEMail = objectiveSystem->State().GetInt( "listPDAEmail_sel_0" );
		objectiveSystem->Activate( false, gameLocal.time );
	}
	objectiveSystemOpen ^= 1;
}

/*
==============
idPlayer::ToggleScoreboard
==============
*/
void idPlayer::ToggleScoreboard( void ) {
	scoreBoardOpen ^= 1;
}

/*
==============
idPlayer::Spectate
==============
*/
void idPlayer::Spectate( bool spectate ) {
	idBitMsg	msg;
	byte		msgBuf[MAX_EVENT_PARAM_SIZE];

	// track invisible player bug
	// all hiding and showing should be performed through Spectate calls
	// except for the private camera view, which is used for teleports
	assert( ( teleportEntity.GetEntity() != NULL ) || ( IsHidden() == spectating ) );

	if ( spectating == spectate ) {
		return;
	}

	spectating = spectate;

	if ( gameLocal.isServer ) {
		msg.Init( msgBuf, sizeof( msgBuf ) );
		msg.WriteBits( spectating, 1 );
		ServerSendEvent( EVENT_SPECTATE, &msg, false, -1 );
	}

	if ( spectating ) {
		// join the spectators
		spectator = this->entityNumber;
		Init();
		StopRagdoll();
		SetPhysics( &physicsObj );
		physicsObj.DisableClip();
		Hide();
		Event_DisableWeapon();
		if ( hud ) {
			hud->HandleNamedEvent( "aim_clear" );
			MPAimFadeTime = 0;
		}
	} else {
		// put everything back together again
		currentWeapon = -1;	// to make sure the def will be loaded if necessary
		Show();
		Event_EnableWeapon();
	}
	SetClipModel();
}

/*
==============
idPlayer::SetClipModel
==============
*/
void idPlayer::SetClipModel( void ) {
	idBounds bounds;

	if ( spectating ) {
		bounds = idBounds( vec3_origin ).Expand( pm_spectatebbox.GetFloat() * 0.5f );
	} else {
		bounds[0].Set( -pm_bboxwidth.GetFloat() * 0.5f, -pm_bboxwidth.GetFloat() * 0.5f, 0 );
		bounds[1].Set( pm_bboxwidth.GetFloat() * 0.5f, pm_bboxwidth.GetFloat() * 0.5f, pm_normalheight.GetFloat() );
	}
	// the origin of the clip model needs to be set before calling SetClipModel
	// otherwise our physics object's current origin value gets reset to 0
	idClipModel *newClip;
	if ( pm_usecylinder.GetBool() ) {
		newClip = new idClipModel( idTraceModel( bounds, 8 ) );
		newClip->Translate( physicsObj.PlayerGetOrigin() );
		physicsObj.SetClipModel( newClip, 1.0f );
	} else {
		newClip = new idClipModel( idTraceModel( bounds ) );
		newClip->Translate( physicsObj.PlayerGetOrigin() );
		physicsObj.SetClipModel( newClip, 1.0f );
	}
}

/*
==============
idPlayer::UseVehicle
==============
*/
void idPlayer::UseVehicle( void ) {
	trace_t	trace;
	idVec3 start, end;
	idEntity *ent;

	if ( GetBindMaster() && GetBindMaster()->IsType( idAFEntity_Vehicle::Type ) ) {
		Show();
		static_cast<idAFEntity_Vehicle*>(GetBindMaster())->Use( this );
	} else {
		start = GetEyePosition();
		end = start + viewAngles.ToForward() * 80.0f;
		gameLocal.clip.TracePoint( trace, start, end, MASK_SHOT_RENDERMODEL, this );
		if ( trace.fraction < 1.0f ) {
			ent = gameLocal.entities[ trace.c.entityNum ];
			if ( ent && ent->IsType( idAFEntity_Vehicle::Type ) ) {
				Hide();
				static_cast<idAFEntity_Vehicle*>(ent)->Use( this );
			}
		}
	}
}

/*
==============
idPlayer::PerformImpulse
==============
*/
void idPlayer::PerformImpulse( int impulse ) {

	if ( gameLocal.isClient ) {
		idBitMsg	msg;
		byte		msgBuf[MAX_EVENT_PARAM_SIZE];

		assert( entityNumber == gameLocal.localClientNum );
		msg.Init( msgBuf, sizeof( msgBuf ) );
		msg.BeginWriting();
		msg.WriteBits( impulse, 6 );
		ClientSendEvent( EVENT_IMPULSE, &msg );
	}

	if ( impulse >= IMPULSE_0 && impulse <= IMPULSE_12 ) 
	{
		// Prevent the player from choosing to switch weapons.
		if ( GetImmobilization() & EIM_WEAPON_SELECT ) 
		{
			return;
		}

		SelectWeapon( impulse, false );
		return;
	}

	// Register the impulse for tracking
	m_ButtonStateTracker.startTracking(impulse);

	switch( impulse )
	{
		case IMPULSE_13:
		{
			Reload();
			break;
		}

		case IMPULSE_14: 
		{
			// If the grabber is active, next weapon increments the distance
			if(m_bGrabberActive)
				g_Global.m_DarkModPlayer->grabber->IncrementDistance( false );

			// Prevent the player from choosing to switch weapons.
			if ( GetImmobilization() & EIM_WEAPON_SELECT ) 
			{
				return;
			}

			NextWeapon();
			break;
		}
		case IMPULSE_15: 
		{
			// If the grabber is active, next weapon increments the distance
			if(m_bGrabberActive)
				g_Global.m_DarkModPlayer->grabber->IncrementDistance( true );

			// Prevent the player from choosing to switch weapons.
			if ( GetImmobilization() & EIM_WEAPON_SELECT ) 
			{
				return;
			}

			PrevWeapon();
			break;
		}
		case IMPULSE_17:
		{
			if ( gameLocal.isClient || entityNumber == gameLocal.localClientNum )
			{
				gameLocal.mpGame.ToggleReady();
			}
			break;
		}

		case IMPULSE_18:
		{
			centerView.Init(gameLocal.time, 200, viewAngles.pitch, 0);
			break;
		}
		case IMPULSE_19:
		{
			// when we're not in single player, IMPULSE_19 is used for showScores
			// otherwise it opens the pda
			if ( !gameLocal.isMultiplayer ) {
				if ( objectiveSystemOpen ) {
					TogglePDA();
				} else if ( weapon_pda >= 0 ) {
					SelectWeapon( weapon_pda, true );
				}
			}
			break;
		}
		case IMPULSE_20:
		{
			if ( gameLocal.isClient || entityNumber == gameLocal.localClientNum )
			{
				gameLocal.mpGame.ToggleTeam();
			}
			break;
		}
		case IMPULSE_22:
		{
			if ( gameLocal.isClient || entityNumber == gameLocal.localClientNum )
			{
				gameLocal.mpGame.ToggleSpectate();
			}
			break;
		}

		case IMPULSE_24:
		{
			if ( gameLocal.isClient || entityNumber == gameLocal.localClientNum ) 
			{
					physicsObj.PerformMantle();
			}
			break;
		}

		// DarkMod: Sophisticated Zombie: For testing purposes only
	    // This tests the hiding spot search function relative to the player
		case IMPULSE_25:		// Test hiding spots.
		{
			DM_LOG(LC_AI, LT_DEBUG).LogString("Attempting hiding spot test");
			idVec3 searchOrigin = GetEyePosition();
			idBounds searchBounds (searchOrigin);
			
			idVec3 forward;
			idVec3 right;
			viewAngles.ToVectors (&forward, &right);
			right.Normalize();
			forward.Normalize();

			idVec3 widthPoint = searchOrigin + (right * 300.0f);
			searchBounds.AddPoint (widthPoint);
			idVec3 widthPoint2 = searchOrigin - (right * 300.0f);
			searchBounds.AddPoint (widthPoint2);  
			idVec3 endPoint = searchOrigin + (forward * 1000.0f);
			searchBounds.AddPoint (endPoint);

			// Get AAS
			idAAS* p_aas = gameLocal.GetAAS( 0 );
			if (p_aas != NULL)
			{
				darkModAASFindHidingSpots::testFindHidingSpots 
				(
					searchOrigin,
					0.35f,
					searchBounds,
					this, // Ignore self as a hiding screen
					p_aas
				);
				DM_LOG(LC_AI, LT_DEBUG).LogString("Done hiding spot test");
			}				
			else
			{
				DM_LOG(LC_AI, LT_WARNING).LogString("No default AAS is present for map");
			}
			
		}
		break;

		case IMPULSE_28:
		{
			if ( gameLocal.isClient || entityNumber == gameLocal.localClientNum )
			{
				gameLocal.mpGame.CastVote( gameLocal.localClientNum, true );
			}
		}
		break;

		case IMPULSE_29:
		{
			if ( gameLocal.isClient || entityNumber == gameLocal.localClientNum )
			{
				gameLocal.mpGame.CastVote( gameLocal.localClientNum, false );
			}
		}
		break;

		case IMPULSE_40:		// TDM: grab item with grabber
		{
			CGrabber *grabber = g_Global.m_DarkModPlayer->grabber;
			idEntity *useEnt = grabber->GetSelected();
			if(useEnt == NULL)
			{
				idEntity *frob = g_Global.m_DarkModPlayer->m_FrobEntity;
				if(frob != NULL)
					grabber->PutInHands(frob, this);
			}
		}
		break;

		case IMPULSE_41:		// TDM Use/Frob
		{
			PerformFrob();
		}
		break;

		case IMPULSE_51:	// Inventory use item
		{
			if(GetImmobilization() & EIM_ITEM_SELECT)
				return;

			inventoryUseItem(true);
			break;
		}

		case IMPULSE_52:	// Inventory drop item
		{
			/* greebo: disabled this allow inventoryDropItem() to be called
					   when an item is held in the grabber hands.
			if(GetImmobilization() & EIM_ITEM_SELECT) {
				return;
			}*/

			inventoryDropItem();
			break;
		}

		/*!
		* Lean forward is impulse 44
		*/
		case IMPULSE_44:		// Lean forward
		{
			if ( gameLocal.isClient || entityNumber == gameLocal.localClientNum ) 
				physicsObj.ToggleLean(90.0);
		}
		break;

		/*!
		* Lean left is impulse 45
		*/
		case IMPULSE_45:		// Lean left
		{
			DM_LOG(LC_SYSTEM, LT_DEBUG)LOGSTRING("Left lean impulse pressed\r");
			if ( gameLocal.isClient || entityNumber == gameLocal.localClientNum ) 
			{
				// Do we need to enter the leaning state?
				DM_LOG(LC_SYSTEM, LT_DEBUG)LOGSTRING("Left leaning started\r");
				physicsObj.ToggleLean(180.0);
				gameLocal.m_Keyboard->ImpulseProcessed(IR_LEAN_LEFT);
			}
		}
		break;

		/*!
		* Lean right is impulse 46
		*/
		case IMPULSE_46:		// Lean right
		{
			DM_LOG(LC_SYSTEM, LT_DEBUG)LOGSTRING("Right lean impulse pressed\r");
			if ( gameLocal.isClient || entityNumber == gameLocal.localClientNum ) 
				physicsObj.ToggleLean(0.0);
		}
		break;

		case IMPULSE_47:	// Inventory previous item
		{
			if(GetImmobilization() & EIM_ITEM_SELECT)
				return;

			inventoryPrevItem();
			break;
		}

		case IMPULSE_48:	// Inventory next item
		{
			if(GetImmobilization() & EIM_ITEM_SELECT)
				return;

			inventoryNextItem();
			break;
		}

		case IMPULSE_49:	// Inventory previous group
		{
			if(GetImmobilization() & EIM_ITEM_SELECT)
				return;

			inventoryPrevGroup();
			break;
		}

		case IMPULSE_50:	// Inventory next group
		{
			if(GetImmobilization() & EIM_ITEM_SELECT)
				return;

			inventoryNextGroup();
			break;
		}
	} 
}

void idPlayer::PerformKeyRelease(int impulse, int holdTime) {
	// The key associated to <impulse> has been released
	
	DM_LOG(LC_FROBBING, LT_INFO)LOGSTRING("Button %d has been released, has been held down %d ms.\r", impulse, holdTime);

	switch (impulse) {
		case IMPULSE_51:
			inventoryUseKeyRelease(holdTime);
			break;
	}
}

bool idPlayer::HandleESC( void ) {
	if ( gameLocal.inCinematic ) {
		return SkipCinematic();
	}

	if ( objectiveSystemOpen ) {
		TogglePDA();
		return true;
	}

	if ( m_overlays.findInteractive() ) {
		// Handle the escape?
		// Not yet implemented.
		// return true?
	}

	return false;
}

bool idPlayer::SkipCinematic( void ) {
	StartSound( "snd_skipcinematic", SND_CHANNEL_ANY, 0, false, NULL );
	return gameLocal.SkipCinematic();
}


/*
==============
idPlayer::GetImmobilization
==============
*/
int idPlayer::GetImmobilization( void ) {
	// Has something changed since the cache was last calculated?
	if ( m_immobilizationCache & EIM_UPDATE ) {
		const idKeyValue *kv;

		// Recalculate the immobilization from scratch.
		m_immobilizationCache = 0;
		kv = m_immobilization.MatchPrefix( "", NULL );
		while ( kv ) {
			//m_immobilizationCache |= m_immobilization.GetInt(kv->GetKey());
			m_immobilizationCache |= atoi(kv->GetValue());
			kv = m_immobilization.MatchPrefix( "", kv );
		}
	}
	return m_immobilizationCache;
}

/*
==============
idPlayer::GetHinderance
==============
*/
float idPlayer::GetHinderance( void ) {
	// Has something changed since the cache was last calculated?
	if ( m_hinderanceCache < 0.0f ) {
		const idKeyValue *kv;

		// Recalculate the hinderance from scratch.
		float mCap = 1.0f, aCap = 1.0f;
		kv = m_hinderance.MatchPrefix( "", NULL );
		while ( kv ) {
			idVec3 vec = m_hinderance.GetVector(kv->GetKey());
			mCap *= vec[0];
			if ( aCap > vec[1] ) {
				aCap = vec[1];
			}
			kv = m_hinderance.MatchPrefix( "", kv );
		}

		if ( aCap > mCap ) {
			aCap = mCap;
		}
		m_hinderanceCache = aCap;
	}
	return m_hinderanceCache;
}

/*
==============
idPlayer::EvaluateControls
==============
*/
void idPlayer::EvaluateControls( void ) 
{
	// check for respawning
	// TDM: Section removed, click-to-respawn functionality not needed for TDM
	// TDM: Added lean key check

	// in MP, idMultiplayerGame decides spawns
	if ( forceRespawn && !gameLocal.isMultiplayer && !g_testDeath.GetBool() ) {
		// in single player, we let the session handle restarting the level or loading a game
		gameLocal.sessionCommand = "died";
	}

	CheckHeldKeys();

	if ( ( usercmd.flags & UCF_IMPULSE_SEQUENCE ) != ( oldFlags & UCF_IMPULSE_SEQUENCE ) )
	{
		PerformImpulse( usercmd.impulse );
	}

	scoreBoardOpen = ( ( usercmd.buttons & BUTTON_SCORES ) != 0 || forceScoreBoard );

	oldFlags = usercmd.flags;

	AdjustSpeed();

	// update the viewangles
	UpdateViewAngles();
}

/*
==============
idPlayer::AdjustSpeed

DarkMod Note: Wow... this was pretty badly written
==============
*/
void idPlayer::AdjustSpeed( void ) 
{
	float speed(0.0f);
	float crouchspeed(0.0f);
	float rate(0.0f);
	float MaxSpeed(0.0f);

	MaxSpeed = pm_walkspeed.GetFloat() * cv_pm_runmod.GetFloat() * GetHinderance();

	if ( spectating )
	{
		speed = pm_spectatespeed.GetFloat();
		bobFrac = 0.0f;
	}
	else if ( noclip && !( usercmd.buttons & BUTTON_RUN ))
	{
		// "Walk" noclip
		speed = pm_noclipspeed.GetFloat();
		bobFrac = 0.0f;
	}
	else if ( noclip && ( usercmd.buttons & BUTTON_RUN ))
	{
		// "run" noclip
		speed = pm_noclipspeed.GetFloat() * cv_pm_runmod.GetFloat();
		bobFrac = 0.0f;
	} 
	// running case
	// DarkMod: removed check for not crouching..
	else if ( !physicsObj.OnLadder() && ( usercmd.buttons & BUTTON_RUN ) && ( usercmd.forwardmove || usercmd.rightmove ) ) 
	{
		if ( !gameLocal.isMultiplayer ) {
			stamina -= MS2SEC( gameLocal.msec );
		}
		if ( stamina < 0 ) {
			stamina = 0;
		}
		if ( ( !pm_stamina.GetFloat() ) || ( stamina > pm_staminathreshold.GetFloat() ) ) {
			bobFrac = 1.0f;
		} else if ( pm_staminathreshold.GetFloat() <= 0.0001f ) {
			bobFrac = 0.0f;
		} else {
			bobFrac = stamina / pm_staminathreshold.GetFloat();
		}

		/**
		*  DarkMod : Removed the effect of stamina on speed
		* uncomment for stamina effecting speed
		**/
		//speed = pm_walkspeed.GetFloat() * ( 1.0f - bobFrac ) + pm_walkspeed*cv_pm_runmod.GetFloat() * bobFrac;
		speed = pm_walkspeed.GetFloat() * cv_pm_runmod.GetFloat();
		crouchspeed = speed * cv_pm_crouchmod.GetFloat();

		// Clamp to encumbrance limits:
		if( speed > MaxSpeed )
			speed = MaxSpeed;
	}
	// standing still, walking, or creeping case
	else 
	{
		rate = pm_staminarate.GetFloat();
		
		// increase 25% faster when not moving
		if ( ( usercmd.forwardmove == 0 ) && ( usercmd.rightmove == 0 ) && ( !physicsObj.OnLadder() || ( usercmd.upmove == 0 ) ) ) 
		{
			 rate *= 1.25f;
		}

		stamina += rate * MS2SEC( gameLocal.msec );
		if ( stamina > pm_stamina.GetFloat() ) 
		{
			stamina = pm_stamina.GetFloat();
		}

		speed = pm_walkspeed.GetFloat();
		bobFrac = 0.0f;

		// apply creep modifier; creep is on button_5
		if( usercmd.buttons & BUTTON_5 )
			speed *= cv_pm_creepmod.GetFloat();

		// apply movement multipliers to crouch as well
		crouchspeed = speed * cv_pm_crouchmod.GetFloat();

		// Clamp to encumbrance limits:
		if( speed > MaxSpeed )
			speed = MaxSpeed;
	}

	// leave this in for speed potions or something
	speed *= PowerUpModifier(SPEED);

	if ( influenceActive == INFLUENCE_LEVEL3 ) 
	{
		speed *= 0.33f;
	}

	physicsObj.SetSpeed( speed, crouchspeed );
}

/*
==============
idPlayer::AdjustBodyAngles
==============
*/
void idPlayer::AdjustBodyAngles( void ) {
	idMat3	lookAxis;
	idMat3	legsAxis;
	bool	blend;
	float	diff;
	float	frac;
	float	upBlend;
	float	forwardBlend;
	float	downBlend;

	if ( health < 0 ) {
		return;
	}

	blend = true;

	if ( !physicsObj.HasGroundContacts() ) {
		idealLegsYaw = 0.0f;
		legsForward = true;
	} else if ( usercmd.forwardmove < 0 ) {
		idealLegsYaw = idMath::AngleNormalize180( idVec3( -usercmd.forwardmove, usercmd.rightmove, 0.0f ).ToYaw() );
		legsForward = false;
	} else if ( usercmd.forwardmove > 0 ) {
		idealLegsYaw = idMath::AngleNormalize180( idVec3( usercmd.forwardmove, -usercmd.rightmove, 0.0f ).ToYaw() );
		legsForward = true;
	} else if ( ( usercmd.rightmove != 0 ) && physicsObj.IsCrouching() ) {
		if ( !legsForward ) {
			idealLegsYaw = idMath::AngleNormalize180( idVec3( idMath::Abs( usercmd.rightmove ), usercmd.rightmove, 0.0f ).ToYaw() );
		} else {
			idealLegsYaw = idMath::AngleNormalize180( idVec3( idMath::Abs( usercmd.rightmove ), -usercmd.rightmove, 0.0f ).ToYaw() );
		}
	} else if ( usercmd.rightmove != 0 ) {
		idealLegsYaw = 0.0f;
		legsForward = true;
	} else {
		legsForward = true;
		diff = idMath::Fabs( idealLegsYaw - legsYaw );
		idealLegsYaw = idealLegsYaw - idMath::AngleNormalize180( viewAngles.yaw - oldViewYaw );
		if ( diff < 0.1f ) {
			legsYaw = idealLegsYaw;
			blend = false;
		}
	}

	if ( !physicsObj.IsCrouching() ) {
		legsForward = true;
	}

	oldViewYaw = viewAngles.yaw;

	AI_TURN_LEFT = false;
	AI_TURN_RIGHT = false;
	if ( idealLegsYaw < -45.0f ) {
		idealLegsYaw = 0;
		AI_TURN_RIGHT = true;
		blend = true;
	} else if ( idealLegsYaw > 45.0f ) {
		idealLegsYaw = 0;
		AI_TURN_LEFT = true;
		blend = true;
	}

	if ( blend ) {
		legsYaw = legsYaw * 0.9f + idealLegsYaw * 0.1f;
	}
	legsAxis = idAngles( 0.0f, legsYaw, 0.0f ).ToMat3();
	animator.SetJointAxis( hipJoint, JOINTMOD_WORLD, legsAxis );

	// calculate the blending between down, straight, and up
	frac = viewAngles.pitch / 90.0f;
	if ( frac > 0.0f ) {
		downBlend		= frac;
		forwardBlend	= 1.0f - frac;
		upBlend			= 0.0f;
	} else {
		downBlend		= 0.0f;
		forwardBlend	= 1.0f + frac;
		upBlend			= -frac;
	}

    animator.CurrentAnim( ANIMCHANNEL_TORSO )->SetSyncedAnimWeight( 0, downBlend );
	animator.CurrentAnim( ANIMCHANNEL_TORSO )->SetSyncedAnimWeight( 1, forwardBlend );
	animator.CurrentAnim( ANIMCHANNEL_TORSO )->SetSyncedAnimWeight( 2, upBlend );

	animator.CurrentAnim( ANIMCHANNEL_LEGS )->SetSyncedAnimWeight( 0, downBlend );
	animator.CurrentAnim( ANIMCHANNEL_LEGS )->SetSyncedAnimWeight( 1, forwardBlend );
	animator.CurrentAnim( ANIMCHANNEL_LEGS )->SetSyncedAnimWeight( 2, upBlend );
}

/*
==============
idPlayer::InitAASLocation
==============
*/
void idPlayer::InitAASLocation( void ) {
	int		i;
	int		num;
	idVec3	size;
	idBounds bounds;
	idAAS	*aas;
	idVec3	origin;

	GetFloorPos( 64.0f, origin );

	num = gameLocal.NumAAS();
	aasLocation.SetGranularity( 1 );
	aasLocation.SetNum( num );	
	for( i = 0; i < aasLocation.Num(); i++ ) {
		aasLocation[ i ].areaNum = 0;
		aasLocation[ i ].pos = origin;
		aas = gameLocal.GetAAS( i );
		if ( aas && aas->GetSettings() ) {
			size = aas->GetSettings()->boundingBoxes[0][1];
			bounds[0] = -size;
			size.z = 32.0f;
			bounds[1] = size;

			aasLocation[ i ].areaNum = aas->PointReachableAreaNum( origin, bounds, AREA_REACHABLE_WALK );
		}
	}
}

/*
==============
idPlayer::SetAASLocation
==============
*/
void idPlayer::SetAASLocation( void ) {
	int		i;
	int		areaNum;
	idVec3	size;
	idBounds bounds;
	idAAS	*aas;
	idVec3	origin;

	if ( !GetFloorPos( 64.0f, origin ) ) {
		return;
	}
	
	for( i = 0; i < aasLocation.Num(); i++ ) {
		aas = gameLocal.GetAAS( i );
		if ( !aas ) {
			continue;
		}

		size = aas->GetSettings()->boundingBoxes[0][1];
		bounds[0] = -size;
		size.z = 32.0f;
		bounds[1] = size;

		areaNum = aas->PointReachableAreaNum( origin, bounds, AREA_REACHABLE_WALK );
		if ( areaNum ) {
			aasLocation[ i ].pos = origin;
			aasLocation[ i ].areaNum = areaNum;
		}
	}
}

/*
==============
idPlayer::GetAASLocation
==============
*/
void idPlayer::GetAASLocation( idAAS *aas, idVec3 &pos, int &areaNum ) const {
	int i;

	if ( aas != NULL ) {
		for( i = 0; i < aasLocation.Num(); i++ ) {
			if ( aas == gameLocal.GetAAS( i ) ) {
				areaNum = aasLocation[ i ].areaNum;
				pos = aasLocation[ i ].pos;
				return;
			}
		}
	}

	areaNum = 0;
	pos = physicsObj.GetOrigin();
}

/*
==============
idPlayer::Move
==============
*/
void idPlayer::Move( void ) 
{
	float newEyeOffset;
	idVec3 savedOrigin;
	idVec3 savedVelocity;
	idVec3 pushVelocity;

	// save old origin and velocity for crashlanding
	savedOrigin = physicsObj.GetOrigin();
	savedVelocity = physicsObj.GetLinearVelocity();

	pushVelocity = physicsObj.GetPushedLinearVelocity();

	// set physics variables
	physicsObj.SetMaxStepHeight( pm_stepsize.GetFloat() );
	physicsObj.SetMaxJumpHeight( pm_jumpheight.GetFloat() );

	if ( noclip ) {
		physicsObj.SetContents( 0 );
		physicsObj.SetMovementType( PM_NOCLIP );
	} else if ( spectating ) {
		physicsObj.SetContents( 0 );
		physicsObj.SetMovementType( PM_SPECTATOR );
	} else if ( health <= 0 ) {
		physicsObj.SetContents( CONTENTS_CORPSE | CONTENTS_MONSTERCLIP );
		physicsObj.SetMovementType( PM_DEAD );
	} else if ( gameLocal.inCinematic || gameLocal.GetCamera() || privateCameraView || ( influenceActive == INFLUENCE_LEVEL2 ) ) {
		physicsObj.SetContents( CONTENTS_BODY );
		physicsObj.SetMovementType( PM_FREEZE );
	} else {
		physicsObj.SetContents( CONTENTS_BODY );
		physicsObj.SetMovementType( PM_NORMAL );
	}
	// SR CONTENTS_RESPONSE FIX
	if( m_StimResponseColl->HasResponse() )
		physicsObj.SetContents( physicsObj.GetContents() | CONTENTS_RESPONSE );

	if ( spectating ) {
		physicsObj.SetClipMask( MASK_DEADSOLID );
	} else if ( health <= 0 ) {
		physicsObj.SetClipMask( MASK_DEADSOLID );
	} else {
		physicsObj.SetClipMask( MASK_PLAYERSOLID );
	}

	physicsObj.SetDebugLevel( g_debugMove.GetBool() );
	physicsObj.SetPlayerInput( usercmd, viewAngles );

	// FIXME: physics gets disabled somehow
	BecomeActive( TH_PHYSICS );
	RunPhysics();

	// update our last valid AAS location for the AI
	SetAASLocation();

	if ( spectating ) {
		newEyeOffset = 0.0f;
	} else if ( health <= 0 ) {
		newEyeOffset = pm_deadviewheight.GetFloat();
	} else if ( physicsObj.IsCrouching() ) {
		newEyeOffset = pm_crouchviewheight.GetFloat();
	} else if ( GetBindMaster() && GetBindMaster()->IsType( idAFEntity_Vehicle::Type ) ) {
		newEyeOffset = 0.0f;
	} else {
		newEyeOffset = pm_normalviewheight.GetFloat();
	}

	if ( EyeHeight() != newEyeOffset ) {
		if ( spectating ) {
			SetEyeHeight( newEyeOffset );
		} else {
			// smooth out duck height changes
			SetEyeHeight( EyeHeight() * pm_crouchrate.GetFloat() + newEyeOffset * ( 1.0f - pm_crouchrate.GetFloat() ) );
		}
	}

	if ( noclip || gameLocal.inCinematic || ( influenceActive == INFLUENCE_LEVEL2 ) ) {
		AI_CROUCH	= false;
		AI_ONGROUND	= ( influenceActive == INFLUENCE_LEVEL2 );
		AI_ONLADDER	= false;
		AI_JUMP		= false;
	} else {
		AI_CROUCH	= physicsObj.IsCrouching();
		AI_ONGROUND	= physicsObj.HasGroundContacts();
		AI_ONLADDER	= physicsObj.OnLadder();
		AI_JUMP		= physicsObj.HasJumped();

		// check if we're standing on top of a monster and give a push if we are
		idEntity *groundEnt = physicsObj.GetGroundEntity();
		if ( groundEnt && groundEnt->IsType( idAI::Type ) ) {
			idVec3 vel = physicsObj.GetLinearVelocity();
			if ( vel.ToVec2().LengthSqr() < 0.1f ) {
				vel.ToVec2() = physicsObj.GetOrigin().ToVec2() - groundEnt->GetPhysics()->GetAbsBounds().GetCenter().ToVec2();
				vel.ToVec2().NormalizeFast();
				vel.ToVec2() *= pm_walkspeed.GetFloat();
			} else {
				// give em a push in the direction they're going
				vel *= 1.1f;
			}
			physicsObj.SetLinearVelocity( vel );
		}
	}

	if ( AI_JUMP ) 
	{
		// bounce the view weapon
 		loggedAccel_t	*acc = &loggedAccel[currentLoggedAccel&(NUM_LOGGED_ACCELS-1)];
		currentLoggedAccel++;
		acc->time = gameLocal.time;
		acc->dir[2] = 200;
		acc->dir[0] = acc->dir[1] = 0;
	}

	// play rope movement sounds
	if( physicsObj.OnRope() )
	{
		int old_vert(0), new_vert(0);
		old_vert = savedOrigin.z / cv_pm_rope_snd_rep_dist.GetInteger();
		new_vert = physicsObj.GetOrigin().z / cv_pm_rope_snd_rep_dist.GetInteger();
		
		if ( old_vert != new_vert ) 
		{
			StartSound( "snd_rope_climb", SND_CHANNEL_ANY, 0, false, NULL );
		}
	}


	// play climbing movement sounds
	if ( AI_ONLADDER ) 
	{
		idStr TempStr, LocalSound, sound;
		bool bSoundPlayed = false;
		sound.Clear();

		int old_vert(0), new_vert(0), old_horiz(0), new_horiz(0);
		old_vert = savedOrigin.z / physicsObj.GetClimbSndRepDistVert();
		new_vert = physicsObj.GetOrigin().z / physicsObj.GetClimbSndRepDistVert();

		old_horiz = physicsObj.GetClimbLateralCoord( savedOrigin ) / physicsObj.GetClimbSndRepDistHoriz();
		new_horiz = physicsObj.GetClimbLateralCoord( physicsObj.GetOrigin() ) / physicsObj.GetClimbSndRepDistHoriz();

		if ( old_vert != new_vert ) 
		{
			LocalSound = "snd_climb_vert_";
			bSoundPlayed = true;
		}
		else if( old_horiz != new_horiz )
		{
			LocalSound = "snd_climb_horiz_";
			bSoundPlayed = true;
		}

		if( bSoundPlayed )
		{
			idStr SurfName = ((idPhysics_Player *) GetPhysics())->GetClimbSurfaceType();
			TempStr = LocalSound + SurfName;
			sound = spawnArgs.GetString( TempStr.c_str() );
			if( sound.IsEmpty() )
			{
				TempStr = LocalSound + "default";
				sound = spawnArgs.GetString( TempStr.c_str() );
			}

			// DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING("Climb sound: %s\r", TempStr.c_str() );
			if ( !sound.IsEmpty() )
				StartSound( TempStr.c_str(), SND_CHANNEL_ANY, 0, false, NULL );
		}
	}

	BobCycle( pushVelocity );
	if ( health > 0 )
	{
		CrashLand( savedOrigin, savedVelocity );
	}
}

/*
==============
idPlayer::UpdateHud
==============
*/
void idPlayer::UpdateHud( void ) {
	idPlayer *aimed;

	if ( !hud ) {
		return;
	}

	if ( entityNumber != gameLocal.localClientNum ) {
		return;
	}

	if ( gameLocal.realClientTime == lastMPAimTime ) {
		if ( MPAim != -1 && gameLocal.gameType == GAME_TDM
			&& gameLocal.entities[ MPAim ] && gameLocal.entities[ MPAim ]->IsType( idPlayer::Type )
			&& static_cast< idPlayer * >( gameLocal.entities[ MPAim ] )->team == team ) {
				aimed = static_cast< idPlayer * >( gameLocal.entities[ MPAim ] );
				hud->SetStateString( "aim_text", gameLocal.userInfo[ MPAim ].GetString( "ui_name" ) );
				hud->SetStateFloat( "aim_color", aimed->colorBarIndex );
				hud->HandleNamedEvent( "aim_flash" );
				MPAimHighlight = true;
				MPAimFadeTime = 0;	// no fade till loosing focus
		} else if ( MPAimHighlight ) {
			hud->HandleNamedEvent( "aim_fade" );
			MPAimFadeTime = gameLocal.realClientTime;
			MPAimHighlight = false;
		}
	}
	if ( MPAimFadeTime ) {
		assert( !MPAimHighlight );
		if ( gameLocal.realClientTime - MPAimFadeTime > 2000 ) {
			MPAimFadeTime = 0;
		}
	}

	// FIXME: this is here for level ammo balancing to see pct of hits 
	hud->SetStateInt( "g_showProjectilePct", g_showProjectilePct.GetInteger() );
	if ( numProjectilesFired ) {
		hud->SetStateString( "projectilepct", va( "Hit %% %.1f", ( (float) numProjectileHits / numProjectilesFired ) * 100 ) );
	} else {
		hud->SetStateString( "projectilepct", "Hit % 0.0" );
	}

	if ( isLagged && gameLocal.isMultiplayer && gameLocal.localClientNum == entityNumber ) {
		hud->SetStateString( "hudLag", "1" );
	} else {
		hud->SetStateString( "hudLag", "0" );
	}

	// Trigger an update of the HUD
	// TODO: This shouldn't be called so often, so cache this
	inventoryChangeSelection(hud, true);
}

/*
==============
idPlayer::UpdateDeathSkin
==============
*/
void idPlayer::UpdateDeathSkin( bool state_hitch ) {
	if ( !( gameLocal.isMultiplayer || g_testDeath.GetBool() ) ) {
		return;
	}
	if ( health <= 0 ) {
		if ( !doingDeathSkin ) {
			deathClearContentsTime = spawnArgs.GetInt( "deathSkinTime" );
			doingDeathSkin = true;
			renderEntity.noShadow = true;
			if ( state_hitch ) {
				renderEntity.shaderParms[ SHADERPARM_TIME_OF_DEATH ] = gameLocal.time * 0.001f - 2.0f;
			} else {
				renderEntity.shaderParms[ SHADERPARM_TIME_OF_DEATH ] = gameLocal.time * 0.001f;
			}
			UpdateVisuals();
		}

		// wait a bit before switching off the content
		if ( deathClearContentsTime && gameLocal.time > deathClearContentsTime ) {
			SetCombatContents( false );
			deathClearContentsTime = 0;
		}
	} else {
		renderEntity.noShadow = false;
		renderEntity.shaderParms[ SHADERPARM_TIME_OF_DEATH ] = 0.0f;
		UpdateVisuals();
		doingDeathSkin = false;
	}
}

/*
==============
idPlayer::StartFxOnBone
==============
*/
void idPlayer::StartFxOnBone( const char *fx, const char *bone ) {
	idVec3 offset;
	idMat3 axis;
	jointHandle_t jointHandle = GetAnimator()->GetJointHandle( bone );

	if ( jointHandle == INVALID_JOINT ) {
		gameLocal.Printf( "Cannot find bone %s\n", bone );
		return;
	}

	if ( GetAnimator()->GetJointTransform( jointHandle, gameLocal.time, offset, axis ) ) {
		offset = GetPhysics()->GetOrigin() + offset * GetPhysics()->GetAxis();
		axis = axis * GetPhysics()->GetAxis();
	}

	idEntityFx::StartFx( fx, &offset, &axis, this, true );
}

void idPlayer::UpdateUnderWaterEffects() {
	if ( physicsObj.GetWaterLevel() >= WATERLEVEL_HEAD ) {
		if (!underWaterEffectsActive) {
			StartSound( "snd_airless", SND_CHANNEL_DEMONIC, 0, false, NULL );
			idStr overlay = spawnArgs.GetString("gui_airless");
			if (!overlay.IsEmpty()) {
				underWaterGUIHandle = CreateOverlay(overlay.c_str(), LAYER_UNDERWATER);
			}
			underWaterEffectsActive = true;
		}
	}
	else {
		if (underWaterEffectsActive) {
			StopSound( SND_CHANNEL_DEMONIC, false );
			if (underWaterGUIHandle != -1) {
				DestroyOverlay(underWaterGUIHandle);
				underWaterGUIHandle = -1;
			}
			underWaterEffectsActive = false;
		}
	}
}

/*
==============
idPlayer::Think

Called every tic for each player
==============
*/
void idPlayer::Think( void )
{
	renderEntity_t *headRenderEnt;
	UpdatePlayerIcons();

	// latch button actions
	oldButtons = usercmd.buttons;

	// grab out usercmd
	usercmd_t oldCmd = usercmd;
	usercmd = gameLocal.usercmds[ entityNumber ];
	buttonMask &= usercmd.buttons;
	usercmd.buttons &= ~buttonMask;

	if ( gameLocal.inCinematic && gameLocal.skipCinematic ) {
		return;
	}

	// clear the ik before we do anything else so the skeleton doesn't get updated twice
	walkIK.ClearJointMods();
	
	// if this is the very first frame of the map, set the delta view angles
	// based on the usercmd angles
	if ( !spawnAnglesSet && ( gameLocal.GameState() != GAMESTATE_STARTUP ) ) {
		spawnAnglesSet = true;
		SetViewAngles( spawnAngles );
		oldFlags = usercmd.flags;
	}

	if ( objectiveSystemOpen || gameLocal.inCinematic || influenceActive ) {
		if ( objectiveSystemOpen && AI_PAIN ) {
			TogglePDA();
		}
		usercmd.forwardmove = 0;
		usercmd.rightmove = 0;
		usercmd.upmove = 0;
	}

	// I'm not sure if this is the best way to do things, since it doesn't yet
	// take into account whether the player is swimming/climbing, etc. But it
	// should be good enough for now. I'll improve it later.
	// -Gildoran
	if ( GetImmobilization() & EIM_MOVEMENT ) {
		usercmd.forwardmove = 0;
		usercmd.rightmove = 0;
	}
	// Note to self: I should probably be thinking about having some sort of
	// flag where the player is kept crouching if that's how they started out.
	if ( GetImmobilization() & EIM_CROUCH && usercmd.upmove < 0 ) {
		usercmd.upmove = 0;
	} else if ( GetImmobilization() & EIM_JUMP && usercmd.upmove > 0 ) {
		usercmd.upmove = 0;
	}

	// log movement changes for weapon bobbing effects
	if ( usercmd.forwardmove != oldCmd.forwardmove ) {
		loggedAccel_t	*acc = &loggedAccel[currentLoggedAccel&(NUM_LOGGED_ACCELS-1)];
		currentLoggedAccel++;
		acc->time = gameLocal.time;
		acc->dir[0] = usercmd.forwardmove - oldCmd.forwardmove;
		acc->dir[1] = acc->dir[2] = 0;
	}

	if ( usercmd.rightmove != oldCmd.rightmove ) {
		loggedAccel_t	*acc = &loggedAccel[currentLoggedAccel&(NUM_LOGGED_ACCELS-1)];
		currentLoggedAccel++;
		acc->time = gameLocal.time;
		acc->dir[1] = usercmd.rightmove - oldCmd.rightmove;
		acc->dir[0] = acc->dir[2] = 0;
	}

	// freelook centering
	if ( ( usercmd.buttons ^ oldCmd.buttons ) & BUTTON_MLOOK ) {
		centerView.Init( gameLocal.time, 200, viewAngles.pitch, 0 );
	}

	// zooming
	if ( ( usercmd.buttons ^ oldCmd.buttons ) & BUTTON_ZOOM ) {
		if ( ( usercmd.buttons & BUTTON_ZOOM ) && weapon.GetEntity() ) {
			zoomFov.Init( gameLocal.time, 200.0f, CalcFov( false ), weapon.GetEntity()->GetZoomFov() );
		} else {
			zoomFov.Init( gameLocal.time, 200.0f, zoomFov.GetCurrentValue( gameLocal.time ), DefaultFov() );
		}
	}

	// if we have an active gui, we will unrotate the view angles as
	// we turn the mouse movements into gui events
	idUserInterface *gui = ActiveGui();
	if ( gui && gui != focusUI ) {
		RouteGuiMouse( gui );
	}

	// set the push velocity on the weapon before running the physics
	if ( weapon.GetEntity() ) {
		weapon.GetEntity()->SetPushVelocity( physicsObj.GetPushedLinearVelocity() );
	}

	EvaluateControls();

	if ( !af.IsActive() ) {
		AdjustBodyAngles();
		CopyJointsFromBodyToHead();
	}

	Move();

	if ( !g_stopTime.GetBool() ) {

		if ( !noclip && !spectating && ( health > 0 ) && !IsHidden() ) {
			TouchTriggers();
		}

		// not done on clients for various reasons. don't do it on server and save the sound channel for other things
		if ( !gameLocal.isMultiplayer ) {
			SetCurrentHeartRate();
			float scale = g_damageScale.GetFloat();
			if ( g_useDynamicProtection.GetBool() && scale < 1.0f && gameLocal.time - lastDmgTime > 500 ) {
				if ( scale < 1.0f ) {
					scale += 0.05f;
				}
				if ( scale > 1.0f ) {
					scale = 1.0f;
				}
				g_damageScale.SetFloat( scale );
			}
		}

		// update GUIs, Items, and character interactions
		UpdateFocus();
		
		UpdateLocation();

		// update player script
		UpdateScript();

		// service animations
		if ( !spectating && !af.IsActive() && !gameLocal.inCinematic ) {
			// Update the button state, this calls PerformKeyRelease() if a button has been released
			m_ButtonStateTracker.update();

			UpdateConditions();

			UpdateAnimState();
			CheckBlink();
		}

		// clear out our pain flag so we can tell if we recieve any damage between now and the next time we think
		AI_PAIN = false;
	}

	// calculate the exact bobbed view position, which is used to
	// position the view weapon, among other things
	CalculateFirstPersonView();

	// this may use firstPersonView, or a thirdPeroson / camera view
	CalculateRenderView();

	FrobCheck();

	idStr strText;
/*
	// TODO: remove this because it is just to determine how to fill out the renderstructure.
	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("RenderViewId: %u\r", renderView->viewID);
	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("x: %u   y: %u   w: %u   h: %u\r", renderView->x, renderView->y, renderView->width, renderView->height);
	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("FovX: %f   FovY: %f\r", renderView->fov_x, renderView->fov_y);
	DM_LOGVECTOR3(LC_LIGHT, LT_DEBUG, "vieworg", renderView->vieworg);
	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("cramZNear: %u   forceUpdate: %u\r", renderView->cramZNear, renderView->forceUpdate);
	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("time: %u\r", renderView->time);
	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("time: %u\r", renderView->globalMaterial);
	for(i = 0; i < MAX_GLOBAL_SHADER_PARMS; i++)
		DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Param[%u]: %f\r", i, renderView->shaderParms[i]);
*/
	if ( spectating ) {
		UpdateSpectating();
	} else if ( health > 0 ) {
		UpdateWeapon();
	}

	UpdateAir();

	// greebo: update underwater overlay and sounds
	UpdateUnderWaterEffects();
	
	UpdateHud();

	UpdatePowerUps();

	UpdateDeathSkin( false );

	if ( gameLocal.isMultiplayer ) {
		DrawPlayerIcons();
	}

	if ( head.GetEntity() ) {
		headRenderEnt = head.GetEntity()->GetRenderEntity();
	} else {
		headRenderEnt = NULL;
	}

	if ( headRenderEnt ) {
		if ( influenceSkin ) {
			headRenderEnt->customSkin = influenceSkin;
		} else {
			headRenderEnt->customSkin = NULL;
		}
	}

	if ( gameLocal.isMultiplayer || g_showPlayerShadow.GetBool() ) {
		renderEntity.suppressShadowInViewID	= 0;
		if ( headRenderEnt ) {
			headRenderEnt->suppressShadowInViewID = 0;
		}
	} else {
		renderEntity.suppressShadowInViewID	= entityNumber+1;
		if ( headRenderEnt ) {
			headRenderEnt->suppressShadowInViewID = entityNumber+1;
		}
	}
	// never cast shadows from our first-person muzzle flashes
	renderEntity.suppressShadowInLightID = LIGHTID_VIEW_MUZZLE_FLASH + entityNumber;
	if ( headRenderEnt ) {
		headRenderEnt->suppressShadowInLightID = LIGHTID_VIEW_MUZZLE_FLASH + entityNumber;
	}

	if ( !g_stopTime.GetBool() ) {
		UpdateAnimation();

        Present();

		UpdateDamageEffects();

		LinkCombat();

		playerView.CalculateShake();
	}

	if ( !( thinkFlags & TH_THINK ) ) {
		gameLocal.Printf( "player %d not thinking?\n", entityNumber );
	}

	if ( g_showEnemies.GetBool() ) {
		idActor *ent;
		int num = 0;
		for( ent = enemyList.Next(); ent != NULL; ent = ent->enemyNode.Next() ) {
			gameLocal.Printf( "enemy (%d)'%s'\n", ent->entityNumber, ent->name.c_str() );
			gameRenderWorld->DebugBounds( colorRed, ent->GetPhysics()->GetBounds().Expand( 2 ), ent->GetPhysics()->GetOrigin() );
			num++;
		}
		gameLocal.Printf( "%d: enemies\n", num );
	}

	// determine if portal sky is in pvs
	gameLocal.portalSkyActive = gameLocal.pvs.CheckAreasForPortalSky( gameLocal.GetPlayerPVS(), GetPhysics()->GetOrigin() );
}

/*
=================
idPlayer::RouteGuiMouse
=================
*/
void idPlayer::RouteGuiMouse( idUserInterface *gui ) {
	sysEvent_t ev;
	const char *command;

	/*if ( gui == m_overlays.findInteractive() ) {
		// Ensure that gui overlays are always able to execute commands,
		// even if the player isn't moving the mouse.
		ev = sys->GenerateMouseMoveEvent( usercmd.mx - oldMouseX, usercmd.my - oldMouseY );
		command = gui->HandleEvent( &ev, gameLocal.time );
		HandleGuiCommands( this, command );
		oldMouseX = usercmd.mx;
		oldMouseY = usercmd.my;
	}*/
		
	if ( usercmd.mx != oldMouseX || usercmd.my != oldMouseY ) {
		ev = sys->GenerateMouseMoveEvent( usercmd.mx - oldMouseX, usercmd.my - oldMouseY );
		command = gui->HandleEvent( &ev, gameLocal.time );
		oldMouseX = usercmd.mx;
		oldMouseY = usercmd.my;
	}
}

/*
==================
idPlayer::LookAtKiller
==================
*/
void idPlayer::LookAtKiller( idEntity *inflictor, idEntity *attacker ) {
	idVec3 dir;
	
	if ( attacker && attacker != this ) {
		dir = attacker->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin();
	} else if ( inflictor && inflictor != this ) {
		dir = inflictor->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin();
	} else {
		dir = viewAxis[ 0 ];
	}

	idAngles ang( 0, dir.ToYaw(), 0 );
	SetViewAngles( ang );
}

/*
==============
idPlayer::Kill
==============
*/
void idPlayer::Kill( bool delayRespawn, bool nodamage ) {
	if ( spectating ) {
		SpectateFreeFly( false );
	} else if ( health > 0 ) {
		godmode = false;
		if ( nodamage ) {
			ServerSpectate( true );
			forceRespawn = true;
		} else {
			Damage( this, this, vec3_origin, "damage_suicide", 1.0f, INVALID_JOINT );
			if ( delayRespawn ) {
				forceRespawn = false;
				int delay = spawnArgs.GetFloat( "respawn_delay" );
				minRespawnTime = gameLocal.time + SEC2MS( delay );
				maxRespawnTime = minRespawnTime + MAX_RESPAWN_TIME;
			}
		}
	}
}

/*
==================
idPlayer::Killed
==================
*/
void idPlayer::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	float delay;

	assert( !gameLocal.isClient );

	// stop taking knockback once dead
	fl.noknockback = true;
	if ( health < -999 ) {
		health = -999;
	}

	if ( AI_DEAD ) {
		AI_PAIN = true;
		return;
	}

	heartInfo.Init( 0, 0, 0, BASE_HEARTRATE );
	AdjustHeartRate( DEAD_HEARTRATE, 10.0f, 0.0f, true );

	AI_DEAD = true;

	this->PostEventMS( &EV_Player_MissionFailed, spawnArgs.GetInt("death_transit_time", "2000") );

	SetAnimState( ANIMCHANNEL_LEGS, "Legs_Death", 4 );
	SetAnimState( ANIMCHANNEL_TORSO, "Torso_Death", 4 );
	SetWaitState( "" );

	animator.ClearAllJoints();

// NOTE: RespawnTime not used anymore in TDM except maybe for third person death later?
	if ( StartRagdoll() ) {
		pm_modelView.SetInteger( 0 );
		minRespawnTime = gameLocal.time + RAGDOLL_DEATH_TIME;
		maxRespawnTime = minRespawnTime + MAX_RESPAWN_TIME;
	} else {
		// don't allow respawn until the death anim is done
		// g_forcerespawn may force spawning at some later time
		delay = spawnArgs.GetFloat( "respawn_delay" );
		minRespawnTime = gameLocal.time + SEC2MS( delay );
		maxRespawnTime = minRespawnTime + MAX_RESPAWN_TIME;
	}

	physicsObj.SetMovementType( PM_DEAD );

	if( physicsObj.GetWaterLevel() >= WATERLEVEL_HEAD )
		StartSound( "snd_death_liquid", SND_CHANNEL_VOICE, 0, false, NULL );
	else
		StartSound( "snd_death", SND_CHANNEL_VOICE, 0, false, NULL );

	StopSound( SND_CHANNEL_BODY2, false );

	fl.takedamage = true;		// can still be gibbed

	// get rid of weapon
	weapon.GetEntity()->OwnerDied();

	// drop the weapon as an item
	DropWeapon( true );

	if ( !g_testDeath.GetBool() ) {
		LookAtKiller( inflictor, attacker );
	}

	if ( gameLocal.isMultiplayer || g_testDeath.GetBool() ) {
		idPlayer *killer = NULL;
		// no gibbing in MP. Event_Gib will early out in MP
		if ( attacker->IsType( idPlayer::Type ) ) {
			killer = static_cast<idPlayer*>(attacker);
			if ( health < -20 ) {
				gibDeath = true;
				gibsDir = dir;
				gibsLaunched = false;
			}
		}
		gameLocal.mpGame.PlayerDeath( this, killer, isTelefragged );
	} else {
		physicsObj.SetContents( CONTENTS_CORPSE | CONTENTS_MONSTERCLIP );
		// SR CONTENTS_RESPONSE FIX
		if( m_StimResponseColl->HasResponse() )
			physicsObj.SetContents( physicsObj.GetContents() | CONTENTS_RESPONSE );
	}

	UpdateVisuals();

	isChatting = false;
}

/*
=====================
idPlayer::GetAIAimTargets

Returns positions for the AI to aim at.
=====================
*/
void idPlayer::GetAIAimTargets( const idVec3 &lastSightPos, idVec3 &headPos, idVec3 &chestPos ) {
	idVec3 offset;
	idMat3 axis;
	idVec3 origin;
	
	origin = lastSightPos - physicsObj.GetOrigin();

	GetJointWorldTransform( chestJoint, gameLocal.time, offset, axis );
	headPos = offset + origin;

	GetJointWorldTransform( headJoint, gameLocal.time, offset, axis );
	chestPos = offset + origin;
}

/*
================
idPlayer::DamageFeedback

callback function for when another entity recieved damage from this entity.  damage can be adjusted and returned to the caller.
================
*/
void idPlayer::DamageFeedback( idEntity *victim, idEntity *inflictor, int &damage ) {
	assert( !gameLocal.isClient );
	damage *= PowerUpModifier( BERSERK );
	if ( damage && ( victim != this ) && victim->IsType( idActor::Type ) ) {
		SetLastHitTime( gameLocal.time );
	}
}

/*
=================
idPlayer::CalcDamagePoints

Calculates how many health and armor points will be inflicted, but
doesn't actually do anything with them.  This is used to tell when an attack
would have killed the player, possibly allowing a "saving throw"
=================
*/
void idPlayer::CalcDamagePoints( idEntity *inflictor, idEntity *attacker, const idDict *damageDef,
							   const float damageScale, const int location, int *health, int *armor ) {
	int		damage;
	int		armorSave;

	damageDef->GetInt( "damage", "20", damage );
	damage = GetDamageForLocation( damage, location );

	idPlayer *player = attacker->IsType( idPlayer::Type ) ? static_cast<idPlayer*>(attacker) : NULL;
	if ( !gameLocal.isMultiplayer ) {
		if ( inflictor != gameLocal.world ) {
			switch ( g_skill.GetInteger() ) {
				case 0: 
					damage *= 0.80f;
					if ( damage < 1 ) {
						damage = 1;
					}
					break;
				case 2:
					damage *= 1.70f;
					break;
				case 3:
					damage *= 3.5f;
					break;
				default:
					break;
			}
		}
	}

	damage *= damageScale;

	// always give half damage if hurting self
	if ( attacker == this ) {
		if ( gameLocal.isMultiplayer ) {
			// only do this in mp so single player plasma and rocket splash is very dangerous in close quarters
			damage *= damageDef->GetFloat( "selfDamageScale", "0.5" );
		} else {
			damage *= damageDef->GetFloat( "selfDamageScale", "1" );
		}
	}

	// check for completely getting out of the damage
	if ( !damageDef->GetBool( "noGod" ) ) {
		// check for godmode
		if ( godmode ) {
			damage = 0;
		}
	}

	// inform the attacker that they hit someone
	attacker->DamageFeedback( this, inflictor, damage );

	// save some from armor
	if ( !damageDef->GetBool( "noArmor" ) ) {
		/*float armor_protection;

		armor_protection = ( gameLocal.isMultiplayer ) ? g_armorProtectionMP.GetFloat() : g_armorProtection.GetFloat();

		armorSave = ceil( damage * armor_protection );
		if ( armorSave >= inventory.armor ) {
			armorSave = inventory.armor;
		}*/
		armorSave = 0;

		if ( !damage ) {
			armorSave = 0;
		} else if ( armorSave >= damage ) {
			armorSave = damage - 1;
			damage = 1;
		} else {
			damage -= armorSave;
		}
	} else {
		armorSave = 0;
	}

	// check for team damage
	if ( gameLocal.gameType == GAME_TDM
		&& !gameLocal.serverInfo.GetBool( "si_teamDamage" )
		&& !damageDef->GetBool( "noTeam" )
		&& player
		&& player != this		// you get self damage no matter what
		&& player->team == team ) {
			damage = 0;
	}

	*health = damage;
	*armor = armorSave;
}

/*
============
Damage

this		entity that is being damaged
inflictor	entity that is causing the damage
attacker	entity that caused the inflictor to damage targ
	example: this=monster, inflictor=rocket, attacker=player

dir			direction of the attack for knockback in global space

damageDef	an idDict with all the options for damage effects

inflictor, attacker, dir, and point can be NULL for environmental effects
============
*/
void idPlayer::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir,
					   const char *damageDefName, const float damageScale, const int location, 
					   trace_t *collision ) 
{
	idVec3		kick;
	int			damage;
	int			armorSave;
	int			knockback;
	idVec3		damage_from;
	idVec3		localDamageVector;	
	float		attackerPushScale;

	// damage is only processed on server
	if ( gameLocal.isClient ) {
		return;
	}
	
	if ( !fl.takedamage || noclip || spectating || gameLocal.inCinematic ) {
		return;
	}

	if ( !inflictor ) {
		inflictor = gameLocal.world;
	}
	if ( !attacker ) {
		attacker = gameLocal.world;
	}

	if ( attacker->IsType( idAI::Type ) ) {
		// don't take damage from monsters during influences
		if ( influenceActive != 0 ) {
			return;
		}
	}

	const idDeclEntityDef *damageDef = gameLocal.FindEntityDef( damageDefName, false );
	if ( !damageDef ) {
		gameLocal.Warning( "Unknown damageDef '%s'", damageDefName );
		return;
	}

	if ( damageDef->dict.GetBool( "ignore_player" ) ) {
		return;
	}

	CalcDamagePoints( inflictor, attacker, &damageDef->dict, damageScale, location, &damage, &armorSave );

	// determine knockback
	damageDef->dict.GetInt( "knockback", "20", knockback );

	if ( knockback != 0 && !fl.noknockback ) {
		if ( attacker == this ) {
			damageDef->dict.GetFloat( "attackerPushScale", "0", attackerPushScale );
		} else {
			attackerPushScale = 1.0f;
		}

		kick = dir;
		kick.Normalize();
		kick *= g_knockback.GetFloat() * knockback * attackerPushScale / 200.0f;
		physicsObj.SetLinearVelocity( physicsObj.GetLinearVelocity() + kick );

		// set the timer so that the player can't cancel out the movement immediately
		physicsObj.SetKnockBack( idMath::ClampInt( 50, 200, knockback * 2 ) );
	}

	// give feedback on the player view and audibly when armor is helping
	/*if ( armorSave ) {
		inventory.armor -= armorSave;

		if ( gameLocal.time > lastArmorPulse + 200 ) {
			StartSound( "snd_hitArmor", SND_CHANNEL_ITEM, 0, false, NULL );
		}
		lastArmorPulse = gameLocal.time;
	}*/
	
	if ( damageDef->dict.GetBool( "burn" ) ) 
	{
		StartSound( "snd_burn", SND_CHANNEL_BODY3, 0, false, NULL );
	}

	if ( g_debugDamage.GetInteger() ) {
		gameLocal.Printf( "client:%i health:%i damage:%i armor:%i\n", 
			entityNumber, health, damage, armorSave );
	}

	// move the world direction vector to local coordinates
	damage_from = dir;
	damage_from.Normalize();
	
	viewAxis.ProjectVector( damage_from, localDamageVector );

	// add to the damage inflicted on a player this frame
	// the total will be turned into screen blends and view angle kicks
	// at the end of the frame
	if ( health > 0 ) {
		playerView.DamageImpulse( localDamageVector, &damageDef->dict );
	}

	// do the damage
	if ( damage > 0 ) {

		if ( !gameLocal.isMultiplayer ) {
			float scale = g_damageScale.GetFloat();
			if ( g_useDynamicProtection.GetBool() && g_skill.GetInteger() < 2 ) {
				if ( gameLocal.time > lastDmgTime + 500 && scale > 0.25f ) {
					scale -= 0.05f;
					g_damageScale.SetFloat( scale );
				}
			}

			if ( scale > 0.0f ) {
				damage *= scale;
			}
		}

		if ( damage < 1 ) {
			damage = 1;
		}

//		int oldHealth = health;
		health -= damage;

		if ( health <= 0 ) {

			if ( health < -999 ) {
				health = -999;
			}

			isTelefragged = damageDef->dict.GetBool( "telefrag" );

			lastDmgTime = gameLocal.time;
			Killed( inflictor, attacker, damage, dir, location );

		} else {
			// force a blink
			blink_time = 0;

			if (!damageDef->dict.GetBool( "no_pain" )) {
				// let the anim script know we took damage
				AI_PAIN = Pain( inflictor, attacker, damage, dir, location );
			}
			
			// FIX: if drowning, stop pain SFX and play drown SFX on voice channel
			if ( damageDef->dict.GetBool( "no_air" ) ) 
			{
				if ( !armorSave && health > 0 ) 
				{
					StopSound( SND_CHANNEL_VOICE, false );
					StartSound( "snd_airGasp", SND_CHANNEL_VOICE, 0, false, NULL );
				}
			}

			if ( !g_testDeath.GetBool() ) {
				lastDmgTime = gameLocal.time;
			}
		}
	} else {
		// don't accumulate impulses
		if ( af.IsLoaded() ) {
			// clear impacts
			af.Rest();

			// physics is turned off by calling af.Rest()
			BecomeActive( TH_PHYSICS );
		}
	}

	lastDamageDef = damageDef->Index();
	lastDamageDir = damage_from;
	lastDamageLocation = location;
}

/*
===========
idPlayer::Teleport
============
*/
void idPlayer::Teleport( const idVec3 &origin, const idAngles &angles, idEntity *destination ) {
	idVec3 org;

	if ( weapon.GetEntity() ) {
		weapon.GetEntity()->LowerWeapon();
	}

	SetOrigin( origin + idVec3( 0, 0, CM_CLIP_EPSILON ) );
	if ( !gameLocal.isMultiplayer && GetFloorPos( 16.0f, org ) ) {
		SetOrigin( org );
	}

	// clear the ik heights so model doesn't appear in the wrong place
	walkIK.EnableAll();

	GetPhysics()->SetLinearVelocity( vec3_origin );

	SetViewAngles( angles );

	legsYaw = 0.0f;
	idealLegsYaw = 0.0f;
	oldViewYaw = viewAngles.yaw;

	if ( gameLocal.isMultiplayer ) {
		playerView.Flash( colorWhite, 140 );
	}

	UpdateVisuals();

	teleportEntity = destination;

	if ( !gameLocal.isClient && !noclip ) {
		if ( gameLocal.isMultiplayer ) {
			// kill anything at the new position or mark for kill depending on immediate or delayed teleport
			gameLocal.KillBox( this, destination != NULL );
		} else {
			// kill anything at the new position
			gameLocal.KillBox( this, true );
		}
	}
}

/*
====================
idPlayer::SetPrivateCameraView
====================
*/
void idPlayer::SetPrivateCameraView( idCamera *camView ) {
	privateCameraView = camView;
	if ( camView ) {
		StopFiring();
		Hide();
	} else {
		if ( !spectating ) {
			Show();
		}
	}
}

/*
====================
idPlayer::DefaultFov

Returns the base FOV
====================
*/
float idPlayer::DefaultFov( void ) const {
	float fov;

	fov = g_fov.GetFloat();
	if ( gameLocal.isMultiplayer ) {
		if ( fov < 90.0f ) {
			return 90.0f;
		} else if ( fov > 110.0f ) {
			return 110.0f;
		}
	}

	return fov;
}

/*
====================
idPlayer::CalcFov

Fixed fov at intermissions, otherwise account for fov variable and zooms.
====================
*/
float idPlayer::CalcFov( bool honorZoom ) {
	float fov;

	if ( fxFov ) {
		return DefaultFov() + 10.0f + cos( ( gameLocal.time + 2000 ) * 0.01 ) * 10.0f;
	}

	if ( influenceFov ) {
		return influenceFov;
	}

	// prevent FOV from zooming if the player is holding an object
	if( g_Global.m_DarkModPlayer->grabber->GetSelected() ) {
		return DefaultFov();
	}

	if ( zoomFov.IsDone( gameLocal.time ) ) {
		fov = ( honorZoom && usercmd.buttons & BUTTON_ZOOM ) && weapon.GetEntity() ? weapon.GetEntity()->GetZoomFov() : DefaultFov();
	} else {
		fov = zoomFov.GetCurrentValue( gameLocal.time );
	}

	// bound normal viewsize
	if ( fov < 1 ) {
		fov = 1;
	} else if ( fov > 179 ) {
		fov = 179;
	}

	return fov;
}

/*
==============
idPlayer::GunTurningOffset

generate a rotational offset for the gun based on the view angle
history in loggedViewAngles
==============
*/
idAngles idPlayer::GunTurningOffset( void ) {
	idAngles	a;

	a.Zero();

	if ( gameLocal.framenum < NUM_LOGGED_VIEW_ANGLES ) {
		return a;
	}

	idAngles current = loggedViewAngles[ gameLocal.framenum & (NUM_LOGGED_VIEW_ANGLES-1) ];

	idAngles	av, base;
	int weaponAngleOffsetAverages;
	float weaponAngleOffsetScale, weaponAngleOffsetMax;

	weapon.GetEntity()->GetWeaponAngleOffsets( &weaponAngleOffsetAverages, &weaponAngleOffsetScale, &weaponAngleOffsetMax );

	av = current;

	// calcualte this so the wrap arounds work properly
	for ( int j = 1 ; j < weaponAngleOffsetAverages ; j++ ) {
		idAngles a2 = loggedViewAngles[ ( gameLocal.framenum - j ) & (NUM_LOGGED_VIEW_ANGLES-1) ];

		idAngles delta = a2 - current;

		if ( delta[1] > 180 ) {
			delta[1] -= 360;
		} else if ( delta[1] < -180 ) {
			delta[1] += 360;
		}

		av += delta * ( 1.0f / weaponAngleOffsetAverages );
	}

	a = ( av - current ) * weaponAngleOffsetScale;

	for ( int i = 0 ; i < 3 ; i++ ) {
		if ( a[i] < -weaponAngleOffsetMax ) {
			a[i] = -weaponAngleOffsetMax;
		} else if ( a[i] > weaponAngleOffsetMax ) {
			a[i] = weaponAngleOffsetMax;
		}
	}

	return a;
}

/*
==============
idPlayer::GunAcceleratingOffset

generate a positional offset for the gun based on the movement
history in loggedAccelerations
==============
*/
idVec3	idPlayer::GunAcceleratingOffset( void ) {
	idVec3	ofs;

	float weaponOffsetTime, weaponOffsetScale;

	ofs.Zero();

	weapon.GetEntity()->GetWeaponTimeOffsets( &weaponOffsetTime, &weaponOffsetScale );

	int stop = currentLoggedAccel - NUM_LOGGED_ACCELS;
	if ( stop < 0 ) {
		stop = 0;
	}
	for ( int i = currentLoggedAccel-1 ; i > stop ; i-- ) {
		loggedAccel_t	*acc = &loggedAccel[i&(NUM_LOGGED_ACCELS-1)];

		float	f;
		float	t = gameLocal.time - acc->time;
		if ( t >= weaponOffsetTime ) {
			break;	// remainder are too old to care about
		}

		f = t / weaponOffsetTime;
		f = ( cos( f * 2.0f * idMath::PI ) - 1.0f ) * 0.5f;
		ofs += f * weaponOffsetScale * acc->dir;
	}

	return ofs;
}

/*
==============
idPlayer::CalculateViewWeaponPos

Calculate the bobbing position of the view weapon
==============
*/
void idPlayer::CalculateViewWeaponPos( idVec3 &origin, idMat3 &axis ) {
	float		scale;
	float		fracsin;
	idAngles	angles;
	int			delta;

	// CalculateRenderView must have been called first
	const idVec3 &viewOrigin = firstPersonViewOrigin;
	const idMat3 &viewAxis = firstPersonViewAxis;

	// these cvars are just for hand tweaking before moving a value to the weapon def
	idVec3	gunpos( g_gun_x.GetFloat(), g_gun_y.GetFloat(), g_gun_z.GetFloat() );

	// as the player changes direction, the gun will take a small lag
	idVec3	gunOfs = GunAcceleratingOffset();
	origin = viewOrigin + ( gunpos + gunOfs ) * viewAxis;

	// on odd legs, invert some angles
	if ( bobCycle & 128 ) {
		scale = -xyspeed;
	} else {
		scale = xyspeed;
	}

	// gun angles from bobbing
	angles.roll		= scale * bobfracsin * 0.005f;
	angles.yaw		= scale * bobfracsin * 0.01f;
	angles.pitch	= xyspeed * bobfracsin * 0.005f;

	// gun angles from turning
	if ( gameLocal.isMultiplayer ) {
		idAngles offset = GunTurningOffset();
		offset *= g_mpWeaponAngleScale.GetFloat();
		angles += offset;
	} else {
		angles += GunTurningOffset();
	}

	idVec3 gravity = physicsObj.GetGravityNormal();

	// drop the weapon when landing after a jump / fall
	delta = gameLocal.time - landTime;
	if ( delta < LAND_DEFLECT_TIME ) {
		origin -= gravity * ( landChange*0.25f * delta / LAND_DEFLECT_TIME );
	} else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME ) {
		origin -= gravity * ( landChange*0.25f * (LAND_DEFLECT_TIME + LAND_RETURN_TIME - delta) / LAND_RETURN_TIME );
	}

	// speed sensitive idle drift
	scale = xyspeed + 40.0f;
	fracsin = scale * sin( MS2SEC( gameLocal.time ) ) * 0.01f;
	angles.roll		+= fracsin;
	angles.yaw		+= fracsin;
	angles.pitch	+= fracsin;

	axis = angles.ToMat3() * viewAxis;
}

/*
===============
idPlayer::OffsetThirdPersonView
===============
*/
void idPlayer::OffsetThirdPersonView( float angle, float range, float height, bool clip ) {
	idVec3			view;
	idVec3			focusAngles;
	trace_t			trace;
	idVec3			focusPoint;
	float			focusDist;
	float			forwardScale, sideScale;
	idVec3			origin;
	idAngles		angles;
	idMat3			axis;
	idBounds		bounds;

	angles = viewAngles;
	GetViewPos( origin, axis );

	if ( angle ) {
		angles.pitch = 0.0f;
	}

	if ( angles.pitch > 45.0f ) {
		angles.pitch = 45.0f;		// don't go too far overhead
	}

	focusPoint = origin + angles.ToForward() * THIRD_PERSON_FOCUS_DISTANCE;
	focusPoint.z += height;
	view = origin;
	view.z += 8 + height;

	angles.pitch *= 0.5f;
	renderView->viewaxis = angles.ToMat3() * physicsObj.GetGravityAxis();

	idMath::SinCos( DEG2RAD( angle ), sideScale, forwardScale );
	view -= range * forwardScale * renderView->viewaxis[ 0 ];
	view += range * sideScale * renderView->viewaxis[ 1 ];

	if ( clip ) {
		// trace a ray from the origin to the viewpoint to make sure the view isn't
		// in a solid block.  Use an 8 by 8 block to prevent the view from near clipping anything
		bounds = idBounds( idVec3( -4, -4, -4 ), idVec3( 4, 4, 4 ) );
		gameLocal.clip.TraceBounds( trace, origin, view, bounds, MASK_SOLID, this );
		if ( trace.fraction != 1.0f ) {
			view = trace.endpos;
			view.z += ( 1.0f - trace.fraction ) * 32.0f;

			// try another trace to this position, because a tunnel may have the ceiling
			// close enough that this is poking out
			gameLocal.clip.TraceBounds( trace, origin, view, bounds, MASK_SOLID, this );
			view = trace.endpos;
		}
	}

	// select pitch to look at focus point from vieword
	focusPoint -= view;
	focusDist = idMath::Sqrt( focusPoint[0] * focusPoint[0] + focusPoint[1] * focusPoint[1] );
	if ( focusDist < 1.0f ) {
		focusDist = 1.0f;	// should never happen
	}

	angles.pitch = - RAD2DEG( atan2( focusPoint.z, focusDist ) );
	angles.yaw -= angle;

	renderView->vieworg = view;
	renderView->viewaxis = angles.ToMat3() * physicsObj.GetGravityAxis();
	renderView->viewID = 0;
}

/*
===============
idPlayer::GetEyePosition
===============
*/
idVec3 idPlayer::GetEyePosition( void ) const {
	idVec3 org;
 
	// use the smoothed origin if spectating another player in multiplayer
	if ( gameLocal.isClient && entityNumber != gameLocal.localClientNum ) {
		org = smoothedOrigin;
	} else {
		org = GetPhysics()->GetOrigin();
	}

	/*!
	* Lean Mod
	* @author sophisticatedZombie (DH)
	* Move eye position due to leaning
	*/
	org += ((idPhysics_Player*)GetPhysics())->GetViewLeanTranslation();

	// This was in SDK untouched
	return org + ( GetPhysics()->GetGravityNormal() * -eyeOffset.z );
}

/*
===============
idPlayer::GetViewPos
===============
*/
void idPlayer::GetViewPos( idVec3 &origin, idMat3 &axis ) const {
	idAngles angles;

	// if dead, fix the angle and don't add any kick
	if ( health <= 0 ) {
		angles.yaw = viewAngles.yaw;
		angles.roll = 40;
		angles.pitch = -15;
		axis = angles.ToMat3();
		origin = GetEyePosition();
	} else {
		origin = GetEyePosition() + viewBob;
		angles = viewAngles + viewBobAngles + ((idPhysics_Player*)GetPhysics())->GetViewLeanAngles() + playerView.AngleOffset();

		axis = angles.ToMat3() * physicsObj.GetGravityAxis();

		// adjust the origin based on the camera nodal distance (eye distance from neck)
		origin += physicsObj.GetGravityNormal() * g_viewNodalZ.GetFloat();
		origin += axis[0] * g_viewNodalX.GetFloat() + axis[2] * g_viewNodalZ.GetFloat();
	}
}

/*
===============
idPlayer::CalculateFirstPersonView
===============
*/
void idPlayer::CalculateFirstPersonView( void ) {
	if ( ( pm_modelView.GetInteger() == 1 ) || ( ( pm_modelView.GetInteger() == 2 ) && ( health <= 0 ) ) ) 
	{
		//	Displays the view from the point of view of the "camera" joint in the player model

		idMat3 axis;
		idVec3 origin;
		idAngles ang;

		/*!
		* Lean mod: 
		* @author: sophisticatedZombie
		* Original line commented out
		* ang = viewBobAngles + playerView.AngleOffset();
		*/
		ang = viewBobAngles + ((idPhysics_Player*) GetPhysics())->GetViewLeanAngles() + playerView.AngleOffset();

		
		ang.yaw += viewAxis[ 0 ].ToYaw();
		
		jointHandle_t joint = animator.GetJointHandle( "camera" );
		animator.GetJointTransform( joint, gameLocal.time, origin, axis );
		firstPersonViewOrigin = ( origin + modelOffset ) * ( viewAxis * physicsObj.GetGravityAxis() ) + physicsObj.GetOrigin() + viewBob + physicsObj.GetViewLeanTranslation();
		firstPersonViewAxis = axis * ang.ToMat3() * physicsObj.GetGravityAxis();
	} else 
	{
		// offset for local bobbing and kicks
		GetViewPos( firstPersonViewOrigin, firstPersonViewAxis );
#if 0
		// shakefrom sound stuff only happens in first person
		firstPersonViewAxis = firstPersonViewAxis * playerView.ShakeAxis();
#endif
	}

	// Set the listener location (on the other side of a door if door leaning)
	if( static_cast<idPhysics_Player *>(GetPhysics())->IsDoorLeaning() 
			&& !gameLocal.inCinematic )
		SetListenerLoc( m_DoorListenLoc );
	else
		SetListenerLoc( firstPersonViewOrigin );
}

/*
==================
idPlayer::GetRenderView

Returns the renderView that was calculated for this tic
==================
*/
renderView_t *idPlayer::GetRenderView( void ) {
	return renderView;
}

/*
==================
idPlayer::CalculateRenderView

create the renderView for the current tic
==================
*/
void idPlayer::CalculateRenderView( void ) {
	int i;
	float range;

	if ( !renderView ) {
		renderView = new renderView_t;
	}
	memset( renderView, 0, sizeof( *renderView ) );

	// copy global shader parms
	for( i = 0; i < MAX_GLOBAL_SHADER_PARMS; i++ ) {
		renderView->shaderParms[ i ] = gameLocal.globalShaderParms[ i ];
	}
	renderView->globalMaterial = gameLocal.GetGlobalMaterial();
	renderView->time = gameLocal.time;

	// calculate size of 3D view
	renderView->x = 0;
	renderView->y = 0;
	renderView->width = SCREEN_WIDTH;
	renderView->height = SCREEN_HEIGHT;
	renderView->viewID = 0;

	// check if we should be drawing from a camera's POV
	if ( !noclip && (gameLocal.GetCamera() || privateCameraView) ) {
		// get origin, axis, and fov
		if ( privateCameraView ) {
			privateCameraView->GetViewParms( renderView );
		} else {
			gameLocal.GetCamera()->GetViewParms( renderView );
		}
	} else {
		if ( g_stopTime.GetBool() ) {
			renderView->vieworg = firstPersonViewOrigin;
			renderView->viewaxis = firstPersonViewAxis;

			if ( !pm_thirdPerson.GetBool() ) {
				// set the viewID to the clientNum + 1, so we can suppress the right player bodies and
				// allow the right player view weapons
				renderView->viewID = entityNumber + 1;
			}
		} else if ( pm_thirdPerson.GetBool() ) {
			OffsetThirdPersonView( pm_thirdPersonAngle.GetFloat(), pm_thirdPersonRange.GetFloat(), pm_thirdPersonHeight.GetFloat(), pm_thirdPersonClip.GetBool() );
		} else if ( pm_thirdPersonDeath.GetBool() ) {
			range = gameLocal.time < minRespawnTime ? ( gameLocal.time + RAGDOLL_DEATH_TIME - minRespawnTime ) * ( 120.0f / RAGDOLL_DEATH_TIME ) : 120.0f;
			OffsetThirdPersonView( 0.0f, 20.0f + range, 0.0f, false );
		} else {
			renderView->vieworg = firstPersonViewOrigin;
			renderView->viewaxis = firstPersonViewAxis;

			// set the viewID to the clientNum + 1, so we can suppress the right player bodies and
			// allow the right player view weapons
			renderView->viewID = entityNumber + 1;
		}
		
		// field of view
		gameLocal.CalcFov( CalcFov( true ), renderView->fov_x, renderView->fov_y );
	}

	if ( renderView->fov_y == 0 ) {
		common->Error( "renderView->fov_y == 0" );
	}

	if ( g_showviewpos.GetBool() ) {
		gameLocal.Printf( "%s : %s\n", renderView->vieworg.ToString(), renderView->viewaxis.ToAngles().ToString() );
	}
}

/*
=============
idPlayer::AddAIKill
=============
*/
void idPlayer::AddAIKill( void ) {
	// greebo: Disabled this routine, no soulcube in TDM
	return;
}

/*
=============
idPlayer::SetSoulCubeProjectile
=============
*/
void idPlayer::SetSoulCubeProjectile( idProjectile *projectile ) {
	soulCubeProjectile = projectile;
}

/*
=============
idPlayer::AddProjectilesFired
=============
*/
void idPlayer::AddProjectilesFired( int count ) {
	numProjectilesFired += count;
}

/*
=============
idPlayer::AddProjectileHites
=============
*/
void idPlayer::AddProjectileHits( int count ) {
	numProjectileHits += count;
}

/*
=============
idPlayer::SetLastHitTime
=============
*/
void idPlayer::SetLastHitTime( int time ) {
	idPlayer *aimed = NULL;

	if ( time && lastHitTime != time ) {
		lastHitToggle ^= 1;
	}
	lastHitTime = time;
	if ( !time ) {
		// level start and inits
		return;
	}
	if ( gameLocal.isMultiplayer && ( time - lastSndHitTime ) > 10 ) {
		lastSndHitTime = time;
		StartSound( "snd_hit_feedback", SND_CHANNEL_ANY, SSF_PRIVATE_SOUND, false, NULL );
	}
	if ( cursor ) {
		cursor->HandleNamedEvent( "hitTime" );
	}
	if ( hud ) {
		if ( MPAim != -1 ) {
			if ( gameLocal.entities[ MPAim ] && gameLocal.entities[ MPAim ]->IsType( idPlayer::Type ) ) {
				aimed = static_cast< idPlayer * >( gameLocal.entities[ MPAim ] );
			}
			assert( aimed );
			// full highlight, no fade till loosing aim
			hud->SetStateString( "aim_text", gameLocal.userInfo[ MPAim ].GetString( "ui_name" ) );
			if ( aimed ) {
				hud->SetStateFloat( "aim_color", aimed->colorBarIndex );
			}
			hud->HandleNamedEvent( "aim_flash" );
			MPAimHighlight = true;
			MPAimFadeTime = 0;
		} else if ( lastMPAim != -1 ) {
			if ( gameLocal.entities[ lastMPAim ] && gameLocal.entities[ lastMPAim ]->IsType( idPlayer::Type ) ) {
				aimed = static_cast< idPlayer * >( gameLocal.entities[ lastMPAim ] );
			}
			assert( aimed );
			// start fading right away
			hud->SetStateString( "aim_text", gameLocal.userInfo[ lastMPAim ].GetString( "ui_name" ) );
			if ( aimed ) {
				hud->SetStateFloat( "aim_color", aimed->colorBarIndex );
			}
			hud->HandleNamedEvent( "aim_flash" );
			hud->HandleNamedEvent( "aim_fade" );
			MPAimHighlight = false;
			MPAimFadeTime = gameLocal.realClientTime;
		}
	}
}

/*
=============
idPlayer::SetInfluenceLevel
=============
*/
void idPlayer::SetInfluenceLevel( int level ) {
	if ( level != influenceActive ) {
		if ( level ) {
			for ( idEntity *ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
				if ( ent->IsType( idProjectile::Type ) ) {
					// remove all projectiles
					ent->PostEventMS( &EV_Remove, 0 );
				}
			}
			if ( weaponEnabled && weapon.GetEntity() ) {
				weapon.GetEntity()->EnterCinematic();
			}
		} else {
			physicsObj.SetLinearVelocity( vec3_origin );
			if ( weaponEnabled && weapon.GetEntity() ) {
				weapon.GetEntity()->ExitCinematic();
			}
		}
		influenceActive = level;
	}
}

/*
=============
idPlayer::SetInfluenceView
=============
*/
void idPlayer::SetInfluenceView( const char *mtr, const char *skinname, float radius, idEntity *ent ) {
	influenceMaterial = NULL;
	influenceEntity = NULL;
	influenceSkin = NULL;
	if ( mtr && *mtr ) {
		influenceMaterial = declManager->FindMaterial( mtr );
	}
	if ( skinname && *skinname ) {
		influenceSkin = declManager->FindSkin( skinname );
		if ( head.GetEntity() ) {
			head.GetEntity()->GetRenderEntity()->shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );
		}
		UpdateVisuals();
	}
	influenceRadius = radius;
	if ( radius > 0.0f ) {
		influenceEntity = ent;
	}
}

/*
=============
idPlayer::SetInfluenceFov
=============
*/
void idPlayer::SetInfluenceFov( float fov ) {
	influenceFov = fov;
}

/*
================
idPlayer::OnLadder
================
*/
bool idPlayer::OnLadder( void ) const {
	return physicsObj.OnLadder();
}

/*
==================
idPlayer::Event_GetButtons
==================
*/
void idPlayer::Event_GetButtons( void ) {
	idThread::ReturnInt( usercmd.buttons );
}

/*
==================
idPlayer::Event_GetMove
==================
*/
void idPlayer::Event_GetMove( void ) {
	idVec3 move( usercmd.forwardmove, usercmd.rightmove, usercmd.upmove );
	idThread::ReturnVector( move );
}

/*
================
idPlayer::Event_GetViewAngles
================
*/
void idPlayer::Event_GetViewAngles( void ) {
	idThread::ReturnVector( idVec3( viewAngles[0], viewAngles[1], viewAngles[2] ) );
}

/*
==================
idPlayer::Event_StopFxFov
==================
*/
void idPlayer::Event_StopFxFov( void ) {
	fxFov = false;
}

/*
==================
idPlayer::StartFxFov 
==================
*/
void idPlayer::StartFxFov( float duration ) { 
	fxFov = true;
	PostEventSec( &EV_Player_StopFxFov, duration );
}

/*
==================
idPlayer::Event_EnableWeapon 
==================
*/
void idPlayer::Event_EnableWeapon( void ) {
	hiddenWeapon = gameLocal.world->spawnArgs.GetBool( "no_Weapons" );
	weaponEnabled = true;
	if ( weapon.GetEntity() ) {
		weapon.GetEntity()->ExitCinematic();
	}
}

/*
==================
idPlayer::Event_DisableWeapon
==================
*/
void idPlayer::Event_DisableWeapon( void ) {
	hiddenWeapon = gameLocal.world->spawnArgs.GetBool( "no_Weapons" );
	weaponEnabled = false;
	if ( weapon.GetEntity() ) {
		weapon.GetEntity()->EnterCinematic();
	}
}

/*
==================
idPlayer::Event_GetCurrentWeapon
==================
*/
void idPlayer::Event_GetCurrentWeapon( void ) {
	const char *weapon;

	if ( currentWeapon >= 0 ) {
		weapon = spawnArgs.GetString( va( "def_weapon%d", currentWeapon ) );
		idThread::ReturnString( weapon );
	} else {
		idThread::ReturnString( "" );
	}
}

/*
==================
idPlayer::Event_GetPreviousWeapon
==================
*/
void idPlayer::Event_GetPreviousWeapon( void ) {
	const char *weapon;

	if ( previousWeapon >= 0 ) {
		int pw = ( gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) ) ? 0 : previousWeapon;
		weapon = spawnArgs.GetString( va( "def_weapon%d", pw) );
		idThread::ReturnString( weapon );
	} else {
		idThread::ReturnString( spawnArgs.GetString( "def_weapon0" ) );
	}
}

/*
==================
idPlayer::Event_SelectWeapon
==================
*/
void idPlayer::Event_SelectWeapon( const char *weaponName ) {
	int i;
	int weaponNum;

	if ( gameLocal.isClient ) {
		gameLocal.Warning( "Cannot switch weapons from script in multiplayer" );
		return;
	}

	if ( hiddenWeapon && gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) ) {
		idealWeapon = weapon_fists;
		weapon.GetEntity()->HideWeapon();
		return;
	}

	weaponNum = -1;
	for( i = 0; i < MAX_WEAPONS; i++ ) {
		const char *weap = spawnArgs.GetString( va( "def_weapon%d", i ) );
		if ( !idStr::Cmp( weap, weaponName ) ) {
			weaponNum = i;
			break;
		}
	}
	// Try to select the weapon, might as well return false
	SelectWeapon(weaponNum, false);
}

/*
==================
idPlayer::Event_GetWeaponEntity
==================
*/
void idPlayer::Event_GetWeaponEntity( void ) {
	idThread::ReturnEntity( weapon.GetEntity() );
}

/*
==================
idPlayer::Event_OpenPDA
==================
*/
void idPlayer::Event_OpenPDA( void ) {
	if ( !gameLocal.isMultiplayer ) {
		TogglePDA();
	}
}

/*
==================
idPlayer::Event_InPDA
==================
*/
void idPlayer::Event_InPDA( void ) {
	idThread::ReturnInt( objectiveSystemOpen );
}

/*
==================
idPlayer::TeleportDeath
==================
*/
void idPlayer::TeleportDeath( int killer ) {
	teleportKiller = killer;
}

/*
==================
idPlayer::Event_ExitTeleporter
==================
*/
void idPlayer::Event_ExitTeleporter( void ) {
	idEntity	*exitEnt;
	float		pushVel;

	// verify and setup
	exitEnt = teleportEntity.GetEntity();
	if ( !exitEnt ) {
		common->DPrintf( "Event_ExitTeleporter player %d while not being teleported\n", entityNumber );
		return;
	}

	pushVel = exitEnt->spawnArgs.GetFloat( "push", "300" );

	if ( gameLocal.isServer ) {
		ServerSendEvent( EVENT_EXIT_TELEPORTER, NULL, false, -1 );
	}

	SetPrivateCameraView( NULL );
	// setup origin and push according to the exit target
	SetOrigin( exitEnt->GetPhysics()->GetOrigin() + idVec3( 0, 0, CM_CLIP_EPSILON ) );
	SetViewAngles( exitEnt->GetPhysics()->GetAxis().ToAngles() );
	physicsObj.SetLinearVelocity( exitEnt->GetPhysics()->GetAxis()[ 0 ] * pushVel );
	physicsObj.ClearPushedVelocity();
	// teleport fx
	playerView.Flash( colorWhite, 120 );

	// clear the ik heights so model doesn't appear in the wrong place
	walkIK.EnableAll();

	UpdateVisuals();

	StartSound( "snd_teleport_exit", SND_CHANNEL_ANY, 0, false, NULL );

	if ( teleportKiller != -1 ) {
		// we got killed while being teleported
		Damage( gameLocal.entities[ teleportKiller ], gameLocal.entities[ teleportKiller ], vec3_origin, "damage_telefrag", 1.0f, INVALID_JOINT );
		teleportKiller = -1;
	} else {
		// kill anything that would have waited at teleport exit
		gameLocal.KillBox( this );
	}
	teleportEntity = NULL;
}

/*
================
idPlayer::ClientPredictionThink
================
*/
void idPlayer::ClientPredictionThink( void ) {
	renderEntity_t *headRenderEnt;

	oldFlags = usercmd.flags;
	oldButtons = usercmd.buttons;

	usercmd = gameLocal.usercmds[ entityNumber ];

	if ( entityNumber != gameLocal.localClientNum ) {

		// ignore attack button of other clients. that's no good for predictions

		usercmd.buttons &= ~BUTTON_ATTACK;

	}



	buttonMask &= usercmd.buttons;
	usercmd.buttons &= ~buttonMask;

	if ( objectiveSystemOpen ) {
		usercmd.forwardmove = 0;
		usercmd.rightmove = 0;
		usercmd.upmove = 0;
	}

	// clear the ik before we do anything else so the skeleton doesn't get updated twice
	walkIK.ClearJointMods();

	if ( gameLocal.isNewFrame ) {
		if ( ( usercmd.flags & UCF_IMPULSE_SEQUENCE ) != ( oldFlags & UCF_IMPULSE_SEQUENCE ) ) {
			PerformImpulse( usercmd.impulse );
		}
	}

	scoreBoardOpen = ( ( usercmd.buttons & BUTTON_SCORES ) != 0 || forceScoreBoard );

	AdjustSpeed();

	UpdateViewAngles();

	// update the smoothed view angles
	if ( gameLocal.framenum >= smoothedFrame && entityNumber != gameLocal.localClientNum ) {
		idAngles anglesDiff = viewAngles - smoothedAngles;
		anglesDiff.Normalize180();
		if ( idMath::Fabs( anglesDiff.yaw ) < 90.0f && idMath::Fabs( anglesDiff.pitch ) < 90.0f ) {
			// smoothen by pushing back to the previous angles
			viewAngles -= gameLocal.clientSmoothing * anglesDiff;
			viewAngles.Normalize180();
		}
		smoothedAngles = viewAngles;
	}
	smoothedOriginUpdated = false;

	if ( !af.IsActive() ) {
		AdjustBodyAngles();
	}

	if ( !isLagged ) {
		// don't allow client to move when lagged
		Move();
	} 

	// update GUIs, Items, and character interactions
	UpdateFocus();

	// service animations
	if ( !spectating && !af.IsActive() ) {
		m_ButtonStateTracker.update();
    	UpdateConditions();
		UpdateAnimState();
		CheckBlink();
	}

	// clear out our pain flag so we can tell if we recieve any damage between now and the next time we think
	AI_PAIN = false;

	// calculate the exact bobbed view position, which is used to
	// position the view weapon, among other things
	CalculateFirstPersonView();

	// this may use firstPersonView, or a thirdPerson / camera view
	CalculateRenderView();

	if ( !gameLocal.inCinematic && weapon.GetEntity() && ( health > 0 ) && !( gameLocal.isMultiplayer && spectating ) ) {
		UpdateWeapon();
	}

	UpdateHud();

	if ( gameLocal.isNewFrame ) {
		UpdatePowerUps();
	}

	UpdateDeathSkin( false );

	if ( head.GetEntity() ) {
		headRenderEnt = head.GetEntity()->GetRenderEntity();
	} else {
		headRenderEnt = NULL;
	}

	if ( headRenderEnt ) {
		if ( influenceSkin ) {
			headRenderEnt->customSkin = influenceSkin;
		} else {
			headRenderEnt->customSkin = NULL;
		}
	}

	if ( gameLocal.isMultiplayer || g_showPlayerShadow.GetBool() ) {
		renderEntity.suppressShadowInViewID	= 0;
		if ( headRenderEnt ) {
			headRenderEnt->suppressShadowInViewID = 0;
		}
	} else {
		renderEntity.suppressShadowInViewID	= entityNumber+1;
		if ( headRenderEnt ) {
			headRenderEnt->suppressShadowInViewID = entityNumber+1;
		}
	}
	// never cast shadows from our first-person muzzle flashes
	renderEntity.suppressShadowInLightID = LIGHTID_VIEW_MUZZLE_FLASH + entityNumber;
	if ( headRenderEnt ) {
		headRenderEnt->suppressShadowInLightID = LIGHTID_VIEW_MUZZLE_FLASH + entityNumber;
	}

	if ( !gameLocal.inCinematic ) {
		UpdateAnimation();
	}

	if ( gameLocal.isMultiplayer ) {
		DrawPlayerIcons();
	}

	Present();

	UpdateDamageEffects();

	LinkCombat();

	if ( gameLocal.isNewFrame && entityNumber == gameLocal.localClientNum ) {
		playerView.CalculateShake();
	}

	// determine if portal sky is in pvs

	pvsHandle_t	clientPVS = gameLocal.pvs.SetupCurrentPVS( GetPVSAreas(), GetNumPVSAreas() );

	gameLocal.portalSkyActive = gameLocal.pvs.CheckAreasForPortalSky( clientPVS, GetPhysics()->GetOrigin() );

	gameLocal.pvs.FreeCurrentPVS( clientPVS );

}

/*
================
idPlayer::GetPhysicsToVisualTransform
================
*/
bool idPlayer::GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis ) {
	if ( af.IsActive() ) {
		af.GetPhysicsToVisualTransform( origin, axis );
		return true;
	}

	// smoothen the rendered origin and angles of other clients
	if ( gameLocal.isClient && gameLocal.framenum >= smoothedFrame && ( entityNumber != gameLocal.localClientNum || selfSmooth ) ) {

		// render origin and axis
		idMat3 renderAxis = viewAxis * GetPhysics()->GetAxis();
		idVec3 renderOrigin = GetPhysics()->GetOrigin() + modelOffset * renderAxis;

		// update the smoothed origin
		if ( !smoothedOriginUpdated ) {
			idVec2 originDiff = renderOrigin.ToVec2() - smoothedOrigin.ToVec2();
			if ( originDiff.LengthSqr() < Square( 100.0f ) ) {
				// smoothen by pushing back to the previous position
				if ( selfSmooth ) {

					assert( entityNumber == gameLocal.localClientNum );

					renderOrigin.ToVec2() -= net_clientSelfSmoothing.GetFloat() * originDiff;

				} else {

					renderOrigin.ToVec2() -= gameLocal.clientSmoothing * originDiff;
				}
			}
			smoothedOrigin = renderOrigin;

			smoothedFrame = gameLocal.framenum;
			smoothedOriginUpdated = true;
		}

		axis = idAngles( 0.0f, smoothedAngles.yaw, 0.0f ).ToMat3();
		origin = ( smoothedOrigin - GetPhysics()->GetOrigin() ) * axis.Transpose();

	} else {

		axis = viewAxis;
		origin = modelOffset;
	}
	return true;
}

/*
================
idPlayer::GetPhysicsToSoundTransform
================
*/
bool idPlayer::GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis ) {
	idCamera *camera;

	if ( privateCameraView ) {
		camera = privateCameraView;
	} else {
		camera = gameLocal.GetCamera();
	}

	if ( camera ) {
		renderView_t view;

		memset( &view, 0, sizeof( view ) );
		camera->GetViewParms( &view );
		origin = view.vieworg;
		axis = view.viewaxis;
		return true;
	} else {
		return idActor::GetPhysicsToSoundTransform( origin, axis );
	}
}

/*
================
idPlayer::WriteToSnapshot
================
*/
void idPlayer::WriteToSnapshot( idBitMsgDelta &msg ) const {
	physicsObj.WriteToSnapshot( msg );
	WriteBindToSnapshot( msg );
	msg.WriteDeltaFloat( 0.0f, deltaViewAngles[0] );
	msg.WriteDeltaFloat( 0.0f, deltaViewAngles[1] );
	msg.WriteDeltaFloat( 0.0f, deltaViewAngles[2] );
	msg.WriteShort( health );
	msg.WriteBits( gameLocal.ServerRemapDecl( -1, DECL_ENTITYDEF, lastDamageDef ), gameLocal.entityDefBits );
	msg.WriteDir( lastDamageDir, 9 );
	msg.WriteShort( lastDamageLocation );
	msg.WriteBits( idealWeapon, idMath::BitsForInteger( MAX_WEAPONS ) );
	msg.WriteBits( weapon.GetSpawnId(), 32 );
	msg.WriteBits( spectator, idMath::BitsForInteger( MAX_CLIENTS ) );
	msg.WriteBits( lastHitToggle, 1 );
	msg.WriteBits( weaponGone, 1 );
	msg.WriteBits( isLagged, 1 );
	msg.WriteBits( isChatting, 1 );
}

/*
================
idPlayer::ReadFromSnapshot
================
*/
void idPlayer::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	int		oldHealth, newIdealWeapon, weaponSpawnId;
	bool	newHitToggle, stateHitch;

	if ( snapshotSequence - lastSnapshotSequence > 1 ) {
		stateHitch = true;
	} else {
		stateHitch = false;
	}
	lastSnapshotSequence = snapshotSequence;

	oldHealth = health;

	physicsObj.ReadFromSnapshot( msg );
	ReadBindFromSnapshot( msg );
	deltaViewAngles[0] = msg.ReadDeltaFloat( 0.0f );
	deltaViewAngles[1] = msg.ReadDeltaFloat( 0.0f );
	deltaViewAngles[2] = msg.ReadDeltaFloat( 0.0f );
	health = msg.ReadShort();
	lastDamageDef = gameLocal.ClientRemapDecl( DECL_ENTITYDEF, msg.ReadBits( gameLocal.entityDefBits ) );
	lastDamageDir = msg.ReadDir( 9 );
	lastDamageLocation = msg.ReadShort();
	newIdealWeapon = msg.ReadBits( idMath::BitsForInteger( MAX_WEAPONS ) );
	weaponSpawnId = msg.ReadBits( 32 );
	spectator = msg.ReadBits( idMath::BitsForInteger( MAX_CLIENTS ) );
	newHitToggle = msg.ReadBits( 1 ) != 0;
	weaponGone = msg.ReadBits( 1 ) != 0;
	isLagged = msg.ReadBits( 1 ) != 0;
	isChatting = msg.ReadBits( 1 ) != 0;

	// no msg reading below this

	if ( weapon.SetSpawnId( weaponSpawnId ) ) {
		if ( weapon.GetEntity() ) {
			// maintain ownership locally
			weapon.GetEntity()->SetOwner( this );
		}
		currentWeapon = -1;
	}

	if ( oldHealth > 0 && health <= 0 ) {
		if ( stateHitch ) {
			// so we just hide and don't show a death skin
			UpdateDeathSkin( true );
		}
		// die
		AI_DEAD = true;
		SetAnimState( ANIMCHANNEL_LEGS, "Legs_Death", 4 );
		SetAnimState( ANIMCHANNEL_TORSO, "Torso_Death", 4 );
		SetWaitState( "" );
		animator.ClearAllJoints();
		if ( entityNumber == gameLocal.localClientNum ) {
			playerView.Fade( colorBlack, 12000 );
		}
		StartRagdoll();
		physicsObj.SetMovementType( PM_DEAD );
		if ( !stateHitch ) {
			StartSound( "snd_death", SND_CHANNEL_VOICE, 0, false, NULL );
		}
		if ( weapon.GetEntity() ) {
			weapon.GetEntity()->OwnerDied();
		}
	} else if ( oldHealth <= 0 && health > 0 ) {
		// respawn
		Init();
		StopRagdoll();
		SetPhysics( &physicsObj );
		physicsObj.EnableClip();
		SetCombatContents( true );
	} else if ( health < oldHealth && health > 0 ) {
		if ( stateHitch ) {
			lastDmgTime = gameLocal.time;
		} else {
			// damage feedback
			const idDeclEntityDef *def = static_cast<const idDeclEntityDef *>( declManager->DeclByIndex( DECL_ENTITYDEF, lastDamageDef, false ) );
			if ( def ) {
				playerView.DamageImpulse( lastDamageDir * viewAxis.Transpose(), &def->dict );
				AI_PAIN = Pain( NULL, NULL, oldHealth - health, lastDamageDir, lastDamageLocation );
				lastDmgTime = gameLocal.time;
			} else {
				common->Warning( "NET: no damage def for damage feedback '%d'\n", lastDamageDef );
			}
		}
	} else if ( health > oldHealth && !stateHitch ) {
		// just pulse, for any health raise
		healthPulse = true;
	}

	// If the player is alive, restore proper physics object

	if ( health > 0 && IsActiveAF() ) {

		StopRagdoll();

		SetPhysics( &physicsObj );

		physicsObj.EnableClip();

		SetCombatContents( true );

	}



	if ( idealWeapon != newIdealWeapon ) {
		if ( stateHitch ) {
			weaponCatchup = true;
		}
		idealWeapon = newIdealWeapon;
		UpdateHudWeapon();
	}

	if ( lastHitToggle != newHitToggle ) {
		SetLastHitTime( gameLocal.realClientTime );
	}

	if ( msg.HasChanged() ) {
		UpdateVisuals();
	}
}

/*
================
idPlayer::WritePlayerStateToSnapshot
================
*/
void idPlayer::WritePlayerStateToSnapshot( idBitMsgDelta &msg ) const {
	msg.WriteByte( bobCycle );
	msg.WriteLong( stepUpTime );
	msg.WriteFloat( stepUpDelta );
}

/*
================
idPlayer::ReadPlayerStateFromSnapshot
================
*/
void idPlayer::ReadPlayerStateFromSnapshot( const idBitMsgDelta &msg ) {
	bobCycle = msg.ReadByte();
	stepUpTime = msg.ReadLong();
	stepUpDelta = msg.ReadFloat();
}

/*
================
idPlayer::ServerReceiveEvent
================
*/
bool idPlayer::ServerReceiveEvent( int event, int time, const idBitMsg &msg ) {

	if ( idEntity::ServerReceiveEvent( event, time, msg ) ) {
		return true;
	}

	// client->server events
	switch( event ) {
		case EVENT_IMPULSE: {
			PerformImpulse( msg.ReadBits( 6 ) );
			return true;
		}
		default: {
			return false;
		}
	}
}

/*
================
idPlayer::ClientReceiveEvent
================
*/
bool idPlayer::ClientReceiveEvent( int event, int time, const idBitMsg &msg ) {
	switch ( event ) {
		case EVENT_EXIT_TELEPORTER:
			Event_ExitTeleporter();
			return true;
		case EVENT_ABORT_TELEPORTER:
			SetPrivateCameraView( NULL );
			return true;
		case EVENT_POWERUP: {
			// greebo: No EVENT_POWERUP handling at the moment
			return true;
		}
		case EVENT_SPECTATE: {
			bool spectate = ( msg.ReadBits( 1 ) != 0 );
			Spectate( spectate );
			return true;
		}
		case EVENT_ADD_DAMAGE_EFFECT: {
			if ( spectating ) {
				// if we're spectating, ignore
				// happens if the event and the spectate change are written on the server during the same frame (fraglimit)
				return true;
			}
			return idActor::ClientReceiveEvent( event, time, msg );
		}
		default: {
			return idActor::ClientReceiveEvent( event, time, msg );
		}
	}
//	return false;
}

/*
================
idPlayer::Hide
================
*/
void idPlayer::Hide( void ) {
	idWeapon *weap;

	idActor::Hide();
	weap = weapon.GetEntity();
	if ( weap ) {
		weap->HideWorldModel();
	}
}

/*
================
idPlayer::Show
================
*/
void idPlayer::Show( void ) {
	idWeapon *weap;
	
	idActor::Show();
	weap = weapon.GetEntity();
	if ( weap ) {
		weap->ShowWorldModel();
	}
}

/*
===============
idPlayer::StartAudioLog
===============
*/
void idPlayer::StartAudioLog( void ) {
	if ( hud ) {
		hud->HandleNamedEvent( "audioLogUp" );
	}
}

/*
===============
idPlayer::StopAudioLog
===============
*/
void idPlayer::StopAudioLog( void ) {
	if ( hud ) {
		hud->HandleNamedEvent( "audioLogDown" );
	}
}

/*
===============
idPlayer::ShowTip
===============
*/
void idPlayer::ShowTip( const char *title, const char *tip, bool autoHide ) {
	if ( tipUp ) {
		return;
	}
	hud->SetStateString( "tip", tip );
	hud->SetStateString( "tiptitle", title );
	hud->HandleNamedEvent( "tipWindowUp" ); 
	if ( autoHide ) {
		PostEventSec( &EV_Player_HideTip, 5.0f );
	}
	tipUp = true;
}

/*
===============
idPlayer::HideTip
===============
*/
void idPlayer::HideTip( void ) {
	hud->HandleNamedEvent( "tipWindowDown" ); 
	tipUp = false;
}

/*
===============
idPlayer::Event_HideTip
===============
*/
void idPlayer::Event_HideTip( void ) {
	HideTip();
}

/*
===============
idPlayer::ShowObjective
===============
*/
void idPlayer::ShowObjective( const char *obj ) {
	hud->HandleNamedEvent( obj );
	objectiveUp = true;
}

/*
===============
idPlayer::HideObjective
===============
*/
void idPlayer::HideObjective( void ) {
	hud->HandleNamedEvent( "closeObjective" );
	objectiveUp = false;
}

/*
===============
idPlayer::Event_StopAudioLog
===============
*/
void idPlayer::Event_StopAudioLog( void ) {
	StopAudioLog();
}

/*
===============
idPlayer::SetSpectateOrigin
===============
*/
void idPlayer::SetSpectateOrigin( void ) {
	idVec3 neworig;

	neworig = GetPhysics()->GetOrigin();
	neworig[ 2 ] += EyeHeight();
	neworig[ 2 ] += 25;
	SetOrigin( neworig );
}

/*
===============
idPlayer::RemoveWeapon
===============
*/
void idPlayer::RemoveWeapon( const char *weap ) {
	if ( weap && *weap ) {
		inventory.Drop( spawnArgs, spawnArgs.GetString( weap ), -1 );
	}
}

/*
===============
idPlayer::CanShowWeaponViewmodel
===============
*/
bool idPlayer::CanShowWeaponViewmodel( void ) const {
	return showWeaponViewModel;
}

/*
===============
idPlayer::SetLevelTrigger
===============
*/
void idPlayer::SetLevelTrigger( const char *levelName, const char *triggerName ) {
	if ( levelName && *levelName && triggerName && *triggerName ) {
		idLevelTriggerInfo lti;
		lti.levelName = levelName;
		lti.triggerName = triggerName;
		levelTriggers.Append( lti );
	}
}

/*
===============
idPlayer::Event_LevelTrigger
===============
*/
void idPlayer::Event_LevelTrigger( void ) {
	idStr mapName = gameLocal.GetMapName();
	mapName.StripPath();
	mapName.StripFileExtension();
	for ( int i = levelTriggers.Num() - 1; i >= 0; i-- ) {
		if ( idStr::Icmp( mapName, levelTriggers[i].levelName) == 0 ){
			idEntity *ent = gameLocal.FindEntity( levelTriggers[i].triggerName );
			if ( ent ) {
				ent->PostEventMS( &EV_Activate, 1, this );
			}
		}
	}
}

/*
===============
idPlayer::Event_Gibbed
===============
*/
void idPlayer::Event_Gibbed( void ) {
	// do nothing
}

/*
==================
idPlayer::Event_GetIdealWeapon 
==================
*/
void idPlayer::Event_GetIdealWeapon( void ) {
	const char *weapon;

	if ( idealWeapon >= 0 ) {
		weapon = spawnArgs.GetString( va( "def_weapon%d", idealWeapon ) );
		idThread::ReturnString( weapon );
	} else {
		idThread::ReturnString( "" );
	}
}

/*
===============
idPlayer::UpdatePlayerIcons
===============
*/
void idPlayer::UpdatePlayerIcons( void ) {
	int time = networkSystem->ServerGetClientTimeSinceLastPacket( entityNumber );
	if ( time > cvarSystem->GetCVarInteger( "net_clientMaxPrediction" ) ) {
		isLagged = true;
	} else {
		isLagged = false;
	}
	// TODO: chatting, PDA, etc?
}

/*
===============
idPlayer::DrawPlayerIcons
===============
*/
void idPlayer::DrawPlayerIcons( void ) {
	if ( !NeedsIcon() ) {
		playerIcon.FreeIcon();
		return;
	}
	playerIcon.Draw( this, headJoint );
}

/*
===============
idPlayer::HidePlayerIcons
===============
*/
void idPlayer::HidePlayerIcons( void ) {
	playerIcon.FreeIcon();
}

/*
===============
idPlayer::NeedsIcon
==============
*/
bool idPlayer::NeedsIcon( void ) {
	// local clients don't render their own icons... they're only info for other clients

	return entityNumber != gameLocal.localClientNum && ( isLagged || isChatting );

}

void idPlayer::AdjustLightgem(void)
{
	idVec3 vDifference;
	double fx, fy;
	double fDistance;
	double fLightgemVal;
	idVec3 vLightColor;
	idVec3 vPlayer;
	idLight *light, *helper;
	trace_t trace;
	CDarkModPlayer *pDM = g_Global.m_DarkModPlayer;
	int i, n, h = -1, l;
	bool bMinOneLight = false, bStump;

	DM_LOG(LC_FUNCTION, LT_DEBUG)LOGSTRING("[%s]\r", __FUNCTION__);

	NextFrame = true;
	fLightgemVal = 0;
	n = pDM->m_LightList.Num();

	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("%u entities found within lightradius\r", n);
	idVec3 vStart(GetEyePosition());
	idVec3 vPlayerPos(GetPhysics()->GetOrigin());
	idVec3 vPlayerSeg[LSG_COUNT];
	idVec3 vLightCone[ELC_COUNT];
	idVec3 vResult[2];
	EIntersection inter;

	vPlayerSeg[LSG_ORIGIN] = vPlayerPos;
	vPlayerSeg[LSG_DIRECTION] = vStart - vPlayerPos;

	for(i = 0; i < n; i++)
	{
		if((light = dynamic_cast<idLight *>(pDM->m_LightList[i])) == NULL)
			continue;

		vPlayer = vPlayerPos;
		idVec3 vLight(light->GetPhysics()->GetOrigin());
		vPlayer.z = vLight.z;
		vDifference = vPlayer - vLight;
		fDistance = vDifference.Length();
		DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Ligth: [%s]  %i  px: %f   py: %f   pz: %f   -   lx: %f   ly: %f   lz: %f   Distance: %f\r", 
			light->name.c_str(), i, vPlayer.x, vPlayer.y, vPlayer.z, vLight.x, vLight.y, vLight.z, fDistance);

		// Fast and cheap test to see if the player could be in the area of the light.
		// Well, it is not exactly cheap, but it is the cheapest test that we can do at this point. :)
		if(fDistance > light->m_MaxLightRadius)
		{
			DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("%s is outside distance: %f/%f\r", light->name.c_str(), light->m_MaxLightRadius, fDistance);
			if(h == -1)
				h = i;
			continue;
		}

		if(light->IsPointlight())
		{
			light->GetLightCone(vLightCone[ELL_ORIGIN], vLightCone[ELA_AXIS], vLightCone[ELA_CENTER]);
			inter = IntersectLineEllipsoid(vPlayerSeg, vLightCone, vResult);

			// If this is a centerlight we have to move the origin from the original origin to where the
			// center of the light is supposed to be.
			// Centerlight means that the center of the ellipsoid is not the same as the origin. It has to
			// be adjusted because if it casts shadows we have to trace to it, and in this case the light
			// might be inside geometry and would be reported as not being visible even though it casts
			// a visible light outside the geometry it is embedded. If it is not a centerlight and has
			// cast shadows enabled, it wouldn't cast any light at all in such a case because it would
			// be blocked by the geometry.
			vLight = vLightCone[ELL_ORIGIN] + vLightCone[ELA_CENTER];
			DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("IntersectLineEllipsoid returned %u\r", inter);
		}
		else
		{
			bStump = false;
			light->GetLightCone(vLightCone[ELC_ORIGIN], vLightCone[ELA_TARGET], vLightCone[ELA_RIGHT], vLightCone[ELA_UP], vLightCone[ELA_START], vLightCone[ELA_END]);
			inter = IntersectLineCone(vPlayerSeg, vLightCone, vResult, bStump);
			DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("IntersectLineCone returned %u\r", inter);
		}

		// The line intersection can only return three states. Either the line is passing
		// through the lightcone in which case we will get two intersection points, the line
		// is not passing through which means that the player is fully outside, or the line
		// is touching the cone in exactly one point. The last case is not really helpfull in
		// our case and doesn't make a difference for the gameplay so we simply ignore it and
		// consider only cases where the player is at least partially inside the cone.
		if(inter != INTERSECT_FULL)
			continue;

		if(light->CastsShadow() == true)
		{
			gameLocal.clip.TracePoint(trace, vStart, vLight, CONTENTS_SOLID|CONTENTS_OPAQUE|CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP
				|CONTENTS_MOVEABLECLIP|CONTENTS_BODY|CONTENTS_CORPSE|CONTENTS_RENDERMODEL
				|CONTENTS_FLASHLIGHT_TRIGGER, this);
			DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("TraceFraction: %f\r", trace.fraction);
			if(trace.fraction < 1.0f)
			{
				DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Light [%s] can not be seen\r", light->name.c_str());
				continue;
			}
		}

		if(vResult[0].z < vResult[1].z)
			l = 0;
		else
			l = 1;

		if(vResult[l].z < vPlayerPos.z)
		{
			fx = vPlayerPos.x;
			fy = vPlayerPos.y;
		}
		else
		{
			fx = vResult[l].x;
			fy = vResult[l].y;
		}

		if(bMinOneLight == false)
			bMinOneLight = true;

		fLightgemVal += light->GetDistanceColor(fDistance, vLightCone[ELL_ORIGIN].x - fx, vLightCone[ELL_ORIGIN].y - fy);
		DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("%s in x/y: %f/%f   Distance:   %f/%f   Brightness: %f\r",
			light->name.c_str(), fx, fy, fLightgemVal, fDistance, light->m_MaxLightRadius);

		// Exchange the position of these lights, so that nearer lights are more
		// at the beginning of the list. You may not use the arrayentry from this point on now.
		// This sorting is not exactly good, but it is very cheap and we don't want to waste
		// time to sort an everchanging array.
		if(h != -1)
		{
			helper = pDM->m_LightList[h];
			pDM->m_LightList[h] = light;
			pDM->m_LightList[i] = helper;
			h = -1;
		}

		// No need to do further calculations when we are fully lit. 0 < n < 1
		if(fLightgemVal >= 1.0f)
				continue;

		// No need to do further calculations when we are fully lit. 0 < n < 1
		if(fLightgemVal >= 1.0f)
		{
			fLightgemVal = 1.0;
			break;
		}
	}

	pDM->m_LightgemValue = DARKMOD_LG_MAX * fLightgemVal;
	if(pDM->m_LightgemValue < DARKMOD_LG_MIN)
		pDM->m_LightgemValue = DARKMOD_LG_MIN;
	else
	if(pDM->m_LightgemValue > DARKMOD_LG_MAX)
		pDM->m_LightgemValue = DARKMOD_LG_MAX;

	// if the player is in a lit area and the lightgem would be totaly dark we set it to at least
	// one step higher.
	if(bMinOneLight == true && pDM->m_LightgemValue <= DARKMOD_LG_MIN)
		pDM->m_LightgemValue++;
}

void idPlayer::UpdateMoveVolumes( void )
{
	// copy step volumes from current cvar value
	m_stepvol_walk = cv_pm_stepvol_walk.GetFloat();
	m_stepvol_run = cv_pm_stepvol_run.GetFloat();
	m_stepvol_creep = cv_pm_stepvol_creep.GetFloat();

	m_stepvol_crouch_walk = cv_pm_stepvol_crouch_walk.GetFloat();
	m_stepvol_crouch_creep = cv_pm_stepvol_crouch_creep.GetFloat();
	m_stepvol_crouch_run = cv_pm_stepvol_crouch_run.GetFloat();
}

/*
=====================
idPlayer::GetMovementVolMod
=====================
*/

float idPlayer::GetMovementVolMod( void )
{
	float returnval;
	bool bCrouched(false);
	
	if( AI_CROUCH )
		bCrouched = true;

	// figure out which of the 6 cases we have:
	if( !AI_RUN && !AI_CREEP )
	{
		if( !bCrouched )
			returnval = m_stepvol_walk;
		else
			returnval = m_stepvol_crouch_walk;
	}

	// NOTE: running always has priority over creeping
	else if( AI_RUN )
	{
		if( !bCrouched )
			returnval = m_stepvol_run;
		else
			returnval = m_stepvol_crouch_run;
	}

	else if( AI_CREEP )
	{
		if( !bCrouched )
			returnval = m_stepvol_creep;
		else
			returnval = m_stepvol_crouch_creep;
	}

	else
	{
		// something unexpected happened
		returnval = 0;
	}

	return returnval;
}

/*
=====================
idPlayer::inventoryNextItem
=====================
*/
void idPlayer::inventoryNextItem()
{
	CInventoryCursor *crsr = InventoryCursor();
	CInventoryItem *prev;

	// If the entity doesn't have an inventory, we don't need to do anything.
	if(crsr == NULL)
		return;

	prev = crsr->GetCurrentItem();
	crsr->GetNextItem();
	if(hud)
		inventoryChangeSelection(hud, true, prev);
}

void idPlayer::inventoryPrevItem()
{
	CInventoryCursor *crsr = InventoryCursor();
	CInventoryItem *prev;

	// If the entity doesn't have an inventory, we don't need to do anything.
	if(crsr == NULL)
		return;

	prev = crsr->GetCurrentItem();
	crsr->GetPrevItem();
	if(hud)
		inventoryChangeSelection(hud, true, prev);
}

void idPlayer::inventoryNextGroup()
{
	InventoryCursor()->GetNextCategory();
}

void idPlayer::inventoryPrevGroup()
{
	InventoryCursor()->GetPrevCategory();
}

void idPlayer::inventoryUseKeyRelease(int holdTime) {
	CInventoryCursor *crsr = InventoryCursor();
	CInventoryItem *it = crsr->GetCurrentItem();

	// Check if there is a valid item selected
	if (it->GetType() != CInventoryItem::IT_DUMMY) {
		// greebo: Notify the inventory item about the key release event
		idEntity* item = it->GetItemEntity();

		idThread* thread = item->CallScriptFunctionArgs(
			"inventoryUseKeyRelease", true, 0, 
			"eef", item, this, static_cast<float>(holdTime)
		);

		if (thread) {
			thread->Start(); // Start the thread immediately.
		}
	}
}

void idPlayer::inventoryUseItem(bool bImpulse)
{
	// If the player has an item that is selected we need to check if this
	// is a usable item (like a key). In this case the use action takes
	// precedence over the frobaction.
	CInventoryCursor *crsr = InventoryCursor();
	CInventoryItem *it = crsr->GetCurrentItem();
	if(it->GetType() != CInventoryItem::IT_DUMMY)
		inventoryUseItem(bImpulse, it->GetItemEntity());
}

void idPlayer::inventoryUseItem(bool bImpulse, idEntity *ent)
{
	// Sanity check
	if (ent == NULL) return;

	idEntity *frob = g_Global.m_DarkModPlayer->m_FrobEntity;

	DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Inventory selection %08lX  Impulse: %u\r", ent, (int)bImpulse);
	if(frob != NULL)
	{
		// We have a frob entity in front of the player

		// Check if the usage should become continuous when this item is active
		m_ContinuousUse = ent->spawnArgs.GetBool("inv_cont_use");
		if(ent->spawnArgs.GetBool("usable"))
		{
			DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Item is usable\r");
			frob->UsedBy(bImpulse, ent);
		}
	}
	else
	{
		if (bImpulse) {
			// greebo: No frob entity highlighted, try to call the "use" method in the entity's scriptobject
			idThread* thread = ent->CallScriptFunctionArgs("inventoryUse", true, 0, "ee", ent, this);
			if (thread) {
				thread->Start(); // Start the thread immediately.
			}
		}
	}
}

void idPlayer::inventoryDropItem()
{
	CGrabber *grabber = g_Global.m_DarkModPlayer->grabber;
	idEntity *heldEntity = grabber->GetSelected();

	// Drop the item in the grabber hands first
	if(heldEntity != NULL)
	{
		grabber->Update( this, false );
	}
	else {
		// Grabber is empty (no item is held), drop the current inventory item
		CInventoryCursor* cursor = InventoryCursor();

		CInventoryItem* item = cursor->GetCurrentItem();
		CInventoryCategory* category = cursor->GetCurrentCategory();

		// Do we have a droppable item in the first place?
		if (item != NULL && item->IsDroppable() && item->GetCount() > 0)
		{
			// Retrieve the actual entity behind the inventory item
			idEntity* ent = item->GetItemEntity();

			// greebo: Try to locate a drop script function on the entity's scriptobject
			const function_t* dropScript = ent->scriptObject.GetFunction(TDM_INVENTORY_DROPSCRIPT);

			// greebo: Only place the entity in the world, if there is no custom dropscript
			// The flashbomb for example is spawning projectiles on its own.
			if (dropScript == NULL) {
				// Drop the item into the grabber hands 
				
				// Stackable items only have one "real" entity for the whole item stack.
				// When the stack size == 1, this entity can be dropped as it is,
				// otherwise we need to spawn a new entity.
				if (item->IsStackable() && item->GetCount() > 1) {
					DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Spawning new entity from stackable inventory item...\r");
					// Spawn a new entity of this type
					idEntity* spawnedEntity;
					const idDict* entityDef = gameLocal.FindEntityDefDict(ent->GetEntityDefName());
					gameLocal.SpawnEntityDef(*entityDef, &spawnedEntity);

					// Replace the entity to be dropped with the newly spawned one.
					ent = spawnedEntity;
				}

				if (!grabber->PutInHands(ent, this)) {
					// The grabber could not put the item into the player hands
					// TODO: Emit a sound?
				}
				// old code: m_Inventory->PutEntityInMap(ent, this, item);
			}
			else {
				// Call the custom drop script
				DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Running inventory drop script...\r");
				idThread* thread = new idThread(dropScript);
				thread->CallFunctionArgs(dropScript, true, "ee", ent, this);
				thread->DelayedStart(0);
			}

			// greebo: Decrease the stack counter, if applicable
			if (item->IsStackable()) {
				item->SetCount(item->GetCount() - 1);

				if (item->GetCount() <= 0) {
					DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Removing empty stackable item from category.\r");
					// Stackable item count reached zero, remove item from category
					category->removeItem(item);
				}
				
				// Check for empty categories after the item has been removed
				if (category->isEmpty()) {
					DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Removing empty inventory category.\r");
					// Switch the cursor to the next category
					cursor->GetNextCategory();
					// Remove category from inventory
					cursor->Inventory()->removeCategory(category);
				}

				UpdateHud();
			}
		}
	}
}

void idPlayer::inventoryChangeSelection(idUserInterface *_hud, bool bUpdate, CInventoryItem *prev)
{
	float opacity( cv_tdm_inv_opacity.GetFloat() );
	int groupvis;
	CInventoryItem::ItemType type = CInventoryItem::IT_ITEM;
	CInventoryItem *cur = NULL;
//	CInventory *inv = NULL;
	idEntity *e = NULL;
	idStr s;
	idThread *thread;

	// Since the player always has at least a loot and a dummy item, this can never be NULL.
	CInventoryCursor *crsr = InventoryCursor();
	cur = crsr->GetCurrentItem();
	if(cur)
		e = cur->GetItemEntity();

	// Notify the previous entity and the new one that they are un-/selected.
	if(prev && cur != prev)
	{
		idEntity *ce = prev->GetItemEntity();
		if(ce) {
			thread = ce->CallScriptFunctionArgs("inventory_item_unselect", true, 0, "eef", ce, prev->GetOwner(), (float)prev->GetOverlay());
		}
	}

	if(cur)
	{
		if(e && bUpdate == true) {
			thread = e->CallScriptFunctionArgs("inventory_item_select", true, 0, "eef", e, cur->GetOwner(), (float)cur->GetOverlay());
		}

		type = cur->GetType();
		switch(type)
		{
			case CInventoryItem::IT_ITEM:
			{
				// We display the default hud if the item doesn't have it's own. Each
				// item, that has it's own gui, is responsible for switching it on and off
				// appropriately with their respective events.
				if(cur->HasHUD() == false)
				{
					// We default the name to something obvious, because the mapper should 
					// see, when playtesting, that he forgot to name the item and groups.
					SetGuiFloat(mInventoryOverlay, "Inventory_GroupVisible", 1.0);
					SetGuiFloat(mInventoryOverlay, "Inventory_ItemVisible", 1.0);
					SetGuiFloat(mInventoryOverlay, "Inventory_ItemStackable", (float)cur->IsStackable());
					SetGuiString(mInventoryOverlay, "Inventory_ItemGroup", cur->Category()->GetName().c_str());
					SetGuiString(mInventoryOverlay, "Inventory_ItemName", cur->GetName());
					SetGuiInt(mInventoryOverlay, "Inventory_ItemCount", cur->GetCount());
					SetGuiString(mInventoryOverlay, "Inventory_ItemIcon", cur->GetIcon().c_str());
				}
			}
			break;

			case CInventoryItem::IT_DUMMY:
			{
				// All objects are set to empty, so we have an empty entry in the
				// inventory.
				s = "";
				SetGuiFloat(mInventoryOverlay, "Inventory_ItemVisible", 0.0);
				SetGuiFloat(mInventoryOverlay, "Inventory_LootVisible", 0.0);
				SetGuiFloat(mInventoryOverlay, "Inventory_GroupVisible", 0.0);
			}
			break;
		}
	}

	groupvis = cv_tdm_inv_groupvis.GetInteger();
	if(groupvis == 2)
	{
		// TODO: implement a fade in/out if the opacity should be temporarily
	}
	else
		opacity = cv_tdm_inv_opacity.GetFloat()*groupvis;

	SetGuiFloat(mInventoryOverlay, "Inventory_Opacity", opacity);
	_hud->StateChanged(gameLocal.time);
}

void idPlayer::Event_GetEyePos( void )
{
	idThread::ReturnVector( firstPersonViewOrigin );
}

void idPlayer::Event_SetImmobilization( const char *source, int type )
{
	if (idStr::Length(source))
	{
		// The user cannot set the update bit directly.
		type &= ~EIM_UPDATE;

		if (type)
		{
			m_immobilization.SetInt( source, type );
		}
		else
		{
			m_immobilization.Delete( source );
		}

		m_immobilizationCache |= EIM_UPDATE;
	}
	else
	{
		gameLocal.Warning( "source was empty; no immobilization set\n" );
	}
}

/*
=====================
idPlayer::Event_GetImmobilization
=====================
*/
void idPlayer::Event_GetImmobilization( const char *source )
{
	if (idStr::Length(source)) {
		idThread::ReturnInt( m_immobilization.GetInt(source) );
	} else {
		idThread::ReturnInt( GetImmobilization() );
	}
}

/*
=====================
idPlayer::Event_GetNextImmobilization
=====================
*/
void idPlayer::Event_GetNextImmobilization( const char *prefix, const char *lastMatch )
{
	// Code is plagarized from getNextKey()
	const idKeyValue *kv;
	const idKeyValue *previous;

	if ( *lastMatch ) {
		previous = m_immobilization.FindKey( lastMatch );
	} else {
		previous = NULL;
	}

	kv = m_immobilization.MatchPrefix( prefix, previous );
	if ( !kv ) {
		idThread::ReturnString( "" );
	} else {
		idThread::ReturnString( kv->GetKey() );
	}
}

/*
=====================
idPlayer::Event_SetHinderance
=====================
*/
void idPlayer::Event_SetHinderance( const char *source, float mCap, float aCap )
{
	if (idStr::Length(source)) {

		if ( mCap < 0.0f ) {
			mCap = 0.0f;
			gameLocal.Warning( "mCap < 0; mCap set to 0\n" );
		} else if ( mCap > 1.0f ) {
			mCap = 1.0f;
			gameLocal.Warning( "mCap > 1; mCap set to 1\n" );
		}
		if ( aCap < 0.0f ) {
			aCap = 0.0f;
			gameLocal.Warning( "aCap < 0; aCap set to 0\n" );
		} else if ( aCap > 1.0f ) {
			aCap = 1.0f;
			gameLocal.Warning( "aCap > 1; aCap set to 1\n" );
		}

		if ( mCap < 1.0f || aCap < 1.0f ) {
			idVec3 vec( mCap, aCap, 0.0f );
			m_hinderance.SetVector( source, vec );
		} else {
			m_hinderance.Delete( source );
		}

		m_hinderanceCache = -1;
	} else {
		gameLocal.Warning( "source was empty; no hinderance set\n" );
	}
}

/*
=====================
idPlayer::Event_GetHinderance
=====================
*/
void idPlayer::Event_GetHinderance( const char *source )
{
	if (idStr::Length(source)) {
		idThread::ReturnVector( m_hinderance.GetVector( source, "1 1 0" ) );
	} else {
		float h = GetHinderance();
		idVec3 vec( h, h, 0.0f );
		idThread::ReturnVector( vec );
	}
}

/*
=====================
idPlayer::Event_GetNextHinderance
=====================
*/
void idPlayer::Event_GetNextHinderance( const char *prefix, const char *lastMatch )
{
	// Code is plagarized from getNextKey()
	const idKeyValue *kv;
	const idKeyValue *previous;

	if ( *lastMatch ) {
		previous = m_hinderance.FindKey( lastMatch );
	} else {
		previous = NULL;
	}

	kv = m_hinderance.MatchPrefix( prefix, previous );
	if ( !kv ) {
		idThread::ReturnString( "" );
	} else {
		idThread::ReturnString( kv->GetKey() );
	}
}

/*
=====================
idPlayer::Event_SetGui
=====================
*/
void idPlayer::Event_SetGui( int handle, const char *guiFile ) {
	if ( !uiManager->CheckGui(guiFile) ) {
		gameLocal.Warning( "Unable to load GUI file: %s\n", guiFile );
		goto Quit;
	}

	if ( !m_overlays.exists( handle ) ) {
		gameLocal.Warning( "Non-existant GUI handle: %d\n", handle );
		goto Quit;
	}

	// Entity GUIs are handled differently from regular ones.
	if ( handle == OVERLAYS_MIN_HANDLE ) {

		assert( hud );
		assert( m_overlays.isExternal( handle ) );

		// We're dealing with an existing unique GUI.
		// We need to read a new GUI into it.

		// Clear the state.
		const idDict &state = hud->State();
		const idKeyValue *kv;
		while ( ( kv = state.MatchPrefix( "" ) ) != NULL )
			hud->DeleteStateVar( kv->GetKey() );

		hud->InitFromFile( guiFile );

	} else if ( !m_overlays.isExternal( handle ) ) {

		bool result = m_overlays.setGui( handle, guiFile );
		assert( result );

		idUserInterface *gui = m_overlays.getGui( handle );
		if ( gui ) {
			gui->SetStateInt( "handle", handle );
			gui->Activate( true, gameLocal.time );
			// Let's set a good default value for whether or not the overlay is interactive.
			m_overlays.setInteractive( handle, gui->IsInteractive() );
		} else {
			gameLocal.Warning( "Unknown error: Unable to load GUI into overlay.\n" );
		}

	} else {
		gameLocal.Warning( "Cannot call setGui() on external handle: %d\n", handle );
	}

	Quit:
	return;
}

void idPlayer::Event_GetInventoryOverlay(void)
{
	idThread::ReturnInt(mInventoryOverlay);
}

void idPlayer::Event_PlayStartSound( void )
{
	StartSound("snd_mission_start", SND_CHANNEL_ANY, 0, false, NULL);
}

void idPlayer::Event_MissionFailed( void )
{
	gameLocal.m_MissionData->Event_MissionFailed();
}

void idPlayer::Event_LoadDeathMenu( void )
{
	forceRespawn = true;
}

/*
================
idPlayer::Event_HoldEntity
================
*/
void idPlayer::Event_HoldEntity( idEntity *ent )
{
	if ( ent )
	{
		bool successful = g_Global.m_DarkModPlayer->grabber->PutInHands( ent, this, 0 );
		idThread::ReturnInt( successful );
	}
	else
	{
		g_Global.m_DarkModPlayer->grabber->Update( this, false );
		idThread::ReturnInt( 1 );
	}
}

/*
================
idPlayer::Event_HeldEntity
================
*/
void idPlayer::Event_HeldEntity( void )
{
	idThread::ReturnEntity(g_Global.m_DarkModPlayer->grabber->GetSelected());
}

void idPlayer::Event_RopeRemovalCleanup(idEntity *RopeEnt)
{
	static_cast<idPhysics_Player *>( GetPhysics() )->RopeRemovalCleanup( RopeEnt );
}

void idPlayer::Event_SetObjectiveState( int ObjIndex, int State )
{
	// when calling this function externally, the "user" indices are
	// used, so subtract one from the index
	gameLocal.m_MissionData->SetCompletionState( ObjIndex - 1, State );
}

void idPlayer::Event_GetObjectiveState( int ObjIndex )
{
	int CompState = gameLocal.m_MissionData->GetCompletionState( ObjIndex - 1 );
	idThread::ReturnInt( CompState );
}

void idPlayer::Event_SetObjectiveComp( int ObjIndex, int CompIndex, int bState )
{
	gameLocal.m_MissionData->SetComponentState_Ext( ObjIndex, CompIndex, (bState != 0) );
}

void idPlayer::Event_GetObjectiveComp( int ObjIndex, int CompIndex )
{
	bool bCompState = gameLocal.m_MissionData->GetComponentState( ObjIndex -1, CompIndex -1 );

	idThread::ReturnInt( (int) bCompState );
}

void idPlayer::Event_ObjectiveUnlatch( int ObjIndex )
{
	gameLocal.m_MissionData->UnlatchObjective( ObjIndex - 1 );
}

void idPlayer::Event_ObjectiveComponentUnlatch( int ObjIndex, int CompIndex )
{
	gameLocal.m_MissionData->UnlatchObjectiveComp( ObjIndex - 1, CompIndex -1 );
}

void idPlayer::Event_SetObjectiveVisible( int ObjIndex, bool bVal )
{
	gameLocal.m_MissionData->Event_SetObjVisible( ObjIndex, bVal );
}

void idPlayer::Event_SetObjectiveOptional( int ObjIndex, bool bVal )
{
	gameLocal.m_MissionData->Event_SetObjMandatory( ObjIndex, !bVal );
}

void idPlayer::Event_SetObjectiveOngoing( int ObjIndex, bool bVal )
{
	gameLocal.m_MissionData->Event_SetObjOngoing( ObjIndex, bVal );
}

void idPlayer::Event_SetObjectiveEnabling( int ObjIndex, const char *strIn )
{
	idStr StrArg = strIn;
	gameLocal.m_MissionData->Event_SetObjEnabling( ObjIndex, StrArg );
}

void idPlayer::Event_GiveHealthPool( float amount ) {
	// Pass the call to the proper member method
	GiveHealthPool(amount);
}

void idPlayer::FrobCheck( void )
{
	trace_t trace;
	float	TraceDist;
	idVec3 delta, VecForward;
	idBounds FrobBounds;

	idVec3	EyePos = GetEyePosition();
	idVec3 start = EyePos;
	idVec3 end = start + viewAngles.ToForward() * g_Global.m_MaxFrobDistance;
	// Do frob trace first, along view axis, record distance traveled
	// Frob collision mask:
	int cm = CONTENTS_SOLID|CONTENTS_OPAQUE|CONTENTS_BODY
		|CONTENTS_CORPSE|CONTENTS_RENDERMODEL
		|CONTENTS_FROBABLE;

	gameLocal.clip.TracePoint(trace, start, end, cm, this);
	TraceDist = g_Global.m_MaxFrobDistance * trace.fraction;

	if( trace.fraction < 1.0f )
	{
		idEntity *ent = gameLocal.entities[ trace.c.entityNum ];
		DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Frob: Direct hit on entity %s\r", ent->name.c_str());
		
		// only frob frobable, non-hidden entities within their frobdistance
		// also, do not frob the ent we are currently holding in our hands
		if( ent->m_bFrobable && !ent->IsHidden() && (TraceDist < ent->m_FrobDistance)
			&& ent != g_Global.m_DarkModPlayer->grabber->GetSelected() )
		{
			DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Entity %s was within frobdistance\r", ent->name.c_str());
			// TODO: Mark as frobbed for this frame
			ent->SetFrobbed(true);
			g_Global.m_DarkModPlayer->m_FrobTrace = trace;

			// we have found our frobbed entity, so exit
#ifdef __linux__
			return;
#else
			goto Quit;
#endif
		}
	}

	//DM_LOG(LC_FROBBING,LT_DEBUG)LOGSTRING("No entity frobbed by direct LOS frob, trying frob radius.\r");
	// IF the trace didn't hit anything frobable, do the radius test:

	FrobBounds.Zero();
	FrobBounds += trace.endpos;
	FrobBounds.ExpandSelf( cv_frob_width.GetFloat() );

	// Optional debug drawing of frob bounds
	if( cv_frob_debug_bounds.GetBool() )
		gameRenderWorld->DebugBounds( colorBlue, FrobBounds );

	idEntity *FrobRangeEnts[ MAX_GENTITIES ];

	int numFrobEnt = gameLocal.clip.EntitiesTouchingBounds( FrobBounds, -1, FrobRangeEnts, MAX_GENTITIES );

	VecForward = viewAngles.ToForward();
	float BestDot = 0;
	float CurrentDot = 0;
	float FrobDistSqr = 0;
	idEntity *BestEnt = NULL;

	for( int i=0; i < numFrobEnt; i++ )
	{
		idEntity *ent = FrobRangeEnts[i];
		if( !ent )
			continue;
		if( !ent->m_FrobDistance || ent->IsHidden() || !ent->m_bFrobable )
			continue;

		FrobDistSqr = ent->m_FrobDistance;
		FrobDistSqr *= FrobDistSqr;
		delta = ent->GetPhysics()->GetOrigin() - EyePos;
		
		if( delta.LengthSqr() > FrobDistSqr )
			continue;

		delta.NormalizeFast();
		CurrentDot = delta * VecForward;
		CurrentDot *= ent->m_FrobBias;

		if( CurrentDot > BestDot )
		{
			BestDot = CurrentDot;
			BestEnt = ent;
		}
	}

	if( BestEnt && BestEnt != g_Global.m_DarkModPlayer->grabber->GetSelected() )
	{
		DM_LOG(LC_FROBBING,LT_DEBUG)LOGSTRING("Frob radius expansion found best entity %s\r", BestEnt->name.c_str() );
		// Mark the entity as frobbed this frame
		BestEnt->SetFrobbed(true);
		g_Global.m_DarkModPlayer->m_FrobTrace = trace;
	}

Quit:
	return;
}

int idPlayer::GetImmobilization( const char *source )
{
	int returnVal = 0;

	if (idStr::Length(source)) 
	{
		returnVal = m_immobilization.GetInt(source);
	} else 
	{
		returnVal = GetImmobilization();
	}

	return returnVal;
}

void idPlayer::SetImmobilization( const char *source, int type )
{
	Event_SetImmobilization( source, type );
}

void idPlayer::SetHinderance( const char *source, float mCap, float aCap )
{
	Event_SetHinderance( source, mCap, aCap );
}

void idPlayer::CheckHeldKeys( void )
{
	CKeyboard* Keyboard = CKeyboard::getInstance();
	CKeyCode ck = Keyboard->GetCurrentKey();
	
	// NOTE: For now, keep this compatible with both a toggle lean and hold lean setup
	
	// Forward lean
	if( Keyboard->ImpulseIsUpdated(IR_LEAN_FORWARD) == true)
	{
		// Check if the key is just reported as repeating or released.
		// If it is released we can unlean, as we don't care about repeats.
		const CKeyCode *key = Keyboard->ImpulseData(IR_LEAN_FORWARD);
		if( !key )
		{
			// Something bad happened
			DM_LOG(LC_SYSTEM, LT_DEBUG)LOGSTRING("Forward leaning stopped (bad pointer to key handler)\r");
			physicsObj.ToggleLean(90.0);
			Keyboard->ImpulseFree(IR_LEAN_FORWARD);
		}
		else if( key->GetPressed() == false )
		{
			DM_LOG(LC_SYSTEM, LT_DEBUG)LOGSTRING("Forward leaning stopped\r");
			physicsObj.ToggleLean(90.0);
			Keyboard->ImpulseProcessed(IR_LEAN_FORWARD);
		}
		else
		{
			DM_LOG(LC_SYSTEM, LT_DEBUG)LOGSTRING("Forward leaning ignored\r");
			Keyboard->ImpulseProcessed(IR_LEAN_FORWARD);
		}
	}

// Left lean
	if( Keyboard->ImpulseIsUpdated(IR_LEAN_LEFT) == true )
	{
		// Check if the key is just reported as repeating or released.
		// If it is released we can unlean, as we don't care about repeats.
		const CKeyCode *key = Keyboard->ImpulseData(IR_LEAN_LEFT);
		if( !key )
		{
			// Something bad happened
			DM_LOG(LC_SYSTEM, LT_DEBUG)LOGSTRING("Left leaning stopped (bad pointer to key handler)\r");
			physicsObj.ToggleLean(180.0);
			Keyboard->ImpulseFree(IR_LEAN_LEFT);
		}
		else if( key->GetPressed() == false )
		{
			DM_LOG(LC_SYSTEM, LT_DEBUG)LOGSTRING("Left leaning stopped\r");
			physicsObj.ToggleLean(180.0);
			Keyboard->ImpulseProcessed(IR_LEAN_LEFT);
		}
		else
		{
			DM_LOG(LC_SYSTEM, LT_DEBUG)LOGSTRING("Left leaning ignored\r");
			Keyboard->ImpulseProcessed(IR_LEAN_LEFT);
		}
		
	}

// Right lean
	if( Keyboard->ImpulseIsUpdated(IR_LEAN_RIGHT) == true)
	{
		// Check if the key is just reported as repeating or released.
		// If it is released we can unlean, as we don't care about repeats.
		const CKeyCode *key = Keyboard->ImpulseData(IR_LEAN_RIGHT);
		if( !key )
		{
			// Something bad happened
			DM_LOG(LC_SYSTEM, LT_DEBUG)LOGSTRING("Right leaning stopped (bad pointer to key handler)\r");
			physicsObj.ToggleLean(0.0);
			Keyboard->ImpulseFree(IR_LEAN_RIGHT);
		}
		else if( key->GetPressed() == false )
		{
			DM_LOG(LC_SYSTEM, LT_DEBUG)LOGSTRING("Right leaning stopped\r");
			physicsObj.ToggleLean(0.0);
			Keyboard->ImpulseProcessed(IR_LEAN_RIGHT);
		}
		else
		{
			DM_LOG(LC_SYSTEM, LT_DEBUG)LOGSTRING("Right leaning ignored\r");
			Keyboard->ImpulseProcessed(IR_LEAN_RIGHT);
		}
	}
}

/*
================
idPlayer::RangedThreatTo

Return nonzero if this entity could potentially
attack the given (target) entity at range, or
entities in general if target is NULL.
i.e. Return 1 if we have a projectile weapon
equipped, and 0 otherwise.
================
*/
float idPlayer::RangedThreatTo(idEntity* target) {
	idWeapon* weaponEnt = weapon.GetEntity();
	
	return weaponEnt->IsRanged();
}

void idPlayer::SetListenerLoc( idVec3 loc )
{
	m_ListenerLoc = loc;
}

idVec3 idPlayer::GetListenerLoc( void )
{
	return m_ListenerLoc;
}

void idPlayer::SetDoorListenLoc( idVec3 loc )
{
	m_DoorListenLoc = loc;
}

idVec3 idPlayer::GetDoorListenLoc( void )
{
	return m_DoorListenLoc;
}

void idPlayer::PerformFrob(void)
{
	idEntity *frob;
	CDarkModPlayer *pDM = g_Global.m_DarkModPlayer;

	// Ignore frobs if player-frobbing is immobilized.
	if ( GetImmobilization() & EIM_FROB )
		goto Quit;

	// if the grabber is currently holding something and frob is pressed,
	// release it.  Do not frob anything new since you're holding an item.
	if( g_Global.m_DarkModPlayer->grabber->GetSelected() )
	{
		g_Global.m_DarkModPlayer->grabber->Update( this );
		goto Quit;
	}

	frob = pDM->m_FrobEntity;
	DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("USE: frob: %08lX\r", frob);

	if(frob != NULL)
	{
		// Fire the STIM_FROB response (if defined) on this entity
		frob->ResponseTrigger(this, ST_FROB);
		
		frob->FrobAction(true);

		// First we have to check wether that entity is an inventory 
		// item. In that case, we have to add it to the inventory and
		// hide the entity.
		CInventoryItem* item = AddToInventory(frob, hud);

		if (item != NULL) {
			// Item has been added to the inventory
			pDM->m_FrobEntity = NULL;

			// greebo: Prevent the grabber from checking the added entity (it may be 
			// entirely removed from the game, which would cause crashes).
			g_Global.m_DarkModPlayer->grabber->RemoveFromClipList(frob);
		}
	}
Quit:
	return;
}

void idPlayer::setHealthPoolTimeInterval(int newTimeInterval, float factor, int stepAmount) {
	healthPoolTimeInterval = newTimeInterval;
	healthPoolTimeIntervalFactor = factor;
	healthPoolStepAmount = stepAmount;
}
