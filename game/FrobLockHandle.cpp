/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod (http://www.thedarkmod.com/)
 
 $Revision$ (Revision of last commit) 
 $Date$ (Date of last commit)
 $Author$ (Author of last commit)
 
******************************************************************************/

// Copyright (C) 2004 Gerhard W. Gruber <sparhawk@gmx.at>
//

#include "precompiled_game.h"
#pragma hdrstop

static bool versioned = RegisterVersionedFile("$Id$");

#include "Game_local.h"
#include "DarkModGlobals.h"
#include "FrobLock.h"
#include "FrobLockHandle.h"

//===============================================================================
// CFrobLockHandle
//===============================================================================
const idEventDef EV_TDM_Handle_GetLock( "GetLock", NULL, 'e' );

CLASS_DECLARATION( CFrobHandle, CFrobLockHandle )
	EVENT( EV_TDM_Handle_GetLock,		CFrobLockHandle::Event_GetLock )
END_CLASS

CFrobLockHandle::CFrobLockHandle() :
	m_FrobLock(NULL)
{}

void CFrobLockHandle::Save(idSaveGame *savefile) const
{
	savefile->WriteObject(m_FrobLock);
}

void CFrobLockHandle::Restore( idRestoreGame *savefile )
{
	savefile->ReadObject(reinterpret_cast<idClass*&>(m_FrobLock));
}

void CFrobLockHandle::Spawn()
{}

CFrobLock* CFrobLockHandle::GetFrobLock()
{
	return m_FrobLock;
}

void CFrobLockHandle::SetFrobLock(CFrobLock* lock)
{
	m_FrobLock = lock;

	// Set the frob master accordingly
	SetFrobMaster(m_FrobLock);
}

void CFrobLockHandle::Event_GetLock()
{
	return idThread::ReturnEntity(m_FrobLock);
}

void CFrobLockHandle::OnOpenPositionReached()
{
	// The handle is "opened", trigger the lock, but only if this is the master handle
	if (IsMasterHandle() && m_FrobLock != NULL)
	{
		m_FrobLock->OpenTargets();
	}

	// Let the handle return to its initial position
	Close(true);
}

void CFrobLockHandle::Tap()
{
	// Invoke the base class first
	CFrobHandle::Tap();
	
	// Only the master handle is allowed to trigger sounds
	if (IsMasterHandle() && m_FrobLock != NULL)
	{
		// Start the appropriate sound
		FrobMoverStartSound(m_FrobLock->IsLocked() ? "snd_tap_locked" : "snd_tap_default");
	}
}

bool CFrobLockHandle::LockIsLocked()
{
	return m_FrobLock ? m_FrobLock->IsLocked() : IsLocked();
}
