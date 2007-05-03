/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 866 $
 * $Date: 2007-03-23 22:25:02 +0100 (Fr, 23 Mar 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/
#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: func_shooter.cpp 870 2007-03-27 14:21:59Z greebo $", init_version);

#include "func_shooter.h"
#include "StimResponse/StimResponseCollection.h"

// Script event interface
const idEventDef EV_ShooterSetState( "shooterSetState", "d" );
const idEventDef EV_ShooterFireProjectile( "shooterFireProjectile", NULL );
const idEventDef EV_ShooterGetState( "shooterGetState", NULL, 'd' );

// Event definitions
CLASS_DECLARATION( idStaticEntity, tdmFuncShooter )
EVENT( EV_Activate,					tdmFuncShooter::Event_Activate )
EVENT( EV_ShooterSetState,			tdmFuncShooter::Event_ShooterSetState )
EVENT( EV_ShooterGetState,			tdmFuncShooter::Event_ShooterGetState )
EVENT( EV_ShooterFireProjectile,	tdmFuncShooter::Event_ShooterFireProjectile )
END_CLASS

/*
===============
tdmFuncShooter::tdmFuncShooter
===============
*/
tdmFuncShooter::tdmFuncShooter( void ) {
	_active = false;
	_lastFireTime = 0;
	_nextFireTime = 0;
	_fireInterval = -1;
	_fireIntervalFuzzyness = 0;
	_lastStimVisit = 0;
	_requiredStimTimeOut = 0;
	_requiredStim = ST_DEFAULT;
}

/*
===============
tdmFuncShooter::Spawn
===============
*/
void tdmFuncShooter::Spawn( void ) {
	// Setup any stims/responses
	m_StimResponseColl->ParseSpawnArgsToStimResponse(&spawnArgs, this);

	_active = !spawnArgs.GetBool("start_off");
	_lastFireTime = 0;
	_fireInterval = spawnArgs.GetInt("fire_interval", "-1");
	_fireIntervalFuzzyness = spawnArgs.GetInt("fire_interval_fuzzyness", "0");
	
	idStr reqStimStr = spawnArgs.GetString("required_stim");

	if (!reqStimStr.IsEmpty()) {
		_requiredStim = CStimResponse::getStimType(reqStimStr);
		_requiredStimTimeOut = spawnArgs.GetInt("required_stim_timeout", "5000");
	}

	if (_active && _fireInterval > 0) {
		BecomeActive( TH_THINK );
		setupNextFireTime();
	}

	// Always react to stims, it may required
	if (_requiredStim != ST_DEFAULT) {
		//DM_LOG(LC_STIM_RESPONSE, LT_INFO)LOGSTRING("tdmFuncShooter is requiring stim %d\r", _requiredStim);
		GetPhysics()->SetContents( GetPhysics()->GetContents() | CONTENTS_RESPONSE );
	}
}

/*
===============
tdmFuncShooter::Save
===============
*/
void tdmFuncShooter::Save( idSaveGame *savefile ) const {
	savefile->WriteBool( _active );
	savefile->WriteInt( _lastFireTime );
	savefile->WriteInt( _nextFireTime );
	savefile->WriteInt( _fireInterval );
	savefile->WriteInt( _fireIntervalFuzzyness );
	savefile->WriteInt( _requiredStim );
	savefile->WriteInt( _requiredStimTimeOut );
	savefile->WriteInt( _lastStimVisit );
}

/*
===============
tdmFuncShooter::Restore
===============
*/
void tdmFuncShooter::Restore( idRestoreGame *savefile ) {
	savefile->ReadBool( _active );
	savefile->ReadInt( _lastFireTime );
	savefile->ReadInt( _nextFireTime );
	savefile->ReadInt( _fireInterval );
	savefile->ReadInt( _fireIntervalFuzzyness );

	int stimType;
	savefile->ReadInt( stimType );
	_requiredStim = static_cast<StimType>(stimType);

	savefile->ReadInt( _requiredStimTimeOut );
	savefile->ReadInt( _lastStimVisit );
}

/*
================
tdmFuncShooter::Event_Activate
================
*/
void tdmFuncShooter::Event_Activate( idEntity *activator ) {
	if ( thinkFlags & TH_THINK ) {
		BecomeInactive( TH_THINK );
		_active = false;
	} else {
		BecomeActive( TH_THINK );
		_active = true;
		_lastFireTime = gameLocal.time;
		setupNextFireTime();
	}
}

void tdmFuncShooter::Event_ShooterGetState() {
	idThread::ReturnInt(_active ? 1 : 0);
}

void tdmFuncShooter::Event_ShooterSetState( bool state ) {
	if (state == _active) {
		// Nothing to change
		return;
	}

	_active = state;

	if (_active) {
		setupNextFireTime();
	}
}

void tdmFuncShooter::Event_ShooterFireProjectile() {
	Fire();
}

void tdmFuncShooter::setupNextFireTime() {
	// Calculate the next fire time
	int randomness = gameLocal.random.RandomFloat() * _fireIntervalFuzzyness - _fireIntervalFuzzyness/2;
	_nextFireTime = gameLocal.time + _fireInterval + randomness;
}

/*
================
tdmFuncShooter::WriteToSnapshot
================
*/
void tdmFuncShooter::WriteToSnapshot( idBitMsgDelta &msg ) const {
	msg.WriteBits( _active ? 1 : 0, 1 );
}

/*
================
tdmFuncShooter::ReadFromSnapshot
================
*/
void tdmFuncShooter::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	_active = msg.ReadBits( 1 ) != 0;
}

void tdmFuncShooter::stimulate(StimType stimId) {
	if (stimId == _requiredStim && _requiredStim != ST_DEFAULT) {
		//DM_LOG(LC_STIM_RESPONSE, LT_INFO)LOGSTRING("Stim is visiting at %d\r", gameLocal.time);
		// Save the time the stim is visiting
		_lastStimVisit = gameLocal.time;
	}
}

void tdmFuncShooter::Fire() {
	_lastFireTime = gameLocal.time;

	// Spawn a projectile
	idStr projectileDef = spawnArgs.GetString("def_projectile");
	if (!projectileDef.IsEmpty()) {
		const idDict* projectileDict = gameLocal.FindEntityDefDict(projectileDef);

		idEntity* ent = NULL;
		gameLocal.SpawnEntityDef(*projectileDict, &ent);

		idProjectile* projectile = dynamic_cast<idProjectile*>(ent);
		if (projectile != NULL) {
			float angle = spawnArgs.GetFloat("angle");
			idVec3 direction( cos(angle), sin(angle), 0 );
			projectile->Launch(GetPhysics()->GetOrigin(), direction, idVec3(0,0,0));
		}
	}

	setupNextFireTime();
}

void tdmFuncShooter::Think() {
	if (_active && _fireInterval > 0 && gameLocal.time > _nextFireTime) {
		// greebo: Check before firing whether we have a required stim
		if (_requiredStim != ST_DEFAULT && _lastStimVisit > 0 && 
			_lastStimVisit + _requiredStimTimeOut >= gameLocal.time)
		{
			// We have a required stim, but it was not too far in the past => fire
			Fire();
		}
		else if (_requiredStim == ST_DEFAULT) {
			// No required stim, fire away
			Fire();
		}
	}
}
