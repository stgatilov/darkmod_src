/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.1  2006/07/27 20:30:40  sparhawk
 * Initial release. Just the skeleton of the spawnable CFrobDoorHandle class.
 *
 *
 ***************************************************************************/

// Copyright (C) 2004 Gerhard W. Gruber <sparhawk@gmx.at>
//

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Source$  $Revision$   $Date$", init_version);

#include "../game/Game_local.h"
#include "DarkModGlobals.h"
#include "BinaryFrobMover.h"
#include "FrobDoor.h"
#include "FrobDoorHandle.h"

//===============================================================================
//CFrobDoorHandle
//===============================================================================

const idEventDef EV_TDM_Handle_GetDoor( "GetDoor", NULL, 'e' );

CLASS_DECLARATION( CBinaryFrobMover, CFrobDoorHandle )
	EVENT( EV_TDM_Handle_GetDoor,		CFrobDoorHandle::Event_GetDoor )
END_CLASS


CFrobDoorHandle::CFrobDoorHandle(void)
: CBinaryFrobMover()
{
	DM_LOG(LC_FUNCTION, LT_DEBUG)LOGSTRING("this: %08lX [%s]\r", this, __FUNCTION__);
	m_Door = NULL;
}

void CFrobDoorHandle::Save(idSaveGame *savefile) const
{
}

void CFrobDoorHandle::Restore( idRestoreGame *savefile )
{
}

void CFrobDoorHandle::WriteToSnapshot( idBitMsgDelta &msg ) const
{
}

void CFrobDoorHandle::ReadFromSnapshot( const idBitMsgDelta &msg )
{
}

void CFrobDoorHandle::Spawn(void)
{
	idStr str;

	CBinaryFrobMover::Spawn();
	LoadTDMSettings();

	spawnArgs.GetString("door_body", "", str);
	m_Door = FindDoor(str);
}


CFrobDoor *CFrobDoorHandle::FindDoor(idStr &name)
{
	idEntity *e;
	CFrobDoor *rc = NULL;

	if((e = gameLocal.FindEntity(name.c_str())) != NULL)
	{
		if((rc = dynamic_cast<CFrobDoor *>(e)) != NULL)
			DM_LOG(LC_SYSTEM, LT_DEBUG)LOGSTRING("door_body [%s] %08lX\r", name.c_str(), e);
		else
			DM_LOG(LC_SYSTEM, LT_ERROR)LOGSTRING("door_body [%s] is not an entity of type CFrobDoor\r", name.c_str());
	}

	return rc;
}

void CFrobDoorHandle::DoneMoving(void)
{
	idMover::DoneMoving();

	DoneStateChange();
}


void CFrobDoorHandle::DoneRotating(void)
{
	idMover::DoneRotating();

	DoneStateChange();
}

CFrobDoor *CFrobDoorHandle::GetDoor(void)
{
	return m_Door;
}

void CFrobDoorHandle::Event_GetDoor(void)
{
	return idThread::ReturnEntity(m_Door);
}
