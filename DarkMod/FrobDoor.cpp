/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.2  2004/11/16 23:56:03  sparhawk
 * Frobcode has been generalized now and resides for all entities in the base classe.
 *
 * Revision 1.1  2004/11/14 20:19:11  sparhawk
 * Initial Release
 *
 *
 ***************************************************************************/

// Copyright (C) 2004 Gerhard W. Gruber <sparhawk@gmx.at>
//

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "../game/Game_local.h"
#include "DarkModGlobals.h"
#include "FrobDoor.h"

//===============================================================================
//CFrobDoor
//===============================================================================

CLASS_DECLARATION( idMover, CFrobDoor )
END_CLASS

CFrobDoor::CFrobDoor(void)
{
	DM_LOG(LC_FUNCTION, LT_DEBUG).LogString("this: %08lX [%s]\r", this, __FUNCTION__);
	m_FrobActionScript = "frob_door";
	m_LinkedLock = NULL;
	m_LinkedOpen = NULL;
}

void CFrobDoor::Save(idSaveGame *savefile) const
{
}

void CFrobDoor::Restore( idRestoreGame *savefile )
{
}

void CFrobDoor::WriteToSnapshot( idBitMsgDelta &msg ) const
{
}

void CFrobDoor::ReadFromSnapshot( const idBitMsgDelta &msg )
{
}

void CFrobDoor::Spawn( void )
{
	idMover::Spawn();

	LoadTDMSettings();
}
