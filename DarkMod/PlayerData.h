/******************************************************************************/
/*                                                                            */
/*         DarkModGlobals (C) by Gerhard W. Gruber in Germany 2004            */
/*                          All rights reserved                               */
/*                                                                            */
/******************************************************************************/

/******************************************************************************
 *
 * PROJECT: DarkMod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 * $Name$
 *
 * $Log$
 * Revision 1.13  2007/02/03 21:56:21  sparhawk
 * Removed old inventories and fixed a bug in the new one.
 *
 * Revision 1.12  2006/08/07 06:52:55  ishtvan
 * added m_FrobTrace variable that gets set by idPlayer::FrobCheck
 *
 * Revision 1.11  2006/07/27 09:01:07  ishtvan
 * added m_FrobEntityPrevious var to store the frob entity of the previous frame
 *
 * Revision 1.10  2005/12/12 02:57:33  ishtvan
 * ammo items that are frobbed go into the D3 inventory
 *
 * added inventory clearing function
 *
 * Revision 1.9  2005/11/26 17:42:45  sparhawk
 * Lightgem cleaned up
 *
 * Revision 1.8  2005/10/24 21:01:12  sparhawk
 * Lightgem interleave added.
 *
 * Revision 1.7  2005/09/24 03:13:49  lloyd
 * Changed CGrabber grabber to CGrabber *grabber
 *
 * Revision 1.6  2005/09/17 00:32:23  lloyd
 * added copyBind event and arrow sticking functionality (additions to Projectile and modifications to idEntity::RemoveBind
 *
 * Revision 1.5  2005/03/21 23:02:03  sparhawk
 * Lightgem extended from 16 to 32 stages
 *
 * Revision 1.4  2005/01/07 02:01:11  sparhawk
 * Lightgem updates
 *
 * Revision 1.3  2004/11/24 21:59:06  sparhawk
 * *) Multifrob implemented
 * *) Usage of items against other items implemented.
 * *) Basic Inventory system added.
 *
 * Revision 1.2  2004/11/03 21:47:17  sparhawk
 * Changed debug LogString for better performance and group settings
 *
 * Revision 1.1  2004/10/31 20:03:36  sparhawk
 * CDarkMod created to contain relevant player data seperate from id code.
 *
 *
 * DESCRIPTION: This file data which is relevant to the player. Examples
 * are: pointer to the inventory, currently highlighted entity and others.
 *
 *****************************************************************************/

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

	// grabber to help with object manipulation
	CGrabber		*grabber;

	/**
	 * FrobEntity is NULL when no entity is highlighted. Otherwise it will point 
	 * to the entity which is currently highlighted.
	 */
	idEntity		*m_FrobEntity;

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
	idEntity	*m_FrobEntityPrevious;

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
