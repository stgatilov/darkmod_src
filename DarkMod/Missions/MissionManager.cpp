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

CMissionManager::CMissionManager() :
	_missionDB(new CMissionDB)
{}

void CMissionManager::Init()
{
	_missionDB->Init();

	// (Re-)generate mission list on start
	ReloadMissionList();
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
		DoRemoveFile(targetPath / cv_tdm_fm_startingmap_file.GetString());
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
				if (pk4file->ContainsFile(cv_tdm_fm_startingmap_file.GetString()))
				{
					destPath = fmPath / cv_tdm_fm_startingmap_file.GetString();
					pk4file->ExtractFileTo(cv_tdm_fm_startingmap_file.GetString(), destPath.string().c_str());
				}

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
