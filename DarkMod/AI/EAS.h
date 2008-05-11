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
#include "../MultiStateMover.h"
#include "../../game/ai/aas_local.h"

class idAASLocal;

/**
 * greebo: The EAS ("Elevator Awareness System") provides some extended
 * routing functionality between AAS clusters for all AI which are able
 * to handle elevators (i.e. travelFlag TFL_ELEVATOR set to 1).
 *
 * This class is tightly bound to its owning idAASLocal class and is always
 * constructed and destructed along with it.
 */
class tdmEAS
{
	// The owning AAS
	idAASLocal* _aas;

	// The list of elevators
	idList< idEntityPtr<CMultiStateMover> > _elevators;
public:
	// Initialise the EAS with a valid AAS reference
	tdmEAS(idAASLocal* aas);

	/**
	 * greebo: Computes the routing tables for the elevators.
	 * All elevators must have been added beforehand using AddElevator().
	 */
	void Compile();

	// Clears all data
	void Clear();

	/**
	 * greebo: Adds a new elevator to the EAS.
	 */
	void AddElevator(CMultiStateMover* mover);

	// Save/Restore routines
	void Save(idSaveGame* savefile) const;
	void Restore(idRestoreGame* savefile);
};

#endif /* __AI_EAS_H__ */
