/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include <time.h>
#include "MissionManager.h"
#include "MissionDB.h"
#include "../ZipLoader/ZipLoader.h"
#include "../Inventory/Inventory.h"

#include "../DarkMod/Http/HttpConnection.h"
#include "../DarkMod/Http/HttpRequest.h"
#include "../pugixml/pugixml.hpp"

CMissionManager::CMissionManager() :
	_missionDB(new CMissionDB)
{}

void CMissionManager::Init()
{
	// (Re-)generate mission list on start
	ReloadMissionList();

	// greebo: Now that any new PK4 files have been copied/moved,
	// reload the mission database.
	_missionDB->Init();

	InitStartingMap();
}

void CMissionManager::Shutdown()
{
	_missionDB->Save();
}

// Returns the number of available missions
int CMissionManager::GetNumMissions()
{
	return _availableMissions.Num();
}

CMissionInfoPtr CMissionManager::GetMissionInfo(int index)
{
	if (index < 0 || index >= _availableMissions.Num())
	{
		return CMissionInfoPtr(); // out of bounds
	}

	// Pass the call to the getbyname method
	return GetMissionInfo(_availableMissions[index]);
}

CMissionInfoPtr CMissionManager::GetMissionInfo(const idStr& name)
{
	return _missionDB->GetMissionInfo(name);
}

void CMissionManager::EraseModFolder(const idStr& name)
{
	CMissionInfoPtr info = GetMissionInfo(name);

	if (info == NULL)
	{
		DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Cannot erase mission folder for mod %s, mission info not found\r", name.c_str());
		return;
	}

	// Delete folder contents
	fs::path missionPath = info->GetMissionFolderPath().c_str();

	if (fs::exists(missionPath))
	{
		fs::remove_all(missionPath);
	}
	else
	{
		DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Cannot erase mission folder for mod %s, mission folder not found\r", missionPath.file_string().c_str());
		return;
	}

	info->ClearMissionFolderSize();
}

void CMissionManager::OnMissionStart()
{
	CMissionInfoPtr info = GetCurrentMissionInfo();

	if (info == NULL)
	{
		DM_LOG(LC_MAINMENU, LT_ERROR)LOGSTRING("Could not find mission info for current mod.\r");
		return;
	}

	time_t seconds;
	tm* timeInfo;

	seconds = time(NULL);
	timeInfo = localtime(&seconds);

	// Mark the current difficulty level as completed
	info->SetKeyValue("last_play_date", va("%d-%02d-%02d", timeInfo->tm_year + 1900, timeInfo->tm_mon + 1, timeInfo->tm_mday));
}

void CMissionManager::OnMissionComplete()
{
	CMissionInfoPtr info = GetCurrentMissionInfo();

	if (info == NULL)
	{
		DM_LOG(LC_MAINMENU, LT_ERROR)LOGSTRING("Could not find mission info for current mod.\r");
		return;
	}

	// Mark the current difficulty level as completed
	info->SetKeyValue(va("mission_completed_%d", gameLocal.m_DifficultyManager.GetDifficultyLevel()), "1");

	idPlayer* player = gameLocal.GetLocalPlayer();

	if (player != NULL)
	{
		int gold, jewelry, goods;
		int total = player->Inventory()->GetLoot(gold, jewelry, goods);

		info->SetKeyValue(va("mission_loot_collected_%d", gameLocal.m_DifficultyManager.GetDifficultyLevel()), idStr(total));
	}
}

CMissionInfoPtr CMissionManager::GetCurrentMissionInfo()
{
	idStr gameBase = cvarSystem->GetCVarString("fs_game_base");

	// We only have a mod if game_base is set correctly, otherwise we're in "darkmod".
	idStr curMission = (!gameBase.IsEmpty()) ? cvarSystem->GetCVarString("fs_game") : "";

	if (curMission.IsEmpty() || curMission == "darkmod") 
	{
		// return NULL when no mission is installed or "darkmod"
		return CMissionInfoPtr();
	}

	return GetMissionInfo(curMission);
}

idStr CMissionManager::GetCurrentMissionName()
{
	CMissionInfoPtr info = GetCurrentMissionInfo();

	return (info != NULL) ? info->modName : "";
}

