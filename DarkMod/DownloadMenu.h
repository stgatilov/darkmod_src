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

#include <map>
#include <boost/shared_ptr.hpp>

// Handles mainmenu that displays list of downloadable mods/PK4 files
class CDownloadMenu
{
private:
	// The index of the first displayed mod
	int _availListTop;
	int _selectedListTop;

	idList<int> _selectedMods;

	/**
	 * greebo: Since mission l10n packs are stored separately, a mission
	 * transfer can consist of several downloads.
	 */
	struct MissionDownload
	{
		int missionDownloadId;	// Download ID of the actual mission pack
		int l10nPackDownloadId;	// Download ID of the L10n Pack, is -1 by default

		MissionDownload() :
			missionDownloadId(-1),
			l10nPackDownloadId(-1)
		{}

		MissionDownload(int missionDownloadId_, int l10nPackDownloadId_ = -1) :
			missionDownloadId(missionDownloadId_),
			l10nPackDownloadId(l10nPackDownloadId_)
		{}
	};

	// A mapping "selected mod id" => download info
	typedef std::map<int, MissionDownload> ActiveDownloads;
	ActiveDownloads _downloads;

public:
	CDownloadMenu();

	// handles main menu commands
	void HandleCommands(const idStr& cmd, idUserInterface* gui);

	// updates the GUI variables
	void UpdateGUI(idUserInterface* gui);

private:
	void StartDownload(idUserInterface* gui);

	void UpdateDownloadProgress(idUserInterface* gui);

	void ShowDownloadResult(idUserInterface* gui);

	void UpdateModDetails(idUserInterface* gui);

	void PerformScreenshotStep(idUserInterface* gui, int step);

	void UpdateNextScreenshotData(idUserInterface* gui, int nextScreenshotNum);
	void UpdateScreenshotItemVisibility(idUserInterface* gui);

	idStr GetMissionDownloadProgressString(int modIndex);
};
typedef boost::shared_ptr<CDownloadMenu> CDownloadMenuPtr;

#endif	/* _DOWNLOAD_MENU_H_ */
