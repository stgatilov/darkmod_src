/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef __MODS_H__
#define	__MODS_H__

#pragma once

#include "boost/filesystem.hpp"
namespace fs = boost::filesystem;

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

	void UpdateSelectedMod(idUserInterface* gui);

	// Installs the given mod
	void InstallMod(int modIndex, idUserInterface* gui);

	// Uninstalls the current FM
	void UninstallMod(idUserInterface* gui);

	// Restarts the game after mod installation
	void RestartGame();

	// Returns the darkmod path
	fs::path GetDarkmodPath();

	// Finds out which map is the starting map (must be called after InitCurrentMod)
	// After this call the CVAR tdm_mapName is initialised and holds the map name.
	void InitStartingMap();

private:
	// The index of the first displayed mod
	int _modTop;

	int _briefingPage;

	// The map which should be the starting point
	idStr _startingMap;
};

#endif	/* !__MODS_H__ */
