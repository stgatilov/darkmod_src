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

class CDarkModPlayer {
public:
	CDarkModPlayer(void);
	~CDarkModPlayer(void);

	/**
	 * FrobEntity is NULL when no entity is highlighted. Otherwise it will point 
	 * to the entity which is currently highlighted.
	 */
	idEntity	*m_FrobEntity;

	/**
	 * FrobDistance is the distance from the player to the entity that is currently
	 * highlighted. If the FrobEntity pointer is NULL then this value is meaningless
	 * as there will be no entity highlighted at the time. This will be needed to
	 * check, if more than one item is within frobrange, which one is closer.
	 */
	float		m_FrobDistance;

	/**
	 * FrobAngle is the angle the player is aiming at the item. The angle is 
	 * normalized so that it is easier to compare it. Along with the FrobDistance
	 * this is also used to determine where the player is currently looking, and which
	 * item should take precedence, if the distance is not enough.
	 * This value is meaningless if FrobEntity is NULL.
	 */
	float		m_FrobAngle;
};

#endif
