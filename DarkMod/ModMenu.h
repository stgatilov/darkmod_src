/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef _MOD_MENU_H_
#define	_MOD_MENU_H_

#pragma once

#include <boost/shared_ptr.hpp>

class CModInfo;
typedef boost::shared_ptr<CModInfo> CModInfoPtr;

// Handles mainmenu that displays list of mods (FMs) and lets user
// chose which one to load. Also handles display of briefing page
class CModMenu
{
private:
	// The index of the first displayed mod
	int _modTop;

	int _briefingPage;

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
	bool PerformVersionCheck(const CModInfoPtr& mission, idUserInterface* gui);

	void UpdateSelectedMod(idUserInterface* gui);

	// Installs the given mission (doesn't accept NULL pointers);
	void InstallMission(const CModInfoPtr& mission, idUserInterface* gui);

	// Uninstalls the current FM
	void UninstallMod(idUserInterface* gui);

	// Restarts the game after mod installation
	void RestartGame();
};

#endif	/* _MOD_MENU_H_ */