int CMissionManager::GetNumNewMissions()
{
	return _newFoundMissions.Num();
}

idStr CMissionManager::GetNewFoundMissionsText()
{
	if (_newFoundMissions.Num() == 0)
	{
		return "";
	}

	idStr text;

	for (int i = 0; i < _newFoundMissions.Num(); ++i)
	{
		CMissionInfoPtr info = GetMissionInfo(_newFoundMissions[i]);

		if (info == NULL) continue;

		text += (text.IsEmpty()) ? "" : "\n";
		text += info->displayName;
	}

	return text;
}

void CMissionManager::ClearNewMissionList()
{
	_newFoundMissions.Clear();
}

void CMissionManager::SearchForNewMissions()
{
	// List all PK4s in the fms/ directory
	MoveList moveList = SearchForNewMissions(".pk4");
	MoveList zipMoveList = SearchForNewMissions(".zip");

	// Merge the zips into the pk4 list
	moveList.merge(zipMoveList);

	DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Found %d new mission packages.\r", static_cast<int>(moveList.size()));
	gameLocal.Printf("Found %d new mission packages.\n", static_cast<int>(moveList.size()));

	// greebo: The D3 engine should no longer hold locks on those files
	// and we can start moving them into their respective locations
	for (MoveList::const_iterator i = moveList.begin(); i != moveList.end(); ++i)
	{
		fs::path targetPath = i->second;

		// Remove any target file first, to overwrite when moving
		DoRemoveFile(targetPath);

		// Move the file
		DoMoveFile(i->first, targetPath);
		
		// Remove the file portion
		targetPath.remove_leaf();

		// Remove any darkmod.txt, splashimage etc. when copying a new PK4. It may contain updated versions of those.
		DoRemoveFile(targetPath / cv_tdm_fm_desc_file.GetString());
		DoRemoveFile(targetPath / cv_tdm_fm_splashimage_file.GetString());
		DoRemoveFile(targetPath / cv_tdm_fm_notes_file.GetString());
	}
}

CMissionManager::MoveList CMissionManager::SearchForNewMissions(const idStr& extension)
{
	idStr fmPath = cv_tdm_fm_path.GetString();
	idFileList* pk4files = fileSystem->ListFiles(fmPath, extension, false, true);

	MoveList moveList;

	fs::path darkmodPath = GetDarkmodPath();

	// Iterate over all found PK4s and check if they're valid
	for (int i = 0; i < pk4files->GetNumFiles(); ++i)
	{
		fs::path pk4path = darkmodPath / pk4files->GetFile(i);

		DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Found %s in FM root folder: %s\r", extension.c_str(), pk4path.file_string().c_str());

		// Does the PK4 file contain a proper description file?
		CZipFilePtr pk4file = CZipLoader::Instance().OpenFile(pk4path.file_string().c_str());

		if (pk4file == NULL)
		{
			DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Could not open PK4 in root folder: %s\r", pk4path.file_string().c_str());
			continue; // failed to open zip file
		}

		if (!pk4file->ContainsFile(cv_tdm_fm_desc_file.GetString()))
		{
			DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Ignoring PK4 file, no 'darkmod.txt' found inside archive: %s\r", pk4path.file_string().c_str());
			continue; // no darkmod.txt
		}

		// Deduce the mod folder name based on the PK4 name
		idStr modName = pk4path.leaf().c_str();
		modName.StripPath();
		modName.StripFileExtension();
		modName.ToLower();

		if (modName.IsEmpty()) continue; // error?

		// Remember this for the user to display
		_newFoundMissions.Append(modName);

		// Clean modName string from any weird characters
		for (int i = 0; i < modName.Length(); ++i)
		{
			if (idStr::CharIsAlpha(modName[i]) || idStr::CharIsNumeric(modName[i])) continue;

			modName[i] = '_'; // replace non-ASCII keys with underscores
		}

		// Assemble the mod folder, e.g. c:/games/doom3/darkmod/fms/outpost
		fs::path modFolder = darkmodPath / cv_tdm_fm_path.GetString() / modName.c_str();

		// Create the fm folder, if necessary
		if (!fs::exists(modFolder))
		{
			DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Mod folder doesn't exist for PK4, creating: %s\r", modFolder.file_string().c_str());
			try
			{
				fs::create_directory(modFolder);
			}
			catch (fs::basic_filesystem_error<fs::path>& e)
			{
				DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Exception while creating folder for PK4: %s\r", e.what());
			}
		}

		// Move the PK4 to that folder
		fs::path targetPath = modFolder / (modName + ".pk4").c_str();

		// Remember to move this file as soon as we're done here
		moveList.push_back(MoveList::value_type(pk4path, targetPath));
	}

	fileSystem->FreeFileList(pk4files);

	return moveList;
}

