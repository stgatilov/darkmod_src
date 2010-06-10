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
	struct ModInfo
	{
		idStr pathToFMPackage;
		idStr title;
		idStr desc;
		idStr author;
		idStr image;

		// Required TDM version
		idStr requiredVersionStr;
		int requiredMajor;
		int requiredMinor;

		ModInfo() :
			requiredMajor(TDM_VERSION_MAJOR),
			requiredMinor(TDM_VERSION_MINOR)
		{}
	};

	// A list of path => path associations for moving files around
	typedef std::list< std::pair<fs::path, fs::path> > MoveList;

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

	// Searches for new PK4 files in the fms/ root folder
	void SearchForNewMods();
	// Sub-routine of SearchForNewMods() investigating the FM folder using the given extension (including dot ".pk4", ".zip")
	MoveList SearchForNewMods(const idStr& extension);

	// Searches for new mods
	void BuildModList();

	// Sort found mods by title
	void SortModList();

	// Compare functor to sort mods by name
	static int ModSortCompare(const int* a, const int* b);

	// Installs the given mod
	void InstallMod(int modIndex, idUserInterface* gui);

	// Uninstalls the current FM
	void UninstallMod(idUserInterface* gui);

	// Find out which mod is currently installed (updates curMod member)
	void InitCurrentMod();

	// Restarts the game after mod installation
	void RestartGame();

	// Returns the darkmod path
	fs::path GetDarkmodPath();

	// Finds out which map is the starting map (must be called after InitCurrentMod)
	// After this call the CVAR tdm_mapName is initialised and holds the map name.
	void InitStartingMap();

	// Loads the contents of the readme.txt file to the GUI
	//idStr GetModNotes(int modIndex);

	// Retries a mod info structure for the given mod (folder) name
	ModInfo GetModInfo(const idStr& modName);
	ModInfo GetModInfo(int modIndex);

	// Convenience method which copies a file from <source> to <dest>
	// If <overwrite> is set to TRUE, any existing destination file will be removed beforehand
	// Note: CopyFile is already #define'd in a stupid WinBase.h header file, hence DoCopyFile.
	bool DoCopyFile(const fs::path& source, const fs::path& dest, bool overwrite = false);

	// Removes the given file, returns TRUE if this succeeded or if file wasn't there in the first place, FALSE on error
	bool DoRemoveFile(const fs::path& fileToRemove);

private:
	// The list of available mods
	idStrList _modsAvailable; 

	// The index of the first displayed mod
	int _modTop;

	int _briefingPage;

	// The name of the current mod
	idStr _curModName;

	// The map which should be the starting point
	idStr _startingMap;
};

#endif	/* !__MODS_H__ */
