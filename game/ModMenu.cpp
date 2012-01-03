/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod (http://www.thedarkmod.com/)
 
 $Revision$ (Revision of last commit) 
 $Date$ (Date of last commit)
 $Author$ (Author of last commit)
 
******************************************************************************/
#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include <string>
#include <boost/filesystem.hpp>

#include "ModMenu.h"
#include "Shop/Shop.h"
#include "Objectives/MissionData.h"
#include "declxdata.h"
#include "ZipLoader/ZipLoader.h"
#include "Missions/MissionManager.h"

#ifdef _WINDOWS
#include <process.h>
#else
#include <limits.h>
#include <unistd.h>
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

			gui->SetStateString("newFoundMissionsText", gameLocal.m_I18N->Translate( "#str_02143" ) ); // New missions available
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
		gui->SetStateString("modInstallProgressText", gameLocal.m_I18N->Translate( "#str_02504" ) + info->displayName); // "Installing Mission Package\n\n"
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
		RestartGame(false); // false == reloadEngine
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
	else if (cmd == "startSelect") // grayman #2933 - save mission start position
	{
		gameLocal.m_StartPosition = gui->GetStateString("startSelect", "");
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
		
		// 07208: "You're about to delete the contents of the mission folder from your disk, including savegames and screenshots:"
		// 07209: "Note that the downloaded mission PK4 in your darkmod/fms/ folder will not be affected by this operation, you're still able to re-install the mission."
		idStr eraseMissionText = va( idStr( gameLocal.m_I18N->Translate( "#str_07208" ) ) + "\n\n%s\n\n" +
					     gameLocal.m_I18N->Translate( "#str_07209" ), info->GetModFolderPath().c_str() );
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

		gameLocal.Printf("DisplayBriefingPage: current page is %d\n", _briefingPage);

		// Tels: Translate it properly
		briefing = gameLocal.m_I18N->Translate( xd->m_data.GetString(page) );

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
		idStr name = gameLocal.m_I18N->Translate( info != NULL ? info->displayName : "");
		gameLocal.m_I18N->MoveArticlesToBack( name );
		gui->SetStateString(guiName,	name );
		gui->SetStateString(guiDesc,	gameLocal.m_I18N->Translate( info != NULL ? info->description : "") );
		gui->SetStateString(guiAuthor,	info != NULL ? info->author : "");
		gui->SetStateString(guiImage,	info != NULL ? info->image : "");
		gui->SetStateBool(guiCompleted,	info != NULL ? info->ModCompleted() : false);
	}

	gui->SetStateBool("isModsScrollUpVisible", _modTop != 0);
	gui->SetStateBool("isModsScrollDownVisible", _modTop + modsPerPage < gameLocal.m_MissionManager->GetNumMods());

	// Update the currently installed mod
	CModInfoPtr curModInfo = gameLocal.m_MissionManager->GetCurrentModInfo();

	gui->SetStateBool("hasCurrentMod", curModInfo != NULL);
	gui->SetStateString("currentModName", gameLocal.m_I18N->Translate( curModInfo != NULL ? curModInfo->displayName : "#str_02189" )); // <No Mission Installed>
	gui->SetStateString("currentModDesc", gameLocal.m_I18N->Translate( curModInfo != NULL ? curModInfo->description : "" ));	
}

