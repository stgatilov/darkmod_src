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
#include "FrobDoor.h"
#include "FrobDoorHandle.h"

//===============================================================================
// CFrobDoorHandle
//===============================================================================
const idEventDef EV_TDM_Handle_GetDoor( "GetDoor", NULL, 'e' );

CLASS_DECLARATION( CFrobHandle, CFrobDoorHandle )
	EVENT( EV_TDM_Handle_GetDoor,		CFrobDoorHandle::Event_GetDoor )
END_CLASS

CFrobDoorHandle::CFrobDoorHandle() :
	m_Door(NULL)
{}

void CFrobDoorHandle::Save(idSaveGame *savefile) const
{
	savefile->WriteObject(m_Door);
}

void CFrobDoorHandle::Restore( idRestoreGame *savefile )
{
	savefile->ReadObject(reinterpret_cast<idClass*&>(m_Door));
}

void CFrobDoorHandle::Spawn()
{}

CFrobDoor* CFrobDoorHandle::GetDoor()
{
	return m_Door;
}

void CFrobDoorHandle::SetDoor(CFrobDoor* door)
{
	m_Door = door;

	// Set the frob master accordingly
	SetFrobMaster(m_Door);
}

void CFrobDoorHandle::Event_GetDoor()
{
	return idThread::ReturnEntity(m_Door);
}

void CFrobDoorHandle::OnOpenPositionReached()
{
	// The handle is "opened", trigger the door, but only if this is the master handle
	if (IsMasterHandle() && m_Door != NULL && !m_Door->IsOpen())
	{
		m_Door->OpenDoor(false);
	}

	// Let the handle return to its initial position
	Close(true);
}

void CFrobDoorHandle::Tap()
{
	// Invoke the base class first
	CFrobHandle::Tap();
	
	// Only the master handle is allowed to trigger sounds
	if (IsMasterHandle() && m_Door != NULL)
	{
		// Start the appropriate sound
		FrobMoverStartSound(m_Door->IsLocked() ? "snd_tap_locked" : "snd_tap_default");
	}
}

bool CFrobDoorHandle::DoorIsLocked()
{
	return m_Door ? m_Door->IsLocked() : IsLocked();
}
