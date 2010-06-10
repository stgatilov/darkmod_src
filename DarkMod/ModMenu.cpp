/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/
// Copyright (C) 2004 Id Software, Inc.
//

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include "ModMenu.h"
#include "../DarkMod/shop.h"
#include "../DarkMod/MissionData.h"
#include "../DarkMod/declxdata.h"
#include "boost/filesystem.hpp"
#include "../DarkMod/ZipLoader/ZipLoader.h"
#include "../DarkMod/Missions/MissionManager.h"

#include <string>
#ifdef _WINDOWS
#include <process.h>
#else
#include <limits.h>
#include <unistd.h>
#endif

CModMenu::CModMenu() :
	_modTop(0)
{}

void CModMenu::Init()
{
	InitStartingMap();
}

void CModMenu::Clear()
{
	_startingMap.Empty();
}

void CModMenu::Save(idSaveGame* savefile) const
{
	// Nothing to save yet
}

void CModMenu::Restore(idRestoreGame* savefile)
{
	// Nothing to restore yet
}

namespace fs = boost::filesystem;

// Handle mainmenu commands
void CModMenu::HandleCommands(const char *menuCommand, idUserInterface *gui)
{
	idStr cmd = menuCommand;

	if (cmd == "refreshMissionList")
	{
		gameLocal.m_MissionManager->ReloadMissionList();

		// Update the GUI state
		UpdateGUI(gui);

		int numNewMissions = gameLocal.m_MissionManager->GetNumNewMissions();

		if (numNewMissions > 0)
		{
			if (numNewMissions > 1)
			{
				gui->SetStateString("newFoundMissionsText", va("%d new missions are available", numNewMissions));
			}
			else
			{
				gui->SetStateString("newFoundMissionsText", va("A new mission is available", numNewMissions));
			}

			gui->SetStateString("newFoundMissionsList", gameLocal.m_MissionManager->GetNewFoundMissionsText());
			gui->HandleNamedEvent("OnNewMissionsFound");

			gameLocal.m_MissionManager->ClearNewMissionList();
		}
	}
	else if (cmd == "loadModNotes")
	{
		// Get selected mod
		int modIndex = gui->GetStateInt("modSelected", "0") + _modTop;

		CMissionInfoPtr info = gameLocal.m_MissionManager->GetMissionInfo(modIndex);

		// Load the readme.txt contents, if available
		gui->SetStateString("ModNotesText", info != NULL ? info->GetMissionNotes() : "");
	}
	else if (cmd == "onMissionSelected")
	{
		UpdateSelectedMod(gui);
	}
	else if (cmd == "eraseSelectedModFromDisk")
	{
		int modIndex = gui->GetStateInt("modSelected", "0") + _modTop;

		CMissionInfoPtr info = gameLocal.m_MissionManager->GetMissionInfo(modIndex);

		if (info != NULL)
		{
			gameLocal.m_MissionManager->EraseModFolder(info->modName);
		}

		gui->HandleNamedEvent("OnSelectedMissionErasedFromDisk");

		// Update the selected mission
		UpdateSelectedMod(gui);
	}
	else if (cmd == "update")
	{
		gameLocal.Error("Deprecated update method called by main menu.");
	}
	else if (cmd == "modsNextPage")
	{
		// Scroll down a page
		_modTop += gui->GetStateInt("modsPerPage", "10");

		if (_modTop > gameLocal.m_MissionManager->GetNumMissions())
		{
			_modTop = 0;
		}

		UpdateGUI(gui);
	}
	else if (cmd == "modsPrevPage")
	{
		// Scroll up a page
		_modTop -= gui->GetStateInt("modsPerPage", "10");

		if (_modTop < 0)
		{
			_modTop = 0;
		}

		UpdateGUI(gui);
	}
	else if (cmd == "darkmodLoad")
	{
		// Get selected mod
		int modIndex = gui->GetStateInt("modSelected", "0") + _modTop;

		InstallMod(modIndex, gui);
	}
	else if (cmd == "darkmodRestart")
	{
		// Get selected mod
		RestartGame();
	}
	else if (cmd == "briefing_show")
	{
		// Display the briefing text
		_briefingPage = 1;
		DisplayBriefingPage(gui);
	}
	else if (cmd == "briefing_scroll_down_request")
	{
		// Display the next page of briefing text
		_briefingPage++;
		DisplayBriefingPage(gui);
	}
	else if (cmd == "briefing_scroll_up_request")
	{
		// Display the previous page of briefing text
		_briefingPage--;
		DisplayBriefingPage(gui);
	}
	else if (cmd == "uninstallMod")
	{
		UninstallMod(gui);
	}
}

