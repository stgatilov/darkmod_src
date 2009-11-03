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
 * CDarkModPlayer is a class that maintains player data. The purpose of this
 * this class is mainly to be indenependent from idPlayer and seperate the code
 * from id's code.
 * Player data will store all additional data that is required like inventory,
 * special player states, currently highlighted entity and others.
 *
 * greebo: TODO: Move frob stuff to idPlayer, this class is a global and it should not hold
 * this kind of stuff, this basically rules out any future multiplayer efforts.
 */
class CDarkModPlayer {
public:
	CDarkModPlayer();

	void Save( idSaveGame *savefile ) const;
	void Restore( idRestoreGame *savefile );

	int						AddLight(idLight *);
	int						RemoveLight(idLight *);

public:
	/**
	 * LightgemValue determines the level of visibillity of the player.
	 * This value is used to light up the lightgem and is defined as
	 * 1 <= N <= 32
	 */
	int							m_LightgemValue;
	/**
	 * Contains the last lightgem value. This is stored for interleaving.
	 */
	float						m_fColVal;

	/**
	 * Each light entity must register here itself. This is used
	 * to calculate the value for the lightgem.
	 *
	 * greebo: TODO: Move this to the Darkmod LAS.
	 */
	idList< idEntityPtr<idLight> >	m_LightList;
};

#endif
