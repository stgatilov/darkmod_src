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

#ifndef FROBDOOR_H
#define FROBDOOR_H

class CFrobDoorHandle;

// Number of clicksounds available
#define	MAX_PIN_CLICKS			14

// Number of clicks that have to be set as a minimum. A pattern of less than 
// 5 clicks is very easy to recognize, so it doesn't make sense to allow less than that.
#define MIN_CLICK_NUM			5
#define MAX_CLICK_NUM			10

/**
 * CFrobDoor is a replacement for idDoor. The reason for this replacement is
 * because idDoor is derived from idMover_binary and can only slide from one
 * position into another. In order to make swinging doors we need to rotate
 * them but this doesn't work with normal idDoors. So CFrobDoor is a mixture
 * of idDoor and idMover.
 */
class CFrobDoor : 
	public CBinaryFrobMover
{
public:
	CLASS_PROTOTYPE( CFrobDoor );

							CFrobDoor();

	virtual					~CFrobDoor();

	void					Spawn();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Open(bool Master);
	virtual void			Close(bool Master);

	/** 
	 * greebo: The OpenDoor method is necessary to give the FrobDoorHandles a 
	 * "low level" open routine. The CFrobDoor::Open() call is re-routed to 
	 * the FrobDoorHandle::Tap() method, so there must be a way to actually
	 * let the door open. Which is what this method does.
	 */
	virtual void			OpenDoor(bool Master);		

	virtual void			Lock(bool Master);
	virtual void			Unlock(bool Master);

	CFrobDoorHandle*		GetDoorhandle();
	// Adds a door handle to this door. A door can have multiple handles
	void					AddDoorhandle(CFrobDoorHandle* handle);

	virtual bool			CanBeUsedBy(const CInventoryItemPtr& item, const bool isFrobUse);					// Overrides idEntity::CanBeUsedBy
	virtual bool			UseBy(EImpulseState impulseState, const CInventoryItemPtr& item);	// Overrides idEntity::UseBy

	/**
	 * Write the proper sound loss value to the soundprop portal data
	 * Called when door spawns, is and when it is opened or closed
	 **/
	void					UpdateSoundLoss();

	/**
	 * Return the double door.  Returns NULL if there is none.
	 **/
	CFrobDoor*				GetDoubleDoor();

	/**
	 * Close the visportal, but only if the double door is also closed.
	 **/
	virtual void			ClosePortal();

	// Override the idEntity frob methods
	virtual void			SetFrobbed(bool val);
	virtual bool			IsFrobbed();

	// angua: returns the number of open peers
	ID_INLINE int			GetOpenPeersNum()
	{
		return m_OpenPeers.Num();
	}

	// Override idEntity::AttackAction to catch attack key presses from the player during lockpicking
	virtual void			AttackAction(idPlayer* player);

	/**
	 * greebo: Override the BinaryFrobMover function to re-route all sounds
	 * to the doorhandle. This avoids sounds being played from door origins,
	 * which is barely audible to the player.
	 */
	virtual int				FrobMoverStartSound(const char* soundName);

protected:

	/**
	 * This will read the spawnargs lockpick_bar, lockpick_rotate and 
	 * lockpick_translate, to setup the parameters how the bar or handle should behave
	 * while it is picked. Also other intialization stuff, that can only be done after all
	 * the entities are loaded, should be done here.
	 */
	virtual void			PostSpawn();

	// angua: flag the AAS area the door is located in with the travel flag TFL_DOOR
	virtual void			SetDoorTravelFlag();
	virtual void			ClearDoorTravelFlag();

	/**
	 * Find out if this door is touching another door, and if they share the same portal
	 * If so, store a pointer to the other door m_DoubleDoor on this door.
	 *
	 * This is posted as an event to be called on all doors after entities spawn
	 **/
	void					FindDoubleDoor();

	/** 
	 * greebo: This automatically searches for handles bound to this door and
	 * sets up the frob_peer, door_handle relationship for mapper's convenience.
	 */
	void					AutoSetupDoorHandles();

	/**
	 * greebo: This is the algorithm for linking the double door via open_peer and lock_peer.
	 */
	void					AutoSetupDoubleDoor();

	// angua: Specialise the CBinaryFrobMover::PreLock method to check whether lock peers are closed
	virtual bool			PreLock(bool bMaster);

	// Specialise the CBinaryFrobMover::OnLock() and OnUnlock() methods to update the peers
	virtual void			OnLock(bool bMaster);
	virtual void			OnUnlock(bool bMaster);

	// Specialise the OnStartOpen/OnStartClose event to send the call to the open peers
	virtual void			OnStartOpen(bool wasClosed, bool bMaster);
	virtual void			OnStartClose(bool wasOpen, bool bMaster);

	// Gets called when the mover finishes its closing move and is fully closed (virtual override)
	virtual void			OnClosedPositionReached();

	// Gets called when the mover is interrupted or blocked (virtual overrides)
	virtual void			OnInterrupt();
	virtual void			OnTeamBlocked(idEntity* blockedEntity, idEntity* blockingEntity);

	// Helper functions to cycle through the m_OpenList members
	void					OpenPeers();
	void					ClosePeers();
	void					OpenClosePeers(bool open);

	// Taps all doorhandles of open peers
	void					TapPeers();

	void					LockPeers();
	void					UnlockPeers();
	void					LockUnlockPeers(bool lock);

	// Accessor functions for adding and removing peers
	void					AddOpenPeer(const idStr& peerName);
	void					RemoveOpenPeer(const idStr& peerName);

	void					AddLockPeer(const idStr& peerName);
	void					RemoveLockPeer(const idStr& peerName);

	// Returns TRUE if all lock peer doors are at their respective "closed" position
	bool					AllLockPeersAtClosedPosition();

	// Updates the position of the attached handles according to the current lockpick state
	void					UpdateHandlePosition();

	// Required events which are called by the PickableLock class
	void					Event_Lock_StatusUpdate();
	void					Event_Lock_OnLockPicked();

	// Script event interface
	void					Event_GetDoorhandle();
	void					Event_OpenDoor(float master);

	// This is called periodically, to handle a pending close request (used for locking doors after closing)
	void					Event_HandleLockRequest();

protected:

	/**
	 * This is a list of slave doors, which should be opened and closed
	 * along with this door.
	 */
	idList<idStr>				m_OpenPeers;

	/** 
	 * This list is the pendant to the above one: m_OpenPeers. It specifies
	 * all names of the doors which should be locked/unlocked along with this one.
	 */
	idList<idStr>				m_LockPeers;

	/**
	 * greebo: This is set to TRUE when the door should be locked as soon as it has
	 * reached its closed position.
	 */
	bool						m_CloseOnLock;

	/**
	* Pointer to the door's partner in a double door.
	* Double door means two doors placed right next to eachother, sharing the
	*	same visportal.
	* 
	* The doubledoor does not necessarily have to be linked in a frob chain,
	*	it could be independently opened.
	**/
	idEntityPtr<CFrobDoor>		m_DoubleDoor;

	/**
	 * Handles that are associated with this door.
	 */
	idList< idEntityPtr<CFrobDoorHandle> >	m_Doorhandles;
};

#endif /* FROBDOOR_H */
