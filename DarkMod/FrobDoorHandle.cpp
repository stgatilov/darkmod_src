/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

// Copyright (C) 2004 Gerhard W. Gruber <sparhawk@gmx.at>
//

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include "../game/game_local.h"
#include "DarkModGlobals.h"
#include "BinaryFrobMover.h"
#include "FrobDoor.h"
#include "FrobDoorHandle.h"

//===============================================================================
//CFrobDoorHandle
//===============================================================================
extern E_SDK_SIGNAL_STATE SigOpen(idEntity *oEntity, void *pData);

const idEventDef EV_TDM_Handle_GetDoor( "GetDoor", NULL, 'e' );
const idEventDef EV_TDM_Handle_Tap( "Tap", NULL );
const idEventDef EV_TDM_Handle_FindDoor( "FindHandleDoor", NULL );

CLASS_DECLARATION( CBinaryFrobMover, CFrobDoorHandle )
	EVENT( EV_TDM_Handle_GetDoor,		CFrobDoorHandle::Event_GetDoor )
	EVENT( EV_TDM_Handle_Tap,			CFrobDoorHandle::Event_Tap )
	EVENT( EV_TDM_Handle_FindDoor,		CFrobDoorHandle::Event_FindHandleDoor)
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
	CBinaryFrobMover::Spawn();
	LoadTDMSettings();

	PostEventMS(&EV_TDM_Handle_FindDoor, 0);

	// Dorhandles are always non-interruptable
	m_bInterruptable = false;
}

void CFrobDoorHandle::Event_FindHandleDoor(void)
{
	idStr str;
	idEntity *cst;

	spawnArgs.GetString("door_body", "", str);
	if(str.Length() > 0 && (cst = FindDoor(str)) != NULL)
	{
		if((m_Door = dynamic_cast<CFrobDoor *>(cst)) != NULL)
			m_Door->SetDoorhandle(this);
	}
	else
	{
		DM_LOG(LC_SYSTEM, LT_ERROR)LOGSTRING("door_body [%s] for handle [%s] not found.\r", str.c_str(), name.c_str());
	}
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

void CFrobDoorHandle::Event_Tap(void)
{
	Tap();
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
bool CFrobDoorHandle::UsedBy(bool bInit, idEntity *e)
{
	if(m_Door)
		return m_Door->UsedBy(bInit, e);

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

void CFrobDoorHandle::DoneStateChange(void)
{
	DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("doorhandle [%s] finished state_change.\r", name.c_str());
}

void CFrobDoorHandle::DoneRotating(void)
{
	CBinaryFrobMover::DoneRotating();
}

void CFrobDoorHandle::DoneMoving(void)
{
	CBinaryFrobMover::DoneMoving();
}

void CFrobDoorHandle::Tap(void)
{
	double signal = 0;
	idStr s;

//	spawnArgs.GetString("door_handle_script", "door_handle_rotate", s);
	spawnArgs.GetString("door_handle_script", "", s);
	if(s.Length() == 0 || m_Door == NULL)
		return;

	signal = m_Door->AddSDKSignal(SigOpen, NULL);
	CallScriptFunctionArgs(s.c_str(), true, 0, "eef", this, m_Door, signal);
}

bool CFrobDoorHandle::isLocked(void)
{
	bool bLocked = m_Locked;

	if(m_Door)
		bLocked = m_Door->isLocked();

	return bLocked;
}

