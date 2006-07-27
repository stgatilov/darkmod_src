/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.1  2006/07/27 20:30:40  sparhawk
 * Initial release. Just the skeleton of the spawnable CFrobDoorHandle class.
 *
 *
 ***************************************************************************/

// Copyright (C) 2004 Gerhard W. Gruber <sparhawk@gmx.at>
//

#ifndef FROBDOORHANDLE_H
#define FROBDOORHANDLE_H

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
	CLASS_PROTOTYPE( CFrobDoorHandle );

							CFrobDoorHandle( void );

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );

	/**
	 * Get the door that is currently associated with this handle.
	 */
	CFrobDoor				*GetDoor(void);
	void					Event_GetDoor(void);

	/**
	 * Find the door, that is associated with this handle.
	 */
	CFrobDoor				*FindDoor(idStr &Doorname);

	/**
	* Overloading idMover::DoneRotating in order to close the portal when door closes
	**/
	void					DoneRotating( void );

	/**
	 *
	 */
	void					DoneMoving(void);

protected:
	/**
	* Pointer to the door that is associated with this handle
	**/
	CFrobDoor					*m_Door;

private:
};

#endif /* !TDMDOORHANDLE_H */
