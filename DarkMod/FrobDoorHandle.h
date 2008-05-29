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

#ifndef FROBDOORHANDLE_H
#define FROBDOORHANDLE_H

class CFrobDoor;

/**
 * CFrobDoorHandle is the complement for CFrobDoors. This is
 * quite similar to the doors itself, because they are attached
 * to it. If a handle is frobbed, instead of the actual door,
 * all calls are basically forwarded to it's door, so that the 
 * player doesn't see a difference. From the player perspective
 * the handle acts the same as the door.
 */
class CFrobDoorHandle : public CBinaryFrobMover {
public:
	friend class CFrobDoor;

	CLASS_PROTOTYPE( CFrobDoorHandle );

							CFrobDoorHandle( void );

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	/**
	 * Get the door that is currently associated with this handle.
	 */
	CFrobDoor				*GetDoor(void);
	void					Event_GetDoor(void);
	void					Event_Tap(void);

	/**
	 * Functions that must be forwarded to the door.
	 */
	void					SetFrobbed(bool val);
	bool					IsFrobbed(void);
	bool					UsedBy(IMPULSE_STATE nState, idEntity *);
	void					FrobAction(bool bMaster);

	// These functions need to be disabled on the handle. Therefore
	// they are provided but empty.
	void					ToggleOpen();
	void					ClosePortal(void);
	void					DoneStateChange(void);

	bool					isLocked(void);

	/**
	 * greebo: This method is invoked directly or it gets called by the attached door.
	 * For instance, a call to CFrobDoor::Open() gets re-routed here first to let 
	 * the handle animation play before actually trying to open the door.
	 *
	 * The Tap() algorithm attempts to rotate the door handle down until and 
	 * calls OpenDoor() when the handle reaches its end rotation/position.
	 */
	void					Tap();

protected:
	/**
	* Pointer to the door that is associated with this handle
	**/
	CFrobDoor				*m_Door;
	bool					m_FrobLock;

private:
};

#endif /* !TDMDOORHANDLE_H */
