#ifndef __MODS_H__
#define	__MODS_H__

#pragma once

// Handles mainmenu that displays list of mods (FMs) and lets user
// chose which one to load. Also handles display of briefing page
class CModMenu
{
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

	// Find out which mod is currently installed (updates curMod member)
	void GetCurrentMod();

	idList<idStr> _modsAvailable; 

	// The index of the first displayed mod
	int _modTop;

	int _briefingPage;
};

#endif	/* !__MODS_H__ */
