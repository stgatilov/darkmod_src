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

#include "DownloadMenu.h"
#include "Missions/MissionManager.h"
#include "Missions/Download.h"
#include "Missions/DownloadManager.h"

namespace
{
	inline const char* GetPlural(int num, const char* singular, const char* plural)
	{
		return (num == 1) ? singular : plural;
	}
}

CDownloadMenu::CDownloadMenu() :
	_availListTop(0),
	_selectedListTop(0)
{}

void CDownloadMenu::HandleCommands(const idStr& cmd, idUserInterface* gui)
{
	if (cmd == "mainmenu_heartbeat")
	{
		// Update download progress
		if (_selectedMods.Num() > 0)
		{
			UpdateDownloadProgress(gui);
		}
		
		// Do we have a pending mission list request?
		if (gameLocal.m_MissionManager->IsDownloadableModsRequestInProgress())
		{
			CMissionManager::RequestStatus status = 
				gameLocal.m_MissionManager->ProcessReloadDownloadableModsRequest();
			
			switch (status)
			{
				case CMissionManager::FAILED:
				{
					gui->HandleNamedEvent("onAvailableMissionsRefreshed"); // hide progress dialog

					// Issue a failure message
					gameLocal.Printf("Connection Error.\n");

					GuiMessage msg;
					msg.title = "Unable to contact Mission Archive";
					msg.message = "Cannot connect to server.";
					msg.type = GuiMessage::MSG_OK;
					msg.okCmd = "close_msg_box";

					gameLocal.AddMainMenuMessage(msg);
				}
				break;

				case CMissionManager::SUCCESSFUL:
				{
					gui->HandleNamedEvent("onAvailableMissionsRefreshed"); // hide progress dialog

					UpdateGUI(gui);
					UpdateDownloadProgress(gui);
				}
				break;

				default: break;
			};
		}

		// Process pending details download request
		if (gameLocal.m_MissionManager->IsModDetailsRequestInProgress())
		{
			CMissionManager::RequestStatus status = 
				gameLocal.m_MissionManager->ProcessReloadModDetailsRequest();

			switch (status)
			{
				case CMissionManager::FAILED:
				{
					gui->HandleNamedEvent("onDownloadableMissionDetailsDownloadFailed"); // hide progress dialog

					// Issue a failure message
					gameLocal.Printf("Connection Error.\n");

					GuiMessage msg;
					msg.title = "Mission Details Download Failed";
					msg.message = "Failed to download the details XML file.";
					msg.type = GuiMessage::MSG_OK;
					msg.okCmd = "close_msg_box";

					gameLocal.AddMainMenuMessage(msg);
				}
				break;

				case CMissionManager::SUCCESSFUL:
				{
					gui->HandleNamedEvent("onDownloadableMissionDetailsLoaded"); // hide progress dialog

					UpdateModDetails(gui);
					UpdateScreenshotItemVisibility(gui);
				}
				break;

				default: break;
			};
		}

		// Process pending screenshot download request
		if (gameLocal.m_MissionManager->IsMissionScreenshotRequestInProgress())
		{
			CMissionManager::RequestStatus status = 
				gameLocal.m_MissionManager->ProcessMissionScreenshotRequest();

			switch (status)
			{
				case CMissionManager::FAILED:
				{
					gui->HandleNamedEvent("onFailedToDownloadScreenshot");

					// Issue a failure message
					gameLocal.Printf("Connection Error.\n");

					GuiMessage msg;
					msg.title = "Mission Screenshot Download Failed";
					msg.message = "Failed to download the screenshot file.";
					msg.type = GuiMessage::MSG_OK;
					msg.okCmd = "close_msg_box";

					gameLocal.AddMainMenuMessage(msg);
				}
				break;

				case CMissionManager::SUCCESSFUL:
				{
					// Load data into GUI
					// Get store "next" number from the GUI
					int nextScreenNum = gui->GetStateInt("av_mission_next_screenshot_num");

					UpdateNextScreenshotData(gui, nextScreenNum);

					// Ready to fade
					gui->HandleNamedEvent("onStartFadeToNextScreenshot");
				}
				break;

				default: break;
			};
		}
	}
	else if (cmd == "refreshAvailableMissionList")
	{
		if (!cv_tdm_allow_http_access.GetBool() || gameLocal.m_HttpConnection == NULL)
		{
			gui->HandleNamedEvent("onAvailableMissionsRefreshed"); // hide progress dialog

			// HTTP Access disallowed, display message
			gameLocal.Printf("HTTP requests disabled, cannot check for available missions.\n");

			GuiMessage msg;
			msg.type = GuiMessage::MSG_OK;
			msg.okCmd = "close_msg_box";
			msg.title = "Unable to contact Mission Archive";
			msg.message = "HTTP Requests have been disabled,\n cannot check for available missions.";

			gameLocal.AddMainMenuMessage(msg);

			return;
		}

		// Clear data before updating the list
		_selectedListTop = 0;
		_selectedMods.Clear();
		UpdateGUI(gui);
		UpdateDownloadProgress(gui);

		// Start refreshing the list, will be handled in mainmenu_heartbeat
		gameLocal.m_MissionManager->StartReloadDownloadableMods();
	}
	else if (cmd == "onDownloadableMissionSelected")
	{
		int selectedMission = gui->GetStateInt("av_mission_selected");

		int missionIndex = selectedMission + _availListTop;

		// Update mission details
		const DownloadableModList& mods = gameLocal.m_MissionManager->GetDownloadableMods();

		if (missionIndex > mods.Num()) return;

		gui->SetStateString("av_mission_title", mods[missionIndex]->title);
		gui->SetStateString("av_mission_author", mods[missionIndex]->author);
		gui->SetStateString("av_mission_release_date", mods[missionIndex]->releaseDate);
		gui->SetStateString("av_mission_version", va("%d", mods[missionIndex]->version));
		gui->SetStateString("av_mission_size", va("%0.1f MB", mods[missionIndex]->sizeMB));

		gui->SetStateBool("av_mission_details_visible", true);

		gui->HandleNamedEvent("UpdateAvailableMissionColours");
	}
	else if (cmd == "onSelectMissionForDownload")
	{
		int selectedMission = gui->GetStateInt("av_mission_selected");
		int missionIndex = selectedMission + _availListTop;

		_selectedMods.AddUnique(missionIndex);

		gui->SetStateInt("av_mission_selected", -1);
		gui->SetStateBool("av_mission_details_visible", false);

		UpdateGUI(gui);
	}
	else if (cmd == "onDeselectMissionForDownload")
	{
		int selectedItem = gui->GetStateInt("dl_mission_selected");
		int index = selectedItem + _selectedListTop;

		if (index >= _selectedMods.Num()) return;

		_selectedMods.Remove(_selectedMods[index]);

		UpdateGUI(gui);
		UpdateDownloadProgress(gui);
	}
	else if (cmd == "onDownloadableMissionScrollUp")
	{
		int numMissionsPerPage = gui->GetStateInt("packagesPerPage", "5");
		_availListTop -= numMissionsPerPage;

		if (_availListTop < 0) _availListTop = 0;

		UpdateGUI(gui);
	}
	else if (cmd == "onDownloadableMissionScrollDown")
	{
		int numMissionsPerPage = gui->GetStateInt("packagesPerPage", "5");
		_availListTop += numMissionsPerPage;

		UpdateGUI(gui);
	}
	else if (cmd == "onSelectedMissionScrollUp")
	{
		int itemsPerPage = gui->GetStateInt("selectedPackagesPerPage", "5");
		_selectedListTop -= itemsPerPage;

		if (_selectedListTop < 0) _selectedListTop = 0;

		UpdateGUI(gui);
	}
	else if (cmd == "onSelectedMissionScrollDown")
	{
		int itemsPerPage = gui->GetStateInt("selectedPackagesPerPage", "5");
		_selectedListTop += itemsPerPage;

		UpdateGUI(gui);
	}
	else if (cmd == "onDownloadableMissionShowDetails")
	{
		int selectedMission = gui->GetStateInt("av_mission_selected");
		int missionIndex = selectedMission + _availListTop;

		// Issue a new download request
		gameLocal.m_MissionManager->StartDownloadingModDetails(missionIndex);

		gui->HandleNamedEvent("onDownloadableMissionDetailsLoaded");
	}
	else if (cmd == "onStartDownload")
	{
		StartDownload(gui);
		UpdateDownloadProgress(gui); // do this first
		UpdateGUI(gui);
	}
	else if (cmd == "onDownloadCompleteConfirm")
	{
		// Let the GUI request another refresh of downloadable missions (with delay)
		gui->HandleNamedEvent("QueueDownloadableMissionListRefresh");
	}
	else if (cmd == "onGetNextScreenshotForAvailableMission")
	{
		PerformScreenshotStep(gui, +1);
		UpdateScreenshotItemVisibility(gui);
	}
	else if (cmd == "onGetPrevScreenshotForAvailableMission")
	{
		PerformScreenshotStep(gui, -1);
		UpdateScreenshotItemVisibility(gui);
	}
}

