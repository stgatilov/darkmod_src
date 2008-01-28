/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2008-01-27 18:53:28 +0200 (Di, 27 Jan 2008) $
 * $Author: greebo $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "DifficultyManager.h"

namespace difficulty {

// Constructor
DifficultyManager::DifficultyManager() :
	_difficulty(0)
{}

void DifficultyManager::Init(idMapFile* mapFile)
{
	DM_LOG(LC_DIFFICULTY, LT_INFO).LogString("Searching for difficulty setting on worldspawn.\r");

	if (mapFile->GetNumEntities() <= 0) {
		return; // no entities!
	}

	// Fetch the worldspawn
	idMapEntity* mapEnt = mapFile->GetEntity(0);
	idDict spawnArgs = mapEnt->epairs;

	int mapDifficulty;
	if (spawnArgs.GetInt("difficulty", "0", mapDifficulty))
	{
		// We have a difficulty spawnarg set on the map's worldspawn, take it as override value
		DM_LOG(LC_DIFFICULTY, LT_DEBUG).LogString("Found overriding difficulty setting on worldspawn entity: %d.\r", mapDifficulty);
		_difficulty = mapDifficulty;
	}

	// Load the difficulty settings from the entityDefs
	LoadDifficultySettings();
}

void DifficultyManager::SetDifficultyLevel(int difficulty)
{
	_difficulty = difficulty;
}

int DifficultyManager::GetDifficultyLevel() const
{
	return _difficulty;
}

void DifficultyManager::Save(idSaveGame* savefile)
{
	savefile->WriteInt(_difficulty);
	for (int i = 0; i < DIFFICULTY_COUNT; i++)
	{
		_globalSettings[i].Save(savefile);
	}
}

void DifficultyManager::Restore(idRestoreGame* savefile)
{
	savefile->ReadInt(_difficulty);
	for (int i = 0; i < DIFFICULTY_COUNT; i++)
	{
		_globalSettings[i].Restore(savefile);
	}
}

void DifficultyManager::ApplyDifficultySettings(idDict& target)
{
	DM_LOG(LC_DIFFICULTY, LT_INFO).LogString("Applying difficulty settings to entity: %s.\r", target.GetString("name"));

	// greebo: Preliminary case: just apply the global settings
	_globalSettings[_difficulty].ApplySettings(target);
}

void DifficultyManager::LoadDifficultySettings()
{
	DM_LOG(LC_DIFFICULTY, LT_INFO).LogString("Trying to load global difficulty settings from entityDefs.\r");

	// greebo: Try to lookup the entityDef for each difficulty level and load the settings
	for (int i = 0; i < DIFFICULTY_COUNT; i++)
	{
		idStr defName("difficulty_settings_");
		defName += i;

		const idDict* difficultyDict = gameLocal.FindEntityDefDict(defName);

		if (difficultyDict != NULL)
		{
			DM_LOG(LC_DIFFICULTY, LT_DEBUG).LogString("Found difficulty settings: %s.\r", defName.c_str());
			_globalSettings[i].InitFromEntityDef(*difficultyDict);
		}
		else
		{
			_globalSettings[i].Clear();
			gameLocal.Warning("DifficultyManager: Could not find default difficulty entityDef for difficulty level %d", i);
		}
	}
}

} // namespace difficulty
