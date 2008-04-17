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
};

#endif /* _MULTI_STATE_MOVER_POSITION_H_ */
