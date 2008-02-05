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

#define DEFAULT_DIFFICULTY_ENTITYDEF "atdm:difficulty_settings_default"
#define DIFFICULTY_ENTITYDEF "atdm:difficulty_settings"

/**
 * greebo: The Difficulty Manager provides methods to load
 *         the various spawnargs into the entities based on the
 *         selected mission difficulty.
 *
 * During initialisation, the manager reads the default difficulty settings 
 * from the def/ folder. The procedure is as follows:
 *
 * 1) Read global default settings from the entities matching DIFFICULTY_ENTITYDEF.
 * 2) Search the map for tdm_difficulty_settings_map entities: these settings
 *    will override any default settings found in step 1 (settings that target the same
 *    entityclass/spawnarg combination will be removed and replaced by the ones defined in the map).
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

	// Clears everything associated with difficulty settings.
	void Clear();

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

	/**
	 * greebo: Checks whether the given entity (represented by "target") is allowed to spawn.
	 * 
	 * @returns: TRUE if the entity should NOT be spawned.
	 */
	bool InhibitEntitySpawn(const idDict& target);

	// Save/Restore methods
	void Save(idSaveGame* savefile);
	void Restore(idRestoreGame* savefile);

private:
	// Loads the default difficulty settings from the entityDefs
	void LoadDefaultDifficultySettings();

	// Loads the map-specific difficulty settings (these will override the default ones)
	void LoadMapDifficultySettings(idMapFile* mapFile);

	// Loads the map-specific difficulty settings from the given map entity
	void LoadMapDifficultySettings(idMapEntity* ent);
};

} // namespace difficulty

#endif /* DIFFICULTY_MANAGER_H */
