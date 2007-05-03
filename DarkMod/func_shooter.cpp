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

CLASS_DECLARATION( idStaticEntity, idFuncShooter )
EVENT( EV_Activate,				idFuncShooter::Event_Activate )
END_CLASS

/*
===============
idFuncShooter::idFuncShooter
===============
*/
idFuncShooter::idFuncShooter( void ) {
	_active = false;
	_lastFireTime = 0;
	_fireInterval = -1;
	// Set the think flag to get called each frame
	thinkFlags |= TH_THINK;
}

/*
===============
idFuncShooter::Spawn
===============
*/
void idFuncShooter::Spawn( void ) {
	_active = !spawnArgs.GetBool("start_off");
	_lastFireTime = gameLocal.time;
	_fireInterval = spawnArgs.GetInt("fire_interval", "-1");

	BecomeActive( TH_THINK );
}

/*
===============
idFuncShooter::Save
===============
*/
void idFuncShooter::Save( idSaveGame *savefile ) const {
	savefile->WriteBool( _active );
}

/*
===============
idFuncShooter::Restore
===============
*/
void idFuncShooter::Restore( idRestoreGame *savefile ) {
	savefile->ReadBool( _active );
}

/*
================
idFuncShooter::Event_Activate
================
*/
void idFuncShooter::Event_Activate( idEntity *activator ) {
	if ( thinkFlags & TH_THINK ) {
		BecomeInactive( TH_THINK );
		_active = false;
	} else {
		BecomeActive( TH_THINK );
		_active = true;
		_lastFireTime = gameLocal.time;
	}
}

/*
================
idFuncShooter::WriteToSnapshot
================
*/
void idFuncShooter::WriteToSnapshot( idBitMsgDelta &msg ) const {
	msg.WriteBits( _active ? 1 : 0, 1 );
}

/*
================
idFuncShooter::ReadFromSnapshot
================
*/
void idFuncShooter::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	_active = msg.ReadBits( 1 ) != 0;
}

void idFuncShooter::Fire() {
	DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Fired at time: %d\r", gameLocal.time);
	_lastFireTime = gameLocal.time;

	// Spawn a projectile
	idStr projectileDef = spawnArgs.GetString("def_projectile");
	if (!projectileDef.IsEmpty()) {
		const idDict* projectileDict = gameLocal.FindEntityDefDict(projectileDef);

		idEntity* ent = NULL;
		gameLocal.SpawnEntityDef(*projectileDict, &ent);

		idProjectile* projectile = dynamic_cast<idProjectile*>(ent);
		if (projectile != NULL) {
			DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Projectile spawned: %s\r", projectile->name.c_str());
			float angle = spawnArgs.GetFloat("angle");
			idVec3 direction( cos(angle), sin(angle), 0 );
			projectile->Launch(GetPhysics()->GetOrigin(), direction, idVec3(0,0,0));
		}
	}
}

void idFuncShooter::Think() {
	if (_active && _fireInterval > 0 && gameLocal.time > _lastFireTime + _fireInterval) {
		Fire();
	}
}
