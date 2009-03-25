/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include "../game/game_local.h"
#include "DarkModGlobals.h"
#include "FrobLock.h"

CLASS_DECLARATION( idStaticEntity, CFrobLock )
	EVENT( EV_PostSpawn,	CFrobLock::PostSpawn )
END_CLASS

void CFrobLock::Save(idSaveGame *savefile) const
{
	m_Lock.Save(savefile);
}

void CFrobLock::Restore( idRestoreGame *savefile )
{
	m_Lock.Restore(savefile);
}

void CFrobLock::Spawn()
{
	// Load the lock spawnargs
	m_Lock.InitFromSpawnargs(spawnArgs);

	/*
	unlock_trigger_delay
	lock_trigger_delay

	unlock_target_N
	lock_target_N

	trigger_targets_on_lock
	trigger_targets_on_unlock
	
	update_target_frobability

	snd_lock
	snd_locked
	snd_unlock
	*/

	PostEventMS(&EV_PostSpawn, 0);
}

void CFrobLock::PostSpawn()
{
	// TODO: Find lever entities
}

void CFrobLock::Lock()
{
	m_Lock.SetLocked(true);
}

void CFrobLock::Unlock()
{
	m_Lock.SetLocked(false);
}

bool CFrobLock::IsLocked()
{
	return m_Lock.IsLocked();
}

bool CFrobLock::IsPickable()
{
	return m_Lock.IsPickable();
}

void CFrobLock::Event_Lock_StatusUpdate()
{
	// TODO: Update lever positions
}

void CFrobLock::Event_Lock_OnLockPicked()
{
	Unlock();
}

void CFrobLock::Event_Lock_OnLockStatusChange()
{
	// TODO: Update frobability
	// TODO: Trigger targets
}
