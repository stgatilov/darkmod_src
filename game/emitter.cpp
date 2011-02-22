// vim:ts=4:sw=4:cindent
/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 4618 $
 * $Date: 2011-02-20 19:10:42 +0100 (Sun, 20 Feb 2011) $
 * $Author: tels $
 *
 ***************************************************************************/

/*
   Copyright (C) 2004 Id Software, Inc.
   Copyrighr (C) 2011 The Dark Mod

func_emitters - have one or more particle models

*/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: misc.cpp 4618 2011-02-20 18:10:42Z tels $", init_version);

//#include "game_local.h"
#include "emitter.h"

CLASS_DECLARATION( idStaticEntity, idFuncEmitter )
EVENT( EV_Activate,				idFuncEmitter::Event_Activate )
END_CLASS

/*
===============
idFuncEmitter::idFuncEmitter
===============
*/
idFuncEmitter::idFuncEmitter( void ) {
	hidden = false;
	m_LOD = NULL;

	m_modelDefHandles.Clear();	// no extra models
	m_modelOffsets.Clear();		// no extra models
	m_modelHandles.Clear();		// no extra models
}

/*
================
idFuncEmitter::~idFuncEmitter
================
*/
idFuncEmitter::~idFuncEmitter( void ) {
	const int num = m_modelDefHandles.Num();
	for (int i = 0; i < num; i++ ) {
		if ( m_modelDefHandles[i] != -1 ) {
			gameRenderWorld->FreeEntityDef( m_modelDefHandles[i] );
			m_modelDefHandles[i] = -1;
		}
	}
}

/*
================
idFuncEmitter::SetModel for additional models
================
*/
void idFuncEmitter::SetModel( int id, const idStr &modelName, const idVec3 &offset ) {
	m_modelHandles.AssureSize( id+1, NULL );
	m_modelDefHandles.AssureSize( id+1, -1 );
	m_modelOffsets.AssureSize( id+1, idVec3(0,0,0) );
	m_modelHandles[id] = renderModelManager->FindModel( modelName );
	m_modelOffsets[id] = offset;
	m_modelDefHandles[id] = -1;

	// need to call Present() the next time we Think():
	BecomeActive( TH_UPDATEVISUALS );
}

/*
================
idFuncEmitter::SetModel for LOD change
================
*/
void idFuncEmitter::SetModel( const char* modelName ) 
{
	m_modelHandles.AssureSize( 1, NULL );
	m_modelDefHandles.AssureSize( 1, -1 );
	m_modelOffsets.AssureSize( 1, idVec3(0,0,0) );

	if ( m_modelDefHandles[0] != -1 ) {
		gameRenderWorld->FreeEntityDef( m_modelDefHandles[0] );
	}
	m_modelDefHandles[0] = -1;
	m_modelHandles[0] = renderModelManager->FindModel( modelName );
	m_modelOffsets[0] = idVec3(0,0,0);

	BecomeActive( TH_UPDATEVISUALS );
}

/*
================
idFuncEmitter::Present
================
*/
void idFuncEmitter::Present( void ) 
{
	if( m_bFrobable )
	{
		UpdateFrobState();
		UpdateFrobDisplay();
	}

	// don't present to the renderer if the entity hasn't changed
	if ( !( thinkFlags & TH_UPDATEVISUALS ) ) 
	{
		return;
	}
	BecomeInactive( TH_UPDATEVISUALS );

	const int num = m_modelHandles.Num();
//		gameLocal.Printf("%s: Have %i models\n", GetName(), num);
	for (int i = 0; i < num; i++ ) {

		if ( !m_modelHandles[i] ) {
			continue;
		}

//		gameLocal.Printf("%s: Presenting model %i\n", GetName(), i);
		renderEntity.origin = GetPhysics()->GetOrigin() + m_modelOffsets[i];
		renderEntity.axis = GetPhysics()->GetAxis();
		renderEntity.hModel = m_modelHandles[i];
		renderEntity.bodyId = i + 1;
		// make each of them have a unique timeoffset, so they do not appear to be in sync
		renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time - gameLocal.random.RandomInt( 32767 ) );

		// add to refresh list
		if ( m_modelDefHandles[i] == -1 ) {
			m_modelDefHandles[i] = gameRenderWorld->AddEntityDef( &renderEntity );
		} else {
			gameRenderWorld->UpdateEntityDef( m_modelDefHandles[i], &renderEntity );
		}
	}
}

/*
===============
idFuncEmitter::Spawn
===============
*/
void idFuncEmitter::Spawn( void ) {
	const idKeyValue *kv;

	if ( spawnArgs.GetBool( "start_off" ) ) {
		hidden = true;
		renderEntity.shaderParms[SHADERPARM_PARTICLE_STOPTIME] = MS2SEC( 1 );
		UpdateVisuals();
	} else {
		hidden = false;
	}

	// check if we have additional models
   	kv = spawnArgs.MatchPrefix( "model", NULL );
	int model = 0;
	while( kv )
	{
		idStr suffix = kv->GetKey();
		idStr modelName = kv->GetValue();
		if ((suffix.Length() >= 9 && suffix.Cmpn("model_lod",9) == 0))
		{
			// ignore "model_lod_2" etc
			kv = spawnArgs.MatchPrefix( "model", kv );
			continue;
		}
		suffix = suffix.Right( suffix.Length() - 5);	// model_2 => "_2"
		idVec3 modelOffset = spawnArgs.GetVector( "offset" + suffix, "0,0,0" );
//		gameLocal.Printf( "FuncEmitter %s: Adding model %i ('%s') at offset %s (suffix %s).\n", GetName(), model, modelName.c_str(), modelOffset.ToString(), suffix.c_str() );
		SetModel( model++, modelName, modelOffset );
		kv = spawnArgs.MatchPrefix( "model", kv );
	}
}