void CModMenu::UpdateSelectedMod(idUserInterface* gui)
{
	// Get selected mod
	int modIndex = gui->GetStateInt("modSelected", "0") + _modTop;

	CMissionInfoPtr info = gameLocal.m_MissionManager->GetMissionInfo(modIndex);

	if (info != NULL)
	{
		bool missionIsCurrentlyInstalled = gameLocal.m_MissionManager->GetCurrentMissionName() == info->modName;
		
		// Don't display the install button if the mod is already installed
		gui->SetStateBool("installModButtonVisible", !missionIsCurrentlyInstalled);
		gui->SetStateBool("hasModNoteButton", info->HasMissionNotes());

		// Set the mod size info
		std::size_t missionSize = info->GetMissionFolderSize();
		idStr missionSizeStr = info->GetMissionFolderSizeString();
		gui->SetStateString("selectedModSize", missionSize > 0 ? missionSizeStr : "-");

		gui->SetStateBool("eraseSelectedModButtonVisible", missionSize > 0 && !missionIsCurrentlyInstalled);

		idStr eraseMissionText = va("You're about to delete the contents of the mission folder from your disk, including savegames and screenshots:"
			"\n\n%s\n\nNote that the downloaded mission PK4 in your darkmod/fms/ folder will not be "
			"affected by this operation, you're still able to re-install the mission.", info->GetMissionFolderPath().c_str());
		gui->SetStateString("eraseMissionText", eraseMissionText);

		gui->SetStateString("selectedModCompleted", info->GetMissionCompletedString());
		gui->SetStateString("selectedModLastPlayDate", info->GetKeyValue("last_play_date", "-"));
	}
	else
	{
		gui->SetStateBool("installModButtonVisible", false);
		gui->SetStateString("selectedModSize", "0 Bytes");
		gui->SetStateBool("eraseSelectedModButtonVisible", false);
		gui->SetStateBool("hasModNoteButton", false);
	}
}

// Displays the current page of briefing text
void CModMenu::DisplayBriefingPage(idUserInterface *gui)
{
	// look up the briefing xdata, which is in "maps/<map name>/mission_briefing"
	idStr briefingData = idStr("maps/") + cv_tdm_mapName.GetString() + "/mission_briefing";

	gameLocal.Printf("DisplayBriefingPage: briefingData is %s\n", briefingData.c_str());

	// Load the XData declaration
	const tdmDeclXData* xd = static_cast<const tdmDeclXData*>(
		declManager->FindType(DECL_XDATA, briefingData, false)
	);

	const char* briefing = "";
	bool scrollDown = false;
	bool scrollUp = false;

	if (xd != NULL)
	{
		gameLocal.Printf("DisplayBriefingPage: xdata found.\n");

		// get page count from xdata
		int numPages = xd->m_data.GetInt("num_pages");

		gameLocal.Printf("DisplayBriefingPage: numPages is %d\n", numPages);

		// ensure current page is between 1 and page count, inclusive
		_briefingPage = idMath::ClampInt(1, numPages, _briefingPage);

		// load up page text
		idStr page = va("page%d_body", _briefingPage);

		gameLocal.Printf("DisplayBriefingPage: current page is %d", _briefingPage);

		briefing = xd->m_data.GetString(page);

		// set scroll button visibility
		scrollDown = numPages > _briefingPage;
		scrollUp = _briefingPage > 1;
	}
	else
	{
		gameLocal.Warning("DisplayBriefingPage: Could not find briefing xdata: %s", briefingData.c_str());
	}

	// update GUI
	gui->SetStateString("BriefingText", briefing);
	gui->SetStateBool("ScrollDownVisible", scrollDown);
	gui->SetStateBool("ScrollUpVisible", scrollUp);
}

