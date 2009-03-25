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

/** 
 * greebo: This class represents a pickable lock. It supports
 * attachment of BinaryFrobMovers which are used as levers.
 */
class CFrobLock :
	public idStaticEntity
{
	// The actual lock implementation
	PickableLock	m_Lock;

public:
	CLASS_PROTOTYPE( CFrobLock );

	void			Spawn();

	bool			IsLocked();
	bool			IsPickable();

	void			Lock();
	void			Unlock();
	void			ToggleLock();

	virtual bool	CanBeUsedBy(const CInventoryItemPtr& item, const bool isFrobUse);	// Overrides idEntity::CanBeUsedBy
	virtual bool	UseBy(EImpulseState impulseState, const CInventoryItemPtr& item);	// Overrides idEntity::UseBy

	virtual void	AttackAction(idPlayer* player); // Override idEntity::AttackAction to catch attack key presses from the player during lockpicking

	void			Save(idSaveGame *savefile) const;
	void			Restore(idRestoreGame *savefile);

protected:
	void			PostSpawn();

	virtual int		FrobLockStartSound(const char* soundName);

	// Required events which are called by the PickableLock class
	void			Event_Lock_StatusUpdate();
	void			Event_Lock_OnLockPicked();
	void			Event_Lock_OnLockStatusChange();
};

#endif /* _FROB_LOCK_H_ */
