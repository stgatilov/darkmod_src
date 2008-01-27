/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1643 $
 * $Date: 2007-11-02 08:00:36 +0100 (Fr, 02 Nov 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/

#ifndef DIFFICULTY_MANAGER_H
#define DIFFICULTY_MANAGER_H

#include "DifficultyMenu.h"
#include "DifficultySettings.h"

namespace difficulty {

/**
 * greebo: The Difficulty Manager provides methods to load
 *         the various spawnargs into the entities based on the
 *         selected mission difficulty.
 *
 *         The manager reads the default difficulty settings from the def/ folder
 *         and applies them to entities on demand. 
 */
class DifficultyManager
{
	// The selected difficulty [0 .. DIFFICULTY_COUNT-1]
	int _difficulty;

	// The global difficulty settings (parsed from the entityDefs)
	DifficultySettings _globalSettings[DIFFICULTY_COUNT];

public:
	// Constructor
	DifficultyManager();

	/**
	 * greebo: Initialises this class. This means loading the global default
	 *         difficulty settings from the entityDef files and the ones
	 *         from the map file (worldspawn setting, map-specific difficulty).
	 */
	void Init(idMapFile* mapFile);
	
	// Accessor methods for the currently chosen difficulty level
	void SetDifficultyLevel(int difficulty);
	int GetDifficultyLevel() const;

	/**
	 * greebo: Applies the spawnarg modifiers of the currently chosen
	 *         difficulty level to the given set of spawnargs.
	 */
	void ApplyDifficultySettings(idDict& target);

	// Save/Restore methods
	void Save(idSaveGame* savefile);
	void Restore(idRestoreGame* savefile);

private:
	// Loads the default difficulty settings from the entityDefs
	void LoadDifficultySettings();
};

} // namespace difficulty

#endif /* DIFFICULTY_MANAGER_H */