void CDownloadMenu::UpdateScreenshotItemVisibility(idUserInterface* gui)
{
	int selectedMission = gui->GetStateInt("av_mission_selected");
	int missionIndex = selectedMission + _availListTop;

	// Check if the screenshot is downloaded already
	const DownloadableModList& mods = gameLocal.m_MissionManager->GetDownloadableMods();

	assert(missionIndex >= 0 && missionIndex < mods.Num());

	int numScreens = mods[missionIndex]->screenshots.Num();

	gui->SetStateBool("av_no_screens_available", numScreens == 0);
	gui->SetStateBool("av_mission_screenshot_prev_visible", numScreens > 1);
	gui->SetStateBool("av_mission_screenshot_next_visible", numScreens > 1);
}

void CDownloadMenu::UpdateNextScreenshotData(idUserInterface* gui, int nextScreenshotNum)
{
	int selectedMission = gui->GetStateInt("av_mission_selected");
	int missionIndex = selectedMission + _availListTop;

	// Check if the screenshot is downloaded already
	const DownloadableModList& mods = gameLocal.m_MissionManager->GetDownloadableMods();

	assert(missionIndex >= 0 && missionIndex < mods.Num());

	if (mods[missionIndex]->screenshots.Num() == 0)
	{
		return; // no screenshots for this mission
	}

	MissionScreenshot& screenshotInfo = *mods[missionIndex]->screenshots[nextScreenshotNum];

	// Update the current screenshot number
	gui->SetStateInt("av_mission_cur_screenshot_num", nextScreenshotNum);

	// Load next screenshot path, remove image extension
	idStr path = screenshotInfo.filename;
	path.StripFileExtension();

	gui->SetStateString("av_mission_next_screenshot", path);
}

