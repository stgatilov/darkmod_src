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
 * greebo: This class is designed specifically for buttons.
 * It doesn't do much more than using the BinaryFrobMover functions for now
 * but I guess we will want some additional functionality in the future.
 */
class CFrobLever : 
	public CBinaryFrobMover 
{
public:

	CLASS_PROTOTYPE( CFrobLever );

	void			Spawn();

	void			Save(idSaveGame *savefile) const;
	void			Restore(idRestoreGame *savefile);

	// Switches the lever state to the given state (true = "on")
	void			SwitchState(bool newState);

	// Calling Operate() toggles the current state
	void			Operate();

	// Virtual overrides of BinaryFrobMover methods
	virtual void	Open(bool Master);
	virtual void	Close(bool Master);

private:
	void			Event_Operate();
	void			Event_Switch(int newState);

	virtual void	ClosePortal();
	virtual void	OpenPortal();

};

#endif /* _FROB_LEVER_H_ */
