/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef _DOWNLOAD_MENU_H_
#define	_DOWNLOAD_MENU_H_

#include <boost/shared_ptr.hpp>

// Handles mainmenu that displays list of downloadable missions
class CDownloadMenu
{
private:
	// The index of the first displayed mission
	int _availListTop;
	int _selectedListTop;

	idList<int> _selectedMissions;

public:
	CDownloadMenu();

	// handles main menu commands
	void HandleCommands(const idStr& cmd, idUserInterface* gui);

	// updates the GUI variables
	void UpdateGUI(idUserInterface* gui);

private:
	void StartDownload(idUserInterface* gui);
};
typedef boost::shared_ptr<CDownloadMenu> CDownloadMenuPtr;

#endif	/* _DOWNLOAD_MENU_H_ */
