/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef _FROB_LEVER_H_
#define _FROB_LEVER_H_

/** 
 * greebo: This class is designed specifically for levers.
 *
 * It builds on top of the BinaryFrobMover class and overrides
 * the relevant virtual event functions to implement proper 
 * two-state lever behaviour.
 */
class CFrobLever : 
	public CBinaryFrobMover 
{
public:

	CLASS_PROTOTYPE( CFrobLever );

	void			Spawn();

	void			Save(idSaveGame *savefile) const;
	void			Restore(idRestoreGame *savefile);

	// Switches the lever state to the given state (true = "open")
	void			SwitchState(bool newState);

	// Calling Operate() toggles the current state
	void			Operate();

protected:
	// Specialise the postspawn event 
	virtual void	PostSpawn();

	// Specialise the BinaryFrobMover events
	virtual void	OnOpenPositionReached();
	virtual void	OnClosedPositionReached();

protected:
	// The latch is to keep track of our visited positions.
	// For instance, the lever should not trigger the targets at
	// its closed position when it wasn't fully opened before.
	bool			m_Latch;

private:
	// Script interface
	void			Event_Operate();
	void			Event_Switch(int newState);
};

#endif /* _FROB_LEVER_H_ */
