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
protected:
	enum ELockpickState
	{
		UNLOCKED = 0,			// Not locked in the first place
		LOCKED,					// Lockpicking not started yet
		LOCKPICKING_STARTED,	// Right before playing the first pin sample sound
		ADVANCE_TO_NEXT_SAMPLE,	// Right after playing the first ping sample sound
		PIN_SAMPLE,				// Playing pick sample sound (including sample delay)
		PIN_SAMPLE_SWEETSPOT,	// Playing sweetspot sound (for pavlov mode, this is the hotspot)
		PIN_DELAY,				// Delay after pattern (for non-pavlov mode, this is the hotspot)
		AFTER_PIN_DELAY,		// Right after the post-pattern delay
		WRONG_LOCKPICK_SOUND,	// Playing wrong lockpick sound
		PIN_SUCCESS,			// Playing pin success sound
		PIN_FAILED,				// Playing pin failed sound
		LOCK_SUCCESS,			// Playing entire lock success
		PICKED,					// Lock is picked
		NUM_LPSTATES,
	};

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

	bool					IsPickable();

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

protected:

	// Fork point to determine what should happen with a certain lockpicking impulse
	bool					ProcessLockpickImpulse(EImpulseState impulseState, int type);

	// Specialised methods to handle certain impulse events
	bool					ProcessLockpickPress(int type);
	bool					ProcessLockpickRepeat(int type);
	bool					ProcessLockpickRelease(int type);

	// During the lockpick "hotspot" phase the player is able to unlock the door
	// when pushing / releasing the right buttons.
	bool					LockpickHotspotActive();

	// For debugging purposes
	void					UpdateLockpickHUD();

	/**
	 * greebo: Play the given sound, this will post a "sound finished" event 
	 * after the sound has been played (+ the given delay in ms).
	 * When the sound is done, the lockpick state will be set to <nextState>.
	 */
	void					PropPickSound(const idStr& picksound, ELockpickState nextState, int additionalDelay = 0);

	/**
	 * greebo: Checks whether the given lockpick type is matching the current pin.
	 * @returns: TRUE on match, FALSE if not matching or lock is not pickable/already picked
	 */
	bool					CheckLockpickType(int type);

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

	/**
	 * greebo: Override the BinaryFrobMover function to re-route all sounds
	 * to the doorhandle. This avoids sounds being played from door origins,
	 * which is barely audible to the player.
	 */
	virtual int			FrobMoverStartSound(const char* soundName);

	/**
	 * Create a random pin pattern for a given pin. Clicks defines the required 
	 * number of clicks for this pin, and BaseCount, defines the minimum number
	 * of clicks, which is always added.
	 */
	idStringList			CreatePinPattern(int Clicks, int BaseCount, int MaxCount, int StrNumLen, idStr &Header);

	// Called when a pin is successfully unlocked
	virtual void			OnLockpickPinSuccess();
	// Called when the player failed to unlock this pin
	virtual void			OnLockpickPinFailure();

	// Updates the position of the attached handles according to the current lockpick state
	void					UpdateHandlePosition();
	float					CalculateHandleMoveFraction();

	// Gets called when a lockpick sound is finished playing
	void					Event_LockpickSoundFinished(ELockpickState nextState);

	// Script event interface
	void					Event_GetDoorhandle();
	void					Event_IsPickable();
	void					Event_OpenDoor(float master);

	// This is called periodically, to handle a pending close request (used for locking doors after closing)
	void					Event_HandleLockRequest();

protected:
	
	// Lockpick state
	ELockpickState				m_LockpickState;

	/**
	 * greebo: This variable keeps track of how many times the player missed
	 * a single pin of the lock. When auto-pick is enabled, the door will be
	 * be unlocked automatically after a certain amount of rounds.
	 */
	int							m_FailedLockpickRounds;

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

	struct PinInfo
	{
		// The pin pattern (a list of sound shader names for each sample)
		idStringList pattern;

		/** 
		 * greebo: This is used for the random handle jiggling while lockpicking.
		 * It holds (for each pin) the posisiton indices the handles should be at,
		 * plus one extra position for the delay after the pattern.
		 *
		 * Example: 
		 * A single pattern has 5 samples: 0 1 2 3 4. When traversing the samples
		 * like this, the handle would move in linear steps to the next pin position.
		 *
		 * Using the info in this variable, the pin positions are mapped to different 
		 * values such that the handle moves randomly, something like this:
		 * 0 3 1 2 4. Here, the sample with the index 1 would be mapped 
		 * to the linear position 3. The first and last position are fixed.
		 */
		idList<int> positions;
	};

	idList<PinInfo>				m_Pins;
	
	bool						m_Pickable;

	/**
	 * greebo: This is set to TRUE when the door should be locked as soon as it has
	 * reached its closed position.
	 */
	bool						m_CloseOnLock;

	/**
	 * FirstLockedPinIndex stores the index that is currently to be picked.
	 * If the value gets == m_Pins.Num() it means all pins are picked
	 * and the lock is sucessfully picked.
	 */
	int							m_FirstLockedPinIndex;

	/**
	 * This stores the index of the current pins soundsample to be played.
	 * The second index of the twodimensional array from m_SoundPinIndex.
	 */
	int							m_SoundPinSampleIndex;

	/**
	 * SoundTimerStarted increased for each soundsample that is started
	 */
	int							m_SoundTimerStarted;

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
