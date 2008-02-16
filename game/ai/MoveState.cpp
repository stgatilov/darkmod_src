/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 2073 $
 * $Date: 2008-02-10 01:13:37 +0100 (So, 10 Feb 2008) $
 * $Author: greebo $
 *
 ***************************************************************************/

// Copyright (C) 2004 Id Software, Inc.
//

#include "MoveState.h"
#include "../../idlib/precompiled.h"
#pragma hdrstop

/*
=====================
idMoveState::idMoveState
=====================
*/
idMoveState::idMoveState() {
	moveType			= MOVETYPE_ANIM;
	moveCommand			= MOVE_NONE;
	moveStatus			= MOVE_STATUS_DONE;
	moveDest.Zero();
	moveDir.Set( 1.0f, 0.0f, 0.0f );
	goalEntity			= NULL;
	goalEntityOrigin.Zero();
	toAreaNum			= 0;
	startTime			= 0;
	duration			= 0;
	speed				= 0.0f;
	range				= 0.0f;
	wanderYaw			= 0;
	nextWanderTime		= 0;
	blockTime			= 0;
	obstacle			= NULL;
	lastMoveOrigin		= vec3_origin;
	lastMoveTime		= 0;
	anim				= 0;
}

/*
=====================
idMoveState::Save
=====================
*/
void idMoveState::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( (int)moveType );
	savefile->WriteInt( (int)moveCommand );
	savefile->WriteInt( (int)moveStatus );
	savefile->WriteVec3( moveDest );
	savefile->WriteVec3( moveDir );
	goalEntity.Save( savefile );
	savefile->WriteVec3( goalEntityOrigin );
	savefile->WriteInt( toAreaNum );
	savefile->WriteInt( startTime );
	savefile->WriteInt( duration );
	savefile->WriteFloat( speed );
	savefile->WriteFloat( range );
	savefile->WriteFloat( wanderYaw );
	savefile->WriteInt( nextWanderTime );
	savefile->WriteInt( blockTime );
	obstacle.Save( savefile );
	savefile->WriteVec3( lastMoveOrigin );
	savefile->WriteInt( lastMoveTime );
	savefile->WriteInt( anim );
}

/*
=====================
idMoveState::Restore
=====================
*/
void idMoveState::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( (int &)moveType );
	savefile->ReadInt( (int &)moveCommand );
	savefile->ReadInt( (int &)moveStatus );
	savefile->ReadVec3( moveDest );
	savefile->ReadVec3( moveDir );
	goalEntity.Restore( savefile );
	savefile->ReadVec3( goalEntityOrigin );
	savefile->ReadInt( toAreaNum );
	savefile->ReadInt( startTime );
	savefile->ReadInt( duration );
	savefile->ReadFloat( speed );
	savefile->ReadFloat( range );
	savefile->ReadFloat( wanderYaw );
	savefile->ReadInt( nextWanderTime );
	savefile->ReadInt( blockTime );
	obstacle.Restore( savefile );
	savefile->ReadVec3( lastMoveOrigin );
	savefile->ReadInt( lastMoveTime );
	savefile->ReadInt( anim );
}
