#ifndef __MODS_H__
#define	__MODS_H__

#pragma once

#define MODS_PER_PAGE 12

// Handles mainmenu that displays list of mods (FMs) and lets user
// chose which one to load.
class CModMenu
{
public:
	CModMenu();
	~CModMenu();

	// handles main menu commands
	void HandleCommands(const char *menuCommand, idUserInterface *gui);

	// updates the GUI variables
	void UpdateGUI(idUserInterface* gui);

private:
	idList<const char *> modsAvailable; 
	unsigned int modTop;
};

#endif	/* !__MODS_H__ */
