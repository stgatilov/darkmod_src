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

void DifficultyManager::Clear()
{
	for (int i = 0; i < DIFFICULTY_COUNT; i++)
	{
		_globalSettings[i].Clear();
	}
}

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

	// Load the default difficulty settings from the entityDefs
	LoadDefaultDifficultySettings();

	LoadMapDifficultySettings(mapFile);
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

bool DifficultyManager::InhibitEntitySpawn(const idDict& target) {
	bool isAllowed(true);

	// Construct the key ("diff_0_spawn")
	idStr key = va("diff_%d_spawn", _difficulty);

	// The entity is allowed to spawn by default, must be set to 0 by the mapper
	isAllowed = target.GetBool(key, "1");

	DM_LOG(LC_DIFFICULTY, LT_INFO).LogString("Entity %s is allowed to spawn: %s.\r", target.GetString("name"), isAllowed ? "YES" : "NO");

	// Return false if the entity is allowed to spawn
	return !isAllowed;
}

void DifficultyManager::LoadDefaultDifficultySettings()
{
	DM_LOG(LC_DIFFICULTY, LT_INFO).LogString("Trying to load default difficulty settings from entityDefs.\r");

	// Construct the entityDef name (e.g. atdm:difficulty_settings_default_0)
	idStr defName(DEFAULT_DIFFICULTY_ENTITYDEF);

	const idDict* difficultyDict = gameLocal.FindEntityDefDict(defName);

	if (difficultyDict != NULL)
	{
		DM_LOG(LC_DIFFICULTY, LT_DEBUG).LogString("Found difficulty settings: %s.\r", defName.c_str());

		// greebo: Try to lookup the entityDef for each difficulty level and load the settings
		for (int i = 0; i < DIFFICULTY_COUNT; i++)
		{
			// Let the setting structure know which level it is referring to
			_globalSettings[i].SetLevel(i);
			// And load the settings
			_globalSettings[i].LoadFromEntityDef(*difficultyDict);
		}
	}
	else
	{
		for (int i = 0; i < DIFFICULTY_COUNT; i++)
		{
			_globalSettings[i].Clear();
		}
		gameLocal.Warning("DifficultyManager: Could not find default difficulty entityDef!");
	}
}

void DifficultyManager::LoadMapDifficultySettings(idMapFile* mapFile)
{
	DM_LOG(LC_DIFFICULTY, LT_INFO).LogString("Trying to load map-specific difficulty settings.\r");

	if (mapFile == NULL) return;

	for (int i = 0; i < mapFile->GetNumEntities(); i++)
	{
		idMapEntity* ent = mapFile->GetEntity(i);

		if (idStr::Icmp(ent->epairs.GetString("classname"), DIFFICULTY_ENTITYDEF) == 0)
		{
			LoadMapDifficultySettings(ent);
		}
	}
}

void DifficultyManager::LoadMapDifficultySettings(idMapEntity* ent)
{
	if (ent == NULL) return;

	// greebo: Let each global settings structure investigate the settings 
	// on this entity.
	for (int i = 0; i < DIFFICULTY_COUNT; i++)
	{
		_globalSettings[i].LoadFromMapEntity(ent);
	}
}

} // namespace difficulty
