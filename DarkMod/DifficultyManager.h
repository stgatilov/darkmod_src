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

/**
 * greebo: The Difficulty Manager provides methods to load
 *         the various spawnargs into the entities based on the
 *         selected mission difficulty.
 *
 *         The manager reads the default difficulty settings from the def/ folder
 *         and applies them to entities on demand. 
 */
class CDifficultyManager
{
	// The selected difficulty [0 .. DIFFICULTY_LEVELS]
	int _difficulty;

public:
	// Constructor
	CDifficultyManager();
	
	void SetDifficultyLevel(int difficulty);
	int GetDifficultyLevel() const;

	// Save/Restore methods
	void Save(idSaveGame* savefile);
	void Restore(idRestoreGame* savefile);
};

#endif /* DIFFICULTY_MANAGER_H */
