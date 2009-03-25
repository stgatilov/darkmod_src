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
#include "Inventory/Item.h"
#include "Inventory/Category.h"

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
	snd_wrong_key
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

void CFrobLock::ToggleLock()
{
	m_Lock.SetLocked(!m_Lock.IsLocked());
}

bool CFrobLock::CanBeUsedBy(const CInventoryItemPtr& item, const bool isFrobUse) 
{
	if (item == NULL) return false;

	assert(item->Category() != NULL);

	const idStr& name = item->Category()->GetName();

	if (name == "Keys")
	{
		// Keys can always be used on doors
		// Exception: for "frob use" this only applies when the mover is locked
		return (isFrobUse) ? IsLocked() : true;
	}
	else if (name == "Lockpicks")
	{
		if (!m_Lock.IsPickable())
		{
			// Lock is not pickable
			DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("FrobLock %s is not pickable\r", name.c_str());
			return false;
		}

		// Lockpicks behave similar to keys
		return (isFrobUse) ? IsLocked() : true;
	}

	return idEntity::CanBeUsedBy(item, isFrobUse);
}

bool CFrobLock::UseBy(EImpulseState impulseState, const CInventoryItemPtr& item)
{
	if (item == NULL) return false;

	assert(item->Category() != NULL);

	// Retrieve the entity behind that item and reject NULL entities
	idEntity* itemEntity = item->GetItemEntity();
	if (itemEntity == NULL) return false;

	// Get the name of this inventory category
	const idStr& itemName = item->Category()->GetName();
	
	if (itemName == "Keys" && impulseState == EPressed) 
	{
		// Keys can be used on button PRESS event, let's see if the key matches
		if (m_UsedBy.FindIndex(itemEntity->name) != -1)
		{
			// just toggle the lock. 
			ToggleLock();
			return true;
		}
		else
		{
			FrobLockStartSound("snd_wrong_key");
			return false;
		}
	}
	else if (itemName == "Lockpicks")
	{
		if (!m_Lock.IsPickable())
		{
			// Lock is not pickable
			DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("FrobLock %s is not pickable\r", name.c_str());
			return false;
		}

		// First we check if this item is a lockpick. It has to be of correct type
		idStr str = itemEntity->spawnArgs.GetString("lockpick_type", "");

		if (str.Length() == 1)
		{
			// Pass the call to the lockpick routine
			return m_Lock.ProcessLockpickImpulse(impulseState, static_cast<int>(str[0]));
		}
		else
		{
			gameLocal.Warning("Wrong 'lockpick_type' spawnarg for lockpicking on item %s, must be a single character.", itemEntity->name.c_str());
			return false;
		}
	}

	return idEntity::UseBy(impulseState, item);
}

void CFrobLock::AttackAction(idPlayer* player)
{
	m_Lock.AttackAction(player);
}

int CFrobLock::FrobLockStartSound(const char* soundName)
{
	// Default implementation: Just play the sound on this entity.
	int length = 0;
	StartSound(soundName, SND_CHANNEL_ANY, 0, false, &length);

	return length;
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