fs::path CMissionManager::GetDarkmodPath()
{
	return fs::path(g_Global.GetDarkmodPath());
}

void CMissionManager::ReloadMissionList()
{
	// Search for new mods (PK4s to be moved, etc.)
	SearchForNewMissions();

	// Build the mission list again
	GenerateMissionList();
}

void CMissionManager::GenerateMissionList()
{
	// Clear the list first
	_availableMissions.Clear();

	// List all folders in the fms/ directory
	idStr fmPath = cv_tdm_fm_path.GetString();
	idFileList* fmDirectories = fileSystem->ListFiles(fmPath, "/", false);

	for (int i = 0; i < fmDirectories->GetNumFiles(); ++i)
	{
		// The name of the FM directory below fms/
		idStr fmDir = fmDirectories->GetFile(i);

		DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Looking for %s file in %s.\r", cv_tdm_fm_desc_file.GetString(), (fmPath + fmDir).c_str());

		// Check for an uncompressed darkmod.txt file
		idStr descFileName = fmPath + fmDir + "/" + cv_tdm_fm_desc_file.GetString();

		if (fileSystem->ReadFile(descFileName, NULL) != -1)
		{
			// File exists, add this as available mod
			_availableMissions.Alloc() = fmDir;
			continue;
		}

		// no "darkmod.txt" file found, check in the PK4 files
		DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("%s file not found, looking for PK4s.\r", descFileName.c_str());

		// Check for PK4s in that folder (and all subdirectories)
		idFileList* pk4files = fileSystem->ListFilesTree(fmPath + fmDir, ".pk4", false);

		DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("%d PK4 files found in %s.\r", pk4files->GetNumFiles(), (fmPath + fmDir).c_str());

		for (int j = 0; j < pk4files->GetNumFiles(); ++j)
		{
			fs::path pk4path = GetDarkmodPath() / pk4files->GetFile(j);

			CZipFilePtr pk4file = CZipLoader::Instance().OpenFile(pk4path.file_string().c_str());

			if (pk4file == NULL)
			{
				DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Could not open PK4: %s\r", pk4path.file_string().c_str());
				continue; // failed to open zip file
			}

			if (pk4file->ContainsFile(cv_tdm_fm_desc_file.GetString()))
			{
				// Hurrah, we've found the darkmod.txt file, extract the contents
				// and attempt to save to folder
				_availableMissions.Alloc() = fmDir;

				fs::path darkmodPath = GetDarkmodPath();
				fs::path fmPath = darkmodPath / cv_tdm_fm_path.GetString() / fmDir.c_str();
				fs::path destPath = fmPath / cv_tdm_fm_desc_file.GetString();

				pk4file->ExtractFileTo(cv_tdm_fm_desc_file.GetString(), destPath.string().c_str());

				// Check for the other meta-files as well
				if (pk4file->ContainsFile(cv_tdm_fm_splashimage_file.GetString()))
				{
					destPath = fmPath / cv_tdm_fm_splashimage_file.GetString();
					pk4file->ExtractFileTo(cv_tdm_fm_splashimage_file.GetString(), destPath.string().c_str());
				}

				if (pk4file->ContainsFile(cv_tdm_fm_notes_file.GetString()))
				{
					destPath = fmPath / cv_tdm_fm_notes_file.GetString();
					pk4file->ExtractFileTo(cv_tdm_fm_notes_file.GetString(), destPath.string().c_str());
				}
			}
		}

		fileSystem->FreeFileList(pk4files);
	}

	fileSystem->FreeFileList(fmDirectories);

	gameLocal.Printf("Found %d mods in the FM folder.\n", _availableMissions.Num());
	DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Found %d mods in the FM folder.\n", _availableMissions.Num());

	// Sort the mod list alphabetically
	SortMissionList();
}

