/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 2168 $
 * $Date: 2008-04-08 21:53:58 +0200 (Di, 08 Apr 2008) $
 * $Author: greebo $
 *
 ***************************************************************************/
#ifndef _MULTI_STATE_MOVER_POSITION_H_
#define _MULTI_STATE_MOVER_POSITION_H_

class CMultiStateMover;

/**
 * greebo: A MultiStateMoverPosition is an entity carrying information
 * about a possible MultiStateMover position (doh!).
 *
 * It can represent a floor of an elevator, for instance. The MultiStateMover
 * itself must target a set of two or more Position entities at spawn time.
 */
class CMultiStateMoverPosition : 
	public idEntity
{
public:
	CLASS_PROTOTYPE( CMultiStateMoverPosition );

	void	Spawn();

	// greebo: These two events are called when the mulitstate mover leaves/arrives the position
	virtual void	OnMultistateMoverArrive(CMultiStateMover* mover);
	virtual void	OnMultistateMoverLeave(CMultiStateMover* mover);

private:
	/**
	 * Calls the mover event script whose name is contained in the given spawnArg.
	 * The script thread is immediately executed, if the function exists. 
	 *
	 * @spawnArg: The spawnarg containing the script function name (e.g. "call_on_leave")
	 */
	void RunMoverEventScript(const idStr& spawnArg, CMultiStateMover* mover);
};

/**
 * greebo: This is the info structure which gets inserted into
 * a local list in the MultiStateMover itself.
 */
struct MoverPositionInfo 
{
	// The name of the position
	idStr name;

	// The position entity
	idEntityPtr<CMultiStateMoverPosition> positionEnt;
};

#endif /* _MULTI_STATE_MOVER_POSITION_H_ */
