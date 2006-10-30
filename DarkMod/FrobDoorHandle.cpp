/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.3  2006/10/30 17:10:25  sparhawk
 * Doorhandles are now working in the first stage.
 *
 * Revision 1.2  2006/10/03 13:13:39  sparhawk
 * Changes for door handles
 *
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
	m_FrobLock = false;
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
	if(m_Door)
		m_Door->SetDoorhandle(this);
	else
		DM_LOG(LC_SYSTEM, LT_WARNING)LOGSTRING("door_body [%s] for handle [%s] not found\r", str.c_str(), name.c_str());
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

CFrobDoor *CFrobDoorHandle::GetDoor(void)
{
	return m_Door;
}

void CFrobDoorHandle::Event_GetDoor(void)
{
	return idThread::ReturnEntity(m_Door);
}

void CFrobDoorHandle::SetFrobbed(bool val)
{
	if(m_FrobLock == false)		// Prevent an infinte loop here.
	{
		m_FrobLock = true;
		idEntity::SetFrobbed(val);
		if(m_Door)
			m_Door->SetFrobbed(val);
		m_FrobLock = false;
	}
	DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("door_handle [%s] %08lX is frobbed\r", name.c_str(), this);
}

bool CFrobDoorHandle::IsFrobbed(void)
{
	// A handle without a door doesn't really make sense, but we don't 
	// want to crash the game just because of it. And after all, a handle
	// MIGHT be used for some other purpose, so it acts like a normal entity
	// if it doesn't has a door.
	if(m_Door)
		return m_Door->IsFrobbed();
	else
		return idEntity::IsFrobbed();
}

// A handle itself can not be used by other objects, so we only
// forward it in case of a door.
bool CFrobDoorHandle::UsedBy(idEntity *e)
{
	if(m_Door)
		return m_Door->UsedBy(e);

	return false;
}

void CFrobDoorHandle::FrobAction(bool bMaster)
{
	if(m_Door)
		m_Door->FrobAction(bMaster);
}

// A handle can't close a portal, so we block it. The same is true for the Done* and statechanges
void CFrobDoorHandle::ClosePortal(void)
{
}

void CFrobDoorHandle::DoneRotating(void)
{
}

void CFrobDoorHandle::DoneMoving(void)
{
}

void CFrobDoorHandle::DoneStateChange(void)
{
}


void CFrobDoorHandle::ToggleOpen(void)
{
}

void CFrobDoorHandle::ToggleLock(void)
{
}