void CDownloadMenu::PerformScreenshotStep(idUserInterface* gui, int step)
{
	int selectedMission = gui->GetStateInt("av_mission_selected");
	int missionIndex = selectedMission + _availListTop;

	// Check if the screenshot is downloaded already
	const DownloadableModList& mods = gameLocal.m_MissionManager->GetDownloadableMods();

	assert(missionIndex >= 0 && missionIndex < mods.Num());

	int numScreens = mods[missionIndex]->screenshots.Num();

	if (numScreens == 0)
	{
		return; // no screenshots for this mission
	}

	int curScreenNum = gui->GetStateInt("av_mission_cur_screenshot_num");
	int nextScreenNum = (curScreenNum + step + numScreens) % numScreens; // ensure index is always positive

	assert(nextScreenNum >= 0);

	// Store the next number in the GUI
	gui->SetStateInt("av_mission_next_screenshot_num", nextScreenNum);

	if (nextScreenNum != curScreenNum || curScreenNum == 0)
	{
		MissionScreenshot& screenshotInfo = *mods[missionIndex]->screenshots[nextScreenNum];

		if (screenshotInfo.filename.IsEmpty())
		{
			// No local file yet, start downloading it
			gui->HandleNamedEvent("onStartDownloadingNextScreenshot");

			// New request
			gameLocal.m_MissionManager->StartDownloadingMissionScreenshot(missionIndex, nextScreenNum);
		}
		else
		{
			// Load data necessary to fade into the GUI
			UpdateNextScreenshotData(gui, nextScreenNum);
			
			// There is a local file, this means we already downloaded that screenshot
			gui->HandleNamedEvent("onStartFadeToNextScreenshot");
		}
	}
}

