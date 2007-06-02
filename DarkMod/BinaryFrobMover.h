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
	* This is the non-script version of GetOpen 
	*/
	bool					isOpen(void)
	{
		return m_Open;
	}

	/**
	* This indicates if the door was interrupted in its last action
	*/
	bool wasInterrupted(void)
	{
		return m_bInterrupted;
	}

	/**
	* This is the non-script version of GetLock
	*/
	virtual bool isLocked(void)
	{
		return m_Locked;
	}


	/**
	* Overload the apply impulse function to see if we should change mover
	* state when impulse is applied
	*
	* Description of function from idEntity::ApplyImpulse
	* apply an impulse to the physics object, 'ent' is the entity applying the impulse
	**/

	virtual void			ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse );

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

	/**
	* Close the portal.  Overloaded on CFrobDoor
	**/
	virtual void			ClosePortal(void);

	/**
	* This is used to test if the mover is moving
	*/
	virtual bool			isMoving();

	/**
	* This is used to test if it is changing state
	*/
	virtual bool isChangingState();

	/**
	* This is used to get the remaining translation left on the clip model
	* if it is moving
	*
	* @param out_translation: Passes back out the translation remaining in
	*	the current movement.
	*
	* @param out_deltaAngles: Passes back out the rotation remaining in
	*	the current movement.
	*
	*/
	void getRemainingMovement
	(
		idVec3& out_deltaPosition,
		idAngles& out_deltaAngles
	);

protected:
	/**
	* By default, a BinaryFrobMover toggles its state when triggered
	**/
	void					Event_Activate( idEntity *activator );

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

	/**
	* The following variables determine the behavior when the binary mover
	* receives an impulse from the physics system.
	*
	* Square of the impulse magnitude thresholds for "opening" and "closing"
	* If these are set to 0 or below, it does nothing.
	**/
	float						m_ImpulseThreshCloseSq;
	float						m_ImpulseThreshOpenSq;

	/**
	* Normalized direction vectors to resolve the impulse along
	* when opening and closing.
	* I.e., the impulse vector * this axis yields the impulse magnitude 
	* that is compared to the threshold.
	*
	* (NOTE: These default to zero, and if they are set to zero,
	* the axis of the impulse is not checked, impulse magnitude is used in comparison)
	**/
	idVec3						m_vImpulseDirOpen;
	idVec3						m_vImpulseDirClose;

private:
};

#endif /* !BINARYFROBMOVER */
