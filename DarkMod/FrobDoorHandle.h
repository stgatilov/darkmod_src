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
 * CFrobDoorHandle is the complement for CFrobDoors. 
 *
 * Basically, if a handle is frobbed instead of the actual door,
 * all calls are forwarded to its door, so that the player doesn't 
 * notice the difference. From the player's perspective frobbing
 * the handle feels the same as frobbing the door.
 *
 * When frobbing a door with a handle attached, the event chain is like this:
 * Frob > Door::Open > Handle::Tap > Handle moves to open pos > Door::OpenDoor
 */
class CFrobDoorHandle : 
	public CBinaryFrobMover
{
public:
	CLASS_PROTOTYPE( CFrobDoorHandle );

							CFrobDoorHandle();

	void					Spawn();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	/**
	 * Functions that must be forwarded to the door.
	 */
	void					SetFrobbed(bool val);
	bool					IsFrobbed();
	bool					UsedBy(IMPULSE_STATE nState, CInventoryItem* item);
	void					FrobAction(bool bMaster);

	// These functions need to be disabled on the handle. Therefore
	// they are provided but empty.
	void					ToggleLock();

	/**
	 * Get/set the door associated with this handle.
	 */
	CFrobDoor*				GetDoor();
	void					SetDoor(CFrobDoor* door);

	// greebo: Returns TRUE if the associated door is locked (not to confuse with CBinaryFrobMover::IsLocked())
	bool					DoorIsLocked();

	/**
	 * greebo: This method is invoked directly or it gets called by the attached door.
	 * For instance, a call to CFrobDoor::Open() gets re-routed here first to let 
	 * the handle animation play before actually trying to open the door.
	 *
	 * The Tap() algorithm attempts to rotate the door handle down until and 
	 * calls OpenDoor() when the handle reaches its end rotation/position.
	 */
	void					Tap();

	/** 
	 * greebo: Accessor methods for the "master" flag. If a door has multiple
	 * handles, only one is allowed to open the door, all others are dummies.
	 *
	 * Note: These methods are mainly for "internal" use by the CFrobDoor class.
	 */
	bool					IsMaster();
	void					SetMaster(bool isMaster);

protected:
	// Specialise the OpenPositionReached method of BinaryFrobMover to trigger the door's Open() routine
	virtual void			OnOpenPositionReached();

	// Script event interface
	void					Event_GetDoor();
	void					Event_Tap();

protected:
	/**
	* Pointer to the door that is associated with this handle
	**/
	CFrobDoor*				m_Door;

	/**
	 * A door can have multiple doorhandles attached, but only the master
	 * handle is allowed to call OpenDoor() to avoid double-triggering.
	 *
	 * This bool defaults to TRUE at spawn time, so the mapper doesn't need
	 * to care about that. If a door has multiple handles, the auto-setup
	 * algorithm takes care of "deactivating" all handles but one.
	 */
	bool					m_Master;

	// A mutable bool to avoid infinite loops when propagating the frob
	bool					m_FrobLock;
};

#endif /* FROBDOORHANDLE_H */