void CDownloadMenu::StartDownload(idUserInterface* gui)
{
	// Add a new download for each selected mission
	const DownloadableModList& mods = gameLocal.m_MissionManager->GetDownloadableMods();

	for (int i = 0; i < _selectedMods.Num(); ++i)
	{
		int missionIndex = _selectedMods[i];

		if (missionIndex > mods.Num()) continue;

		const DownloadableMod& mod = *mods[missionIndex];

		// The filename is deduced from the mod name found on the website
		idStr targetPath = g_Global.GetDarkmodPath().c_str();
		targetPath += "/";
		targetPath += cv_tdm_fm_path.GetString();
		targetPath += mod.modName + ".pk4";

		CDownloadPtr download(new CDownload(mod.downloadLocations, targetPath));

		// Check for valid PK4 files after download
		download->EnableValidPK4Check(true);

		int id = gameLocal.m_DownloadManager->AddDownload(download);

		// Store this ID
		_downloads[missionIndex] = id;
	}

	// Let the download manager start its downloads
	gameLocal.m_DownloadManager->ProcessDownloads();
}

void CDownloadMenu::UpdateModDetails(idUserInterface* gui)
{
	// Get the selected mod index
	int selectedMod = gui->GetStateInt("av_mission_selected");
	int modIndex = selectedMod + _availListTop;

	const DownloadableModList& mods = gameLocal.m_MissionManager->GetDownloadableMods();

	if (modIndex < 0 || modIndex >= mods.Num())
	{
		return;
	}

	if (!mods[modIndex]->detailsLoaded)
	{
		GuiMessage msg;
		msg.type = GuiMessage::MSG_OK;
		msg.okCmd = "close_msg_box";
		msg.title = "Code Logic Error";
		msg.message = "No mission details loaded.";

		gameLocal.AddMainMenuMessage(msg);

		return;
	}

	gui->SetStateString("av_mission_title", mods[modIndex]->title);
	gui->SetStateString("av_mission_author", mods[modIndex]->author);
	gui->SetStateString("av_mission_release_date", mods[modIndex]->releaseDate);
	gui->SetStateString("av_mission_version", va("%d", mods[modIndex]->version));
	gui->SetStateString("av_mission_size", va("%0.1f MB", mods[modIndex]->sizeMB));

	gui->SetStateString("av_mission_description", mods[modIndex]->description);
}

