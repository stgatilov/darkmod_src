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
const idEventDef EV_TDM_Handle_GetDoor( "GetDoor", NULL, 'e' );
const idEventDef EV_TDM_Handle_Tap( "Tap", NULL );

CLASS_DECLARATION( CBinaryFrobMover, CFrobDoorHandle )
	EVENT( EV_TDM_Handle_GetDoor,		CFrobDoorHandle::Event_GetDoor )
	EVENT( EV_TDM_Handle_Tap,			CFrobDoorHandle::Event_Tap )
END_CLASS

CFrobDoorHandle::CFrobDoorHandle() :
	m_Door(NULL),
	m_FrobLock(false)
{}

void CFrobDoorHandle::Save(idSaveGame *savefile) const
{
	savefile->WriteObject(m_Door);
	savefile->WriteBool(m_FrobLock);
}

void CFrobDoorHandle::Restore( idRestoreGame *savefile )
{
	savefile->ReadObject(reinterpret_cast<idClass*&>(m_Door));
	savefile->ReadBool(m_FrobLock);
}

void CFrobDoorHandle::Spawn(void)
{
	// Dorhandles are always non-interruptable
	m_bInterruptable = false;

	// greebo: The handle itself must never locked, otherwise it can't move in Tap()
	m_Locked = false;
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
bool CFrobDoorHandle::UsedBy(IMPULSE_STATE nState, CInventoryItem* item)
{
	if(m_Door)
		return m_Door->UsedBy(nState, item);

	return false;
}

void CFrobDoorHandle::FrobAction(bool bMaster)
{
	if(m_Door)
		m_Door->FrobAction(bMaster);
}

// A handle can't close or open a portal, so we block it. The same is true for the Done* and statechanges
void CFrobDoorHandle::ClosePortal(void)
{
}

void CFrobDoorHandle::OpenPortal(void)
{
}


void CFrobDoorHandle::ToggleOpen(void)
{
	if( !m_Rotating && !m_Translating )
	{
		Open(true);
	}
}

void CFrobDoorHandle::ToggleLock() 
{}

void CFrobDoorHandle::DoneStateChange(void)
{
	DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("doorhandle [%s] finished state_change.\r", name.c_str());
	CBinaryFrobMover::DoneStateChange();

	if (m_Open)
	{
		// The handle is "opened", trigger the door
		if (m_Door != NULL && !m_Door->IsOpen())
		{
			m_Door->OpenDoor(false);
		}

		Close(true);
	}
}

void CFrobDoorHandle::Tap()
{
	// Trigger the handle movement
	ToggleOpen();

	if (m_Door != NULL)
	{
		// Start the appropriate sound
		idStr snd = m_Door->IsLocked() ? "snd_tap_locked" : "snd_tap_default";
		StartSound(snd, SND_CHANNEL_ANY, 0, false, NULL);
	}
}

bool CFrobDoorHandle::DoorIsLocked()
{
	return m_Door ? m_Door->IsLocked() : m_Locked;
}
