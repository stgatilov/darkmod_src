/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef PLAYERDATA_H
#define PLAYERDATA_H

#include "Grabber.h"

/**
 * CInventoryItem is a metaclass for storing entities in the inventory.
 * There are several properties for various items which are not directly
 * stored in the item itself, because they are independent from the
 * entity and only related to the inventory.
 *
 * Playercoordinates are:
 * x = forward/backward
 * y = left/right
 * z = up/down
 */

/**
 * CDarkModPlayer is a class that maintains player data. The purpose of this
 * this class is mainly to be indenependent from idPlayer and seperate the code
 * from id's code.
 * Player data will store all additional data that is required like inventory,
 * special player states, currently highlighted entity and others.
 */
class CDarkModPlayer {
public:
	CDarkModPlayer(void);
	~CDarkModPlayer(void);

	void Save( idSaveGame *savefile ) const;
	void Restore( idRestoreGame *savefile );

	// grabber to help with object manipulation
	CGrabber		*grabber;

	/**
	 * FrobEntity is NULL when no entity is highlighted. Otherwise it will point 
	 * to the entity which is currently highlighted.
	 */
	idEntityPtr<idEntity>	m_FrobEntity;

	/**
	* Frobbed joint and frobbed clipmodel ID if an AF has been frobbed
	* Set to INVALID and -1 if the frobbed entity is not an AF
	**/
	jointHandle_t	m_FrobJoint;
	int				m_FrobID;

	/**
	* The trace that was done for frobbing
	* Read off by idEntity::UpdateFrob when something has been newly frobbed
	**/
	trace_t			m_FrobTrace;

	/**
	* Frob entity in the previous frame
	* We need this to detect when something was frobbed but now is not
	* Cannot rely on m_FrobEntity for this, because it could change to a new
	* entity before the old entity is updated.
	**/
	idEntityPtr<idEntity>	m_FrobEntityPrevious;

	unsigned long			AddLight(idLight *);
	unsigned long			RemoveLight(idLight *);

public:
	/**
	 * LightgemValue determines the level of visibillity of the player.
	 * This value is used to light up the lightgem and is defined as
	 * 1 <= N <= 16
	 */
	int							m_LightgemValue;
	/**
	 * Contains the last lightgem value. This is stored for interleaving.
	 */
	float						m_fColVal;

	/**
	 * Each light entity must register here itself. This is used
	 * to calculate the value for the lightgem.
	 */
	idList<idLight *>			m_LightList;
};

#endif