void CModMenu::UpdateGUI(idUserInterface* gui)
{
	int modsPerPage = gui->GetStateInt("modsPerPage", "10");

	// Display the name of each FM
	for (int modIndex = 0; modIndex < modsPerPage; ++modIndex)
	{
		idStr guiName = va("mod%d_name", modIndex);
		idStr guiDesc = va("mod%d_desc", modIndex);
		idStr guiAuthor = va("mod%d_author", modIndex);
		idStr guiImage = va("mod%d_image", modIndex);
		idStr guiAvailable = va("modAvail%d", modIndex);
		idStr guiCompleted = va("modCompleted%d", modIndex);

		int available = 0;

		int missionIndex = _modTop + modIndex;
		int numMissions = gameLocal.m_MissionManager->GetNumMissions();

		CMissionInfoPtr info;

		// Retrieve the mission info
		if (_modTop + modIndex < numMissions)
		{
			info = gameLocal.m_MissionManager->GetMissionInfo(missionIndex);
		}

		gui->SetStateInt(guiAvailable,	info != NULL ? 1 : 0);
		gui->SetStateString(guiName,	info != NULL ? info->displayName : "");
		gui->SetStateString(guiDesc,	info != NULL ? info->description : "");
		gui->SetStateString(guiAuthor,	info != NULL ? info->author : "");
		gui->SetStateString(guiImage,	info != NULL ? info->image : "");
		gui->SetStateBool(guiCompleted,	info != NULL ? info->MissionCompleted() : false);
	}

	gui->SetStateBool("isModsScrollUpVisible", _modTop != 0);
	gui->SetStateBool("isModsScrollDownVisible", _modTop + modsPerPage < gameLocal.m_MissionManager->GetNumMissions());

	// Update the currently installed mod
	CMissionInfoPtr curModInfo = gameLocal.m_MissionManager->GetCurrentMissionInfo();

	gui->SetStateBool("hasCurrentMod", curModInfo != NULL);
	gui->SetStateString("currentModName", curModInfo != NULL ? curModInfo->displayName : "<No Mission Installed>");
	gui->SetStateString("currentModDesc", curModInfo != NULL ? curModInfo->description : "");	
}

void CModMenu::InitStartingMap()
{
	_startingMap.Empty();

	idStr curModName = gameLocal.m_MissionManager->GetCurrentMissionName();

	if (curModName.IsEmpty())
	{
		return;
	}

	// Find out which is the starting map of the current mod
	fs::path doomPath(fileSystem->RelativePathToOSPath("", "fs_savepath"));
	doomPath /= "..";

	fs::path startingMapPath(cv_tdm_fm_path.GetString());
	startingMapPath = startingMapPath / gameLocal.m_MissionManager->GetCurrentMissionName().c_str() / cv_tdm_fm_startingmap_file.GetString();

	char* buffer = NULL;

	if (fileSystem->ReadFile(startingMapPath.string().c_str(), reinterpret_cast<void**>(&buffer)) != -1)
	{
		// We have a startingmap
		_startingMap = buffer;
		fileSystem->FreeFile(reinterpret_cast<void*>(buffer));

		cv_tdm_mapName.SetString(_startingMap);
	}
	else
	{
		gameLocal.Warning("No '%s' file for the current mod: %s", startingMapPath.string().c_str(), gameLocal.m_MissionManager->GetCurrentMissionName().c_str());
	}
}

fs::path CModMenu::GetDarkmodPath()
{
	return fs::path(g_Global.GetDarkmodPath());
}

void CModMenu::InstallMod(int modIndex, idUserInterface* gui)
{
	// Path to the parent directory
	fs::path parentPath(fileSystem->RelativePathToOSPath("", "fs_savepath"));
	parentPath = parentPath.remove_leaf().remove_leaf();

	// Sanity-check
	if (modIndex < 0 || modIndex >= gameLocal.m_MissionManager->GetNumMissions())
	{
		return;
	}

	CMissionInfoPtr info = gameLocal.m_MissionManager->GetMissionInfo(modIndex);
	const idStr& modDirName = info->modName;

	// Check the required TDM version of this FM
	if (info->requiredMajor > TDM_VERSION_MAJOR || info->requiredMinor > TDM_VERSION_MINOR)
	{
		gui->SetStateString("requiredVersionCheckFailText", 
			va("Cannot install this mission, as it requires\n%s v%d.%02d.\n\nYou are running %s v%d.%02d. Please run the tdm_update application to update your installation.",
			GAME_VERSION, info->requiredMajor, info->requiredMinor, GAME_VERSION, TDM_VERSION_MAJOR, TDM_VERSION_MINOR));
		gui->HandleNamedEvent("OnRequiredVersionCheckFail");
		return;
	}

	// Issue the named command to the GUI
	gui->SetStateString("modInstallProgressText", "Installing Mission Package\n\n" + info->displayName);
	gui->HandleNamedEvent("OnModInstallationStart");

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
		CMissionManager::DoCopyFile(pk4fileOsPath, targetFile, true);
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
		CMissionManager::DoCopyFile(darkmodPath / "DoomConfig.cfg", doomConfigPath, true);
	}
	else
	{
		// No, don't sync DoomConfig.cfg, but at least copy a basic one over there if it doesn't exist
		if (!fs::exists(doomConfigPath))
		{
			DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("DoomConfig.cfg not found in FM folder, copy over from darkmod.\r");

			CMissionManager::DoCopyFile(darkmodPath / "DoomConfig.cfg", doomConfigPath);
		}
	}

	// Check if the config.spec file already exists in the mod folder
	fs::path configSpecPath = targetFolder / "config.spec";
	if (!fs::exists(configSpecPath))
	{
		// Copy the config.spec file from darkmod/ to the new mod/
		CMissionManager::DoCopyFile(darkmodPath / "config.spec", configSpecPath);
	}

	gui->HandleNamedEvent("OnModInstallationFinished");
}

