/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 2168 $
 * $Date: 2008-04-08 21:53:58 +0200 (Di, 08 Apr 2008) $
 * $Author: greebo $
 *
 ***************************************************************************/
#ifndef _MULTI_STATE_MOVER_H_
#define _MULTI_STATE_MOVER_H_

/**
 * greebo: A MultiState mover is an extension to the vanilla D3 elevators.
 *
 * In contrast to the idElevator class, this multistate mover draws the floor
 * information from the MultiStateMoverPosition entities which are targetted
 * by this class.
 * 
 * The MultiStateMover will start moving when it's triggered (e.g. by buttons), 
 * where the information where to go is contained on the triggering button.
 */
class CMultiStateMover : 
	public idElevator
{
public:
	CLASS_PROTOTYPE( CMultiStateMover );

	CMultiStateMover();

	void	Spawn();

	void	Save(idSaveGame *savefile) const;
	void	Restore(idRestoreGame *savefile);

	void	Activate(idEntity* activator);

private:
	void	Event_Activate(idEntity* activator);
	void	Event_PostSpawn();
};

#endif /* _MULTI_STATE_MOVER_H_ */
