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
// CFrobDoorHandle
//===============================================================================
const idEventDef EV_TDM_Handle_GetDoor( "GetDoor", NULL, 'e' );
const idEventDef EV_TDM_Handle_Tap( "Tap", NULL );

CLASS_DECLARATION( CBinaryFrobMover, CFrobDoorHandle )
	EVENT( EV_TDM_Handle_GetDoor,		CFrobDoorHandle::Event_GetDoor )
	EVENT( EV_TDM_Handle_Tap,			CFrobDoorHandle::Event_Tap )
END_CLASS

CFrobDoorHandle::CFrobDoorHandle() :
	m_Door(NULL),
	m_Master(true),
	m_FrobLock(false)
{}

void CFrobDoorHandle::Save(idSaveGame *savefile) const
{
	savefile->WriteObject(m_Door);
	savefile->WriteBool(m_Master);
	savefile->WriteBool(m_FrobLock);
}

void CFrobDoorHandle::Restore( idRestoreGame *savefile )
{
	savefile->ReadObject(reinterpret_cast<idClass*&>(m_Door));
	savefile->ReadBool(m_Master);
	savefile->ReadBool(m_FrobLock);
}

void CFrobDoorHandle::Spawn()
{
	// Dorhandles are always non-interruptable
	m_bInterruptable = false;

	// greebo: The handle itself must never locked, otherwise it can't move in Tap()
	m_Locked = false;
}

CFrobDoor* CFrobDoorHandle::GetDoor()
{
	return m_Door;
}

void CFrobDoorHandle::SetDoor(CFrobDoor* door)
{
	m_Door = door;
}

void CFrobDoorHandle::Event_GetDoor()
{
	return idThread::ReturnEntity(m_Door);
}

void CFrobDoorHandle::Event_Tap()
{
	Tap();
}

void CFrobDoorHandle::SetFrobbed(bool val)
{
	if (m_FrobLock == false)		// Prevent an infinte loop here.
	{
		m_FrobLock = true;

		idEntity::SetFrobbed(val);

		if (m_Door != NULL)
		{
			m_Door->SetFrobbed(val);
		}

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
	return (m_Door != NULL) ? m_Door->IsFrobbed() : idEntity::IsFrobbed();
}

bool CFrobDoorHandle::CanBeUsedBy(const CInventoryItemPtr& item, bool isFrobUse)
{
	// Pass the call to the door, if we have one, otherwise let the base class handle it
	return (m_Door != NULL) ? m_Door->CanBeUsedBy(item, isFrobUse) : idEntity::CanBeUsedBy(item, isFrobUse);
}

bool CFrobDoorHandle::UseBy(EImpulseState impulseState, const CInventoryItemPtr& item)
{
	// Pass the call to the door, if we have one, otherwise let the base class handle it
	return (m_Door != NULL) ? m_Door->UseBy(impulseState, item) : idEntity::UseBy(impulseState, item);
}

void CFrobDoorHandle::FrobAction(bool bMaster)
{
	if (m_Door != NULL)
	{
		m_Door->FrobAction(bMaster);
	}
}

void CFrobDoorHandle::ToggleLock() 
{}

bool CFrobDoorHandle::IsMaster()
{
	return m_Master;
}

void CFrobDoorHandle::SetMaster(bool isMaster)
{
	m_Master = isMaster;
}

void CFrobDoorHandle::UpdatePosition(float fraction)
{
	idQuat newRotation;
	newRotation.Slerp(m_ClosedAngles.ToQuat(), m_OpenAngles.ToQuat(), fraction);

	const idAngles& curAngles = physicsObj.GetLocalAngles();
	idAngles newAngles = newRotation.ToAngles().Normalize360();

	if (!(curAngles - newAngles).Normalize180().Compare(idAngles(0,0,0), 0.01f))
	{
		Event_RotateTo(newAngles);
	}

	MoveToLocalPos(m_ClosedOrigin + (m_OpenOrigin - m_ClosedOrigin)*fraction);

	UpdateVisuals();
}

void CFrobDoorHandle::OnOpenPositionReached()
{
	// The handle is "opened", trigger the door, but only if this is the master handle
	if (m_Master && m_Door != NULL && !m_Door->IsOpen())
	{
		m_Door->OpenDoor(false);
	}

	// Let the handle return to its initial position
	Close(true);
}

void CFrobDoorHandle::Tap()
{
	// Trigger the handle movement
	ToggleOpen();

	// Only the master handle is allowed to trigger sounds
	if (m_Master && m_Door != NULL)
	{
		// Start the appropriate sound
		FrobMoverStartSound(m_Door->IsLocked() ? "snd_tap_locked" : "snd_tap_default");
	}
}

bool CFrobDoorHandle::DoorIsLocked()
{
	return m_Door ? m_Door->IsLocked() : m_Locked;
}