/*
===============
idFuncEmitter::Save
===============
*/
void idFuncEmitter::Save( idSaveGame *savefile ) const {
	savefile->WriteBool( hidden );

	const int num = m_modelDefHandles.Num();
	savefile->WriteInt( num );
	for (int i = 0; i < num; i++ ) {
		savefile->WriteInt( m_modelDefHandles[i] );
		savefile->WriteVec3( m_modelOffsets[i] );
	}
}

/*
================
idFuncEmitter::Think
================
*/
void idFuncEmitter::Think( void ) 
{
	// will also do LOD thinking:
	idEntity::Think();

	// extra models? Do LOD thinking for them:
	const int num = m_modelDefHandles.Num();
	// start with 1
/*	for (int i = 1; i < num; i++)
	{
		// TODO:
	}
*/
	// did our model(s) change?
	Present();
}

/*
===============
idFuncEmitter::Restore
===============
*/
void idFuncEmitter::Restore( idRestoreGame *savefile ) {
	savefile->ReadBool( hidden );
	int num;
	savefile->ReadInt( num );

	m_modelDefHandles.Clear();
	m_modelHandles.Clear();
	m_modelOffsets.Clear();
	m_modelDefHandles.SetNum(num);
	m_modelHandles.SetNum(num);
	m_modelOffsets.SetNum(num);
	for (int i = 0; i < num; i++ ) {
		savefile->ReadInt( m_modelDefHandles[i] );
		savefile->ReadVec3( m_modelOffsets[i] );
		// TODO
		m_modelHandles[i] = NULL;
	}
}

/*
================
idFuncEmitter::Event_Activate
================
*/
void idFuncEmitter::Event_Activate( idEntity *activator ) {
	if ( hidden || spawnArgs.GetBool( "cycleTrigger" ) ) {
		renderEntity.shaderParms[SHADERPARM_PARTICLE_STOPTIME] = 0;
		renderEntity.shaderParms[SHADERPARM_TIMEOFFSET] = -MS2SEC( gameLocal.time );
		hidden = false;
	} else {
		renderEntity.shaderParms[SHADERPARM_PARTICLE_STOPTIME] = MS2SEC( gameLocal.time );
		hidden = true;
	}
	UpdateVisuals();
}

/*
================
idFuncEmitter::WriteToSnapshot
================
*/
void idFuncEmitter::WriteToSnapshot( idBitMsgDelta &msg ) const {
	msg.WriteBits( hidden ? 1 : 0, 1 );
	msg.WriteFloat( renderEntity.shaderParms[ SHADERPARM_PARTICLE_STOPTIME ] );
	msg.WriteFloat( renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] );
}

/*
================
idFuncEmitter::ReadFromSnapshot
================
*/
void idFuncEmitter::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	hidden = msg.ReadBits( 1 ) != 0;
	renderEntity.shaderParms[ SHADERPARM_PARTICLE_STOPTIME ] = msg.ReadFloat();
	renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = msg.ReadFloat();
	if ( msg.HasChanged() ) {
		UpdateVisuals();
	}
}


/*
===============================================================================

idFuncSplat

===============================================================================
*/


const idEventDef EV_Splat( "<Splat>" );
CLASS_DECLARATION( idFuncEmitter, idFuncSplat )
EVENT( EV_Activate,		idFuncSplat::Event_Activate )
EVENT( EV_Splat,		idFuncSplat::Event_Splat )
END_CLASS

/*
===============
idFuncSplat::idFuncSplat
===============
*/
idFuncSplat::idFuncSplat( void ) {
}

/*
===============
idFuncSplat::Spawn
===============
*/
void idFuncSplat::Spawn( void ) {
}

/*
================
idFuncSplat::Event_Splat
================
*/
void idFuncSplat::Event_Splat( void ) {
	const char *splat = NULL;
	int count = spawnArgs.GetInt( "splatCount", "1" );
	for ( int i = 0; i < count; i++ ) {
		splat = spawnArgs.RandomPrefix( "mtr_splat", gameLocal.random );
		if ( splat && *splat ) {
			float size = spawnArgs.GetFloat( "splatSize", "128" );
			float dist = spawnArgs.GetFloat( "splatDistance", "128" );
			float angle = spawnArgs.GetFloat( "splatAngle", "0" );
			gameLocal.ProjectDecal( GetPhysics()->GetOrigin(), GetPhysics()->GetAxis()[2], dist, true, size, splat, angle );
		}
	}
	StartSound( "snd_splat", SND_CHANNEL_ANY, 0, false, NULL );
}

/*
================
idFuncSplat::Event_Activate
================
*/
void idFuncSplat::Event_Activate( idEntity *activator ) {
	idFuncEmitter::Event_Activate( activator );
	PostEventSec( &EV_Splat, spawnArgs.GetFloat( "splatDelay", "0.25" ) );
	StartSound( "snd_spurt", SND_CHANNEL_ANY, 0, false, NULL );
}


