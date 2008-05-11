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
	~CModMenu();

	// handles main menu commands
	void HandleCommands(const char *menuCommand, idUserInterface *gui);

	// updates the GUI variables
	void UpdateGUI(idUserInterface* gui);

	// displays the current briefing page
	void DisplayBriefingPage(idUserInterface *gui);

private:
	idList<const char *> modsAvailable; 
	unsigned int modTop;
	int briefingPage;
};

#endif	/* !__MODS_H__ */
