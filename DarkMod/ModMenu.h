#ifndef __MODS_H__
#define	__MODS_H__

#pragma once

#define MODS_PER_PAGE 10

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

	idList<idStr> modsAvailable; 

	// The index of the first displayed mod
	int modTop;

	int briefingPage;
};

#endif	/* !__MODS_H__ */
