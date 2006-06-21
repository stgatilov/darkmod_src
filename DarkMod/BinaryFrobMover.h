/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.1  2006/06/21 15:02:27  sparhawk
 * FrobDoor derived now from BinaryFrobMover
 *
 * Revision 1.13  2006/05/07 21:52:12  ishtvan
 * *) fixed interruption on opening problem
 * *) Added 'interruptable' spawnarg
 * *) Added offset position for translation in case the item starts inbetween states
 * *) Added translation speed variable
 *
 * Revision 1.12  2006/05/06 20:23:35  sparhawk
 * Fixed problem with determining when the animation is finished.
 *
 * Revision 1.11  2006/05/03 21:31:21  sparhawk
 * Statechange callback script added.
 *
 * Revision 1.10  2006/05/02 20:39:33  sparhawk
 * Translation added
 *
 * Revision 1.9  2006/04/29 22:10:56  sparhawk
 * Added some script functions to query the state of a door.
 *
 * Revision 1.8  2006/01/22 09:20:24  ishtvan
 * rewrote to match new soundprop interface
 *
 * Revision 1.7  2005/09/29 04:03:08  ishtvan
 * added support for double doors
 *
 * Revision 1.6  2005/09/27 08:07:15  ishtvan
 * *) Added new member vars to support multiple frob functionality
 *
 * *) Overloaded idMover::RotationDone()
 *
 * Revision 1.5  2005/04/07 08:42:38  ishtvan
 * Added placeholder method GetSoundLoss, which is called by CsndProp
 *
 * Revision 1.4  2004/11/24 21:59:06  sparhawk
 * *) Multifrob implemented
 * *) Usage of items against other items implemented.
 * *) Basic Inventory system added.
 *
 * Revision 1.3  2004/11/21 01:02:03  sparhawk
 * Doors can now be properly opened and have sound.
 *
 * Revision 1.2  2004/11/16 23:56:03  sparhawk
 * Frobcode has been generalized now and resides for all entities in the base classe.
 *
 * Revision 1.1  2004/11/14 20:19:12  sparhawk
 * Initial Release
 *
 *
 ***************************************************************************/

// Copyright (C) 2004 Gerhard W. Gruber <sparhawk@gmx.at>
//

#ifndef BINARYFROBMOVER_H
#define BINARYFROBMOVER_H

/**
 * CBinaryFrobMover is a replacement for idDoor. The reason for this replacement is
 * because idDoor is derived from idMover_binary and can only slide from one
 * position into another. In order to make swinging doors we need to rotate
 * them but this doesn't work with normal idDoors. So CBinaryFrobMover is a mixture
 * of idDoor and idMover.
 */
class CBinaryFrobMover : public idMover {
public:
	CLASS_PROTOTYPE( CBinaryFrobMover );

							CBinaryFrobMover( void );

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );

	virtual void			Open(bool Master);
	virtual void			Close(bool Master);
	virtual void			Lock(bool Master);
	virtual void			Unlock(bool Master);

	virtual void			ToggleOpen(void);
	virtual void			ToggleLock(void);
	virtual void			GetOpen(void);
	virtual void			GetLock(void);

	/**
	* Overloading idMover::DoneRotating in order to close the portal when door closes
	**/
	virtual void			DoneRotating( void );

	/**
	 *
	 */
	virtual void			DoneMoving(void);

	/**
	 * A helper function that implements the finalisation for rotations or movings.
	 */
	virtual void			DoneStateChange(void);
	virtual void			CallStateScript(void);

protected:
	/**
	* m_Open is only set to false when the door is completely closed
	**/
	bool						m_Open;
	bool						m_Locked;

	/**
	* Stores whether the player intends to open or close the door
	* Useful for determining what to do when the door is stopped midway.
	**/
	bool						m_bIntentOpen;

	bool						m_StateChange;

	/**
	* Determines whether the door may be interrupted.  Read from spawnargs.
	**/
	bool						m_bInterruptable;

	/**
	* Set to true if the door was stopped mid-rotation
	**/
	bool						m_bInterrupted;

	/**
	* Read from the spawnargs, interpreted into m_OpenAngles and m_ClosedAngles
	**/
	idAngles					m_Rotate;

	/**
	 * Original position
	 */
	idVec3						m_StartPos;

	/**
	 * Vector that specifies the direction and length of the translation.
	 * This is needed for doors that don't rotate, but slide to open.
	 */
	idVec3						m_Translation;

	/**
	* Translation speed in doom units per second, read from spawnargs.
	* Defaults to zero.  
	* If set to zero, D3's physics chooses the speed based on a constant time of movement.
	**/
	float						m_TransSpeed;

	/**
	* Door angles when completely closed
	**/
	idAngles					m_ClosedAngles;

	/**
	* Door angles when completely open
	**/
	idAngles					m_OpenAngles;

	/**
	 * Scriptfunction that is called, whenever the door is finished rotating
	 * or translating. i.E. when the statechange is completed.
	 * The function gets as parameters the current state:
	 * DoorComplete(boolean open, boolean locked, boolean interrupted);
	 */
	idStr						m_CompletionScript;

	bool						m_Rotating;
	bool						m_Translating;

private:
};

#endif /* !BINARYFROBMOVER */
