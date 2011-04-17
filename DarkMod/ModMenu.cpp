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

#include <string>
#include <boost/filesystem.hpp>

#include "ModMenu.h"
#include "../DarkMod/Shop/Shop.h"
#include "../DarkMod/Objectives/MissionData.h"
#include "../DarkMod/declxdata.h"
#include "../DarkMod/ZipLoader/ZipLoader.h"
#include "../DarkMod/Missions/MissionManager.h"

#ifdef _WINDOWS
#include <process.h>
#else
#include <limits.h>
#include <unistd.h>
#endif

#ifdef MACOS_X
#include <mach-o/dyld.h>
#endif

CModMenu::CModMenu() :
	_modTop(0)
{}

namespace fs = boost::filesystem;

// Handle mainmenu commands
void CModMenu::HandleCommands(const idStr& cmd, idUserInterface* gui)
{
	if (cmd == "refreshMissionList")
	{
		int numNewMods = gameLocal.m_MissionManager->GetNumNewMods();

		if (numNewMods > 0)
		{
			// Update mission DB records
			gameLocal.m_MissionManager->RefreshMetaDataForNewFoundMods();

			gui->SetStateString("newFoundMissionsText", "New missions available");
			gui->SetStateString("newFoundMissionsList", gameLocal.m_MissionManager->GetNewFoundModsText());
			gui->HandleNamedEvent("OnNewMissionsFound");

			gameLocal.m_MissionManager->ClearNewModList();
		}

		gameLocal.m_MissionManager->ReloadModList();

		// Update the GUI state
		UpdateGUI(gui);
	}
	else if (cmd == "mainMenuStartup")
	{
		gui->SetStateBool("curModIsCampaign", gameLocal.m_MissionManager->CurrentModIsCampaign());
	}
	else if (cmd == "loadModNotes")
	{
		// Get selected mod
		int modIndex = gui->GetStateInt("modSelected", "0") + _modTop;

		CModInfoPtr info = gameLocal.m_MissionManager->GetModInfo(modIndex);

		// Load the readme.txt contents, if available
		gui->SetStateString("ModNotesText", info != NULL ? info->GetModNotes() : "");
	}
	else if (cmd == "onMissionSelected")
	{
		UpdateSelectedMod(gui);
	}
	else if (cmd == "eraseSelectedModFromDisk")
	{
		int modIndex = gui->GetStateInt("modSelected", "0") + _modTop;

		CModInfoPtr info = gameLocal.m_MissionManager->GetModInfo(modIndex);

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

		if (_modTop > gameLocal.m_MissionManager->GetNumMods())
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
	else if (cmd == "onClickInstallSelectedMission")
	{
		// Get selected mod
		int modIndex = gui->GetStateInt("modSelected", "0") + _modTop;

		CModInfoPtr info = gameLocal.m_MissionManager->GetModInfo(modIndex);

		if (info == NULL) return; // sanity check

		// Issue the named command to the GUI
		gui->SetStateString("modInstallProgressText", "Installing Mission Package\n\n" + info->displayName);
	}
	else if (cmd == "installSelectedMission")
	{
		// Get selected mod
		int modIndex = gui->GetStateInt("modSelected", "0") + _modTop;

		CModInfoPtr info = gameLocal.m_MissionManager->GetModInfo(modIndex);

		if (info == NULL) return; // sanity check

		if (!PerformVersionCheck(info, gui))
		{
			return; // version check failed
		}

		InstallMod(info, gui);
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

	CModInfoPtr info = gameLocal.m_MissionManager->GetModInfo(modIndex);

	if (info != NULL)
	{
		bool missionIsCurrentlyInstalled = gameLocal.m_MissionManager->GetCurrentModName() == info->modName;
		
		// Don't display the install button if the mod is already installed
		gui->SetStateBool("installModButtonVisible", !missionIsCurrentlyInstalled);
		gui->SetStateBool("hasModNoteButton", info->HasModNotes());

		// Set the mod size info
		std::size_t missionSize = info->GetModFolderSize();
		idStr missionSizeStr = info->GetModFolderSizeString();
		gui->SetStateString("selectedModSize", missionSize > 0 ? missionSizeStr : "-");

		gui->SetStateBool("eraseSelectedModButtonVisible", missionSize > 0 && !missionIsCurrentlyInstalled);

		idStr eraseMissionText = va("You're about to delete the contents of the mission folder from your disk, including savegames and screenshots:"
			"\n\n%s\n\nNote that the downloaded mission PK4 in your darkmod/fms/ folder will not be "
			"affected by this operation, you're still able to re-install the mission.", info->GetModFolderPath().c_str());
		gui->SetStateString("eraseMissionText", eraseMissionText);

		gui->SetStateString("selectedModCompleted", info->GetModCompletedString());
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
void CModMenu::DisplayBriefingPage(idUserInterface* gui)
{
	// look up the briefing xdata, which is in "maps/<map name>/mission_briefing"
	idStr briefingData = idStr("maps/") + gameLocal.m_MissionManager->GetCurrentStartingMap() + "/mission_briefing";

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
		int numMissions = gameLocal.m_MissionManager->GetNumMods();

		CModInfoPtr info;

		// Retrieve the mission info
		if (_modTop + modIndex < numMissions)
		{
			info = gameLocal.m_MissionManager->GetModInfo(missionIndex);
		}

		gui->SetStateInt(guiAvailable,	info != NULL ? 1 : 0);
		gui->SetStateString(guiName,	info != NULL ? info->displayName : "");
		gui->SetStateString(guiDesc,	info != NULL ? info->description : "");
		gui->SetStateString(guiAuthor,	info != NULL ? info->author : "");
		gui->SetStateString(guiImage,	info != NULL ? info->image : "");
		gui->SetStateBool(guiCompleted,	info != NULL ? info->ModCompleted() : false);
	}

	gui->SetStateBool("isModsScrollUpVisible", _modTop != 0);
	gui->SetStateBool("isModsScrollDownVisible", _modTop + modsPerPage < gameLocal.m_MissionManager->GetNumMods());

	// Update the currently installed mod
	CModInfoPtr curModInfo = gameLocal.m_MissionManager->GetCurrentModInfo();

	gui->SetStateBool("hasCurrentMod", curModInfo != NULL);
	gui->SetStateString("currentModName", curModInfo != NULL ? curModInfo->displayName : "<No Mission Installed>");
	gui->SetStateString("currentModDesc", curModInfo != NULL ? curModInfo->description : "");	
}

bool CModMenu::PerformVersionCheck(const CModInfoPtr& mission, idUserInterface* gui)
{
	// Check the required TDM version of this FM
	if (CompareVersion(TDM_VERSION_MAJOR, TDM_VERSION_MINOR, mission->requiredMajor, mission->requiredMinor) == OLDER)
	{
		gui->SetStateString("requiredVersionCheckFailText", 
			va("Cannot install this mission, as it requires\n%s v%d.%02d.\n\nYou are running %s v%d.%02d. Please run the tdm_update application to update your installation.",
			GAME_VERSION, mission->requiredMajor, mission->requiredMinor, GAME_VERSION, TDM_VERSION_MAJOR, TDM_VERSION_MINOR));

		gui->HandleNamedEvent("OnRequiredVersionCheckFail");

		return false;
	}

	return true; // version check passed
}

void CModMenu::InstallMod(const CModInfoPtr& mod, idUserInterface* gui)
{
	assert(mod != NULL);
	assert(gui != NULL);

	// Perform the installation
	CMissionManager::InstallResult result = gameLocal.m_MissionManager->InstallMod(mod->modName);

	if (result != CMissionManager::INSTALLED_OK)
	{
		idStr msg;

		switch (result)
		{
		case CMissionManager::COPY_FAILURE:
			msg = "Could not copy files. Maybe the target files are write protected or you're running out of disk space or lacking the required permissions?";
			break;
		default:
			msg = "No further explanation available. Well, this was kind of unexpected.";
		};

		// Feed error messages to GUI
		gui->SetStateString("modInstallationFailedText", msg);
		gui->HandleNamedEvent("OnModInstallationFailed");
	}
	else
	{
		gui->HandleNamedEvent("OnModInstallationFinished");
	}
}

void CModMenu::UninstallMod(idUserInterface* gui)
{
	gameLocal.m_MissionManager->UninstallMod();

	gui->HandleNamedEvent("OnModUninstallFinished");
}

void CModMenu::RestartGame()
{
	// Path to the parent directory
	fs::path parentPath(fileSystem->RelativePathToOSPath("", "fs_savepath"));
	parentPath = parentPath.remove_leaf().remove_leaf();

	// Path to the darkmod directory
	fs::path darkmodPath = g_Global.GetDarkmodPath();

	// path to tdmlauncher
#ifdef _WINDOWS
	fs::path launcherExe(darkmodPath / "tdmlauncher.exe");
#elif __linux__
	fs::path launcherExe(darkmodPath / "tdmlauncher.linux");
#elif MACOS_X
	fs::path launcherExe(darkmodPath / "tdmlauncher.macosx");
#else
#error 'Unsupported platform.'
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
#elif defined(__linux__)
	// TDM launcher needs to know where the engine is located, pass this as first argument
	char exepath[PATH_MAX] = {0};
	readlink("/proc/self/exe", exepath, sizeof(exepath));

	enginePath = fs::path(exepath);
#elif defined (MACOS_X)
	char exepath[4096] = {0};
	uint32_t size = sizeof(exepath);
	
	if (_NSGetExecutablePath(exepath, &size) != 0)
	{
		DM_LOG(LC_MAINMENU, LT_ERROR)LOGSTRING("Cannot read executable path, buffer too small\r");
	}
	
	enginePath = fs::path(exepath);
#else
#error Unsupported Platform
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