// Compare functor to sort missions by title
int CMissionManager::MissionSortCompare(const int* a, const int* b)
{
	// Get the mission titles (fs_game stuff)
	CMissionInfoPtr aInfo = gameLocal.m_MissionManager->GetMissionInfo(*a);
	CMissionInfoPtr bInfo = gameLocal.m_MissionManager->GetMissionInfo(*b);

	if (aInfo == NULL || bInfo == NULL) return 0;

	return aInfo->displayName.Icmp(bInfo->displayName);
}

void CMissionManager::SortMissionList()
{
	// greebo: idStrList has a specialised algorithm, preventing me
	// from using a custom sort algorithm, hence this ugly thing here
	idList<int> indexList;

	indexList.SetNum(_availableMissions.Num());
	for (int i = 0; i < _availableMissions.Num(); ++i)
	{
		indexList[i] = i;
	}

	indexList.Sort( CMissionManager::MissionSortCompare );

	idStrList temp = _availableMissions;

	for (int i = 0; i < indexList.Num(); ++i)
	{
		_availableMissions[i] = temp[indexList[i]];
	}
}

bool CMissionManager::DoCopyFile(const fs::path& source, const fs::path& dest, bool overwrite)
{
	if (overwrite)
	{
		try
		{
			// According to docs, remove() doesn't throw if file doesn't exist
			fs::remove(dest);
			DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Destination file %s already exists, has been removed before copying.\r", dest.string().c_str());
		}
		catch (fs::basic_filesystem_error<fs::path>& e)
		{
			// Don't care about removal error
			DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Caught exception while removing destination file %s: %s\r", dest.string().c_str(), e.what());
		}
	}

	// Copy the source file to the destination
	try
	{
		fs::copy_file(source, dest);
		DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("File successfully copied to %s.\r", dest.string().c_str());

		return true;
	}
	catch (fs::basic_filesystem_error<fs::path>& e)
	{
		DM_LOG(LC_MAINMENU, LT_ERROR)LOGSTRING("Exception while coyping file from %s to %s: %s\r", 
			source.string().c_str(), dest.string().c_str(), e.what());

		return false;
	}
}

bool CMissionManager::DoRemoveFile(const fs::path& fileToRemove)
{
	try
	{
		fs::remove(fileToRemove);
		DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Removed file in %s\r", fileToRemove.file_string().c_str());

		return true;
	}
	catch (fs::basic_filesystem_error<fs::path>& e)
	{
		DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Exception while removing file: %s\r", e.what());
		return false;
	}
}

bool CMissionManager::DoMoveFile(const fs::path& fromPath, const fs::path& toPath)
{
	try
	{
		fs::rename(fromPath, toPath);
		DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Moved %s to %s\r", fromPath.file_string().c_str(), toPath.file_string().c_str());

		return true;
	}
	catch (fs::basic_filesystem_error<fs::path>& e)
	{
		DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Exception while moving file: %s\r", e.what());

		return false;
	}
}

void CMissionManager::InitStartingMap()
{
	_curStartingMap.Empty();

	idStr curModName = GetCurrentMissionName();

	if (curModName.IsEmpty())
	{
		return;
	}

	// Find out which is the starting map of the current mod
	char* buffer = NULL;

	if (fileSystem->ReadFile(cv_tdm_fm_startingmap_file.GetString(), reinterpret_cast<void**>(&buffer)) != -1)
	{
		// We have a startingmap
		_curStartingMap = buffer;
		fileSystem->FreeFile(reinterpret_cast<void*>(buffer));

		cv_tdm_mapName.SetString(_curStartingMap);
	}
	else
	{
		gameLocal.Warning("No '%s' file for the current mod: %s", cv_tdm_fm_startingmap_file.GetString(), GetCurrentMissionName().c_str());
	}
}

CMissionManager::InstallResult CMissionManager::InstallMission(int index)
{
	if (index < 0 || index >= _availableMissions.Num())
	{
		gameLocal.Warning("Index out of bounds in MissionManager::InstallMission().");
		return INDEX_OUT_OF_BOUNDS; // out of bounds
	}

	// Pass the call to the getbyname method
	return InstallMission(_availableMissions[index]);
}