void CModMenu::UninstallMod(idUserInterface* gui)
{
	// To uninstall the current FM, just clear the FM name in currentfm.txt	

	// Path to the darkmod directory
	fs::path darkmodPath = GetDarkmodPath();

	// Path to file that holds the current FM name
	fs::path currentFMPath(darkmodPath / cv_tdm_fm_current_file.GetString());

	DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Trying to clear current FM name in %s\r", currentFMPath.file_string().c_str());

	if (CMissionManager::DoRemoveFile(currentFMPath))
	{
		DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Current FM file removed: %s.\r", currentFMPath.string().c_str());
	}
	else
	{
		// Log removal error
		DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Caught exception while removing current FM file %s.\r", currentFMPath.string().c_str());
	}

	gui->HandleNamedEvent("OnModUninstallFinished");
}

void CModMenu::RestartGame()
{
	// Path to the parent directory
	fs::path parentPath(fileSystem->RelativePathToOSPath("", "fs_savepath"));
	parentPath = parentPath.remove_leaf().remove_leaf();

	// Path to the darkmod directory
	fs::path darkmodPath = GetDarkmodPath();

	// path to tdmlauncher
#ifdef _WINDOWS
	fs::path launcherExe(darkmodPath / "tdmlauncher.exe");
#else
	fs::path launcherExe(darkmodPath / "tdmlauncher.linux");
#endif

	if (!fs::exists(launcherExe))
	{
		gameLocal.Error("Could not find tdmlauncher!");
		return;
	}

	fs::path enginePath;

#ifdef _WINDOWS
	// Get the command line of the current process
	idStr cmdLine = GetCommandLine();

	int d3Pos = cmdLine.Find("DOOM3.exe", false);
	cmdLine = cmdLine.Mid(0, d3Pos + 9);
	cmdLine.StripLeadingOnce("\"");
	cmdLine.StripLeading(" ");
	cmdLine.StripLeading("\t");

	enginePath = cmdLine.c_str();
#else
	// TDM launcher needs to know where the engine is located, pass this as first argument
	char exepath[PATH_MAX] = {0};
	readlink("/proc/self/exe", exepath, sizeof(exepath));

	enginePath = fs::path(exepath);
#endif

	// command line to spawn tdmlauncher
	idStr commandLine(launcherExe.file_string().c_str());

	idStr engineArgument(enginePath.string().c_str());

	// greebo: Optional delay between restarts to fix sound system release issues in some Linux systems
	idStr additionalDelay = "";
	if (cv_tdm_fm_restart_delay.GetInteger() > 0)
	{
		additionalDelay = " --delay=";
		additionalDelay += cv_tdm_fm_restart_delay.GetString();
	}

#ifdef _WINDOWS
	// Create a tdmlauncher process, setting the working directory to the doom directory
	STARTUPINFO siStartupInfo;
	PROCESS_INFORMATION piProcessInfo;

	memset(&siStartupInfo, 0, sizeof(siStartupInfo));
	memset(&piProcessInfo, 0, sizeof(piProcessInfo));

	siStartupInfo.cb = sizeof(siStartupInfo);

	CreateProcess(NULL, (LPSTR) (commandLine + " " + engineArgument + additionalDelay).c_str(), NULL, NULL,  false, 0, NULL,
		parentPath.file_string().c_str(), &siStartupInfo, &piProcessInfo);
#else
	// start tdmlauncher
	DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Starting tdmlauncher %s with argument %s\r", commandLine.c_str(), engineArgument.c_str());
	if (execlp(commandLine.c_str(), commandLine.c_str(), engineArgument.c_str(), "pause", NULL) == -1)
	{
		int errnum = errno;
		gameLocal.Error("execlp failed with error code %d: %s", errnum, strerror(errnum));
	}

	_exit(EXIT_FAILURE);
#endif

	// Issue the quit command to the game
	cmdSystem->BufferCommandText( CMD_EXEC_NOW, "quit" );
}
