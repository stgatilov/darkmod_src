/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2008-05-11 18:53:28 +0200 (Su, 11 May 2008) $
 * $Author: greebo $
 *
 ***************************************************************************/

#ifndef __AI_EAS_H__
#define __AI_EAS_H__

#include "../idlib/precompiled.h"

/**
 * greebo: The EAS ("Elevator Awareness System") provides some extended
 * routing functionality between AAS clusters for all AI which are able
 * to handle elevators (i.e. travelFlag TFL_ELEVATOR set to 1).
 */
class tdmEAS
{
public:
	// Save/Restore routines
	void Save(idSaveGame* savefile) const;
	void Restore(idRestoreGame* savefile);
};

#endif /* __AI_EAS_H__ */
