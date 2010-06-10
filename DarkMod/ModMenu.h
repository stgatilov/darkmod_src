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

#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

class CMissionInfo;
typedef boost::shared_ptr<CMissionInfo> CMissionInfoPtr;

// Handles mainmenu that displays list of mods (FMs) and lets user
// chose which one to load. Also handles display of briefing page
class CModMenu
{
public:
	CModMenu();

	// handles main menu commands
	void HandleCommands(const char *menuCommand, idUserInterface *gui);

	// updates the GUI variables
	void UpdateGUI(idUserInterface* gui);

	// displays the current briefing page
	void DisplayBriefingPage(idUserInterface *gui);

private:

	// Performs the version check and returns TRUE if positive,
	// returns FALSE otherwise (and issues failure calls to the given GUI)
	bool PerformVersionCheck(const CMissionInfoPtr& mission, idUserInterface* gui);

	void UpdateSelectedMod(idUserInterface* gui);

	// Installs the given mission (doesn't accept NULL pointers);
	void InstallMission(const CMissionInfoPtr& mission, idUserInterface* gui);

	// Uninstalls the current FM
	void UninstallMod(idUserInterface* gui);

	// Restarts the game after mod installation
	void RestartGame();

	// Returns the darkmod path
	fs::path GetDarkmodPath();

private:
	// The index of the first displayed mod
	int _modTop;

	int _briefingPage;
};

#endif	/* !__MODS_H__ */