CMissionManager::InstallResult CMissionManager::InstallMission(const idStr& name)
{
	// Path to the parent directory
	fs::path parentPath(fileSystem->RelativePathToOSPath("", "fs_savepath"));
	parentPath = parentPath.remove_leaf().remove_leaf();

	CMissionInfoPtr info = GetMissionInfo(name); // result is always non-NULL

	const idStr& modDirName = info->modName;

	// Ensure that the target folder exists
	fs::path targetFolder = parentPath / modDirName.c_str();

	if (!fs::create_directory(targetFolder))
	{
		// Directory exists, not a problem, but log this
		DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("FM targetFolder already exists: %s\r", targetFolder.string().c_str());
	}

	// Path to the darkmod directory
	fs::path darkmodPath = GetDarkmodPath();

	// Copy all PK4s from the FM folder (and all subdirectories)
	idFileList* pk4Files = fileSystem->ListFilesTree(info->pathToFMPackage, ".pk4", false);

	for (int i = 0; i < pk4Files->GetNumFiles(); ++i)
	{
		// Source file (full OS path)
		fs::path pk4fileOsPath = GetDarkmodPath() / pk4Files->GetFile(i);

		// Target location
		fs::path targetFile = targetFolder / pk4fileOsPath.leaf();

		DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Copying file %s to %s\r", pk4fileOsPath.string().c_str(), targetFile.string().c_str());

		// Use boost::filesystem instead of id's (comments state that copying large files can be problematic)
		//fileeSystem->CopyFile(pk4fileOsPath, targetFile.string().c_str());

		// Copy the PK4 file and make sure any target file with the same name is removed beforehand
		if (!DoCopyFile(pk4fileOsPath, targetFile, true))
		{
			// Failed copying
			return COPY_FAILURE;
		}
	}

	fileSystem->FreeFileList(pk4Files);

	// Path to file that holds the current FM name
	fs::path currentFMPath(darkmodPath / cv_tdm_fm_current_file.GetString());

	DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Trying to save current FM name to %s\r", currentFMPath.file_string().c_str());

	// Save the name of the new mod
	FILE* currentFM = fopen(currentFMPath.file_string().c_str(), "w+");

	if (currentFM != NULL)
	{
		fputs(modDirName, currentFM);
		fclose(currentFM);
	}
	else
	{
		DM_LOG(LC_MAINMENU, LT_ERROR)LOGSTRING("Could not save current FM name to %s\r", currentFMPath.file_string().c_str());
	}

	DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Successfully saved current FM name to %s\r", currentFMPath.file_string().c_str());

	// Assemble the path to the FM's DoomConfig.cfg
	fs::path doomConfigPath = targetFolder / "DoomConfig.cfg";
	
	// Check if we should synchronise DoomConfig.cfg files
	if (cv_tdm_fm_sync_config_files.GetBool())
	{
		// Yes, sync DoomConfig.cfg

		// Always copy the DoomConfig.cfg from darkmod/ to the new mod/
		// Remove any DoomConfig.cfg that might exist there beforehand
		if (!DoCopyFile(darkmodPath / "DoomConfig.cfg", doomConfigPath, true))
		{
			// Failed copying
			return COPY_FAILURE;
		}
	}
	else
	{
		// No, don't sync DoomConfig.cfg, but at least copy a basic one over there if it doesn't exist
		if (!fs::exists(doomConfigPath))
		{
			DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("DoomConfig.cfg not found in FM folder, copy over from darkmod.\r");

			if (!DoCopyFile(darkmodPath / "DoomConfig.cfg", doomConfigPath))
			{
				// Failed copying
				return COPY_FAILURE;
			}
		}
	}

	// Check if the config.spec file already exists in the mod folder
	fs::path configSpecPath = targetFolder / "config.spec";
	if (!fs::exists(configSpecPath))
	{
		// Copy the config.spec file from darkmod/ to the new mod/
		if (!DoCopyFile(darkmodPath / "config.spec", configSpecPath))
		{
			// Failed copying
			return COPY_FAILURE;
		}
	}

	return INSTALLED_OK;
}

