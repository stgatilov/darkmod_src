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
class CFrobDoor : public CBinaryFrobMover {
public:
	typedef enum {
		HANDLE_POS_ORIGINAL,	// Reset it to the original starting value
		HANDLE_POS_SAMPLE		// Position it to a sample index.
	} EHandleReset;

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

	void					ProcessLockpick(int cType, ELockpickSoundsample nSampleType);
	void					LockpickTimerEvent(int cType, ELockpickSoundsample nSoundSample);

	void					SetHandlePosition(EHandleReset, int msec, int pin_index = 0, int sample_index = 0);

	/**
	 * Init will read the spawnargs lockpick_bar, lockpick_rotate and 
	 * lockpick_translate, to setup the parameters how the bar or handle should behave
	 * while it is picked. Also other intialization stuff, that can only be done after all
	 * the entities are loaded, should be done here.
	 */
	void					Event_Init(void);

protected:
	/**
	 * Create a random pin pattern for a given pin. Clicks defines the required 
	 * number of clicks for this pin, and BaseCount, defines the minimum number
	 * of clicks, which is always added.
	 */
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
	 * Handle that is associated with this door, if the door has one.
	 */
	idEntityPtr<CFrobDoorHandle>	m_Doorhandle;

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

private:
};

#endif /* !TDMDOOR_H */