void CDownloadMenu::UpdateGUI(idUserInterface* gui)
{
	const DownloadableModList& mods = gameLocal.m_MissionManager->GetDownloadableMods();

	gui->SetStateBool("av_no_download_available", mods.Num() == 0);

	bool downloadInProgress = gui->GetStateBool("mission_download_in_progress");

	bool updateInList = false;
	
	int numModsPerPage = gui->GetStateInt("packagesPerPage", "5");
	
	for (int i = 0; i < numModsPerPage; ++i)
	{
		// Apply page offset
		int modIndex = i + _availListTop;
		bool missionExists = modIndex < mods.Num();

		bool missionSelected = _selectedMods.FindIndex(modIndex) != -1;
		gui->SetStateBool(va("av_mission_avail_%d", i), missionExists && !missionSelected && !downloadInProgress);
		gui->SetStateBool(va("av_mission_selected_%d", i), missionSelected);

		if (missionExists)
		{
			idStr title = mods[modIndex]->title;

			if (mods[modIndex]->isUpdate)
			{
				title += "*";
				updateInList = true;
			}

			gui->SetStateString(va("av_mission_name_%d", i), title);
		}
		else
		{
			gui->SetStateString(va("av_mission_name_%d", i), "");
		}
	}

	gui->SetStateBool("av_mission_update_in_list", updateInList);
	gui->SetStateBool("av_mission_scroll_up_visible", _availListTop > 0);
	gui->SetStateBool("av_mission_scroll_down_visible", _availListTop + numModsPerPage < mods.Num());

	int numSelectedModsPerPage = gui->GetStateInt("selectedPackagesPerPage", "5");

	// Missions in the download queue
	for (int i = 0; i < numSelectedModsPerPage; ++i)
	{
		// Apply page offset
		int listIndex = i + _selectedListTop;
		bool listItemExists = listIndex < _selectedMods.Num();

		// Get the referenced mission index, -1 ==> no mission
		int modIndex = listItemExists ? _selectedMods[listIndex] : -1;

		gui->SetStateBool(va("dl_mission_avail_%d", i), listItemExists);
		gui->SetStateString(va("dl_mission_name_%d", i), modIndex != -1 ? mods[modIndex]->title : "");

		// Find the download ID and initialise the value to empty if no download is existing yet
		ActiveDownloads::const_iterator it = _downloads.find(modIndex);

		if (it == _downloads.end())
		{
			gui->SetStateString(va("dl_mission_progress_%d", i), "queued ");
			continue;
		}
	}

	gui->SetStateBool("dl_mission_scroll_up_visible", _selectedListTop > 0);
	gui->SetStateBool("dl_mission_scroll_down_visible", _selectedListTop + numSelectedModsPerPage < _selectedMods.Num());

	gui->SetStateInt("dl_mission_count", _selectedMods.Num());
	gui->SetStateBool("dl_button_available", _selectedMods.Num() > 0 && !downloadInProgress);
	gui->SetStateBool("dl_button_visible", !downloadInProgress);

	gui->HandleNamedEvent("UpdateAvailableMissionColours");
	gui->HandleNamedEvent("UpdateSelectedMissionColours");
}

void CDownloadMenu::UpdateDownloadProgress(idUserInterface* gui)
{
	int numSelectedModsPerPage = gui->GetStateInt("selectedPackagesPerPage", "5");

	bool downloadsInProgress = false;

	// Check if we have any mission downloads pending.
	// Don't use DownloadManager::DownloadInProgress(), as there might be different kind of downloads in progress
	for (ActiveDownloads::const_iterator it = _downloads.begin(); it != _downloads.end(); ++it)
	{
		CDownloadPtr download = gameLocal.m_DownloadManager->GetDownload(it->second);

		if (download == NULL) continue;

		if (download->GetStatus() == CDownload::IN_PROGRESS)
		{
			downloadsInProgress = true;
			break;
		}
	}

	// Missions in the download queue
	for (int i = 0; i < numSelectedModsPerPage; ++i)
	{
		// Apply page offset
		int listIndex = i + _selectedListTop;
		bool listItemExists = listIndex < _selectedMods.Num();

		// Get the referenced mod index, -1 ==> no mod
		int modIndex = listItemExists ? _selectedMods[listIndex] : -1;

		if (!listItemExists || modIndex == -1)
		{
			gui->SetStateString(va("dl_mission_progress_%d", i), "");
			continue;
		}

		// Find the download ID
		ActiveDownloads::const_iterator it = _downloads.find(modIndex);

		if (it == _downloads.end())
		{
			gui->SetStateString(va("dl_mission_progress_%d", i), "queued ");
			continue;
		}
		
		CDownloadPtr download = gameLocal.m_DownloadManager->GetDownload(it->second);

		if (download == NULL) continue;

		switch (download->GetStatus())
		{
		case CDownload::NOT_STARTED_YET:
			gui->SetStateString(va("dl_mission_progress_%d", i), "queued ");
			break;
		case CDownload::FAILED:
			gui->SetStateString(va("dl_mission_progress_%d", i), "failed ");
			break;
		case CDownload::IN_PROGRESS:
			gui->SetStateString(va("dl_mission_progress_%d", i), va("%0.0f%s", download->GetProgressFraction()*100, "% "));
			break;
		case CDownload::SUCCESS:
			gui->SetStateString(va("dl_mission_progress_%d", i), "100% ");
			break;
		};
	}

	// Update the "in progress" state flag 
	bool prevDownloadsInProgress = gui->GetStateBool("mission_download_in_progress");
	
	gui->SetStateBool("mission_download_in_progress", downloadsInProgress);

	if (prevDownloadsInProgress != downloadsInProgress)
	{
		gui->HandleNamedEvent("UpdateAvailableMissionColours");

		if (downloadsInProgress == false)
		{
			// Fire the "finished downloaded" event
			ShowDownloadResult(gui);
		}
	}
}

