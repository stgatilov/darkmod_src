/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef _MISSION_MANAGER_H_
#define _MISSION_MANAGER_H_

#include "MissionInfo.h"
#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>

// Shared_ptr typedef
#include "../pugixml/pugixml.hpp"
typedef boost::shared_ptr<pugi::xml_document> XmlDocumentPtr;

class CMissionDB;
typedef boost::shared_ptr<CMissionDB> CMissionDBPtr;

namespace fs = boost::filesystem;

const char* const TMP_MISSION_SCREENSHOT_PREFIX = "previewshot_";
const char* const TMP_MISSION_SCREENSHOT_FOLDER = "_missionshots"; // relative to fms/

struct MissionScreenshot
{
	// Image filename relative to the fms/<mission>/ folder
	// This will empty if the screenie hasn't been downloaded yet
	idStr	filename;

	// The server-relative URL for downloading that screenshot
	idStr	serverRelativeUrl;

	// Get the local image file name without path, e.g. "monastery01.jpg"
	idStr GetLocalFilename() const
	{
		idStr temp;
		serverRelativeUrl.ExtractFileName(temp);

		idStr ext;
		temp.ExtractFileExtension(ext);

		temp.StripTrailingOnce(ext);
		temp.StripTrailingOnce(".");

		// Locally We save screenshots as JPG
		return temp + ".jpg";
	}

	// Returns the image filename including extension
	idStr GetRemoteFilename() const
	{
		idStr temp;
		serverRelativeUrl.ExtractFileName(temp);
		
		return temp;
	}

	// Returns the file extension of the server file (lowercase, without dot, e.g. "png")
	idStr GetRemoteFileExtension() const
	{
		idStr temp;
		serverRelativeUrl.ExtractFileExtension(temp);
		temp.ToLower();

		return temp;
	}
};
typedef boost::shared_ptr<MissionScreenshot> MissionScreenshotPtr;

struct DownloadableMission
{
	int		id;				// ID of the mission in the online database
	idStr	modName;		// usually the name of the pk4 (e.g. "heart")
	idStr	title;			// the title or display name ("Heart of Lone Salvation")
	idStr	author;			// author/s
	float	sizeMB;			// size in MB
	idStr	releaseDate;	// date of release in ISO format (2010-12-30)
	idStr	language;		// the language ("english")
	int		version;		// version number, initial release carries version 1
	bool	isUpdate;		// whether this mission is an update of one already installed

	// The list of HTTP download URLs
	idStringList downloadLocations;

	// Begin Initially empty variables, need to be filled per request by the mission manager

	// This is false by default, is set to true once the mission manager has filled in the details
	bool	detailsLoaded;

	// A single- or multi-line text, as defined in the online FM database
	idStr	description;

	// A list of screenshots as downloaded from the mission database
	idList<MissionScreenshotPtr> screenshots;

	// End Initially empty variables

	// Default constructor
	DownloadableMission() :
		id(-1), // invalid ID
		version(1),
		isUpdate(false),
		detailsLoaded(false)
	{}

	// Static sort compare functor, sorting by mission title
	typedef DownloadableMission* DownloadableMissionPtr;

	static int SortCompareTitle(const DownloadableMissionPtr* a, const DownloadableMissionPtr* b)
	{
		return idStr::Cmp((*a)->title, (*b)->title);
	}

	// Gets the local path to the screenshot image (relative to darkmod path, e.g. fms/_missionshots/preview_monst02.jpg)
	idStr GetLocalScreenshotPath(int screenshotNum) const
	{
		assert(screenshotNum >= 0 && screenshotNum < screenshots.Num());

		return cv_tdm_fm_path.GetString() + 
			   idStr(TMP_MISSION_SCREENSHOT_FOLDER) + "/" + 
			   TMP_MISSION_SCREENSHOT_PREFIX +
			   screenshots[screenshotNum]->GetLocalFilename();
	}
};
// Use raw pointers in the DownloadableMissionList
// to allow the use of the qsort algorithm as used in idStr::Sort()
typedef idList<DownloadableMission*> DownloadableMissionList;

/**
 * greebo: A campaign defines a certain map sequence. In the simplest
 * setup the mapsequence has only one map file name in it, in the 
 * most complicated case each sequence step has a finite set of 
 * map files (the second mission in a campaign might consist of 
 * two actual map files the player is switching in between).
 */
struct MapSequenceElement
{
	// The list of names applicable for that sequence element
	idList<idStr> mapNames;
};
typedef idList<MapSequenceElement> MapSequence;

class CMissionManager
{
private:
	CMissionDBPtr _missionDB;

	// A plain list of available fs_game names
	idStringList _availableMissions;

	// A list of path => path associations for moving files around
	typedef std::list< std::pair<fs::path, fs::path> > MoveList;

	// The list of new mods
	idStringList _newFoundMissions;

	// The map file which should be loaded next (e.g. "patently_dangerous")
	idStr _curStartingMap;

	// The map sequence as defined in the map sequence file of campaigns
	// The first mission has index 0
	MapSequence _mapSequence;

	DownloadableMissionList _downloadableMissions;

	// The ID of the "Downloading mission list from server" message
	int _refreshMissionListDownloadId;

	// The ID of the "get mission details" request
	int _missionDetailsDownloadId;

	// The ID of the screenshot download
	int _missionScreenshotDownloadId;

public:
	enum InstallResult
	{
		INSTALLED_OK,
		INDEX_OUT_OF_BOUNDS,
		COPY_FAILURE,
	};