void CMissionManager::UninstallMission()
{
	// To uninstall the current FM, just clear the FM name in currentfm.txt	

	// Path to the darkmod directory
	fs::path darkmodPath = GetDarkmodPath();

	// Path to file that holds the current FM name
	fs::path currentFMPath(darkmodPath / cv_tdm_fm_current_file.GetString());

	DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Trying to clear current FM name in %s\r", currentFMPath.file_string().c_str());

	if (DoRemoveFile(currentFMPath))
	{
		DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Current FM file removed: %s.\r", currentFMPath.string().c_str());
	}
	else
	{
		// Log removal error
		DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Caught exception while removing current FM file %s.\r", currentFMPath.string().c_str());
	}
}

void CMissionManager::ReloadDownloadableMissions()
{
	_downloadableMissions.Clear();

	if (gameLocal.m_HttpConnection == NULL) return;

	// Form a new HTTP request
	CHttpRequestPtr req = gameLocal.m_HttpConnection->CreateRequest("http://www.mindplaces.com/darkmod/missiondb/get_available_missions.php");

	req->Perform();

	// Check Request Status
	if (req->GetStatus() != CHttpRequest::OK)
	{
		gameLocal.Printf("Connection Error.\n");

		GuiMessage msg;
		msg.title = "Unable to contact Mission Archive";
		msg.message = "Cannot connect to server.";
		msg.type = GuiMessage::MSG_OK;
		msg.okCmd = "close_msg_box";

		gameLocal.AddMainMenuMessage(msg);
		return;
	}

	XmlDocumentPtr doc = req->GetResultXml();

	/* Example XML Snippet
	
	<mission id="11" title="Living Expenses" releaseDate="2010-01-02" size="5.9" author="Sonosuke">
		<downloadLocation language="English" url="http://www.bloodgate.com/mirrors/tdm/pub/pk4/fms/living_expenses.pk4"/>
		<downloadLocation language="German" url="http://www.bloodgate.com/mirrors/tdm/pub/pk4/fms/living_expenses_de.pk4"/>
	</mission>
	
	*/

	pugi::xpath_node_set nodes = doc->select_nodes("//tdm/availableMissions//mission");

	for (pugi::xpath_node_set::const_iterator i = nodes.begin(); i != nodes.end(); ++i)	
	{
		pugi::xml_node node = i->node();

		DownloadableMission mission;

		mission.title = node.attribute("title").value();
		mission.sizeMB = node.attribute("size").as_float();
		mission.author = node.attribute("author").value();
		mission.releaseDate = node.attribute("releaseDate").value();
		mission.modName = node.attribute("internalName").value();
		mission.version = node.attribute("version").as_int();
		mission.isUpdate = false;

		bool missionExists = false;

		// Check if this mission is already downloaded
		for (int j = 0; j < _availableMissions.Num(); ++j)
		{
			if (idStr::Icmp(_availableMissions[j], mission.modName) == 0)
			{
				missionExists = true;
				break;
			}
		}

		if (missionExists)
		{
			// Check Mission version, there might be an update available
			if (_missionDB->MissionInfoExists(mission.modName))
			{
				CMissionInfoPtr missionInfo = _missionDB->GetMissionInfo(mission.modName);

				idStr versionStr = missionInfo->GetKeyValue("downloaded_version", "1");
				int existingVersion = atoi(versionStr.c_str());

				if (existingVersion >= mission.version)
				{
					continue; // Our version is up to date
				}
				else
				{
					mission.isUpdate = true;
				}
			}
		}

		pugi::xpath_node_set downloadLocations = node.select_nodes("downloadLocation");

		for (pugi::xpath_node_set::const_iterator loc = downloadLocations.begin(); loc != downloadLocations.end(); ++loc)	
		{
			pugi::xml_node locNode = loc->node();

			// Only accept English downloadlinks
			if (idStr::Icmp(locNode.attribute("language").value(), "english") != 0) continue;

			mission.downloadLocations.Append(locNode.attribute("url").value());
		}

		// Only add missions with valid locations
		if (mission.downloadLocations.Num() > 0)
		{
			_downloadableMissions.Append(mission);
		}
	}
}

const DownloadableMissionList& CMissionManager::GetDownloadableMissions() const
{
	return _downloadableMissions;
}
