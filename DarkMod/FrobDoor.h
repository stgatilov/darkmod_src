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

class CStimResponseTimer;
class CFrobDoorHandle;

// Number of clicksounds available
#define	MAX_PIN_CLICKS			14

// Number of clicks that have to be set as a minimum. A pattern of less than 
// 5 clicks is very easy to recognize, so it doesn't make sense to allow less than that.
#define MIN_CLICK_NUM			5
#define MAX_CLICK_NUM			10

typedef enum
{
	LPSOUND_INIT,				// Initial call (impulse has been triggered)
	LPSOUND_REPEAT,				// Call from the keyboardhandler for repeated presses
	LPSOUND_RELEASED,			// Call from the keyboardhandler for released presses
	LPSOUND_PIN_SAMPLE,			// Callback for pin sample
	LPSOUND_PIN_FAILED,			// Callback when the pin failed sound is finished
	LPSOUND_PIN_SUCCESS,		// Callback for the success sound sample
	LPSOUND_WRONG_LOCKPICK,		// Callback for the wrong lockpick sample
	LPSOUND_LOCK_PICKED			// Callback for the pin picked
} ELockpickSoundsample;

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
	typedef enum {
		HANDLE_POS_ORIGINAL,	// Reset it to the original starting value
		HANDLE_POS_SAMPLE		// Position it to a sample index.
	} EHandleReset;

public:
	CLASS_PROTOTYPE( CFrobDoor );

							CFrobDoor();

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

	bool					UsedBy(IMPULSE_STATE nState, CInventoryItem* item);

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

	void					ProcessLockpick(int cType, ELockpickSoundsample nSampleType);
	void					LockpickTimerEvent(int cType, ELockpickSoundsample nSoundSample);

	void					SetHandlePosition(EHandleReset, int msec, int pin_index = 0, int sample_index = 0);

	void					PropPickSound(idStr &picksound, int cType, ELockpickSoundsample nSampleType, int time, EHandleReset nHandlePos, int PinIndex, int SampleIndex);

protected:
	/**
	 * This will read the spawnargs lockpick_bar, lockpick_rotate and 
	 * lockpick_translate, to setup the parameters how the bar or handle should behave
	 * while it is picked. Also other intialization stuff, that can only be done after all
	 * the entities are loaded, should be done here.
	 */
	virtual void			PostSpawn();

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

	// Specialise the CBinaryFrobMover::OnLock() and OnUnlock() methods to update the peers
	virtual void			OnLock(bool bMaster);
	virtual void			OnUnlock(bool bMaster);

	// Specialise the OnStartOpen/OnStartClose event to send the call to the open peers
	virtual void			OnStartOpen(bool wasClosed, bool bMaster);
	virtual void			OnStartClose(bool wasOpen, bool bMaster);

	// Gets called when the mover finishes its closing move and is fully closed (virtual override)
	virtual void			OnClosedPositionReached();

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

	/**
	 * greebo: Override the BinaryFrobMover function to re-route all sounds
	 * to the doorhandle. This avoids sounds being played from door origins,
	 * which is barely audible to the player.
	 */
	virtual void			FrobMoverStartSound(const char* soundName);

	/**
	 * Create a random pin pattern for a given pin. Clicks defines the required 
	 * number of clicks for this pin, and BaseCount, defines the minimum number
	 * of clicks, which is always added.
	 */
	idStringList*			CreatePinPattern(int Clicks, int BaseCount, int MaxCount, int StrNumLen, idStr &Header);

	// Script event interface
	void					Event_GetDoorhandle();
	void					Event_IsPickable();
	void					Event_OpenDoor(float master);

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

	idList<idStringList *>		m_Pins;
	idList<idStringList *>		m_RandomPins;
	bool						m_Pickable;

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

	/**
	 * Bar is the movable part of a lock that should jiggle, while
	 * the lock is being picked. The default is to use the doorhandle
	 * but if lockpick_bar is set on the door, then this is used instead.
	 */
	idEntityPtr<idEntity>		m_Bar;

	/**
	 * These fractions define the stepping that a handle is moved for each pin,
	 * while it is picked. This will define the boundary of the jiggling effect
	 * to indicate the progress of the picking for each pin. The value is 
	 * determined by the boundary that a handle is moving when it is tapped.
	 * For example: If a handle rotates 45 degree (default for a regular door
	 * handle) and the lock has 6 pins, then the fraction is 45/6 = 7.5 degrees
	 * per successfully picked pin.
	 */
	bool						m_PinTranslationFractionFlag;
	bool						m_PinRotationFractionFlag;
	idVec3						m_PinTranslationFraction;
	idAngles					m_PinRotationFraction;

	/**
	 * These fractions define the stepping for a pin. These are recalculated
	 * whenever a new pin is used, because they only make sense within a given
	 * pin and each pin can have it's individual number of samples.
	 */
	idVec3						m_SampleTranslationFraction;
	idAngles					m_SampleRotationFraction;

	idVec3						m_OriginalPosition;
	idAngles					m_OriginalAngle;

	bool						m_KeyReleased;
};

#endif /* FROBDOOR_H */