bool CModMenu::PerformVersionCheck(const CModInfoPtr& mission, idUserInterface* gui)
{
	// Check the required TDM version of this FM
	if (CompareVersion(TDM_VERSION_MAJOR, TDM_VERSION_MINOR, mission->requiredMajor, mission->requiredMinor) == OLDER)
	{
		gui->SetStateString("requiredVersionCheckFailText", 
			// "Cannot install this mission, as it requires\n%s v%d.%02d.\n\nYou are running v%d.%02d. Please run the tdm_update application to update your installation.",
			va( gameLocal.m_I18N->Translate( "#str_07210" ),
			GAME_VERSION, mission->requiredMajor, mission->requiredMinor, TDM_VERSION_MAJOR, TDM_VERSION_MINOR));

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

void CModMenu::RestartGame(bool restartProcess)
{
	if (restartProcess)
	{
		// Path to the darkmod directory
		fs::path darkmodPath = g_Global.GetDarkmodPath();

		// Path to the game executable
		fs::path enginePath = g_Global.GetEnginePath();

		DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Engine Path: %s\r", enginePath.file_string().c_str());

		// path to tdmlauncher
		fs::path launcherExe;

#ifdef _WINDOWS
		launcherExe = darkmodPath / "tdmlauncher.exe";
#elif __linux__
		launcherExe = darkmodPath / "tdmlauncher.linux";
#elif MACOS_X
		launcherExe = darkmodPath / "tdmlauncher.macosx";

		if (!fs::exists(launcherExe))
		{
			// Did not find tdmlauncher.macosx at the save path (~/Library/Application Support/Doom 3/darkmod)
			// try to find it next to the engine path
			DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Did not find tdmlauncher.macosx at the save path %s\r", launcherExe.file_string().c_str());
		
			launcherExe = enginePath;

			// remove executable
			enginePath.remove_filename();

			launcherExe /= "../../../darkmod/tdmlauncher.macosx";

			DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Trying to find tdmlauncher.macosx at %s\r", launcherExe.file_string().c_str());
		}

#else
	#error 'Unsupported platform.'
#endif

		if (!fs::exists(launcherExe))
		{
			DM_LOG(LC_MAINMENU, LT_ERROR)LOGSTRING("Could not find TDM Launcher at %s\r", launcherExe.file_string().c_str());

			gameLocal.Error("Could not find tdmlauncher!");
			return;
		}

		DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("TDM Launcher Path: %s\r", launcherExe.file_string().c_str());

		// command line to spawn tdmlauncher
		idStr commandLine(launcherExe.file_string().c_str());

		idStr engineArgument(enginePath.string().c_str());

		// greebo: Optional delay between restarts to fix sound system release issues in some Linux systems
		idStr additionalDelay = "";
		int restartDelay = cv_tdm_fm_restart_delay.GetInteger();
#ifndef _WINDOWS
		// always use at least 100ms on linux/macos, or the old process might still run while the 
		// new process is already starting up:
		restartDelay += 100;
#endif

		if (restartDelay > 0)
		{
			additionalDelay = va(" --delay=%i", restartDelay);
		}

#ifdef _WINDOWS
		// Create a tdmlauncher process, setting the working directory to the doom directory
		STARTUPINFO siStartupInfo;
		PROCESS_INFORMATION piProcessInfo;

		memset(&siStartupInfo, 0, sizeof(siStartupInfo));
		memset(&piProcessInfo, 0, sizeof(piProcessInfo));

		siStartupInfo.cb = sizeof(siStartupInfo);

		CreateProcess(NULL, (LPSTR) (commandLine + " " + engineArgument + additionalDelay).c_str(), NULL, NULL,  false, 0, NULL,
			enginePath.file_string().c_str(), &siStartupInfo, &piProcessInfo);
#else
		// start tdmlauncher
		DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Starting tdmlauncher %s with argument %s\r", commandLine.c_str(), engineArgument.c_str());
		if (execlp(commandLine.c_str(), commandLine.c_str(), engineArgument.c_str(), additionalDelay.c_str(), NULL) == -1)
		{
			int errnum = errno;
			gameLocal.Error("execlp failed with error code %d: %s", errnum, strerror(errnum));
			_exit(EXIT_FAILURE);
		}

#endif

		// Issue the quit command to the game
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, "quit" );
	}
	else
	{
		// We restart the game by issuing a restart engine command only, this activates any newly installed mod
		cmdSystem->SetupReloadEngine(idCmdArgs());
	}
}