	// Status of various requests managed by this class (download mission list, 
	enum RequestStatus
	{
		NOT_IN_PROGRESS,
		IN_PROGRESS,
		FAILED,
		SUCCESSFUL,
	};

public:
	CMissionManager();

	~CMissionManager();

	// This initialises the list of available missions
	void Init();

	// Save/Restore data
	void Save(idSaveGame* savefile) const;
	void Restore(idRestoreGame* savefile);

	// Should be called when the game is shutting down
	void Shutdown();

	// Returns the number of available missions
	int GetNumMissions();

	// Returns the mission info by index (or NULL if out of bounds)
	CMissionInfoPtr GetMissionInfo(int index);

	// returns the mission info by name (always non-NULL)
	CMissionInfoPtr GetMissionInfo(const idStr& name);

	// Returns the info structure for the currently ongoing mod/mission (or NULL if none)
	CMissionInfoPtr GetCurrentMissionInfo();

	// Returns the name of the currently installed mod/mission
	idStr GetCurrentMissionName();

	// greebo: Returns the (file)name of the current mission (there might be multiple missions 
	// in a campaign, this method returns the one that should be loaded next).
	// Example: "patently_dangerous", no file extension, no maps/ prefix.
	const idStr& GetCurrentStartingMap() const;

	// Returns TRUE if the currently installed mod is a campaign
	bool CurrentModIsCampaign();

	void EraseModFolder(const idStr& name);

	// Called by MissionData when the player completed a mission
	void OnMissionComplete();

	// Called by gameLocal when the player start/loads a mission
	void OnMissionStart();

	// Clears the mission list and searches for mods to install, then calls GenerateMissionList()
	void ReloadMissionList();

	// The number of newly available missions
	int GetNumNewMissions();

	idStr GetNewFoundMissionsText();

	void ClearNewMissionList();

	// Reload darkmod.txt for newly downloaded/found missions, to update any outdated mission db entries
	void RefreshMetaDataForNewFoundMissions();

	// Installs mission (by index)
	InstallResult InstallMission(int index);

	// Installs mission (by fs_game name)
	InstallResult InstallMission(const idStr& name);

	// Uninstalls the currently installed FM, basically clearing our currentfm.txt
	void UninstallMission();

	// --------- Downloadable Mission List Request --------

	// Checks online for available missions, returns the download ID for progress checking
	int StartReloadDownloadableMissions();

	// Returns true if the mission list download is currently in progress,
	// call ProcessReloadDownloadableMissionsRequest() to process it
	bool IsDownloadableMissionsRequestInProgress();

	// Processes the pending mission list download request. Returns the download status
	// for reference (FAILED, SUCCESS, etc.)
	RequestStatus ProcessReloadDownloadableMissionsRequest();

	// -------- Mission Details Request ----------

	// Starts a new request to download details of the given mission number
	int StartDownloadingMissionDetails(int missionNum);

	// Returns true if the mission details download is currently in progress,
	// call ProcessReloadMissionDetailsRequest() to process it
	bool IsMissionDetailsRequestInProgress();

	// Processes the pending mission details download request. Returns the status
	// for reference (FAILED, SUCCESS, etc.)
	RequestStatus ProcessReloadMissionDetailsRequest();

	// -------- Screenshot Requests -----------
	bool IsMissionScreenshotRequestInProgress();
	int StartDownloadingMissionScreenshot(int missionIndex, int screenshotNum);
	CMissionManager::RequestStatus ProcessMissionScreenshotRequest();

	// Accessor to the downloadble mission list
	const DownloadableMissionList& GetDownloadableMissions() const;

	// Convenience method which copies a file from <source> to <dest>
	// If <overwrite> is set to TRUE, any existing destination file will be removed beforehand
	// Note: CopyFile is already #define'd in a stupid WinBase.h header file, hence DoCopyFile.
	static bool DoCopyFile(const fs::path& source, const fs::path& dest, bool overwrite = false);

	// Removes the given file, returns TRUE if this succeeded or if file wasn't there in the first place, FALSE on error
	static bool DoRemoveFile(const fs::path& fileToRemove);

	// Moves the given file, from <fromPath> to <toPath>
	static bool DoMoveFile(const fs::path& fromPath, const fs::path& toPath);

private:
	// Finds out which map is the starting map (must be called after InitCurrentMod)
	void InitStartingMap();

	// Attempts to read the map sequence file for the current mod
	void InitMapSequence();

	void SearchForNewMissions();

	// Sub-routine of SearchForNewMissions() investigating the FM folder
	// using the given extension (including dot ".pk4", ".zip")
	MoveList SearchForNewMissions(const idStr& extension);

	// Returns the path to the "darkmod" base
	fs::path GetDarkmodPath();

	// Finds all available missions
	void GenerateMissionList();

	// Sorts all missions by display name
	void SortMissionList();

	// Compare functor to sort mods by display name
	static int MissionSortCompare(const int* a, const int* b);

	// Loads the mission list from the given XML
	void LoadMissionListFromXml(const XmlDocumentPtr& doc);

	// Loads mission details from the given XML, storing the data in the mission with the given number
	void LoadMissionDetailsFromXml(const XmlDocumentPtr& doc, int missionNum);

	// Request status according to the pending download
	RequestStatus GetRequestStatusForDownloadId(int downloadId);

	// Sorts the mission list
	void SortDownloadableMissions();

	// Replaces stuff like &#13;
	idStr ReplaceXmlEntities(const idStr& input);

	bool ProcessMissionScreenshot(const fs::path& tempFilename, DownloadableMission& mission, int screenshotNum);
};
typedef boost::shared_ptr<CMissionManager> CMissionManagerPtr;

#endif /* _MISSION_MANAGER_H_ */
