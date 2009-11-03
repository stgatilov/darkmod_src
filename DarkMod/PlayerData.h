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

public:
};

#endif
