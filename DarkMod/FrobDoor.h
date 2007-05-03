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

/**
 * CFrobDoor is a replacement for idDoor. The reason for this replacement is
 * because idDoor is derived from idMover_binary and can only slide from one
 * position into another. In order to make swinging doors we need to rotate
 * them but this doesn't work with normal idDoors. So CFrobDoor is a mixture
 * of idDoor and idMover.
 */
class CFrobDoor : public CBinaryFrobMover {
public:
	CLASS_PROTOTYPE( CFrobDoor );

							CFrobDoor( void );
							~CFrobDoor( void );

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );

	void					Open(bool Master);
	void					OpenDoor(bool Master);		// Needed for the handle to riger the open state
	void					Close(bool Master);
	void					Lock(bool Master);
	void					Unlock(bool Master);

	void					GetPickable(void);
	void					GetDoorhandle(void);

	bool					UsedBy(bool bInit, idEntity *);

	/**
	* Write the proper sound loss value to the soundprop portal data
	* Called when door spawns, is and when it is opened or closed
	**/
	void					UpdateSoundLoss(void);

	/**
	* Find out if this door is touching another door, and if they share the same portal
	* If so, store a pointer to the other door m_DoubleDoor on this door.
	*
	* This is posted as an event to be called on all doors after entities spawn
	**/
	void					FindDoubleDoor( void );

	/**
	* Return the double door.  Returns NULL if there is none.
	**/
	CFrobDoor *				GetDoubleDoor( void );

	/**
	* Close the visportal, but only if the double door is also closed.
	**/
	void					ClosePortal( void );

	void					SetDoorhandle(CFrobDoorHandle *);
	void					SetFrobbed(bool val);
	bool					IsFrobbed(void);

	void					DoneStateChange(void);
	void					ToggleOpen(void);
	void					ToggleLock(void);

protected:
	// Create a random pin pattern for a given pin. Clicks defines the required 
	// number of clicks for this pin, and BaseCount, defines the minimum number
	// of clicks, which is always added.
	idStringList				*CreatePinPattern(int Clicks, int BaseCount);

protected:
	/**
	 * LinkedOpen will point to a door that is to be switched when this
	 * one is triggered. Note that the next door is flipped! This means
	 * it will change it's state according to it's current state. So if
	 * this door is open and the other one is closed this door will be
	 * closed and the other one will be opened. If both are open and they
	 * are used, both are closed and vice versa. With this pointer you can
	 * also create a chain of doors by each door pointing to the next one.
	 * Of ocurse the last door in the chain should NOT point to the first
	 * door, otherwise it will result in an endless loop.
	 */
	idStr						m_MasterOpen;
	idList<idStr>				m_OpenList;

	/**
	 * This member is the same as m_LinkedOpen, only for locks. This means
	 * that, if this door is locked, or unlocked, all other associated doors
	 * will also be locked or unlocked. Again the state depends on the respective
	 * entity state and not on the action itself. This means that if one door
	 * is locked and the other is unlocked, the lockstate will reverse. If both
	 * are locked or unlocked, both will become unlocked or locked.
	 * This way you can create i.e. a safety catch were always one door is open
	 * and the other one is closed. Or you can create a set of doors that all are
	 * locked when this one is unlocked.
	 */
	idStr						m_MasterLock;
	idList<idStr>				m_LockList;

	idList<idStringList *>		m_Pins;
	// Once a pin is successfully picked it should stay so, so we have to remember that state.
	idList<bool>				m_PinsPicked;
	bool						m_Pickable;

	/**
	* Pointer to the door's partner in a double door.
	* Double door means two doors placed right next to eachother, sharing the
	*	same visportal.
	* 
	* The doubledoor does not necessarily have to be linked in a frob chain,
	*	it could be independently opened.
	**/
	CFrobDoor					*m_DoubleDoor;

	/**
	 * Handle that is associated with this door, if the door has one.
	 */
	CFrobDoorHandle				*m_Doorhandle;

	/**
	 * The picktimer is used to time the lockpicking. Since the usekey
	 * is a button, it will fire every frame, so we have to use a delay
	 * that allows us to execute the next try on the lockpick after a 
	 * certain time.
	 */
	CStimResponseTimer			*m_PickTimer;

	/**
	 * FirstLockedPinIndex stores the index that is currently to be picked.
	 * If the value gets == m_Pins.Num() it means all pins are picked
	 * and the lock is sucessfully picked.
	 */
	int							m_FirstLockedPinIndex;

	/**
	 * When lockpicking is started, it will loop over all the pinpatterns.
	 * This means that the pins are a twodimensional array where each pin has
	 * N samples associated with it.
	 * Here we store which pin is currently processed.
	 */
	int							m_SoundPinIndex;

	/**
	 * This stores the index of the current pins soundsample to be played.
	 * The second index of the twodimensional array from m_SoundPinIndex.
	 */
	int							m_SoundPinSampleIndex;

private:
};

#endif /* !TDMDOOR_H */
