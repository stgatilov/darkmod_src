#ifndef __MODS_H__
#define	__MODS_H__

#pragma once

// Handles mainmenu that displays list of mods (FMs) and lets user
// chose which one to load. Also handles display of briefing page
class CModMenu
{
	struct ModInfo
	{
		idStr pathToFMPackage;
		idStr title;
		idStr desc;
		idStr author;
		idStr image;
	};

public:
	CModMenu();

	// Initialises the mod menu and builds the list of available mods
	void Init();

	void Clear();

	// Save/Restore routines
	void Save(idSaveGame* savefile) const;
	void Restore(idRestoreGame* savefile);

	// handles main menu commands
	void HandleCommands(const char *menuCommand, idUserInterface *gui);

	// updates the GUI variables
	void UpdateGUI(idUserInterface* gui);

	// displays the current briefing page
	void DisplayBriefingPage(idUserInterface *gui);

private:

	// Searches for new mods
	void LoadModList();

	// Installs the given mod
	void InstallMod(int modIndex, idUserInterface* gui);

	// Find out which mod is currently installed (updates curMod member)
	void InitCurrentMod();

	// Finds out which map is the starting map (must be called after InitCurrentMod)
	// After this call the CVAR tdm_mapName is initialised and holds the map name.
	void InitStartingMap();

	// Retries a mod info structure for the given mod (folder) name
	ModInfo GetModInfo(const idStr& modName);
	ModInfo GetModInfo(int modIndex);

	// The list of available mods
	idList<idStr> _modsAvailable; 

	// The index of the first displayed mod
	int _modTop;

	int _briefingPage;

	// The name of the current mod
	idStr _curModName;

	// The map which should be the starting point
	idStr _startingMap;
};

#endif	/* !__MODS_H__ */
