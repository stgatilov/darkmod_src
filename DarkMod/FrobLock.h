/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef _FROB_LOCK_H_
#define _FROB_LOCK_H_

#include "PickableLock.h"
#include "FrobLockHandle.h"

/** 
 * greebo: This class represents a pickable lock. It supports
 * attachment of BinaryFrobMovers which are used as levers.
 */
class CFrobLock :
	public idStaticEntity
{
	// The actual lock implementation
	PickableLock	m_Lock;

	/**
	 * Handles that are associated with this lock.
	 */
	idList< idEntityPtr<CFrobLockHandle> >	m_Lockhandles;

	// The last time we issues an "Update handle" call
	int				m_LastHandleUpdateTime;

public:
	CLASS_PROTOTYPE( CFrobLock );

	CFrobLock();

	void			Spawn();

	bool			IsLocked();
	bool			IsPickable();

	void			Lock();
	void			Unlock();
	void			ToggleLock();

	// Attempt to open the lock, this usually triggers the handle movement
	void			Open();

	// This tries to open/lock/unlock any targetted frobmovers
	void			OpenTargets();
	void			CloseTargets();
	void			ToggleOpenTargets();
	void			LockTargets();
	void			UnlockTargets();
	void			CloseAndLockTargets();

	virtual bool	CanBeUsedBy(const CInventoryItemPtr& item, const bool isFrobUse);	// Overrides idEntity::CanBeUsedBy
	virtual bool	UseBy(EImpulseState impulseState, const CInventoryItemPtr& item);	// Overrides idEntity::UseBy

	virtual void	AttackAction(idPlayer* player); // Override idEntity::AttackAction to catch attack key presses from the player during lockpicking

	void			Save(idSaveGame *savefile) const;
	void			Restore(idRestoreGame *savefile);

protected:
	void			PostSpawn();

	void			UpdateHandlePosition();

	// Adds a lockhandle to this lock. A lock can have multiple handles
	void			AddLockHandle(CFrobLockHandle* handle);

	/** 
	 * greebo: This automatically searches for handles bound to this lock and
	 * sets up the frob_peer, lock_handle relationship for mapper's convenience.
	 */
	void			AutoSetupLockHandles();

	virtual int		FrobLockStartSound(const char* soundName);

	virtual void	OnLock();
	virtual void	OnUnlock();

	// Required events which are called by the PickableLock class
	void			Event_Lock_StatusUpdate();
	void			Event_Lock_OnLockPicked();
	void			Event_Lock_OnLockStatusChange(int locked);

	// Private events, used for deferred triggering of lock/unlock/general targets
	void			Event_TriggerTargets();
	void			Event_TriggerLockTargets();
	void			Event_TriggerUnlockTargets();

	// Called by the frob action script
	void			Event_Open();

	void			Event_ClearPlayerImmobilization(idEntity* player);
};

#endif /* _FROB_LOCK_H_ */