void CDownloadMenu::ShowDownloadResult(idUserInterface* gui)
{
	// greebo: Let the mod list be refreshed
	// We need the information from darkmod.txt later down this road
	gameLocal.m_MissionManager->ReloadModList();

	int successfulDownloads = 0;
	int failedDownloads = 0;

	const DownloadableModList& mods = gameLocal.m_MissionManager->GetDownloadableMods();

	for (ActiveDownloads::iterator i = _downloads.begin(); i != _downloads.end(); ++i)
	{
		CDownloadPtr download = gameLocal.m_DownloadManager->GetDownload(i->second);

		if (download == NULL) continue;

		if (i->first > mods.Num()) continue;

		const DownloadableMod& mod = *mods[i->first];

		switch (download->GetStatus())
		{
		case CDownload::NOT_STARTED_YET:
			gameLocal.Warning("Some downloads haven't been processed?");
			break;
		case CDownload::FAILED:
			failedDownloads++;
			break;
		case CDownload::IN_PROGRESS:
			gameLocal.Warning("Some downloads still in progress?");
			break;
		case CDownload::SUCCESS:
			{
				successfulDownloads++;

				// Save the mission version into the MissionDB for later use
				CModInfoPtr missionInfo = gameLocal.m_MissionManager->GetModInfo(mod.modName);
				missionInfo->SetKeyValue("downloaded_version", idStr(mod.version).c_str());
			}
			break;
		};
	}

	gameLocal.Printf("Successful downloads: %d\nFailed downloads: %d\n", successfulDownloads, failedDownloads);

	// Display the popup box
	GuiMessage msg;
	msg.type = GuiMessage::MSG_OK;
	msg.okCmd = "close_msg_box;onDownloadCompleteConfirm";
	msg.title = "Mission Download Result";
	msg.message = "";

	if (successfulDownloads > 0)
	{
		msg.message += va("%d %s been successfully downloaded. "
						  "You'll find the %s in the 'New Mission' page.", 
						  successfulDownloads, 
						  GetPlural(successfulDownloads, "mission has", "missions have"),
						  GetPlural(successfulDownloads, "mission", "missions")); 
	}
	
	if (failedDownloads > 0)
	{
		msg.message += va("\n%d %s couldn't be downloaded. "
						  "Please check your disk space (or maybe some file is "
						  "write protected) and try again.", failedDownloads,
						  GetPlural(failedDownloads, "mission", "missions"));
	}

	gameLocal.AddMainMenuMessage(msg);

	// Remove all downloads
	for (ActiveDownloads::iterator i = _downloads.begin(); i != _downloads.end(); ++i)
	{
		gameLocal.m_DownloadManager->RemoveDownload(i->second);
	}

	_downloads.clear();
}
